/**
 * @file h5read_processed.cxx
 * @brief Implementation file for HDF5 dataset access utilities.
 */

#include "dx2/h5/h5read_processed.hpp"

namespace h5read_processed_utils {

#pragma region Callbacks
herr_t scan_group_callback(hid_t loc_id, const char *name, const H5L_info2_t *,
                           void *op_data) {
  auto *context = static_cast<GroupScanContext *>(op_data);

  // Get object info
  H5O_info2_t obj_info;
  if (H5Oget_info_by_name3(loc_id, name, &obj_info, H5O_INFO_BASIC,
                           H5P_DEFAULT) < 0) {
    return 0; // Skip unreadable entries
  }

  if (obj_info.type == H5O_TYPE_DATASET) {
    context->datasets.push_back(context->group_name + "/" + std::string(name));
  }

  return 0;
}

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
    dx2_log::error(fmt::format("Unable to get object info for: {}", full_path));
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
    h5utils::H5Group group_id(H5Gopen2(loc_id, name, H5P_DEFAULT));
    if (group_id) {
      traverse_hdf5(group_id, full_path, *(traverse_data->datasets),
                    *(traverse_data->visited_groups));
    } else {
      dx2_log::error(fmt::format("Unable to open group: {}", full_path));
    }
  }

  return 0; // Continue iteration
}

#pragma endregion
#pragma region Group Traversal

void traverse_hdf5(hid_t loc_id, const std::string &path,
                   std::vector<std::string> &datasets,
                   std::unordered_set<std::string> &visited_groups) {
  // std::cout << "Traversing: " << (path.empty() ? "/" : path) << std::endl;

  TraverseData traverse_data = {&datasets, path, &visited_groups};
  H5Literate2(loc_id, H5_INDEX_NAME, H5_ITER_NATIVE, NULL, group_iterator,
              &traverse_data);
}

#pragma endregion
} // namespace h5read_processed_utils

#pragma region Public API
std::vector<std::string> get_datasets_in_group(std::string_view filename,
                                               std::string_view group_name) {
  std::string fname(filename);
  std::string gpath(group_name);
  // Open file
  h5utils::H5File file(H5Fopen(fname.c_str(), H5F_ACC_RDONLY, H5P_DEFAULT));
  if (!file) {
    throw std::runtime_error("Error: Unable to open file: " + fname);
  }

  // Open group
  h5utils::H5Group group(H5Gopen2(file, gpath.c_str(), H5P_DEFAULT));
  if (!group) {
    dx2_log::warning(fmt::format("Missing group '{}', skipping.", gpath));
    return {};
  }

  h5read_processed_utils::GroupScanContext context{gpath, {}};

  // Iterate over immediate children
  H5Literate2(group, H5_INDEX_NAME, H5_ITER_NATIVE, nullptr,
              h5read_processed_utils::scan_group_callback, &context);

  return context.datasets;
}

std::vector<std::string>
get_datasets_in_group_recursive(std::string_view filename,
                                std::string_view group_name) {
  std::string fname(filename);
  std::string gpath(group_name);

  std::vector<std::string> datasets;
  std::unordered_set<std::string> visited_groups;

  // Open the HDF5 file
  h5utils::H5File file(H5Fopen(fname.c_str(), H5F_ACC_RDONLY, H5P_DEFAULT));
  if (!file) {
    dx2_log::error(fmt::format("Unable to open file: {}", fname));
  }

  // Open the group
  // hid_t group = H5Gopen2(file, gpath.c_str(), H5P_DEFAULT);
  h5utils::H5Group group(H5Gopen2(file, gpath.c_str(), H5P_DEFAULT));
  if (!group) {
    dx2_log::warning(fmt::format("Missing group '{}', skipping.", gpath));
    return {};
  }

  // Start traversal from the group
  h5read_processed_utils::traverse_hdf5(group, gpath, datasets, visited_groups);

  return datasets;
}

void read_experiment_metadata(hid_t group_id,
                              std::vector<uint64_t> &experiment_ids,
                              std::vector<std::string> &identifiers) {
  if (H5Aexists(group_id, "experiment_ids") > 0) {
    h5utils::H5Attr attr(H5Aopen(group_id, "experiment_ids", H5P_DEFAULT));
    h5utils::H5Space space(H5Aget_space(attr));
    hssize_t num_elements = H5Sget_simple_extent_npoints(space);

    experiment_ids.resize(num_elements);
    H5Aread(attr, H5T_NATIVE_ULLONG, experiment_ids.data());
  }

  if (H5Aexists(group_id, "identifiers") > 0) {
    h5utils::H5Attr attr(H5Aopen(group_id, "identifiers", H5P_DEFAULT));
    h5utils::H5Type type(H5Aget_type(attr));
    h5utils::H5Space space(H5Aget_space(attr));
    hssize_t num_elements = H5Sget_simple_extent_npoints(space);

    std::vector<char *> raw_strings(num_elements);
    identifiers.resize(num_elements);
    H5Aread(attr, type, raw_strings.data());

    for (hssize_t i = 0; i < num_elements; ++i) {
      identifiers[i] = std::string(raw_strings[i]);
    }
  }
}

std::string get_dataset_name(std::string_view path) {
  size_t pos = path.find_last_of('/');
  if (pos == std::string_view::npos) {
    return std::string(path); // No '/' found, return the whole path
  }
  return std::string(path.substr(pos + 1));
}
#pragma endregion
