/**
 * @file h5dispatch.hpp
 * @brief Supported column types and their HDF5 mappings.
 *
 * This header provides:
 * - ColumnTypes, the single list of C++ types a reflection table column
 *   can hold, and the compile-time helpers generated from it.
 * - H5TypeTraits, the HDF5 description of each type, and make_h5_type,
 *   which creates a datatype owned by the caller.
 * - dispatch_h5_dataset_type, which maps an HDF5 dataset to a column type.
 */

#pragma once

#include "dx2/h5/h5utils.hpp"
#include "dx2/logging.hpp"
#include <hdf5.h>
#include <iostream>
#include <stdexcept>
#include <type_traits>
#include <variant>

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

template <template <typename> class Wrap, typename List> struct variant_of;
template <template <typename> class Wrap, typename... Ts>
struct variant_of<Wrap, type_list<Ts...>> {
  using type = std::variant<Wrap<Ts>...>;
};

/// std::variant<Wrap<Ts>...> for each Ts in List.
template <template <typename> class Wrap, typename List>
using variant_of_t = typename variant_of<Wrap, List>::type;

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
 * @brief Calls f(type_tag<T>) for each T in a type_list until one returns
 * true.
 *
 * @return true if any call returned true.
 */
template <typename F, typename... Ts>
bool any_of_types(type_list<Ts...>, F &&f) {
  return (f(type_tag<Ts>) || ...);
}

/**
 * @brief Dispatch based on the type of an HDF5 dataset.
 *
 * Matches the dataset's HDF5 class, size and (for integers) sign against
 * H5TypeTraits for each entry in ColumnTypes, and invokes the callback
 * with the first matching type tag.
 *
 * @tparam Callback A callable with templated operator()(type_tag<T>).
 * @param dataset_id The HDF5 dataset to inspect.
 * @param cb The callback to invoke with the matched type tag.
 *
 * @throws std::runtime_error if no column type matches.
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

  bool matched = any_of_types(ColumnTypes{}, [&](auto tag) {
    using T = typename decltype(tag)::type;
    using Traits = H5TypeTraits<T>;
    if (Traits::h5_class != cls || sizeof(T) != size ||
        (cls == H5T_INTEGER && Traits::h5_sign != sign)) {
      return false;
    }
    cb(tag);
    return true;
  });

  if (!matched) {
    dx2_log::error("Unsupported HDF5 dataset type:");
    fmt::print(stderr,
               "  • Class: {}\n"
               "  • Size : {}\n"
               "  • Sign : {}\n",
               static_cast<int>(cls), size, static_cast<int>(sign));
    throw std::runtime_error("No matching column type.");
  }
}

} // namespace h5dispatch
