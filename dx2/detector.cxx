/**
 * @file detector.cxx
 * @brief Implementation file for Detector and Panel classes.
 */

#include "dx2/detector.hpp"
#include <set>
#include <optional>

double attenuation_length(double mu, double t0, Vector3d s1, Vector3d fast,
                          Vector3d slow, Vector3d origin) {
  Vector3d normal = fast.cross(slow);
  double distance = origin.dot(normal);
  if (distance < 0) {
    normal = -normal;
  }
  double cos_t = s1.dot(normal);
  // DXTBX_ASSERT(mu > 0 && cos_t > 0);
  return (1.0 / mu) - (t0 / cos_t + 1.0 / mu) * exp(-mu * t0 / cos_t);
}

std::array<double, 2> parallax_correction(double mu, double t0,
                                          std::array<double, 2> xy,
                                          Vector3d fast, Vector3d slow,
                                          Vector3d origin) {
  // Construct ray direction: s₁ = r₀ + x·f̂ + y·ŝ
  Vector3d ray_direction = origin + xy[0] * fast + xy[1] * slow;

  // Normalize to unit vector: |s₁| = 1
  ray_direction.normalize();

  // Calculate attenuation length using sensor physics
  double attenuation_offset =
      attenuation_length(mu, t0, ray_direction, fast, slow, origin);

  // Apply parallax correction:
  // x' = x + (s₁·f̂)·o  (correction along fast axis)
  // y' = y + (s₁·ŝ)·o  (correction along slow axis)
  double corrected_x = xy[0] + (ray_direction.dot(fast)) * attenuation_offset;
  double corrected_y = xy[1] + (ray_direction.dot(slow)) * attenuation_offset;

  return std::array<double, 2>{corrected_x, corrected_y};
}

// Panel implementations

Vector3d Panel::get_origin() const { return origin_; }

Vector3d Panel::get_fast_axis() const { return fast_axis_; }

Vector3d Panel::get_slow_axis() const { return slow_axis_; }

Vector3d Panel::get_normal() const { return normal_; }

std::array<double, 2> Panel::get_image_size_mm() const {
  return {image_size_[0] * pixel_size_[0], image_size_[1] * pixel_size_[1]};
}

double Panel::get_directed_distance() const { return origin_.dot(normal_); }

bool Panel::has_parallax_correction() const { return parallax_correction_; }

double Panel::get_mu() const { return mu_; }

double Panel::get_thickness() const { return thickness_; }

void Panel::update(Matrix3d d) {
  d_ = d;
  D_ = d_.inverse();
  fast_axis_ = {d(0, 0), d(1, 0), d(2, 0)};
  slow_axis_ = {d(0, 1), d(1, 1), d(2, 1)};
  origin_ = {d(0, 2), d(1, 2), d(2, 2)};
  normal_ = fast_axis_.cross(slow_axis_);
}

Panel::Panel(json panel_data) {
  Vector3d fast{{panel_data["fast_axis"][0], panel_data["fast_axis"][1],
                 panel_data["fast_axis"][2]}};
  Vector3d slow{{panel_data["slow_axis"][0], panel_data["slow_axis"][1],
                 panel_data["slow_axis"][2]}};
  Vector3d origin{{panel_data["origin"][0], panel_data["origin"][1],
                   panel_data["origin"][2]}};
  Matrix3d d_matrix{{fast[0], slow[0], origin[0]},
                    {fast[1], slow[1], origin[1]},
                    {fast[2], slow[2], origin[2]}};
  origin_ = origin;
  fast_axis_ = fast;
  slow_axis_ = slow;
  normal_ = fast_axis_.cross(slow_axis_);
  d_ = d_matrix;
  D_ = d_.inverse();
  pixel_size_ = {{panel_data["pixel_size"][0], panel_data["pixel_size"][1]}};
  image_size_ = {{panel_data["image_size"][0], panel_data["image_size"][1]}};
  image_size_mm_ = {{image_size_[0] * pixel_size_[0], image_size_[1] * pixel_size_[1]}};
  trusted_range_ = {
      {panel_data["trusted_range"][0], panel_data["trusted_range"][1]}};
  type_ = panel_data["type"];
  name_ = panel_data["name"];
  thickness_ = panel_data["thickness"];
  mu_ = panel_data["mu"];
  raw_image_offset_ = {
      {panel_data["raw_image_offset"][0], panel_data["raw_image_offset"][1]}};
  gain_ = panel_data["gain"];
  pedestal_ = panel_data["pedestal"];
  pixel_to_mm_strategy_ = panel_data["px_mm_strategy"]["type"];
  if (pixel_to_mm_strategy_ != std::string("SimplePxMmStrategy")) {
    parallax_correction_ = true;
  }
}

