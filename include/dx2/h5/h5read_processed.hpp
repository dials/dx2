#pragma once

#include <cassert>
#include <chrono>
#include <cstring>
#include <hdf5.h>
#include <hdf5_hl.h>
#include <iostream>
#include <string>
#include <vector>

template <typename T> struct H5Data {
  std::vector<T> data;       // Flat data array
  std::vector<size_t> shape; // Dimensions of the data
};

template <typename T>
std::vector<T> read_array_from_h5_file(const std::string &filename,
                                       const std::string &dataset_name) {
  H5Data<T> h5_data;
  read_array_from_h5_file(filename, dataset_name, h5_data);
  return h5_data.data;
}

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
