#pragma once

#include <experimental/mdspan>
#include <map>
#include <memory>
#include <string>
#include <variant>
#include <vector>

namespace stdex = std::experimental;

/**
 * @brief Base struct for type-erased HDF5 data.
 */
struct BaseH5Data {
  virtual ~BaseH5Data() = default;
};

/**
 * @brief Struct to store dataset data and its `mdspan` view dynamically.
 */
template <typename T> struct H5Data : public BaseH5Data {
  std::vector<T> data;
  std::vector<size_t> shape;
  // Use `mdspan` to provide a view of the data
  stdex::mdspan<T, stdex::dextents<size_t, stdex::dynamic_extent>> view;

  H5Data(std::vector<T> data_, std::vector<size_t> shape_)
      : data(std::move(data_)), shape(std::move(shape_)),
        view(data.data(), shape_.data()) {}
};

/**
 * @brief Reads an HDF5 dataset dynamically, determining its type and shape at
 * runtime.
 *
 * @param filename The path to the HDF5 file.
 * @param dataset_name The name of the dataset.
 * @return A unique pointer to the dataset.
 */
std::unique_ptr<BaseH5Data>
read_dataset_from_h5_file(const std::string &filename,
                          const std::string &dataset_name);

template <typename T>
std::unique_ptr<H5Data<T>>
read_dataset_from_h5_file_t(hid_t dataset, hid_t dataspace, size_t num_elements,
                            const std::vector<size_t> &shape) {
  std::vector<T> data(num_elements);

  hid_t native_type = (std::is_same_v<T, int>)     ? H5T_NATIVE_INT
                      : (std::is_same_v<T, float>) ? H5T_NATIVE_FLOAT
                                                   : H5T_NATIVE_DOUBLE;

  herr_t status =
      H5Dread(dataset, native_type, H5S_ALL, H5S_ALL, H5P_DEFAULT, data.data());
  if (status < 0) {
    throw std::runtime_error("Error: Unable to read dataset.");
  }

  return std::make_unique<H5Data<T>>(std::move(data), shape);
}

/**
 * @brief Reads an array from an HDF5 file.
 *
 * @param filename The path to the HDF5 file.
 * @param dataset_name The name of the dataset.
 * @return A vector containing the data.
 */
template <typename T>
std::vector<T> read_array_from_h5_file(const std::string &filename,
                                       const std::string &dataset_name) {
  auto h5_data =
      read_dataset_from_h5_file_t<T>(dataset, dataspace, num_elements, shape);
  auto h5_data_t = dynamic_cast<H5Data<T> *>(h5_data.get());
  if (h5_data_t == nullptr) {
    throw std::runtime_error("Error: Dataset is not of the correct type.");
  }

  return h5_data_t->data;
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