json Panel::to_json() const {
  json panel_data;
  panel_data["name"] = name_;
  panel_data["type"] = type_;
  panel_data["fast_axis"] = fast_axis_;
  panel_data["slow_axis"] = slow_axis_;
  panel_data["origin"] = origin_;
  panel_data["raw_image_offset"] = raw_image_offset_;
  panel_data["image_size"] = image_size_;
  panel_data["pixel_size"] = pixel_size_;
  panel_data["trusted_range"] = trusted_range_;
  panel_data["thickness"] = thickness_;
  panel_data["mu"] = mu_;
  panel_data["mask"] = std::array<int, 0>{};
  panel_data["identifier"] = "";
  panel_data["gain"] = gain_;
  panel_data["pedestal"] = pedestal_,
  panel_data["px_mm_strategy"] = {{"type", "ParallaxCorrectedPxMmStrategy"}};
  return panel_data;
}

Matrix3d Panel::get_d_matrix() const { return d_; }

Matrix3d Panel::get_D_matrix() const { return D_; }

std::optional<std::array<double, 2>> Panel::get_ray_intersection(const Vector3d &s1) const {
  Vector3d v = D_ * s1;
  if (v[2] <= 0){
    return {};
  }
  std::array<double, 2> pxy;
  pxy[0] = v[0] / v[2];
  pxy[1] = v[1] / v[2];
  /** Check if the coordinate is invalid */
  if (pxy[0] < 0 || pxy[1] < 0 || pxy[0] > image_size_mm_[0] || pxy[1] > image_size_mm_[1]){
    return {};
  }
  return pxy; // in mmm
}

std::array<double, 2> Panel::px_to_mm(double x, double y) const {
  double x1 = x * pixel_size_[0];
  double x2 = y * pixel_size_[1];
  if (!parallax_correction_) {
    return std::array<double, 2>{x1, x2};
  }
  Vector3d fast = d_.col(0);
  Vector3d slow = d_.col(1);
  Vector3d origin = d_.col(2);
  Vector3d s1 = origin + x1 * fast + x2 * slow;
  s1.normalize();
  double o = attenuation_length(mu_, thickness_, s1, fast, slow, origin);
  double c1 = x1 - (s1.dot(fast)) * o;
  double c2 = x2 - (s1.dot(slow)) * o;
  return std::array<double, 2>{c1, c2};
}

// Input x and y are in mm
Vector3d Panel::get_lab_coord(double x_mm, double y_mm) const {
  return d_ * Vector3d(x_mm, y_mm, 1.0);
}

std::array<double, 2> Panel::mm_to_px(double x, double y) const {
  std::array<double, 2> mm_coord{x, y};

  if (parallax_correction_) {
    // Extract detector geometry
    Vector3d fast = d_.col(0);   // Fast axis direction
    Vector3d slow = d_.col(1);   // Slow axis direction
    Vector3d origin = d_.col(2); // Panel origin position
    mm_coord =
        parallax_correction(mu_, thickness_, mm_coord, fast, slow, origin);
  }

  // Convert mm to pixels by dividing by pixel size
  double pixel_x = mm_coord[0] / pixel_size_[0];
  double pixel_y = mm_coord[1] / pixel_size_[1];

  return std::array<double, 2>{pixel_x, pixel_y};
}

std::array<double, 2> Panel::get_pixel_size() const { return pixel_size_; }

