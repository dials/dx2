#include "dx2/reflection.hpp"
#include "dx2/h5/h5read_processed.hpp"
#include "dx2/tensor.hpp"

template <typename Predicate, typename T>
void ReflectionTable::select(std::string table, size_t column, Predicate pred) {
  // Ensure that predicate is callable with a parameter of type T
  static_assert(std::is_invocable_r_v<bool, decltype(pred), T>,
                "Predicate must be callable with a parameter of type T and "
                "return a bool.");

  auto it = dataset_map_.find(table);
  if (it == dataset_map_.end()) {
    throw std::runtime_error("Error: Table not found in reflection table.");
  }

  auto tensor = dynamic_cast<Tensor<T> *>(it->second.get());
  if (tensor == nullptr) {
    throw std::runtime_error("Error: Table is not of the correct type.");
  }

  // Filter rows based on predicate and get rows removed
  auto mismatching_rows = tensor->filter_rows(column, pred);

  // Remove rows from all other tables
  for (auto &pair : dataset_map_) {
    // Skip the table we are filtering
    if (pair.first == table)
      continue;
    // Get the tensor
    auto table_tensor = dynamic_cast<Tensor<T> *>(pair.second.get());
    // Remove rows from the table
    if (table_tensor != nullptr) {
      table_tensor->remove_rows(mismatching_rows);
    }
  }
}