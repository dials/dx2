#include "dx2/h5/h5read_processed.hpp"

#include <chrono>
#include <hdf5.h>
#include <iostream>
#include <map>
#include <string>
#include <unordered_set>
#include <vector>

#pragma region Implementation

template <typename T>
void read_array_from_h5_file(const std::string &filename,
                             const std::string &dataset_name,
                             H5Data<T> &h5_data) {
  auto start_time = std::chrono::high_resolution_clock::now();

  hid_t file = H5Fopen(filename.c_str(), H5F_ACC_RDONLY, H5P_DEFAULT);
  if (file < 0) {
    throw std::runtime_error("Error: Unable to open file: " + filename);
  }

  try {
    hid_t dataset = H5Dopen(file, dataset_name.c_str(), H5P_DEFAULT);
    if (dataset < 0) {
      H5Fclose(file);
      throw std::runtime_error("Error: Unable to open dataset: " +
                               dataset_name);
    }

    try {
      hid_t dataspace = H5Dget_space(dataset);
      if (dataspace < 0) {
        H5Dclose(dataset);
        H5Fclose(file);
        throw std::runtime_error(
            "Error: Unable to get dataspace for dataset: " + dataset_name);
      }

      int ndims = H5Sget_simple_extent_ndims(dataspace);
      if (ndims < 0) {
        H5Sclose(dataspace);
        H5Dclose(dataset);
        H5Fclose(file);
        throw std::runtime_error("Error: Unable to get dataset rank.");
      }

      std::vector<hsize_t> dims(ndims);
      H5Sget_simple_extent_dims(dataspace, dims.data(), nullptr);

      h5_data.shape.assign(dims.begin(), dims.end());
      size_t num_elements = H5Sget_simple_extent_npoints(dataspace);
      h5_data.data.resize(num_elements);

      // Determine HDF5 type mapping
      hid_t native_type;
      if constexpr (std::is_same_v<T, int>) {
        native_type = H5T_NATIVE_INT;
      } else if constexpr (std::is_same_v<T, float>) {
        native_type = H5T_NATIVE_FLOAT;
      } else if constexpr (std::is_same_v<T, double>) {
        native_type = H5T_NATIVE_DOUBLE;
      } else if constexpr (std::is_same_v<T, size_t>) {
        native_type =
            H5T_NATIVE_ULONG; // or H5T_NATIVE_UINT64, depending on system
      } else {
        H5Sclose(dataspace);
        H5Dclose(dataset);
        H5Fclose(file);
        throw std::runtime_error("Unsupported data type for HDF5 reading.");
      }

      // Validate dataset type before reading
      hid_t dataset_type = H5Dget_type(dataset);
      if (H5Tequal(dataset_type, native_type) == 0) {
        H5Tclose(dataset_type);
        H5Sclose(dataspace);
        H5Dclose(dataset);
        H5Fclose(file);
        throw std::runtime_error(
            "Error: Dataset type does not match requested type.");
      }
      H5Tclose(dataset_type);

      // Read dataset
      herr_t status = H5Dread(dataset, native_type, H5S_ALL, H5S_ALL,
                              H5P_DEFAULT, h5_data.data.data());
      if (status < 0) {
        H5Sclose(dataspace);
        H5Dclose(dataset);
        H5Fclose(file);
        throw std::runtime_error("Error: Unable to read dataset: " +
                                 dataset_name);
      }

      // Cleanup
      H5Sclose(dataspace);
      H5Dclose(dataset);
      H5Fclose(file);

      auto end_time = std::chrono::high_resolution_clock::now();
      double elapsed_time =
          std::chrono::duration<double>(end_time - start_time).count();
      std::cout << "READ TIME for " << dataset_name << " : " << elapsed_time
                << "s" << std::endl;

    } catch (...) {
      H5Dclose(dataset);
      H5Fclose(file);
      throw;
    }
  } catch (...) {
    H5Fclose(file);
    throw;
  }
}

// Forward declaration
void traverse_hdf5(hid_t loc_id, const std::string &path,
                   std::vector<std::string> &datasets,
                   std::unordered_set<std::string> &visited_groups);

struct TraverseData {
  std::vector<std::string> *datasets;
  std::string current_path;
  std::unordered_set<std::string> *visited_groups;
};

