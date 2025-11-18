#pragma once
#include <Eigen/Dense>
#include <math.h>
#include <nlohmann/json.hpp>

using json = nlohmann::json;
using Eigen::Matrix3d;
using Eigen::Vector3d;

double attenuation_length(double mu, double t0, Vector3d s1, Vector3d fast,
                          Vector3d slow, Vector3d origin);

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
                                          Vector3d origin);

class Panel {
  // A class to represent a single "panel" of a detector (i.e. what data are
  // considered to be described by a single set of panel parameters for the
  // purposes of data processing, which may consist of several real detector
  // modules).
public:
  Panel() = default;
  Panel(json panel_data);
  Panel(double distance, std::array<double, 2> beam_center,
        std::array<double, 2> pixel_size, std::array<int, 2> image_size,
        const std::string &fast_axis = "x", const std::string &slow_axis = "-y",
        const double thickness = 0.0, const double mu = 0.0);
  Matrix3d get_d_matrix() const;
  Matrix3d get_D_matrix() const;
  std::array<double, 2> px_to_mm(double x, double y) const;
  std::array<double, 2> mm_to_px(double x, double y) const;
  Vector3d get_lab_coord(double x_mm, double y_mm) const;
  std::optional<std::array<double, 2>>
  get_ray_intersection(const Vector3d &s1) const;
  std::array<double, 2> get_pixel_size() const;
  json to_json() const;
  Vector3d get_origin() const;
  Vector3d get_fast_axis() const;
  Vector3d get_slow_axis() const;
  Vector3d get_normal() const;
  std::array<double, 2> get_image_size_mm() const;
  double get_directed_distance() const;
  bool has_parallax_correction() const;
  bool is_coord_valid_mm(const std::array<double, 2> xy) const;
  double get_mu() const;
  double get_thickness() const;
  void update(Matrix3d d);
  void set_correction_parameters(double thickness, double mu,
                                 bool parallax_correction);

protected:
  // panel_frame items
  Vector3d origin_{{0.0, 0.0, 100.0}}; // needs to be set
  Vector3d fast_axis_{{1.0, 0.0, 0.0}};
  Vector3d slow_axis_{{0.0, 1.0, 0.0}};
  Vector3d normal_{{0.0, 0.0, 1.0}};
  Matrix3d d_{{1, 0, 0}, {0, 1, 0}, {0, 0, 100.0}};
  Matrix3d D_{{1, 0, 0}, {0, 1, 0}, {0, 0, 0.01}};
  //  panel data
  std::array<double, 2> pixel_size_{{0.075, 0.075}};
  std::array<int, 2> image_size_{{0, 0}};
  std::array<double, 2> image_size_mm_{{0, 0}};
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

struct intersection {
  int panel_id;
  std::array<double, 2> xymm;
};

// Define a simple detector, for now is just a vector of panels without any
// hierarchy.
class Detector {
public:
  Detector() = default;
  Detector(json detector_data);
  Detector(std::vector<Panel> panels);
  json to_json() const;
  std::vector<Panel> panels() const;
  std::optional<intersection> get_ray_intersection(const Vector3d &s1) const;
  void update(Matrix3d d);

protected:
  std::vector<Panel> _panels{};
};