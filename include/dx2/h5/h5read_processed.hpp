#pragma once

#include <map>
#include <string>
#include <vector>

template <typename T> struct H5Data {
  std::vector<T> data;       // Flat data array
  std::vector<size_t> shape; // Dimensions of the data
};

/**
 * @brief Reads a dataset from an HDF5 file into an std::vector.
 *
 * @tparam T The type of data to read (e.g., int, double).
 * @param filename The path to the HDF5 file.
 * @param dataset_name The name of the dataset to read.
 * @param h5_data The struct to store the data and shape.
 * @return A std::vector containing the data from the dataset.
 * @throws std::runtime_error If the file, dataset, or datatype cannot be opened
 * or read.
 */
template <typename T>
void read_array_from_h5_file(const std::string &filename,
                             const std::string &dataset_name,
                             H5Data<T> &h5_data);

template <typename T>
std::vector<T> read_array_from_h5_file(const std::string &filename,
                                       const std::string &dataset_name) {
  H5Data<T> h5_data;
  read_array_from_h5_file(filename, dataset_name, h5_data);
  return h5_data.data;
}

/**
 * @brief Discovers all datasets in an HDF5 file.
 *
 * @param filename The path to the HDF5 file.
 * @return A vector containing the dataset paths.
 */
std::vector<std::string> discover_datasets(const std::string &filename);

/**
 * @brief Discovers all datasets in a group in an HDF5 file.
 *
 * @param filename The path to the HDF5 file.
 * @param group_name The name of the group to search.
 * @return A vector containing the dataset paths.
 */
std::vector<std::string> get_datasets_in_group(const std::string &filename,
                                               const std::string &group_name);

/**
 * @brief Extracts the dataset name from a full path.
 *
 * @param path The full path to the dataset.
 * @return The name of the dataset.
 */
std::string get_dataset_name(const std::string &path);