const std::set<std::string> valid_axes = {"x", "-x", "y", "-y"};
const std::map<std::string, Vector3d> axis_map = {
    {"x", Vector3d(1.0, 0.0, 0.0)},
    {"-x", Vector3d(-1.0, 0.0, 0.0)},
    {"y", Vector3d(0.0, 1.0, 0.0)},
    {"-y", Vector3d(0.0, -1.0, 0.0)}};
Panel::Panel(double distance, std::array<double, 2> beam_center,
             std::array<double, 2> pixel_size, std::array<int, 2> image_size,
             const std::string &fast_axis, const std::string &slow_axis,
             double thickness, double mu)
    : pixel_size_(pixel_size), image_size_(image_size), thickness_(thickness),
      mu_(mu) {
  if (valid_axes.find(fast_axis) == valid_axes.end()) {
    throw std::invalid_argument("Invalid fast_axis: " + fast_axis);
  }
  if (valid_axes.find(slow_axis) == valid_axes.end()) {
    throw std::invalid_argument("Invalid fast_axis: " + slow_axis);
  }
  image_size_mm_[0] = image_size_[0] * pixel_size_[0];
  image_size_mm_[1] = image_size_[1] * pixel_size_[1];
  origin_ = {0., 0., -1.0 * distance};
  fast_axis_ = axis_map.find(fast_axis)->second;
  slow_axis_ = axis_map.find(slow_axis)->second;
  origin_ -= beam_center[0] * pixel_size_[0] * fast_axis_;
  origin_ -= beam_center[1] * pixel_size_[1] * slow_axis_;
  normal_ = fast_axis_.cross(slow_axis_);
  Matrix3d d_matrix{{fast_axis_[0], slow_axis_[0], origin_[0]},
                    {fast_axis_[1], slow_axis_[1], origin_[1]},
                    {fast_axis_[2], slow_axis_[2], origin_[2]}};
  d_ = d_matrix;
  D_ = d_.inverse();
  // If mu and thickness are given, default assumption is to turn on parallax
  // correction.
  if (mu_ > 0.0 && thickness_ > 0.0) {
    parallax_correction_ = true;
    std::string pixel_to_mm_strategy_{"ParallaxCorrectedPxMmStrategy"};
  }
}

void Panel::set_correction_parameters(double thickness, double mu,
                                      bool parallax_correction = true) {
  thickness_ = thickness;
  mu_ = mu;
  if (parallax_correction) {
    parallax_correction_ = true;
    pixel_to_mm_strategy_ = "ParallaxCorrectedPxMmStrategy";
  } else {
    parallax_correction_ = false;
    pixel_to_mm_strategy_ = "SimplePxMmStrategy";
  }
}

// Detector implementations

Detector::Detector(json detector_data) {
  json panel_data = detector_data["panels"];
  for (json::iterator it = panel_data.begin(); it != panel_data.end(); ++it) {
    _panels.push_back(Panel(*it));
  }
}

json Detector::to_json() const {
  json detector_data;
  std::vector<json> panels_array;
  for (auto p = _panels.begin(); p != _panels.end(); ++p) {
    panels_array.push_back(p->to_json());
  }
  detector_data["panels"] = panels_array;
  return detector_data;
}

std::vector<Panel> Detector::panels() const { return _panels; }

void Detector::update(Matrix3d d) { _panels[0].update(d); }

std::optional<intersection> Detector::get_ray_intersection(const Vector3d &s1) const {
  double w_max = 0;
  std::array<double, 2> pxy = {0,0};
  int panel_id = -1;
  for (int i=0;i<_panels.size();++i){
    const Panel& p = _panels[i];
    Vector3d v = p.get_D_matrix() * s1;
    if (v[2] > w_max){
      auto intersect = p.get_ray_intersection(s1);
      if (intersect.has_value()){
        pxy = intersect.value();
        w_max = v[2];
        panel_id = i;
      }
    }
  }
  if (w_max == 0.0 || panel_id == -1){
    return {};
  }
  return intersection{panel_id, pxy};
}
