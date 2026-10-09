/**
 * @file reflection.hpp
 * @brief Lightweight, type-safe reflection table for HDF5 datasets.
 *
 * This header defines a flexible structure for interacting with HDF5
 * datasets as typed reflection tables. It allows datasets to be loaded
 * into typed columns, filtered by row selection, queried by column, and
 * written back to disk.
 *
 * - `ReflectionTable` manages a collection of columns loaded from an
 *   HDF5 group.
 * - Each column is a `TypedColumn<T>` held in a `std::variant` with one
 *   alternative per type in `h5dispatch::ColumnTypes`, the same model as
 *   the DIALS reflection_table (a dxtbx flex_table over a boost::variant).
 *   A column type outside that list is a compile error.
 * - Support for adding, querying, selecting, and writing columns.
 */
#pragma once

#include "dx2/h5/h5dispatch.hpp"
#include "dx2/h5/h5read_processed.hpp"
#include "dx2/h5/h5utils.hpp"
#include "dx2/h5/h5write.hpp"
#include "dx2/logging.hpp"
#include <array>
#include <chrono>
#include <experimental/mdspan>
#include <functional>
#include <iomanip>
#include <optional>
#include <random>
#include <sstream>
#include <string>
#include <type_traits>
#include <unordered_set>
#include <variant>
#include <vector>

#pragma region Type Helpers
/*
 * Type aliases for centralised control of storage types.
 */

// Internal storage type for the data
template <typename T> using storage_type = std::vector<T>;
// Type alias for mdspan (currently using backport std::experimental::mdspan)
template <typename T>
using mdspan_type =
    std::experimental::mdspan<T, std::experimental::dextents<size_t, 2>>;
#pragma endregion

#pragma region Column Definition
/**
 * @brief A named column of values of one type, stored row-major.
 *
 * A plain value type: copying a column copies its data. The mdspan view is
 * built on demand rather than stored, since a stored view would keep
 * pointing at the source's buffer after a copy.
 *
 * @tparam T A type in h5dispatch::ColumnTypes.
 */
template <typename T> struct TypedColumn {
  using value_type = T;

  /**
   * @brief Constructs a column from a shape and matching flat data.
   *
   * @param name The name of the column.
   * @param shape {rows} or {rows, cols}.
   * @param data Flat vector of values in row-major order.
   * @throws std::runtime_error if the shape is not rank 1 or 2.
   */
  TypedColumn(std::string name, std::vector<size_t> shape, storage_type<T> data)
      : name(std::move(name)), shape(std::move(shape)), data(std::move(data)) {
    if (this->shape.size() != 1 && this->shape.size() != 2) {
      throw std::runtime_error("TypedColumn: unsupported rank " +
                               std::to_string(this->shape.size()));
    }
  }

  size_t rows() const { return shape[0]; }
  size_t cols() const { return shape.size() > 1 ? shape[1] : 1; }

  /// Returns a rows x cols view over the column data.
  mdspan_type<T> span() { return mdspan_type<T>(data.data(), rows(), cols()); }

  /// Returns a read-only rows x cols view over the column data.
  mdspan_type<const T> span() const {
    return mdspan_type<const T>(data.data(), rows(), cols());
  }

  /**
   * @brief Returns a copy of the column keeping only the given rows.
   *
   * @param selected_rows Indices of rows to keep, in output order.
   */
  TypedColumn<T> filtered(const std::vector<size_t> &selected_rows) const {
    storage_type<T> new_data;
    new_data.reserve(selected_rows.size() * cols());
    for (size_t row : selected_rows) {
      auto first = data.begin() + row * cols();
      new_data.insert(new_data.end(), first, first + cols());
    }
    return TypedColumn<T>(name, {selected_rows.size(), cols()},
                          std::move(new_data));
  }

  std::string name;          ///< Name of the dataset
  std::vector<size_t> shape; ///< {rows} or {rows, cols}
  storage_type<T> data;      ///< Row-major values
};
#pragma endregion

#pragma region Predicate Helpers
/**
 * @brief Represents a type-safe row predicate bound to a specific
 * column.
 *
 * This struct is used to associate a named column with a predicate
 * function that inspects values from the corresponding column. It
 * enables statically typed, variadic filtering of table rows.
 *
 * @tparam T The expected type of the column.
 */
