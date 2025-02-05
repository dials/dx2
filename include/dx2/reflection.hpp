#pragma once

#include "dx2/h5/h5read_processed.hpp"
#include "dx2/tensor.hpp"
#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

class ReflectionTable {
public:
  /**
   * @brief Default constructor
   */
  ReflectionTable() = default;

  /**
   * @brief Constructs a ReflectionTable from an HDF5 file.
   *
   * @param h5_filepath Path to the HDF5 file.
   */
  ReflectionTable(const std::string &h5_filepath) : h5_filepath_(h5_filepath) {
    load_data();
  }

  /**
   * @brief Adds a table to the reflection table.
   *
   * @tparam T The data type of the table.
   * @param table_group The path/name of the table.
   * @param data The data to add to the table.
   * @param shape The shape of the table's data.
   * @param layout The layout of the table's data (Default: Layout::RowMajor).
   */
  template <typename T>
  void add_table(const std::string &table_group, const std::vector<T> data,
                 const std::vector<size_t> shape,
                 Layout layout = Layout::RowMajor) {
    dataset_map_[table_group] =
        std::make_unique<Tensor<T>>(shape, data, layout);
  }

  template <typename T>
  void add_table(const std::string &table_group, H5Data<T> h5_data) {
    dataset_map_[table_group] = std::make_unique<Tensor<T>>(
        h5_data.shape, std::move(h5_data.data), Layout::RowMajor);
  }

  /**
   * @brief Selects reflections based on a predicate applied to a specified
   * column.
   *
   * @param table The table to apply the predicate to.
   * @param column The column to apply the predicate to.
   * @param predicate A function that returns `true` for rows to be kept.
   */
  template <typename Predicate, typename T>
  void select(std::string table, size_t column, Predicate pred);

  /**
   * @brief Retrieves a reference to a column's data.
   *
   * @param column The column to retrieve data from.
   * @return A vector containing the column's data.
   */
  std::vector<double> column(size_t) const;

private:
  /// Dataset path -> Tensor mapping
  std::unordered_map<std::string, std::unique_ptr<BaseTensor>> dataset_map_;
  std::string h5_filepath_;

  /**
   * @brief Loads data from the HDF5 file into the reflection table.
   */
  void load_data() {
    std::vector<std::string> dataset_paths = discover_datasets(h5_filepath_);
    for (const auto &path : dataset_paths) {
      // std::string dataset_name = get_dataset_name(path);
      H5Data<double> h5_data;
      read_array_from_h5_file(h5_filepath_, path, h5_data);
      add_table(path, h5_data);
    }
  }
};
