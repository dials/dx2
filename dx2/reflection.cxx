/**
 * @file reflection.cxx
 * @brief Implementation file for ReflectionTable class.
 */

#include "dx2/reflection.hpp"
#include "dx2/utils.hpp"
#include <algorithm>
#include <chrono>
#include <unordered_set>

#pragma region Constructors
// ReflectionTable constructors implementation
ReflectionTable::ReflectionTable() {
  // Generate default experiment IDs and identifiers
  generate_new_attributes();
}

ReflectionTable::ReflectionTable(const std::vector<uint64_t> &experiment_ids,
                                 const std::vector<std::string> &identifiers)
    : experiment_ids(experiment_ids), identifiers(identifiers) {}

ReflectionTable::ReflectionTable(const std::string &h5_filepath)
    : h5_filepath(h5_filepath) {
  auto start = std::chrono::high_resolution_clock::now(); // ⏱ Start timer

  // Discover all datasets in the default reflection group
  std::vector<std::string> datasets =
      get_datasets_in_group(h5_filepath, DEFAULT_REFL_GROUP);

  if (datasets.empty()) {
    dx2_log::warning(
        fmt::format("No datasets found in group '{}'", DEFAULT_REFL_GROUP));
  }

  // Open the HDF5 file
  h5utils::H5File file(
      H5Fopen(h5_filepath.c_str(), H5F_ACC_RDONLY, H5P_DEFAULT));
  if (!file) {
    throw std::runtime_error("Could not open file: " + h5_filepath);
  }

  // Open group and read experiment metadata
  h5utils::H5Group group(
      H5Gopen2(file, DEFAULT_REFL_GROUP.c_str(), H5P_DEFAULT));
  if (group) {
    read_experiment_metadata(group, experiment_ids, identifiers);
  }

  // Loop over every dataset path in the group
  for (const auto &dataset : datasets) {
    std::string dataset_name = get_dataset_name(dataset);

    // Open the specific dataset (within the file opened above)
    h5utils::H5Dataset dataset_id(H5Dopen2(file, dataset.c_str(), H5P_DEFAULT));
    if (!dataset_id) {
      dx2_log::warning(fmt::format("Could not open dataset '{}'", dataset));
      continue;
    }

    try {
      h5dispatch::dispatch_h5_dataset_type(dataset_id, [&](auto tag) {
        using T = typename decltype(tag)::type;
        auto result =
            read_array_with_shape_from_h5_file<T>(h5_filepath, dataset);

        data.push_back(std::make_unique<TypedColumn<T>>(
            dataset_name, result.shape, result.data));

        dx2_log::debug("Loaded column: {} with type {}", dataset_name,
                       typeid(T).name());
      });
    } catch (const std::exception &e) {
      dx2_log::warning(
          fmt::format("Skipping dataset '{}': {}", dataset, e.what()));
      // Continue to the next dataset
    }
  }

  dx2_log::debug("Loaded {} column(s) from group '{}'", data.size(),
                 DEFAULT_REFL_GROUP);

  auto end = std::chrono::high_resolution_clock::now(); // ⏱ End timer
  dx2_log::debug("ReflectionTable loaded in {:.4f}s",
                 std::chrono::duration<double>(end - start).count());
}
#pragma endregion

#pragma region Metadata Access
// Metadata access methods
const std::vector<uint64_t> &ReflectionTable::get_experiment_ids() const {
  return experiment_ids;
}

void ReflectionTable::set_experiment_ids(const std::vector<uint64_t> &ids) {
  experiment_ids = ids;
  max_experiment_id =
      *std::max_element(experiment_ids.begin(), experiment_ids.end());
}

const std::vector<std::string> &ReflectionTable::get_identifiers() const {
  return identifiers;
}

void ReflectionTable::set_identifiers(const std::vector<std::string> &ids) {
  identifiers = ids;
}

std::pair<uint64_t, std::string> ReflectionTable::generate_new_attributes() {
  // Generate a new experiment ID and identifier pair
  uint64_t experiment_id = max_experiment_id++;
  std::string identifier = ersatz_uuid4();

  // Add to the lists
  experiment_ids.push_back(experiment_id);
  identifiers.push_back(identifier);

  dx2_log::debug("Generated new experiment ID: {} and identifier: {}",
                 experiment_id, identifier);

  // Return the new attributes
  return std::make_pair(experiment_id, identifier);
}

