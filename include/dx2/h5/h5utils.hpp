/**
 * @file h5utils.hpp
 * @brief RAII utilities for safe and scoped management of HDF5
 * resources.
 *
 * This header provides:
 *
 * - `H5Cleanup<D>`: A generic RAII wrapper for HDF5 identifiers,
 *   ensuring automatic resource cleanup via the correct close function.
 * - `H5TypeRef`: A borrowed HDF5 type identifier that `H5Cleanup`
 *   refuses to take ownership of.
 * - Type aliases for commonly used HDF5 object types (e.g., files,
 *   groups, datasets).
 * - `make_v18_file_access_plist`: A file access property list that
 *   allows attributes larger than 64 KiB.
 * - `H5ErrorSilencer`: A RAII guard that temporarily disables HDF5
 *   error output and restores the original handler on scope exit.
 *
 * These utilities are designed to make HDF5 usage in C++ safer, less
 * error-prone, and exception-friendly by tying resource management and
 * error suppression to object lifetime.
 */

#pragma once

#include <hdf5.h>
#include <utility>

namespace h5utils {

/**
 * @brief Non-owning handle to an HDF5 datatype owned elsewhere.
 *
 * Converts implicitly to hid_t so it passes straight into HDF5 calls.
 * Constructing an owning H5Cleanup from it is a compile error, which
 * prevents a second owner closing the type behind the real owner's back.
 * A direct H5Tclose on the converted id is not prevented.
 */
struct H5TypeRef {
  hid_t id;
  operator hid_t() const { return id; }
};

/// RAII wrapper for HDF5 resources to ensure proper cleanup
template <herr_t(D)(hid_t)> struct H5Cleanup {
  H5Cleanup() : id(-1) {}
  explicit H5Cleanup(hid_t id) : id(id) {}
  explicit H5Cleanup(H5TypeRef) = delete;
  H5Cleanup(const H5Cleanup &) = delete;
  H5Cleanup(H5Cleanup &&other) noexcept : id(other.id) { other.id = -1; }
  H5Cleanup &operator=(H5Cleanup &&other) noexcept {
    std::swap(id, other.id);
    return *this;
  }
  ~H5Cleanup() {
    if (id >= 0) {
      D(id);
    }
  }

  operator hid_t() const { return id; }
  explicit operator bool() const { return id >= 0; }
  hid_t id;
};

// Common aliases for specific HDF5 object types
using H5File = H5Cleanup<H5Fclose>;
using H5Group = H5Cleanup<H5Gclose>;
using H5Dataset = H5Cleanup<H5Dclose>;
using H5Attr = H5Cleanup<H5Aclose>;
using H5Space = H5Cleanup<H5Sclose>;
using H5Type = H5Cleanup<H5Tclose>;
using H5Plist = H5Cleanup<H5Pclose>;

/**
 * @brief Create a file access property list that writes the HDF5 1.8
 * file format or newer.
 *
 * Attributes larger than 64 KiB need this format. Files written with it
 * need HDF5 >= 1.8 to read.
 *
 * @return An owning handle to the property list, or an invalid handle if
 * HDF5 could not create or configure it.
 */
inline H5Plist make_v18_file_access_plist() {
  H5Plist fapl(H5Pcreate(H5P_FILE_ACCESS));
  if (fapl &&
      H5Pset_libver_bounds(fapl, H5F_LIBVER_V18, H5F_LIBVER_LATEST) < 0) {
    return H5Plist();
  }
  return fapl;
}

/**
 * @brief RAII guard that temporarily suppresses HDF5 error output.
 *
 * Upon construction, this disables the HDF5 error printing mechanism.
 * When destroyed, it restores the previous error handler.
 *
 * This avoids global suppression and ensures clean restoration even on
 * exceptions.
 */
class H5ErrorSilencer {
public:
  H5ErrorSilencer() {
    H5Eget_auto2(H5E_DEFAULT, &old_func, &old_client_data);
    H5Eset_auto2(H5E_DEFAULT, nullptr, nullptr);
  }

  ~H5ErrorSilencer() { H5Eset_auto2(H5E_DEFAULT, old_func, old_client_data); }

private:
  H5E_auto2_t old_func = nullptr;
  void *old_client_data = nullptr;
};

} // namespace h5utils