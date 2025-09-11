/**
 * @file h5read_processed.hpp
 * @brief HDF5 dataset access utilities for reflection table loading.
 *
 * This header provides functions for reading typed datasets from HDF5
 * files along with their dimensional metadata. It includes fast,
 * shape-aware readers, recursive traversal of group structures, and
 * scoped utilities for extracting datasets from flat HDF5 groups such
 * as `/dials/processing/group_0`.
 *
 * Utilities in `h5read_processed_utils` focus on immediate dataset
 * access, while recursive tools allow for full introspection of nested
 * groups.
 *
 * Intended for internal use in processing pipelines.
 */

#pragma once

#include "dx2/h5/h5utils.hpp"
#include "dx2/logging.hpp"
#include <cassert>
#include <chrono>
#include <cstring>
#include <experimental/mdspan>
#include <hdf5.h>
#include <hdf5_hl.h>
#include <string>
#include <typeindex>
#include <unordered_set>
#include <vector>

namespace h5read_processed_utils {

#pragma region Internal Structs

/**
 * @brief Container for shaped HDF5 data.
 *
 * Holds the raw flat data vector and shape information for a dataset.
 */
template <typename T> struct H5ArrayData {
  std::vector<T> data;
  std::vector<size_t> shape;
};

/**
 * @brief Internal context for scanning a single HDF5 group.
 *
 * Used to pass state to the `H5Literate2` callback for flat dataset collection.
 */
struct GroupScanContext {
  std::string group_name;            // Name of the group being scanned
  std::vector<std::string> datasets; // Collected dataset paths
};

///  @brief Traverse state context for recursive dataset discovery.
struct TraverseData {
  std::vector<std::string> *datasets;
  std::string current_path;
  std::unordered_set<std::string> *visited_groups;
};

#pragma endregion
#pragma region Callbacks

/**
 * @brief Callback for `H5Literate2` that collects only datasets.
 *
 * Appends fully-qualified dataset names to the context, skipping subgroups.
 *
 * @param loc_id Current group/location ID.
 * @param name Name of the object (dataset or group).
 * @param info Unused link info.
 * @param op_data Pointer to a `GroupScanContext`.
 * @return 0 on success, non-zero to stop iteration.
 */
herr_t scan_group_callback(hid_t loc_id, const char *name, const H5L_info2_t *,
                           void *op_data);

/**
 * @brief Callback for `H5Literate2` that walks both groups and datasets.
 *
 * Recursively visits groups and appends dataset paths to `TraverseData`.
 */
herr_t group_iterator(hid_t loc_id, const char *name, const H5L_info2_t *info,
                      void *op_data);

#pragma endregion
#pragma region Group Traversal

/**
 * @brief Recursively traverses an HDF5 group and collects dataset paths.
 *
 * @param loc_id HDF5 group ID to start from.
 * @param path Path prefix for entries.
 * @param datasets Output vector to populate.
 * @param visited_groups Prevents revisiting cycles.
 */
void traverse_hdf5(hid_t loc_id, const std::string &path,
                   std::vector<std::string> &datasets,
                   std::unordered_set<std::string> &visited_groups);

#pragma endregion
} // namespace h5read_processed_utils

#pragma region Public API
/**
 * @brief Lists all *immediate* datasets in an HDF5 group (non-recursive).
 *
 * Only returns datasets directly under `group_name`, not nested in subgroups.
 *
 * @param filename Path to the HDF5 file.
 * @param group_name Path to the group (e.g., "/dials/processing/group_0").
 * @return Vector of full dataset paths.
 */
std::vector<std::string> get_datasets_in_group(std::string_view filename,
                                               std::string_view group_name);

/**
 * @brief Reads an HDF5 dataset and returns its data and shape.
 *
 * @tparam T Type of data to read (e.g., `double`, `int`).
 * @param filename Path to the HDF5 file.
 * @param dataset_name Full path to the dataset.
 * @return H5ArrayData<T> containing flat data and shape vector.
 */
template <typename T>
h5read_processed_utils::H5ArrayData<T>
read_array_with_shape_from_h5_file(std::string_view filename,
                                   std::string_view dataset_name) {
  auto start_time = std::chrono::high_resolution_clock::now();

  std::string fname(filename);
  std::string dset_name(dataset_name);

  // Open the HDF5 file
  h5utils::H5File file(H5Fopen(fname.c_str(), H5F_ACC_RDONLY, H5P_DEFAULT));
  if (!file) {
    throw std::runtime_error("Error: Unable to open file: " + fname);
  }

  // Open the dataset
  h5utils::H5Dataset dataset(H5Dopen(file, dset_name.c_str(), H5P_DEFAULT));
  if (!dataset) {
    throw std::runtime_error("Error: Unable to open dataset: " + dset_name);
  }

  // Get the datatype and check size
  h5utils::H5Type datatype(H5Dget_type(dataset));
  if (H5Tget_size(datatype) != sizeof(T)) {
    throw std::runtime_error(
        "Error: Dataset type size does not match expected type size.");
  }

  // Get the dataspace and the number of elements
  h5utils::H5Space dataspace(H5Dget_space(dataset));
  int ndims = H5Sget_simple_extent_ndims(dataspace);
  if (ndims <= 0) {
    throw std::runtime_error("Error: Dataset has invalid dimensionality.");
  }

  // Get the dimensions of the dataset
  std::vector<hsize_t> dims(ndims);
  H5Sget_simple_extent_dims(dataspace, dims.data(), nullptr);

  // Calculate the number of elements
  size_t num_elements = 1;
  for (auto d : dims)
    num_elements *= d;

  // Allocate a vector to hold the data
  std::vector<T> data_out(num_elements);
  // Read the data into the vector
  herr_t status = H5Dread(dataset, datatype, H5S_ALL, H5S_ALL, H5P_DEFAULT,
                          data_out.data());
  if (status < 0) {
    throw std::runtime_error("Error: Unable to read dataset: " + dset_name);
  }

  auto end_time = std::chrono::high_resolution_clock::now();
  double elapsed_time =
      std::chrono::duration<double>(end_time - start_time).count();
  dx2_log::debug("READ TIME for {} : {:.4f}s", dset_name, elapsed_time);

  return {std::move(data_out), std::vector<size_t>(dims.begin(), dims.end())};
}

/**
 * @brief Reads an HDF5 dataset into a flat vector (no shape metadata).
 *
 * @tparam T Type of the dataset (e.g., `float`, `int64_t`).
 * @param filename HDF5 file path.
 * @param dataset_name Full path to the dataset.
 * @return Vector of raw values.
 */
template <typename T>
std::vector<T> read_array_from_h5_file(std::string_view filename,
                                       std::string_view dataset_name) {
  return read_array_with_shape_from_h5_file<T>(filename, dataset_name).data;
}

/**
 * @brief Recursively finds all datasets in a group and subgroups.
 *
 * @param filename Path to the HDF5 file.
 * @param group_name Name of the top-level group to search.
 * @return Vector of full dataset paths.
 */
std::vector<std::string>
get_datasets_in_group_recursive(std::string_view filename,
                                std::string_view group_name);

void read_experiment_metadata(hid_t group_id,
                              std::vector<uint64_t> &experiment_ids,
                              std::vector<std::string> &identifiers);

/**
 * @brief Extracts just the leaf name from a full HDF5 dataset path.
 *
 * @param path Full dataset path (e.g., `/a/b/c`).
 * @return The base name (`c`).
 */
std::string get_dataset_name(std::string_view path);
#pragma endregion