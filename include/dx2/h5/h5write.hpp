#pragma once

#include <hdf5.h>
#include <stdexcept>
#include <string>
#include <vector>

/**
 * @brief Recursively traverses or creates groups in an HDF5 file based
 * on the given path.
 *
 * This function takes a parent group identifier and a path string, and
 * recursively traverses or creates the groups specified in the path. If
 * a group in the path does not exist, it is created.
 *
 * @param parent The identifier of the parent group in the HDF5 file.
 * @param path The path of groups to traverse or create, specified as a
 * string with '/' as the delimiter.
 * @return The identifier of the final group in the path.
 * @throws std::runtime_error If a group cannot be created or opened.
 */
hid_t traverse_or_create_groups(hid_t parent, const std::string &path);

/**
 * @brief Deduce the shape of a nested container.
 *
 * This helper function recursively determines the shape of nested
 * containers. This is used to determine the shape of the dataset to be
 * created in an HDF5 file.
 *
 * @tparam Container The type of the container.
 * @param container The container whose shape is to be determined.
 * @return A vector of dimensions representing the shape of the
 * container.
 */
template <typename Container>
std::vector<hsize_t> deduce_shape(const Container &container) {
  if (container.empty()) {
    return {0};
  }
  if constexpr (std::is_arithmetic_v<typename Container::value_type>) {
    // Base case: container holds arithmetic types (e.g., double, int)
    return {container.size()};
  } else {
    // Recursive case: container holds other containers

    // Check that all inner containers have the same size
    size_t inner_size = container.begin()->size();
    for (const auto &sub_container : container) {
      if (sub_container.size() != inner_size) {
        throw std::runtime_error("Cannot deduce shape: inner containers have "
                                 "different sizes.");
      }
    }

    auto sub_shape = deduce_shape(*container.begin());
    sub_shape.insert(sub_shape.begin(), container.size());
    return sub_shape;
  }
}

/**
 * @brief Flatten nested containers into a 1D vector.
 *
 * This helper function recursively flattens nested containers into a 1D
 * vector for writing to HDF5. If the input container is already 1D, it
 * simply returns it.
 *
 * @tparam Container The type of the container.
 * @param container The container to flatten.
 * @return A flat vector containing all elements of the input container
 * in a linear order.
 */
template <typename Container> auto flatten(const Container &container) {
  // Determine the type of the elements in the container
  using ValueType = typename Container::value_type;

  // Base case: If the container holds arithmetic types (e.g., int, double),
  // it is already 1D, so we return a copy of the container as a std::vector.
  if constexpr (std::is_arithmetic_v<ValueType>) {
    return std::vector<ValueType>(container.begin(), container.end());
  } else {
    // Recursive case: The container holds nested containers, so we need to
    // flatten them.

    // Determine the type of elements in the flattened result.
    // This is deduced by recursively calling flatten on the first
    // sub-container.
    using InnerType =
        typename decltype(flatten(*container.begin()))::value_type;

    // Create a vector to store the flattened data
    std::vector<InnerType> flat_data;

    // Iterate over the outer container
    for (const auto &sub_container : container) {
      // Recursively flatten each sub-container
      auto sub_flat = flatten(sub_container);

      // Append the flattened sub-container to the result
      flat_data.insert(flat_data.end(), sub_flat.begin(), sub_flat.end());
    }

    // Return the fully flattened data
    return flat_data;
  }
}

/**
 * @brief Writes multidimensional data to an HDF5 file.
 *
 * This function writes a dataset to an HDF5 file. The dataset's shape
 * is determined dynamically based on the input container.
 *
 * @tparam Container The type of the container holding the data.
 * @param filename The path to the HDF5 file.
 * @param dataset_path The full path to the dataset, including group
 * hierarchies.
 * @param data The data to write to the dataset.
 * @throws std::runtime_error If the dataset cannot be created or data
 * cannot be written.
 */
template <typename Container>
void write_data_to_h5_file(const std::string &filename,
                           const std::string &dataset_path,
                           const Container &data);