template <typename T> struct ColumnPredicate {
  ///< The name of the column to apply the predicate on.
  std::string column_name;
  ///< A predicate function over the column mdspan and row index.
  std::function<bool(const mdspan_type<const T> &, size_t)> predicate;
};

namespace logic {

/**
 * @brief Logical combination strategy for find_rows.
 */
enum class LogicalOp {
  Or, ///< Union of all matching rows (default)
  And ///< Intersection of all matching rows
};

} // namespace logic
#pragma endregion

#pragma region ReflectionTable
class ReflectionTable {
private:
  /// One alternative per entry in h5dispatch::ColumnTypes.
  using Column = h5dispatch::variant_of_t<TypedColumn, h5dispatch::ColumnTypes>;

  // Views returned by column<T>() point into each column's buffer, which
  // survives the column vector reallocating only if columns are moved
  // rather than copied.
  static_assert(std::is_nothrow_move_constructible_v<Column>);

  std::vector<Column> data;
  std::string h5_filepath;
  const std::string DEFAULT_REFL_GROUP = "/dials/processing/group_0";
  std::vector<uint64_t> experiment_ids;
  std::vector<std::string> identifiers;
  uint64_t max_experiment_id = 0;

  /// Get the number of rows in the first column, assuming all columns
  /// have the same number of rows
  size_t get_row_count() const;

  static const std::string &column_name(const Column &col) {
    return std::visit(
        [](const auto &c) -> const std::string & { return c.name; }, col);
  }

  static size_t column_rows(const Column &col) {
    return std::visit([](const auto &c) { return c.rows(); }, col);
  }

  /**
   * @brief Finds the column named `name` holding T.
   *
   * @tparam Self ReflectionTable or const ReflectionTable; the returned
   * pointer carries the same constness.
   * @return The column, or nullptr if no column with this name holds T.
   */
  template <typename T, typename Self>
  static auto find_column(Self &self, const std::string &name)
      -> std::conditional_t<std::is_const_v<Self>, const TypedColumn<T>,
                            TypedColumn<T>> * {
    static_assert(h5dispatch::is_column_type_v<T>,
                  "Unsupported column type; see h5dispatch::ColumnTypes");
    for (auto &col : self.data) {
      auto *typed = std::get_if<TypedColumn<T>>(&col);
      if (typed && typed->name == name) {
        return typed;
      }
    }
    return nullptr;
  }

  /**
   * @brief Merges a vector of row indices into a set.
   *
   * This is used to accumulate matches from multiple predicates
   * during a logical OR combination.
   *
   * @param set The target set to insert into.
   * @param rows The row indices to insert.
   */
  void merge_into_set(std::unordered_set<size_t> &set,
                      const std::vector<size_t> &rows) const;

  /**
   * @brief Evaluate a single ColumnPredicate against the table.
   *
   * This method retrieves the column referenced by the predicate,
   * and evaluates the predicate function for each row. A list of
   * row indices for which the predicate returned true is returned.
   *
   * The column is accessed via `column<T>(...)` which returns an
   * `mdspan` view into the column data. The predicate receives this
   * view and an index `i`, and should return true if that row is to be kept.
   *
   * Example predicate:
   *   [](const auto &span, size_t i) { return span(i, 2) > 1.0; }
   * will filter rows where the third component (z) exceeds 1.0.
   *
   * @tparam T The type of the column.
   * @param condition The column predicate to evaluate.
   * @return A vector of matching row indices.
   * @throws std::runtime_error if the column is missing or of the wrong type.
   */
  template <typename T>
  std::vector<size_t>
  evaluate_column_predicate(const ColumnPredicate<T> &condition) const {
    auto opt = column<T>(condition.column_name);
    if (!opt)
      throw std::runtime_error("Column not found or wrong type: " +
                               condition.column_name);

    const mdspan_type<const T> &span = *opt;
    std::vector<size_t> matches;
    for (size_t i = 0; i < span.extent(0); ++i) {
      if (condition.predicate(span, i)) {
        matches.push_back(i);
      }
    }
    return matches;
  }

  /**
   * @brief Type trait used to ensure all variadic arguments are
   * ColumnPredicate<T>.
   */
  template <typename T> struct is_column_predicate : std::false_type {};