// Callback function for iterating through HDF5 objects
herr_t group_iterator(hid_t loc_id, const char *name, const H5L_info2_t *info,
                      void *op_data) {
  TraverseData *traverse_data = static_cast<TraverseData *>(op_data);
  std::string full_path =
      traverse_data->current_path.empty()
          ? "/" + std::string(name)
          : traverse_data->current_path + "/" + std::string(name);

  // std::cout << "Visiting: " << full_path << std::endl;

  // Ensure this path is not visited twice
  if (traverse_data->visited_groups->find(full_path) !=
      traverse_data->visited_groups->end()) {
    // std::cout << "Skipping already visited group: " << full_path <<
    // std::endl;
    return 0; // Continue iteration
  }

  // Get object info
  H5O_info2_t obj_info;
  if (H5Oget_info_by_name3(loc_id, name, &obj_info, H5O_INFO_BASIC,
                           H5P_DEFAULT) < 0) {
    std::cerr << "Error: Unable to get object info for: " << full_path
              << std::endl;
    return -1;
  }

  if (obj_info.type == H5O_TYPE_DATASET) {
    // std::cout << "Dataset found: " << full_path << std::endl;
    traverse_data->datasets->push_back(full_path);
  } else if (obj_info.type == H5O_TYPE_GROUP) {
    // std::cout << "Entering group: " << full_path << std::endl;

    // Mark the group as visited
    traverse_data->visited_groups->insert(full_path);

    // Open the group to recurse into it
    hid_t group_id = H5Gopen2(loc_id, name, H5P_DEFAULT);
    if (group_id >= 0) {
      traverse_hdf5(group_id, full_path, *(traverse_data->datasets),
                    *(traverse_data->visited_groups));
      H5Gclose(group_id);
    } else {
      std::cerr << "Error: Unable to open group: " << full_path << std::endl;
    }
  }

  return 0; // Continue iteration
}

/// Traverse an HDF5 file and collect all datasets
void traverse_hdf5(hid_t loc_id, const std::string &path,
                   std::vector<std::string> &datasets,
                   std::unordered_set<std::string> &visited_groups) {
  // std::cout << "Traversing: " << (path.empty() ? "/" : path) << std::endl;

  TraverseData traverse_data = {&datasets, path, &visited_groups};
  H5Literate2(loc_id, H5_INDEX_NAME, H5_ITER_NATIVE, NULL, group_iterator,
              &traverse_data);
}

std::vector<std::string> discover_datasets(const std::string &filename) {
  std::vector<std::string> datasets;
  std::unordered_set<std::string> visited_groups;

  // Open the HDF5 file
  hid_t file = H5Fopen(filename.c_str(), H5F_ACC_RDONLY, H5P_DEFAULT);
  if (file < 0) {
    throw std::runtime_error("Error: Unable to open file: " + filename);
  }

  // Start traversal from root
  traverse_hdf5(file, "", datasets, visited_groups);

  // Close the file
  H5Fclose(file);

  return datasets;
}

std::vector<std::string> get_datasets_in_group(const std::string &filename,
                                               const std::string &group_name) {
  std::vector<std::string> datasets;
  std::unordered_set<std::string> visited_groups;

  // Open the HDF5 file
  hid_t file = H5Fopen(filename.c_str(), H5F_ACC_RDONLY, H5P_DEFAULT);
  if (file < 0) {
    throw std::runtime_error("Error: Unable to open file: " + filename);
  }

  // Open the group
  hid_t group = H5Gopen2(file, group_name.c_str(), H5P_DEFAULT);
  if (group < 0) {
    H5Fclose(file);
    throw std::runtime_error("Error: Unable to open group: " + group_name);
  }

  // Start traversal from the group
  traverse_hdf5(group, group_name, datasets, visited_groups);

  // Close the group and file
  H5Gclose(group);
  H5Fclose(file);

  return datasets;
}

std::string get_dataset_name(const std::string &path) {
  size_t pos = path.find_last_of('/');
  if (pos == std::string::npos) {
    return path; // No '/' found, return the whole path
  }
  return path.substr(pos + 1);
}

#pragma endregion Implementation

#pragma region Explicit template instantiation

/*
 * Explicit template instantiations are required for the compiler to
 * generate the necessary code used in the tests
 */

template void read_array_from_h5_file<int>(const std::string &filename,
                                           const std::string &dataset_name,
                                           H5Data<int> &h5_data);
template void read_array_from_h5_file<double>(const std::string &filename,
                                              const std::string &dataset_name,
                                              H5Data<double> &h5_data);
template void
read_array_from_h5_file<unsigned long>(const std::string &filename,
                                       const std::string &dataset_name,
                                       H5Data<unsigned long> &h5_data);

#pragma endregion Explicit template instantiation