#pragma once

#include <array>
#include <cassert>
#include <execution>
#include <iostream>
#include <mutex>
#include <sstream>
#include <stdexcept>
#include <unordered_set>
#include <vector>

/// Exception thrown when an index is out of bounds.
class OutOfBoundsException : public std::runtime_error {
public:
  OutOfBoundsException(const std::string &message)
      : std::runtime_error(message) {}
};

/// Defines memory layout for the tensor
enum class Layout { RowMajor, ColumnMajor };

/// Base class for type-erased Tensor objects
class BaseTensor {
public:
  virtual ~BaseTensor() = default;
};

/**
 * @class Tensor
 * @brief A generic, multi-dimensional tensor class.
 *
 * This class provides a dynamically allocated, multi-dimensional array
 * with row-major or column-major ordering. Supports efficient element
 * access using variadic templates.
 *
 * @tparam T The data type stored in the tensor (e.g., int, float,
 * double).
 */
template <typename T> class Tensor : public BaseTensor {
public:
#pragma region Constructors
  /**
   * @brief Default constructor.
   *
   * Creates an empty tensor with no shape or data.
   */
  Tensor() = default;

  /**
   * @class Tensor
   * @brief A generic, multi-dimensional tensor class.
   *
   * This class provides a dynamically allocated, multi-dimensional
   * array with row-major or column-major ordering. Supports efficient
   * element access using variadic templates.
   *
   * @tparam T The data type stored in the tensor (e.g., int, float,
   * double).
   */
  explicit Tensor(std::vector<size_t> shape, Layout layout = Layout::RowMajor)
      : shape_(std::move(shape)), layout_(layout),
        strides_(compute_strides(shape_, layout)) {
    data_.resize(size()); // Allocate memory
  }

  /**
   * @brief Constructs a tensor with a given shape and pre-existing
   * data.
   *
   * @param shape The shape of the tensor as a vector of sizes per
   * dimension.
   * @param data A vector containing the pre-initialized data.
   * @param layout The memory layout (RowMajor or ColumnMajor, default
   * is RowMajor).
   * @note The size of `data` must match the product of dimensions in
   * `shape`.
   */
  Tensor(std::vector<size_t> shape, std::vector<T> data,
         Layout layout = Layout::RowMajor)
      : shape_(std::move(shape)), layout_(layout),
        strides_(compute_strides(shape_, layout)), data_(std::move(data)) {
    assert(data_.size() == size() && "Data size must match tensor shape.");
  }

#pragma endregion Constructors
#pragma region Operators

  /**
   * @brief Accesses an element in the tensor (mutable).
   *
   * Uses variadic templates and fold expressions to support arbitrary
   * dimensional indexing, enforcing correct bounds.
   *
   * @param indices The indices for each dimension.
   * @return A reference to the element at the specified indices.
   */
  template <typename... Indices> T &operator()(Indices... indices) {
    static_assert(sizeof...(indices) > 0,
                  "At least one index must be provided.");
    assert(sizeof...(indices) == shape_.size() &&
           "Incorrect number of indices for tensor dimensions.");

    /*
     * The following fold expression expands a parameter pack into an
     * std::array. It effectively converts `operator()(i, j, k, ...)`
     * into an array `{i, j, k, ...}`.
     *
     * - `static_cast<size_t>(indices)...`: Converts each parameter to
     *   `size_t`.
     * - `{...}` is the pack expansion, creating an array of indices.
     *
     * Example: Tensor<int> tensor({3, 3, 3}); tensor(1, 2, 0) =>
     *   std::array<size_t, 3> idx = {1, 2, 0};
     */

    std::array<size_t, sizeof...(indices)> idx{static_cast<size_t>(indices)...};

    check_bounds(idx);
    return data_[compute_offset(idx)];
  }

  /**
   * @brief Accesses an element in the tensor (read-only).
   *
   * @param indices The indices for each dimension.
   * @return A const reference to the element at the specified
   * indices.
   */
  template <typename... Indices> const T &operator()(Indices... indices) const {
    // Call the mutable version of the operator to avoid code duplication
    return const_cast<T &>(
        const_cast<const Tensor *>(this)->operator()(indices...));
  }

#pragma endregion Operators
#pragma region Accessors

  /**
   * @brief Returns the shape of the tensor.
   * @return A constant reference to a vector containing the tensor's
   * shape.
   */
  const std::vector<size_t> &shape() const { return shape_; }

  /**
   * @brief Returns the total number of elements in the tensor.
   * @return The total number of elements.
   */
  size_t size() const {
    size_t total_size = 1;
    for (size_t dim : shape_) {
      total_size *= dim;
    }
    return total_size;
  }

#pragma endregion Accessors
#pragma region Filters

  /**
   * @brief Finds all rows where elements in a given column do not match a
   * predicate.
   *
   * @param col_index The column index to check.
   * @param pred The predicate function to apply to elements.
   * @return An unordered set containing the rows where the predicate is false.
   */
  template <typename Predicate>
  std::unordered_set<size_t> mismatch_rows(size_t col_index,
                                           Predicate pred) const {
    // Ensure that predicate is callable with a parameter of type T
    static_assert(std::is_invocable_r_v<bool, decltype(pred), T>,
                  "Predicate must be callable with a parameter of type T and "
                  "return a bool.");

    std::unordered_set<size_t> mismatching_rows;
    std::mutex mutex;
    std::vector<size_t> row_indices(num_rows());

    // Generate row indices for parallel processing
    std::iota(row_indices.begin(), row_indices.end(), 0);

    std::for_each(std::execution::par, row_indices.begin(), row_indices.end(),
                  [&](size_t row) {
                    size_t index = row * num_cols() + col_index;
                    if (!pred(data_[index])) {
                      std::lock_guard<std::mutex> lock(mutex);
                      mismatching_rows.insert(row);
                    }
                  });

    return mismatching_rows;
  }

  /**
   * @brief Removes rows from the tensor based on a set of row indices.
   *
   * @param mismatching_rows The set of row indices to remove.
   */
  void remove_rows(const std::unordered_set<size_t> &mismatching_rows) {
    std::unordered_set<size_t> co_row_indices;
    std::mutex mutex;

    // Compute all row element indices to remove
    std::for_each(std::execution::par, mismatching_rows.begin(),
                  mismatching_rows.end(), [&](size_t row) {
                    auto indices = compute_co_row_indices(row);
                    std::lock_guard<std::mutex> lock(mutex);
                    co_row_indices.insert(indices.begin(), indices.end());
                  });

    remove_indices(co_row_indices);
  }

  /**
   * @brief Filters rows based on a predicate applied to a column.
   *
   * This function filters rows based on a predicate applied to a
   * specific column. It removes rows that the predicate returns false
   * for and returns the rows that were removed.
   *
   * @param col_index The column index to check.
   * @param pred The predicate function to apply to elements.
   * @return An unordered set containing the removed rows.
   */
  template <typename Predicate>
  std::unordered_set<size_t> filter_rows(size_t col_index, Predicate pred) {
    // Ensure that predicate is callable with a parameter of type T
    static_assert(std::is_invocable_r_v<bool, decltype(pred), T>,
                  "Predicate must be callable with a parameter of type T and "
                  "return a bool.");

    // Get the rows that do not match the predicate
    std::unordered_set<size_t> mismatching_rows =
        mismatch_rows(col_index, pred);

    // Remove the mismatching rows from the tensor
    remove_rows(mismatching_rows);

    return mismatching_rows;
  }

#pragma endregion Filters
#pragma region Modifiers

  /**
   * @brief Resizes the tensor to a new shape.
   *
   * @param new_shape The new shape of the tensor.
   */
  void resize(const std::vector<size_t> &new_shape) {
    std::vector<T> new_data(new_shape[0] * new_shape[1], T{});
    size_t min_rows = std::min(new_shape[0], shape_[0]);
    size_t min_cols = std::min(new_shape[1], shape_[1]);

    // Copy data from old tensor to new tensor
    for (size_t row = 0; row < min_rows; ++row) {
      for (size_t col = 0; col < min_cols; ++col) {
        new_data[row * new_shape[1] + col] = data_[row * shape_[1] + col];
      }
    }

    data_ = std::move(new_data);
    shape_ = new_shape;
    strides_ = compute_strides(shape_, layout_);
  }

  /**
   * @brief Removes elements from the tensor based on a set of indices.
   *
   * @param indices The set of indices to remove.
   */
  void remove_indices(const std::unordered_set<size_t> &indices) {
    data_.erase(
        std::remove_if(std::execution::par_unseq, data_.begin(), data_.end(),
                       [&](const T &value) {
                         size_t idx = &value - &data_[0]; // Compute index
                         return indices.find(idx) != indices.end();
                       }),
        data_.end());

    // Update shape
    shape_[0] = data_.size() / num_cols();
    strides_ = compute_strides(shape_, layout_);
  }

#pragma endregion Modifiers
#pragma region Private members
private:
  std::vector<T> data_;         // Storage for the tensor elements.
  std::vector<size_t> shape_;   // The dimensions of the tensor.
  Layout layout_;               // Memory layout (row-major or column-major).
  std::vector<size_t> strides_; // Strides for fast indexing.

#pragma endregion Private members
#pragma region Private methods

  inline size_t num_rows() const { return shape_.empty() ? 0 : shape_[0]; }
  inline size_t num_cols() const { return shape_.size() < 2 ? 0 : shape_[1]; }

  /**
   * @brief Computes the memory strides for efficient element access.
   *
   * The stride determines how much to jump in memory when moving
   * along a dimension.
   *
   * @param shape The shape of the tensor.
   * @param layout The memory layout (RowMajor or ColumnMajor).
   * @return A vector containing the computed strides.
   *
   * @note function is static as it does not depend on the instance
   */
  static std::vector<size_t> compute_strides(const std::vector<size_t> &shape,
                                             Layout layout) {
    std::vector<size_t> strides(shape.size());
    size_t stride = 1;

    switch (layout) {
    case Layout::RowMajor:
      // Compute strides from last to first
      for (int i = shape.size() - 1; i >= 0; --i) {
        strides[i] = stride;
        stride *= shape[i];
      }
      break;
    case Layout::ColumnMajor:
      // Compute strides from first to last
      for (size_t i = 0; i < shape.size(); ++i) {
        strides[i] = stride;
        stride *= shape[i];
      }
      break;
    }
    return strides;
  }

  /**
   * @brief Ensures that all indices are within bounds.
   *
   * @param indices The array of indices to validate.
   * @throws std::abort() if an index is out of bounds.
   */
  template <size_t N>
  void check_bounds(const std::array<size_t, N> &indices) const {
    // Vector to store indices that are out of bounds
    std::vector<size_t> out_of_bounds_indices;
    out_of_bounds_indices.reserve(indices.size());

    // Check each index
    for (size_t i = 0; i < indices.size(); ++i) {
      if (indices[i] >= shape_[i]) {
        out_of_bounds_indices.push_back(i);
      }
    }

    // If there are any out-of-bounds indices, print an error message and abort
    if (!out_of_bounds_indices.empty()) {
      std::ostringstream oss;
      for (size_t i : out_of_bounds_indices) {
        oss << "Error: Index " << indices[i]
            << " is out of bounds for dimension " << i
            << " (size: " << shape_[i] << ")\n";
      }
      throw OutOfBoundsException(oss.str());
    }
  }

  /**
   * @brief Computes the flattened memory offset from
   * multi-dimensional indices.
   *
   * Given a multi-dimensional index, computes the corresponding 1D
   * index using strides.
   *
   * @param indices The array of multi-dimensional indices.
   * @return The flattened memory offset.
   */
  template <size_t N>
  size_t compute_offset(const std::array<size_t, N> &indices) const {
    size_t offset = 0;
    for (size_t i = 0; i < indices.size(); ++i) {
      offset += indices[i] * strides_[i];
    }
    return offset;
  }

  /**
   * @brief Computes the row index from a flattened index.
   *
   * @param index The flattened index.
   *
   * @return The row index.
   */
  inline size_t compute_row(size_t index) const {
    switch (layout_) {
    case Layout::RowMajor:
      return static_cast<size_t>(index / num_cols());
    case Layout::ColumnMajor:
      return static_cast<size_t>(index % num_rows());
    default:
      throw std::runtime_error("Unsupported layout.");
    }
  }

  /**
   * @brief Calculates the indices of all co-row elements to a given
   * index.
   *
   * @param row The row index to compute co-row indices for.
   *
   * @return An unordered set containing the indices of all co-row elements.
   */
  inline std::unordered_set<size_t> compute_co_row_indices(size_t row) const {
    std::unordered_set<size_t> indices;

    switch (layout_) {
    case Layout::RowMajor:
      // In RowMajor layout, elements of a row are stored contiguously
      for (size_t i = 0; i < num_cols(); ++i) {
        indices.insert(row * num_cols() + i);
      }
      break;
    case Layout::ColumnMajor:
      // In ColumnMajor layout, elements of a column are stored contiguously
      for (size_t i = 0; i < num_cols(); ++i) {
        indices.insert(row + i * num_rows());
      }
    }

    return indices;
  }
};

#pragma endregion Private methods