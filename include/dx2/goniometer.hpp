#pragma once

#include <Eigen/Dense>
#include <nlohmann/json.hpp>

using json = nlohmann::json;
using Eigen::Matrix3d;
using Eigen::Vector3d;

Matrix3d axis_and_angle_as_matrix(Vector3d axis, double angle,
                                  bool deg = false);

class Goniometer {
  // A class to represent a multi-axis goniometer.
public:
  Goniometer() = default;
  Goniometer(std::vector<Vector3d> axes, std::vector<double> angles,
             std::vector<std::string> names, std::size_t scan_axis);
  Goniometer(json goniometer_data);
  Matrix3d get_setting_rotation() const;
  Matrix3d get_sample_rotation() const;
  Vector3d get_rotation_axis() const;
  json to_json() const;

protected:
  void init(); // Sets the matrices from the axes and angles
  Matrix3d calculate_setting_rotation();
  Matrix3d calculate_sample_rotation();
  Matrix3d sample_rotation_{{1, 0, 0}, {0, 1, 0}, {0, 0, 1}};  // F
  Vector3d rotation_axis_{{1.0, 0.0, 0.0}};                    // R'
  Matrix3d setting_rotation_{{1, 0, 0}, {0, 1, 0}, {0, 0, 1}}; // S
  std::vector<Vector3d> axes_{{1.0, 0.0, 0.0}};
  std::vector<double> angles_{{0.0}};
  std::vector<std::string> names_{"omega"};
  std::size_t scan_axis_{0};
};