  template <typename T>
  struct is_column_predicate<ColumnPredicate<T>> : std::true_type {};

public:
#pragma region Constructors
  /// Re-exported type aliase for convenience
  using BoolEnum = h5dispatch::BoolEnum;

  ReflectionTable();

  /**
   * @brief Constructs a ReflectionTable with given experiment IDs and
   * identifiers.
   *
   * @param experiment_ids A vector of experiment IDs.
   * @param identifiers A vector of identifiers.
   */
  ReflectionTable(const std::vector<uint64_t> &experiment_ids,
                  const std::vector<std::string> &identifiers);

  /**
   * @brief Constructs a ReflectionTable from an HDF5 file.
   *
   * This constructor loads datasets from the specified HDF5 file and
   * populates the table with the data.
   *
   * @param h5_filepath The path to the HDF5 file.
   */
  ReflectionTable(const std::string &h5_filepath);
#pragma endregion

#pragma region Metadata Access
  /**
   * @brief Get the list of experiment IDs.
   */
  const std::vector<uint64_t> &get_experiment_ids() const;

  /**
   * @brief Set the list of experiment IDs.
   */
  void set_experiment_ids(const std::vector<uint64_t> &ids);

  /**
   * @brief Get the list of identifiers.
   */
  const std::vector<std::string> &get_identifiers() const;

  /**
   * @brief Set the list of identifiers.
   */
  void set_identifiers(const std::vector<std::string> &ids);

  /**
   * @brief Generate a new experiment ID and identifier.
   *
   * This function generates a new experiment ID and a random
   * identifier, adds them to the internal metadata, and returns them as
   * a pair.
   */
  std::pair<uint64_t, std::string> generate_new_attributes();

  /**
   * @brief Get a list of all column names in the table.
   */
  std::vector<std::string> get_column_names() const;

  /**
   * @brief Returns the number of rows (reflections) in the table.
   *
   * All columns are required to share the same row count, so this is the
   * length of any column. Returns 0 if the table has no columns.
   */
  size_t size() const;
#pragma endregion

#pragma region Column Access
  /**
   * @brief Retrieves a view (mdspan) of a typed column by name.
   *
   * Looks for a column matching both the given name and type `T`. On a
   * mutable table the view is writable; on a const table the overload
   * below returns a read-only view.
   *
   * @tparam T The expected type of the column (e.g., int, double).
   * @param name The name of the column to retrieve.
   * @return An optional containing the mdspan view, or std::nullopt if no
   * column with this name holds `T`.
   *
   * @note The view is only valid while the column exists in this table.
   */
  template <typename T>
  std::optional<mdspan_type<T>> column(const std::string &name) {
    if (auto *typed = find_column<T>(*this, name)) {
      return typed->span();
    }
    return std::nullopt;
  }

  /// Read-only view of a typed column on a const table.
  template <typename T>
  std::optional<mdspan_type<const T>> column(const std::string &name) const {
    if (const auto *typed = find_column<T>(*this, name)) {
      return typed->span();
    }
    return std::nullopt;
  }

  /**
   * @brief Finds all rows matching any or all of the given typed
   * predicates.
   *
   * This function accepts any number of typed predicates, each bound to
   * a specific column. It returns the union or intersection of all
   * matching rows, depending on the specified logical operation.
   *
   * Uses a parameter pack to accept a variadic number of predicates,
   * and expands them into individual calls to evaluate_column_predicate.
   * The resulting vectors are combined based on the logical operator.
   *
   * @tparam Predicates Parameter pack of ColumnPredicate<T> types.
   * @param op Logical combination (AND or OR).
   * @param conds The predicates to apply to rows.
   * @return A vector of row indices matching the logical combination.
   */
  template <typename... Predicates>
  std::vector<size_t> find_rows(logic::LogicalOp op,
                                const Predicates &...conds) const {
    // Ensure all predicates are of the correct type
    static_assert((is_column_predicate<Predicates>::value && ...),
                  "All arguments to find_rows must be ColumnPredicate<T>");

    // Expand the parameter pack into a vector of row-index vectors
    std::vector<std::vector<size_t>> all_matches = {
        evaluate_column_predicate(conds)...};
    std::unordered_set<size_t> result_set;

    if (op == logic::LogicalOp::Or) {
      // Combine all matching row indices (set union)
      for (const auto &matches : all_matches) {
        result_set.insert(matches.begin(), matches.end());
      }
    } else if (op == logic::LogicalOp::And) {
      // Combine only common row indices (set intersection)
      if (all_matches.empty())
        return {};

      // Start with the first match set as the initial intersection base
      std::unordered_set<size_t> intersection(all_matches[0].begin(),
                                              all_matches[0].end());

      // Iteratively refine the intersection by retaining only elements
      // that are also present in the next predicate's results
      for (size_t i = 1; i < all_matches.size(); ++i) {
        std::unordered_set<size_t> next(all_matches[i].begin(),
                                        all_matches[i].end());

        // Iterate over current intersection set
        for (auto it = intersection.begin(); it != intersection.end();) {
          // If the current value is not present in the next result set,
          // erase it from the intersection set
          if (next.find(*it) == next.end()) {
            it = intersection.erase(it); // erase returns the next iterator
          } else {
            ++it; // move to the next element if it's still present
          }
        }
      }

      result_set = std::move(intersection);
    }

    return std::vector<size_t>(result_set.begin(), result_set.end());
  }

