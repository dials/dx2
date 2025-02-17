#pragma once

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