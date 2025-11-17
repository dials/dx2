#include "dx2/utils.hpp"
#include <Eigen/Dense>
#include <math.h>
#include <random>
#include <sstream>
#include <iomanip>

using Eigen::Vector3d;

double angle_between_vectors_degrees(Vector3d v1, Vector3d v2) {
  double l1 = v1.norm();
  double l2 = v2.norm();
  double dot = v1.dot(v2);
  double normdot = dot / (l1 * l2);
  if (std::abs(normdot - 1.0) < 1E-6) {
    return 0.0;
  }
  if (std::abs(normdot + 1.0) < 1E-6) {
    return 180.0;
  }
  double angle = std::acos(normdot) * 180.0 / M_PI;
  return angle;
}

/**
   * @brief Generate a pseudo-random UUID-like identifier.
   *
   * This function replicates the behaviour of the Python function
   * `ersatz_uuid4` from the dxtbx library. It generates a 128-bit
   * random value and formats it as a UUID-style string using
   * little-endian byte order, without enforcing RFC 4122 compliance.
   *
   * The output is a 36-character string in the format:
   * `xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx`, where each `x` is a
   * hexadecimal digit.
   *
   * @return A string representing the generated UUID-like identifier.
   *
   * @note This function does not set the version or variant bits as
   *       specified in RFC 4122. It is intended for internal use where
   *       uniqueness is sufficient, and compliance with UUID standards
   *       is unnecessary.
   */
std::string ersatz_uuid4() {
  // Generate 16 random bytes
  std::array<unsigned char, 16> bytes;
  std::random_device rd;
  std::uniform_int_distribution<int> dist(0, 255);
  for (auto &b : bytes) {
    b = static_cast<unsigned char>(dist(rd));
  }

  // Convert bytes to a single 128-bit hex string (little endian)
  std::ostringstream oss;
  for (auto it = bytes.rbegin(); it != bytes.rend(); ++it) {
    oss << std::hex << std::setw(2) << std::setfill('0')
        << static_cast<int>(*it);
  }
  std::string hex = oss.str();

  // Format as UUID: 8-4-4-4-12
  std::ostringstream uuid;
  uuid << hex.substr(0, 8) << "-" << hex.substr(8, 4) << "-"
       << hex.substr(12, 4) << "-" << hex.substr(16, 4) << "-"
       << hex.substr(20, 12);

  return uuid.str();
}