  /**
   * @brief Finds all rows matching all of the given typed predicates.
   *
   * This overload enables simpler syntax when AND is desired (default).
   * Because default parameters don't work with parameter packs,
   * we provide this wrapper to make the interface intuitive.
   *
   * @tparam Predicates Parameter pack of ColumnPredicate<T> types.
   * @param conds The predicates to apply to rows.
   * @return A vector of row indices matching all predicates.
   */
  template <typename... Predicates>
  std::vector<size_t> find_rows(const Predicates &...conds) const {
    return find_rows(logic::LogicalOp::And, conds...);
  }

  /**
   * @brief Returns a new ReflectionTable with only the specified rows.
   *
   * @param selected_rows The list of row indices to retain.
   * @return A new ReflectionTable containing only the selected rows.
   */
  ReflectionTable select(const std::vector<size_t> &selected_rows) const;

  /**
   * @brief Returns a new ReflectionTable from a boolean mask.
   *
   * Converts a boolean mask into row indices and calls index-based select.
   *
   * @param mask A boolean mask, where `true` indicates a row to keep.
   * @return A new ReflectionTable containing only the selected rows.
   */
  ReflectionTable select(const std::vector<bool> &mask) const;
#pragma endregion

#pragma region Column Modification
  /**
   * @brief Adds a new column to the table with arbitrary shape.
   *
   * This is the most general column-adding interface. You must specify
   * the full shape of the column explicitly. The column is stored using
   * a flat data buffer matching this shape in row-major order.
   *
   * @tparam T The data type of the column (e.g., `double`, `int`,
   * `std::array<T, N>`).
   * @param name The name of the column.
   * @param shape A vector describing the shape of the column (e.g.,
   * `{N}` for 1D, `{N, M}` for 2D).
   * @param column_data A flat vector of `T` values, whose total number
   * of elements must match the product of `shape`.
   *
   * @throws std::runtime_error if shape does not match existing row
   * count (if table is non-empty).
   */
  template <typename T>
  void add_column(const std::string &name, const std::vector<size_t> &shape,
                  const std::vector<T> &column_data) {
    // Check if the type T is a bool. If so, convert to BoolEnum and add column.
    if constexpr (std::is_same_v<T, bool>) {
      std::vector<h5dispatch::BoolEnum> converted(column_data.size());
      for (size_t i = 0; i < column_data.size(); ++i) {
        converted[i] = column_data[i] ? h5dispatch::BoolEnum::TRUE
                                      : h5dispatch::BoolEnum::FALSE;
      }
      add_column<h5dispatch::BoolEnum>(name, shape, converted);
    } else {
      static_assert(h5dispatch::is_column_type_v<T>,
                    "Unsupported column type; see h5dispatch::ColumnTypes");
      TypedColumn<T> col(name, shape, column_data);

      // Check for duplicate column names
      for (const auto &existing_col : data) {
        if (column_name(existing_col) == name) {
          throw std::runtime_error("Column with name already exists: " + name);
        }
      }

      // Ensure row count consistency
      if (!data.empty() && col.rows() != get_row_count()) {
        throw std::runtime_error("Row count mismatch when adding column: " +
                                 name);
      }

      // Add the new column to the table
      data.emplace_back(std::move(col));
    }
  }

