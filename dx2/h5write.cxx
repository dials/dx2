#include "dx2/h5/h5write.hpp"

#pragma region Implementation

hid_t traverse_or_create_groups(hid_t parent, const std::string &path) {
  // Strip leading '/' characters, if any, to prevent empty group names
  size_t start_pos = path.find_first_not_of('/');
  if (start_pos == std::string::npos) {
    return parent; // Return parent if the path is entirely '/'
  }
  std::string cleaned_path = path.substr(start_pos);

  /*
   * This is the base case for recursion. When the path is empty, we
   * have reached the final group in the path and we return the parent
   * group.
   */
  if (cleaned_path.empty()) {
    return parent;
  }

  // Split the path into the current group name and the remaining path
  size_t pos = cleaned_path.find('/');
  std::string group_name =
      (pos == std::string::npos) ? cleaned_path : cleaned_path.substr(0, pos);
  std::string remaining_path =
      (pos == std::string::npos) ? "" : cleaned_path.substr(pos + 1);

  // Attempt to open the group. If it does not exist, create it.
  H5Eset_auto2(H5E_DEFAULT, NULL, NULL); // Suppress errors to stdout when
  // trying to open a file/group that may not exist.
  hid_t next_group = H5Gopen(parent, group_name.c_str(), H5P_DEFAULT);
  if (next_group < 0) {
    next_group = H5Gcreate(parent, group_name.c_str(), H5P_DEFAULT, H5P_DEFAULT,
                           H5P_DEFAULT);
    if (next_group < 0) {
      std::runtime_error("Error: Unable to create or open group: " +
                         group_name);
    }
  }

  // Recurse to the next group in the hierarchy
  hid_t final_group = traverse_or_create_groups(next_group, remaining_path);

  // Close the current group to avoid resource leaks, except for the final group
  if (next_group != final_group) {
    H5Gclose(next_group);
  }

  return final_group;
}

void write_data_to_h5_file(const std::string &filename,
                           const std::string &dataset_path,
                           const std::vector<double> &data,
                           const std::vector<hsize_t> &shape) {
  H5Eset_auto2(H5E_DEFAULT, NULL, NULL);
  hid_t file = H5Fopen(filename.c_str(), H5F_ACC_RDWR, H5P_DEFAULT);
  if (file < 0) {
    file = H5Fcreate(filename.c_str(), H5F_ACC_TRUNC, H5P_DEFAULT, H5P_DEFAULT);
    if (file < 0) {
      throw std::runtime_error("Error: Unable to create or open file: " +
                               filename);
    }
  }

  try {
    // Separate the dataset path into group path and dataset name
    size_t last_slash_pos = dataset_path.find_last_of('/');
    if (last_slash_pos == std::string::npos) {
      throw std::runtime_error("Error: Invalid dataset path, no '/' found: " +
                               dataset_path);
    }

    std::string group_path = dataset_path.substr(0, last_slash_pos);
    std::string dataset_name = dataset_path.substr(last_slash_pos + 1);

    // Traverse or create the groups leading to the dataset
    hid_t group = traverse_or_create_groups(file, group_path);

    // Check if dataset exists
    hid_t dataset = H5Dopen(group, dataset_name.c_str(), H5P_DEFAULT);
    if (dataset < 0) {
      // Dataset does not exist, create it
      hid_t dataspace = H5Screate_simple(shape.size(), shape.data(), NULL);
      if (dataspace < 0) {
        throw std::runtime_error(
            "Error: Unable to create dataspace for dataset: " + dataset_name);
      }

      dataset = H5Dcreate(group, dataset_name.c_str(), H5T_NATIVE_DOUBLE,
                          dataspace, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
      if (dataset < 0) {
        H5Sclose(dataspace);
        throw std::runtime_error("Error: Unable to create dataset: " +
                                 dataset_name);
      }

      H5Sclose(dataspace);
    } else {
      // Dataset exists, check if the shape matches
      hid_t existing_space = H5Dget_space(dataset);
      int ndims = H5Sget_simple_extent_ndims(existing_space);
      std::vector<hsize_t> existing_dims(ndims);
      H5Sget_simple_extent_dims(existing_space, existing_dims.data(), NULL);
      H5Sclose(existing_space);

      if (existing_dims != shape) {
        H5Dclose(dataset);
        throw std::runtime_error(
            "Error: Dataset shape mismatch. Cannot overwrite dataset: " +
            dataset_name);
      }

      // Dataset exists and has the correct shape, proceed to overwrite
    }

    // Write the data to the dataset
    herr_t status = H5Dwrite(dataset, H5T_NATIVE_DOUBLE, H5S_ALL, H5S_ALL,
                             H5P_DEFAULT, data.data());
    if (status < 0) {
      H5Dclose(dataset);
      throw std::runtime_error("Error: Unable to write data to dataset: " +
                               dataset_name);
    }

    // Cleanup resources
    H5Dclose(dataset);
    H5Gclose(group);
  } catch (...) {
    H5Fclose(file);
    throw; // Re-throw the exception to propagate it upwards
  }

  // Close the file
  H5Fclose(file);
}

template <typename Container>
void write_data_to_h5_file(const std::string &filename,
                           const std::string &dataset_path,
                           const Container &data) {
  std::vector<hsize_t> shape = deduce_shape(data);
  auto flat_data = flatten(data);

  write_data_to_h5_file(filename, dataset_path, flat_data, shape);
}

#pragma endregion Implementation

#pragma region Explicit template instantiation

/*
 * Explicit template instantiations are required for the compiler to
 * generate the necessary code used in the tests
 */

template void write_data_to_h5_file(const std::string &filename,
                                    const std::string &dataset_path,
                                    const std::vector<double> &data);

template void
write_data_to_h5_file(const std::string &filename,
                      const std::string &dataset_path,
                      const std::vector<std::vector<double>> &data);

#pragma endregion Explicit template instantiation