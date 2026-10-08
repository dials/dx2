/**
 * @file h5dispatch.hpp
 * @brief Central type registry and dispatch utilities for HDF5-C++ interop.
 *
 * This header provides:
 * - A centralised registry of supported C++/HDF5 type mappings.
 * - Functions to dispatch based on HDF5 dataset types or C++ type_index.
 * - A clean extensible structure to support future type additions.
 */

#pragma once

#include "dx2/h5/h5utils.hpp"
#include "dx2/logging.hpp"
#include <hdf5.h>
#include <iostream>
#include <stdexcept>
#include <type_traits>
#include <typeindex>
#include <vector>

using namespace h5utils;

namespace h5dispatch {

/**
 * @brief Template wrapper used to tag types in dispatch callbacks.
 *
 * This enables dispatch functions to pass types as compile-time values.
 */
template <typename T> struct type_tag_t {
  using type = T;
};
template <typename T> constexpr type_tag_t<T> type_tag{};

enum class BoolEnum : uint8_t { FALSE = 0, TRUE = 1 };

/// Compile-time list of types.
template <typename... Ts> struct type_list {};

/**
 * @brief Every C++ type a reflection table column can hold.
 *
 * The single list of supported types. Adding a type means adding it here
 * and giving it an H5TypeTraits specialisation.
 */
using ColumnTypes = type_list<int, int64_t, uint64_t, double, BoolEnum>;

template <typename T, typename List> struct type_list_contains;
template <typename T, typename... Ts>
struct type_list_contains<T, type_list<Ts...>>
    : std::bool_constant<(std::is_same_v<T, Ts> || ...)> {};

/// True if T is one of ColumnTypes.
template <typename T>
inline constexpr bool is_column_type_v =
    type_list_contains<T, ColumnTypes>::value;

/**
 * @brief HDF5 description of a column type.
 *
 * Each specialisation gives the HDF5 class and sign used to recognise a
 * dataset of this type on read, and make(), which creates a new datatype
 * owned by the caller. Predefined H5T_NATIVE_* types cannot be closed, so
 * make() returns a copy that can. A type without a specialisation fails
 * to compile.
 */
template <typename T> struct H5TypeTraits; // Primary template, never defined

#pragma region H5TypeTraits specialisations
template <> struct H5TypeTraits<int> {
  static constexpr H5T_class_t h5_class = H5T_INTEGER;
  static constexpr H5T_sign_t h5_sign = H5T_SGN_2;
  static H5Type make() { return H5Type(H5Tcopy(H5T_NATIVE_INT)); }
};

template <> struct H5TypeTraits<int64_t> {
  static constexpr H5T_class_t h5_class = H5T_INTEGER;
  static constexpr H5T_sign_t h5_sign = H5T_SGN_2;
  static H5Type make() { return H5Type(H5Tcopy(H5T_NATIVE_LLONG)); }
};

template <> struct H5TypeTraits<uint64_t> {
  static constexpr H5T_class_t h5_class = H5T_INTEGER;
  static constexpr H5T_sign_t h5_sign = H5T_SGN_NONE;
  static H5Type make() { return H5Type(H5Tcopy(H5T_NATIVE_ULLONG)); }
};

template <> struct H5TypeTraits<double> {
  static constexpr H5T_class_t h5_class = H5T_FLOAT;
  static constexpr H5T_sign_t h5_sign = H5T_SGN_ERROR;
  static H5Type make() { return H5Type(H5Tcopy(H5T_NATIVE_DOUBLE)); }
};

/// A uint8 enum rather than H5T_NATIVE_HBOOL, which bit-packs.
template <> struct H5TypeTraits<BoolEnum> {
  static constexpr H5T_class_t h5_class = H5T_ENUM;
  static constexpr H5T_sign_t h5_sign = H5T_SGN_NONE;
  static H5Type make() {
    H5Type enum_type(H5Tenum_create(H5T_NATIVE_UINT8));
    uint8_t false_val = 0;
    uint8_t true_val = 1;
    H5Tenum_insert(enum_type, "FALSE", &false_val);
    H5Tenum_insert(enum_type, "TRUE", &true_val);
    return enum_type;
  }
};
#pragma endregion

/**
 * @brief Creates the HDF5 datatype for a column type.
 *
 * @tparam T A type in ColumnTypes.
 * @return A new datatype owned by the caller.
 * @throws std::runtime_error if HDF5 fails to create the type.
 */
template <typename T> H5Type make_h5_type() {
  static_assert(is_column_type_v<T>, "T is not in h5dispatch::ColumnTypes");
  H5Type type = H5TypeTraits<T>::make();
  if (!type) {
    throw std::runtime_error("Failed to create HDF5 datatype.");
  }
  return type;
}

/**
 * @brief Metadata describing a single registered type mapping.
 */
struct H5TypeInfo {
  std::type_index cpp_type; ///< The C++ type_index of the registered type
  H5T_class_t h5_class;     ///< HDF5 class (integer, float, etc.)
  size_t h5_size;           ///< Size in bytes
  H5T_sign_t h5_sign;       ///< Sign info for integers (signed/unsigned)
};

template <typename... Ts>
std::vector<H5TypeInfo> make_type_registry(type_list<Ts...>) {
  return {H5TypeInfo{typeid(Ts), H5TypeTraits<Ts>::h5_class, sizeof(Ts),
                     H5TypeTraits<Ts>::h5_sign}...};
}

/**
 * @brief Runtime registry of the supported type mappings.
 *
 * Generated from ColumnTypes and H5TypeTraits, and holds no HDF5 handles.
 */
inline const std::vector<H5TypeInfo> &get_supported_types() {
  static const std::vector<H5TypeInfo> registry =
      make_type_registry(ColumnTypes{});
  return registry;
}

/**
 * @brief Internal dispatcher used by public-facing dispatchers.
 *
 * Given a matcher function, this tries all registered types and invokes
 * the callback with the matching one.
 *
 * @tparam Callback A callable that takes a type_tag<T>.
 * @tparam MatchFn A predicate function to match an H5TypeInfo entry.
 * @param match_fn The matcher used to find the appropriate type.
 * @param cb The callback to invoke once a match is found.
 *
 * @throws std::runtime_error if no match is found.
 */
template <typename Callback, typename MatchFn>
void dispatch_registered_type(const MatchFn &match_fn, Callback &&cb) {
  for (const auto &entry : get_supported_types()) {
    if (match_fn(entry)) {
      // Dispatch using the appropriate type tag
      if (entry.cpp_type == typeid(int))
        cb(type_tag<int>);
      else if (entry.cpp_type == typeid(int64_t))
        cb(type_tag<int64_t>);
      else if (entry.cpp_type == typeid(uint64_t))
        cb(type_tag<uint64_t>);
      else if (entry.cpp_type == typeid(double))
        cb(type_tag<double>);
      else if (entry.cpp_type == typeid(BoolEnum))
        cb(type_tag<BoolEnum>);
      else
        throw std::runtime_error("Type registered but not dispatchable.");
      return;
    }
  }
  throw std::runtime_error("No matching registered type.");
}

/**
 * @brief Dispatch based on the type of an HDF5 dataset.
 *
 * Inspects the HDF5 type of the dataset and invokes the callback
 * with the appropriate type tag.
 *
 * @tparam Callback A callable with templated operator()(type_tag<T>).
 * @param dataset_id The HDF5 dataset to inspect.
 * @param cb The callback to invoke with the matched type tag.
 *
 * @throws std::runtime_error if no supported match is found.
 */
template <typename Callback>
void dispatch_h5_dataset_type(hid_t dataset_id, Callback &&cb) {
  H5Type type_id(H5Dget_type(dataset_id));
  if (!type_id) {
    throw std::runtime_error("Failed to get dataset type.");
  }

  H5T_class_t cls = H5Tget_class(type_id);
  size_t size = H5Tget_size(type_id);
  H5T_sign_t sign = H5Tget_sign(type_id);

  // Matcher lambda for registered types
  auto matcher = [cls, size, sign](const H5TypeInfo &entry) {
    return entry.h5_class == cls && entry.h5_size == size &&
           (cls != H5T_INTEGER || entry.h5_sign == sign);
  };

  try {
    dispatch_registered_type(matcher, std::forward<Callback>(cb));
  } catch (...) {
    dx2_log::error("Unsupported HDF5 dataset type:");
    fmt::print(stderr,
               "  • Class: {}\n"
               "  • Size : {}\n"
               "  • Sign : {}\n",
               static_cast<int>(cls), size, static_cast<int>(sign));
    throw;
  }
}

/**
 * @brief Dispatch based on a std::type_index (e.g., from a column).
 *
 * Looks up the registered type and invokes the callback with the type tag.
 *
 * @tparam Callback A callable with templated operator()(type_tag<T>).
 * @param t The std::type_index to match.
 * @param cb The callback to invoke with the matched type tag.
 *
 * @throws std::runtime_error if the type is not in the registry.
 */
template <typename Callback>
void dispatch_column_type(const std::type_index &t, Callback &&cb) {
  auto matcher = [t](const H5TypeInfo &entry) { return entry.cpp_type == t; };
  dispatch_registered_type(matcher, std::forward<Callback>(cb));
}

} // namespace h5dispatch