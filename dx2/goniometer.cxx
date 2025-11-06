/**
 * @file goniometer.cxx
 * @brief Implementation file for Goniometer class.
 */

#include "dx2/goniometer.hpp"
#include <math.h>

Matrix3d axis_and_angle_as_matrix(Vector3d axis, double angle, bool deg) {
  double q0 = 0.0;
  double q1 = 0.0;
  double q2 = 0.0;
  double q3 = 0.0;
  if (deg) {
    angle *= M_PI / 180.0;
  }
  if (!(std::fmod(angle, 2.0 * M_PI))) {
    q0 = 1.0;
  } else {
    double h = 0.5 * angle;
    q0 = std::cos(h);
    double s = std::sin(h);
    Vector3d n = axis / axis.norm();
    q1 = n[0] * s;
    q2 = n[1] * s;
    q3 = n[2] * s;
  }
  Matrix3d m{{2 * (q0 * q0 + q1 * q1) - 1, 2 * (q1 * q2 - q0 * q3),
              2 * (q1 * q3 + q0 * q2)},
             {2 * (q1 * q2 + q0 * q3), 2 * (q0 * q0 + q2 * q2) - 1,
              2 * (q2 * q3 - q0 * q1)},
             {2 * (q1 * q3 - q0 * q2), 2 * (q2 * q3 + q0 * q1),
              2 * (q0 * q0 + q3 * q3) - 1}};
  return m;
}

void Goniometer::init() {
  // Sets the matrices from the axes and angles
  setting_rotation_ = calculate_setting_rotation();
  sample_rotation_ = calculate_sample_rotation();
  rotation_axis_ = axes_[scan_axis_];
}

Matrix3d Goniometer::calculate_setting_rotation() {
  Matrix3d setting_rotation{{1, 0, 0}, {0, 1, 0}, {0, 0, 1}};
  for (std::size_t i = scan_axis_ + 1; i < axes_.size(); i++) {
    Matrix3d R = axis_and_angle_as_matrix(axes_[i], angles_[i], true);
    setting_rotation = R * setting_rotation;
  }
  return setting_rotation;
}

Matrix3d Goniometer::calculate_sample_rotation() {
  Matrix3d sample_rotation{{1, 0, 0}, {0, 1, 0}, {0, 0, 1}};
  for (std::size_t i = 0; i < scan_axis_; i++) {
    Matrix3d R = axis_and_angle_as_matrix(axes_[i], angles_[i], true);
    sample_rotation = R * sample_rotation;
  }
  return sample_rotation;
}

Goniometer::Goniometer(std::vector<Vector3d> axes, std::vector<double> angles,
                       std::vector<std::string> names, std::size_t scan_axis)
    : axes_{axes.begin(), axes.end()}, angles_{angles.begin(), angles.end()},
      names_{names.begin(), names.end()}, scan_axis_{scan_axis} {
  if (scan_axis_ >= axes_.size()) {
    throw std::invalid_argument(
        "Goniometer scan axis number is out of range of axis length");
  }
  init();
}

Goniometer::Goniometer(Matrix3d sample_rotation, Vector3d rotation_axis,
                       Matrix3d setting_rotation)
    : sample_rotation_{sample_rotation}, rotation_axis_{rotation_axis},
      setting_rotation_{setting_rotation} {}

Matrix3d Goniometer::get_setting_rotation() const { return setting_rotation_; }

Matrix3d Goniometer::get_sample_rotation() const { return sample_rotation_; }

Vector3d Goniometer::get_rotation_axis() const { return rotation_axis_; }

Goniometer::Goniometer(json goniometer_data) {
  // The goniometer data can either be single or multi axis form.
  std::vector<std::string> multi_axis_keys = {"axes", "angles", "names",
                                              "scan_axis"};
  std::vector<std::string> single_axis_keys = {
      "rotation_axis", "fixed_rotation", "setting_rotation"};

  for (const auto &key : multi_axis_keys) {
    if (goniometer_data.find(key) == goniometer_data.end()) {
      // Could be a single axis gonio - they only provide rotation, fixed and
      // setting
      for (const auto &akey : single_axis_keys) {
        if (goniometer_data.find(akey) == goniometer_data.end()) {
          throw std::invalid_argument("Key " + key +
                                      " is missing from the input goniometer "
                                      "JSON - treating as single axis but" +
                                      " key " + akey + " also missing.");
        }
      }
      // We can create from the rotation axis data.
      rotation_axis_ = Vector3d(goniometer_data["rotation_axis"][0],
                                goniometer_data["rotation_axis"][1],
                                goniometer_data["rotation_axis"][2]);
      json setting = goniometer_data["setting_rotation"];
      setting_rotation_ << setting[0], setting[1], setting[2], setting[3],
          setting[4], setting[5], setting[6], setting[7], setting[8]; // F
      json sample = goniometer_data["fixed_rotation"];
      sample_rotation_ << sample[0], sample[1], sample[2], sample[3], sample[4],
          sample[5], sample[6], sample[7], sample[8]; // S
      return;
    }
  }
  std::vector<Vector3d> axes;
  std::vector<std::string> names;
  std::vector<double> angles;
  for (json::iterator it = goniometer_data["axes"].begin();
       it != goniometer_data["axes"].end(); it++) {
    Vector3d axis;
    axis[0] = (*it)[0];
    axis[1] = (*it)[1];
    axis[2] = (*it)[2];
    axes.push_back(axis);
  }
  axes_ = axes;
  for (json::iterator it = goniometer_data["angles"].begin();
       it != goniometer_data["angles"].end(); it++) {
    angles.push_back(*it);
  }
  angles_ = angles;
  for (json::iterator it = goniometer_data["names"].begin();
       it != goniometer_data["names"].end(); it++) {
    names.push_back(*it);
  }
  names_ = names;
  scan_axis_ = goniometer_data["scan_axis"];
  init();
}

json Goniometer::to_json() const {
  json goniometer_data;
  if (axes_.size() > 0) {
    // Multi-axis format
    goniometer_data["axes"] = axes_;
    goniometer_data["angles"] = angles_;
    goniometer_data["names"] = names_;
    goniometer_data["scan_axis"] = scan_axis_;
  } else {
    // Single-axis format
    goniometer_data["rotation_axis"] = rotation_axis_;
    goniometer_data["fixed_rotation"] = std::vector<double>{
        sample_rotation_(0, 0), sample_rotation_(0, 1), sample_rotation_(0, 2),
        sample_rotation_(1, 0), sample_rotation_(1, 1), sample_rotation_(1, 2),
        sample_rotation_(2, 0), sample_rotation_(2, 1), sample_rotation_(2, 2)};
    goniometer_data["setting_rotation"] =
        std::vector<double>{setting_rotation_(0, 0), setting_rotation_(0, 1),
                            setting_rotation_(0, 2), setting_rotation_(1, 0),
                            setting_rotation_(1, 1), setting_rotation_(1, 2),
                            setting_rotation_(2, 0), setting_rotation_(2, 1),
                            setting_rotation_(2, 2)};
  }
  return goniometer_data;
}
