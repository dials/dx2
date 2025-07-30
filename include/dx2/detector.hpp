#ifndef DX2_MODEL_DETECTOR_H
#define DX2_MODEL_DETECTOR_H
#include <Eigen/Dense>
#include <math.h>
#include <nlohmann/json.hpp>

using json = nlohmann::json;
using Eigen::Matrix3d;
using Eigen::Vector3d;

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

/**
 * Apply parallax correction to mm coordinates for conversion back to pixels.
 * This is the reverse of the parallax correction applied in px_to_mm.
 *
 * Given mm coordinates (x,y), construct the ray direction:
 * s₁ = origin + x·fast + y·slow, then normalize |s₁| = 1
 *
 * Calculate attenuation length: o = f(μ, t₀, s₁)
 * Apply correction: x' = x + (s₁·fast)·o, y' = y + (s₁·slow)·o
 *
 * @param mu Linear attenuation coefficient μ (mm⁻¹)
 * @param t0 Sensor thickness t₀ (mm)
 * @param xy The (x,y) mm coordinate to correct
 * @param fast Detector fast direction vector f̂
 * @param slow Detector slow direction vector ŝ
 * @param origin Detector origin vector r₀
 * @return Corrected mm coordinates (x',y') ready for pixel conversion
 */
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

class Panel {
  // A class to represent a single "panel" of a detector (i.e. what data are
  // considered to be described by a single set of panel parameters for the
  // purposes of data processing, which may consist of several real detector
  // modules).
public:
  Panel() = default;
  Panel(json panel_data);
  Matrix3d get_d_matrix() const;
  std::array<double, 2> px_to_mm(double x, double y) const;
  std::array<double, 2> mm_to_px(double x, double y) const;
  std::array<double, 2> get_ray_intersection(Vector3d s1) const;
  std::array<double, 2> get_pixel_size() const;
  json to_json() const;
  Vector3d get_origin() const;
  Vector3d get_fast_axis() const;
  Vector3d get_slow_axis() const;
  Vector3d get_normal() const;
  std::array<double, 2> get_image_size_mm() const;
  double get_directed_distance() const;
  void update(Matrix3d d);

protected:
  // panel_frame items
  Vector3d origin_{{0.0, 0.0, 100.0}}; // needs to be set
  Vector3d fast_axis_{{1.0, 0.0, 0.0}};
  Vector3d slow_axis_{{0.0, 1.0, 0.0}};
  Vector3d normal_{{0.0, 0.0, 1.0}};
  Matrix3d d_{{1, 0, 0}, {0, 1, 0}, {0, 0, 100.0}};
  Matrix3d D_{{1, 0, 0}, {0, 1, 0}, {0, 0, 0.01}};
  // double distance_{100.0};
  //  panel data
  std::array<double, 2> pixel_size_{{0.075, 0.075}};
  std::array<int, 2> image_size_{{0, 0}};
  std::array<double, 2> trusted_range_{0.0, 65536.0};
  std::string type_{"SENSOR_PAD"};
  std::string name_{"module"};
  // also identifier and material present in dxtbx serialization.
  double thickness_{0.0};
  double mu_{0.0};
  std::array<int, 2> raw_image_offset_{{0, 0}}; // what's this?
  // also mask would usually be here - is this what we want still?
  // panel
  double gain_{1.0};
  double pedestal_{0.0};
  std::string pixel_to_mm_strategy_{"SimplePxMmStrategy"}; // just the name here
  bool parallax_correction_ = false;
};

Vector3d Panel::get_origin() const { return origin_; }
Vector3d Panel::get_fast_axis() const { return fast_axis_; }
Vector3d Panel::get_slow_axis() const { return slow_axis_; }
Vector3d Panel::get_normal() const { return normal_; }
std::array<double, 2> Panel::get_image_size_mm() const {
  return {image_size_[0] * pixel_size_[0], image_size_[1] * pixel_size_[1]};
}
double Panel::get_directed_distance() const { return origin_.dot(normal_); }
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

std::array<double, 2> Panel::get_ray_intersection(Vector3d s1) const {
  Vector3d v = D_ * s1;
  // assert v[2] > 0
  std::array<double, 2> pxy;
  pxy[0] = v[0] / v[2];
  pxy[1] = v[1] / v[2];
  // FIXME check is valid
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

/**
 * Convert millimeter coordinates to pixel coordinates.
 * Applies parallax correction if enabled for the panel, then converts to
 * pixels.
 * @param x X coordinate in millimeters
 * @param y Y coordinate in millimeters
 * @return Array containing [x_pixels, y_pixels]
 */
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

/**
 * Get the pixel size for this panel.
 * @return Array containing [x_pixel_size, y_pixel_size] in millimeters
 */
std::array<double, 2> Panel::get_pixel_size() const { return pixel_size_; }

// Define a simple detector, for now is just a vector of panels without any
// hierarchy.
class Detector {
public:
  Detector() = default;
  Detector(json detector_data);
  json to_json() const;
  std::vector<Panel> panels() const;
  void update(Matrix3d d);

protected:
  std::vector<Panel> _panels{};
};

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

#endif // DX2_MODEL_DETECTOR_H