std::vector<std::string> ReflectionTable::get_column_names() const {
  std::vector<std::string> names;
  for (const auto &col : data) {
    names.push_back(col->get_name());
  }
  return names;
}
#pragma endregion

#pragma region Private Helper Methods
// Private helper methods
size_t ReflectionTable::get_row_count() const {
  if (data.empty())
    return 0;
  return data.front()->get_shape()[0];
}

void ReflectionTable::merge_into_set(std::unordered_set<size_t> &set,
                                     const std::vector<size_t> &rows) const {
  set.insert(rows.begin(), rows.end());
}
#pragma endregion

#pragma region Selection Methods
// Selection methods
ReflectionTable
ReflectionTable::select(const std::vector<size_t> &selected_rows) const {
  ReflectionTable filtered;
  filtered.h5_filepath = this->h5_filepath;

  for (const auto &col : data) {
    filtered.data.push_back(col->clone_filtered(selected_rows));
  }

  // Copy experiment_ids and identifiers
  filtered.experiment_ids = this->experiment_ids;
  filtered.identifiers = this->identifiers;

  return filtered;
}

ReflectionTable ReflectionTable::select(const std::vector<bool> &mask) const {
  std::vector<size_t> selected_rows;
  for (size_t i = 0; i < mask.size(); ++i) {
    if (mask[i]) {
      selected_rows.push_back(i);
    }
  }
  return select(selected_rows);
}
#pragma endregion

#pragma region Write
// Write method
void ReflectionTable::write(std::string_view filename,
                            std::string_view group) const {
  std::string fname(filename);
  std::string gpath(group);

  // Suppress errors when opening non-existent files, groups, datasets..
  H5ErrorSilencer silencer;

  // 🗂️ Ensure the file exists or create it before writing
  h5utils::H5File file(H5Fopen(fname.c_str(), H5F_ACC_RDWR, H5P_DEFAULT));
  if (!file) {
    file = h5utils::H5File(
        H5Fcreate(fname.c_str(), H5F_ACC_TRUNC, H5P_DEFAULT, H5P_DEFAULT));
    if (!file) {
      throw std::runtime_error("Failed to create or open file: " + fname);
    }
  }

  // Open or create group
  h5utils::H5Group group_id = traverse_or_create_groups(file, gpath);
  if (!group_id) {
    throw std::runtime_error("Failed to create or open group: " + gpath);
  }

  // Check if experiment IDs and identifiers are mapped
  if (experiment_ids.size() != identifiers.size()) {
    dx2_log::warning("Experiment IDs and identifiers not correctly mapped!");
  }

  // Check if ID column exists
  std::vector<std::string> names = get_column_names();
  if (std::find(names.begin(), names.end(), "id") == names.end()) {
    dx2_log::warning("No 'id' column found! Did you forget to add it?");
  }

  // Write metadata
  write_experiment_metadata(group_id, experiment_ids, identifiers);

  // 🔁 Write all columns
  for (const auto &col : data) {
    // 🏗️ Construct full dataset path: group + column name
    const std::string &name = col->get_name();

    // Define a lambda that writes a column of a specific type T
    auto write_col = [&](auto tag) {
      using T = typename decltype(tag)::type;

      // 🧪 Try to cast the typeless base pointer to TypedColumn<T>
      const auto *typed = col->as<T>();
      if (!typed) {
        // This should not happen unless type registry is inconsistent
        dx2_log::error(
            fmt::format("Internal type mismatch for column '{}'", name));
        return;
      }

      // 💾 Call the HDF5 writer to write raw data to file
      // Handle the case of N x 1 shaped data, want it written to disk as
      // shape=(N,) rather than shape=(N,1)
      std::vector<hsize_t> write_shape;
      if ((typed->shape.size() == 2) && (typed->shape[1] == 1)) {
        write_shape = std::vector<hsize_t>({typed->shape[0]});
      } else {
        write_shape =
            std::vector<hsize_t>(typed->shape.begin(), typed->shape.end());
      }
      write_raw_data_to_h5_group<T>(group_id, name, typed->data.data(),
                                    write_shape);
    };

    // 🌀 Dispatch the column type and invoke the write_col lambda
    try {
      h5dispatch::dispatch_column_type(col->get_type(), write_col);
    } catch (const std::exception &e) {
      // ⚠️ If the type is unsupported or an error occurs, print warning
      dx2_log::warning(fmt::format("Skipping column {}: {}", name, e.what()));
    }
  }
}
#pragma endregion