  /**
   * @brief Adds a 1D column to the table.
   *
   * This is a convenience overload for scalar or vector-style columns
   * where each row has a single value. Internally calls the shape-based
   * overload with a shape of `{N}`.
   *
   * @tparam T The data type of the column (e.g., `int`, `double`).
   * @param name The name of the column.
   * @param column_data A vector of `T` values, one per row.
   */
  template <typename T>
  void add_column(const std::string &name, const std::vector<T> &column_data) {
    add_column(name, std::vector<size_t>{column_data.size()}, column_data);
  }

  /**
   * @brief Adds a 2D column to the table from row and column dimensions.
   *
   * Use this for matrix-style data where each row is a fixed-length vector.
   * Internally calls the shape-based overload with a shape of `{rows, cols}`.
   *
   * @tparam T The data type of the column elements.
   * @param name The name of the column.
   * @param rows Number of rows (typically must match existing row count).
   * @param cols Number of columns per row.
   * @param column_data Flat vector of values in row-major order (size must be
   * `rows * cols`).
   */
  template <typename T>
  void add_column(const std::string &name, const size_t rows, const size_t cols,
                  const std::vector<T> &column_data) {
    add_column(name, std::vector<size_t>{rows, cols}, column_data);
  }

  /**
   * @brief Replaces the data of an existing column.
   *
   * Unlike `add_column`, this requires a column of the given name to
   * already exist and replaces it in place (preserving column order).
   * The replacement is wholesale, so the element type and shape may
   * change, but the new row count must match the rest of the table.
   *
   * @tparam T The data type of the column.
   * @param name The name of the (existing) column to update.
   * @param shape A vector describing the shape of the new column data.
   * @param column_data A flat vector of `T` values for the column.
   *
   * @throws std::runtime_error if no column with `name` exists or the row
   * count does not match the other columns.
   */
  template <typename T>
  void update_column(const std::string &name, const std::vector<size_t> &shape,
                     const std::vector<T> &column_data) {
    // Check if the type T is a bool. If so, convert to BoolEnum and update.
    if constexpr (std::is_same_v<T, bool>) {
      std::vector<h5dispatch::BoolEnum> converted(column_data.size());
      for (size_t i = 0; i < column_data.size(); ++i) {
        converted[i] = column_data[i] ? h5dispatch::BoolEnum::TRUE
                                      : h5dispatch::BoolEnum::FALSE;
      }
      update_column<h5dispatch::BoolEnum>(name, shape, converted);
    } else {
      static_assert(h5dispatch::is_column_type_v<T>,
                    "Unsupported column type; see h5dispatch::ColumnTypes");

      // Locate the existing column (strict: it must already exist)
      auto it = std::find_if(data.begin(), data.end(), [&](const Column &c) {
        return column_name(c) == name;
      });
      if (it == data.end()) {
        throw std::runtime_error("Column not found for update: " + name);
      }

      TypedColumn<T> col(name, shape, column_data);

      // Ensure row count consistency with the other columns in the table
      for (const auto &other : data) {
        if (column_name(other) != name) {
          if (col.rows() != column_rows(other)) {
            throw std::runtime_error(
                "Row count mismatch when updating column: " + name);
          }
          break;
        }
      }

      // Replace in place, preserving the column's position
      *it = std::move(col);
    }
  }

  /**
   * @brief Replaces an existing 1D column.
   */
  template <typename T>
  void update_column(const std::string &name,
                     const std::vector<T> &column_data) {
    update_column(name, std::vector<size_t>{column_data.size()}, column_data);
  }

  /**
   * @brief Replaces an existing 2D column.
   */
  template <typename T>
  void update_column(const std::string &name, const size_t rows,
                     const size_t cols, const std::vector<T> &column_data) {
    update_column(name, std::vector<size_t>{rows, cols}, column_data);
  }
#pragma endregion

#pragma region Write
  /**
   * @brief Writes all columns to an HDF5 file under the given group.
   *
   * This function iterates through all stored columns and writes each one
   * to disk as its h5dispatch::H5TypeTraits datatype. A column that HDF5
   * fails to write is skipped with a warning.
   *
   * @param filename Output HDF5 file path.
   * @param group Target group inside the HDF5 file (defaults to
   *              /dials/processing/group_0).
   */
  void write(std::string_view filename,
             std::string_view group = "/dials/processing/group_0") const;
#pragma endregion
};

#pragma endregion
