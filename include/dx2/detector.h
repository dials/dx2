#ifndef DX2_MODEL_DETECTOR_H
#define DX2_MODEL_DETECTOR_H
#include <math.h>
#include <Eigen/Dense>
#include <nlohmann/json.hpp>

using json = nlohmann::json;
using Eigen::Matrix3d;
using Eigen::Vector3d;

// End goal would be to separate everything out by detector panel on data
// as well as model side?

double attenuation_length(double mu, double t0,
                                   Vector3d s1,
                                   Vector3d fast,
                                   Vector3d slow,
                                   Vector3d origin) {
  Vector3d normal = fast.cross(slow);
  double distance = origin.dot(normal);
  if (distance < 0) {
    normal = -normal;
  }
  double cos_t = s1.dot(normal);
  //DXTBX_ASSERT(mu > 0 && cos_t > 0);
  return (1.0 / mu) - (t0 / cos_t + 1.0 / mu) * exp(-mu * t0 / cos_t);
}

class Panel {
// A class to represent a single-panel detector i.e. non-hierarchical
public:
    Panel()=default;
    Panel(json panel_data);
    Matrix3d get_d_matrix() const;
    std::array<double, 2> px_to_mm(double x, double y) const;
    json to_json() const;
protected:
    // panel_frame items
    Vector3d origin_{{0.0,0.0,100.0}}; //needs to be set
    Vector3d fast_axis_{{1.0,0.0,0.0}};
    Vector3d slow_axis_{{0.0,1.0,0.0}};
    Vector3d normal_{{0.0,0.0,1.0}};
    Matrix3d d_{{1,0,0},{0,1,0},{0,0,100.0}};
    Matrix3d D_{{1,0,0},{0,1,0},{0,0,0.01}};
    //double distance_{100.0};
    // panel data
    std::array<double, 2> pixel_size_{{0.075,0.075}};
    std::array<int, 2> image_size_{{0,0}};
    std::array<double, 2> trusted_range_{0.0,65536.0};
    std::string type_{"SENSOR_PAD"};
    std::string name_{"module"};
    // also identifier and material present in dxtbx serialization.
    double thickness_{0.0};
    double mu_{0.0};
    std::array<int, 2> raw_image_offset_{{0,0}}; //what's this?
    // also mask would usually be here - is this what we want still?
    //panel
    double gain_{1.0};
    double pedestal_{0.0};
    std::string pixel_to_mm_strategy_{"SimplePxMmStrategy"}; // just the name here
    bool parallax_correction_ = false;
};


Panel::Panel(json panel_data){
    Vector3d fast{{panel_data["fast_axis"][0], panel_data["fast_axis"][1], panel_data["fast_axis"][2]}};
    Vector3d slow{{panel_data["slow_axis"][0], panel_data["slow_axis"][1], panel_data["slow_axis"][2]}};
    Vector3d origin{{panel_data["origin"][0], panel_data["origin"][1], panel_data["origin"][2]}};
    Matrix3d d_matrix{{fast[0], slow[0], origin[0]},{fast[1], slow[1], origin[1]},{fast[2], slow[2], origin[2]}};
    origin_ = origin;
    fast_axis_ = fast;
    slow_axis_ = slow;
    normal_ = fast_axis_.cross(slow_axis_);
    d_ = d_matrix;
    D_ = d_.inverse();
    pixel_size_ = {{panel_data["pixel_size"][0], panel_data["pixel_size"][1]}};
    image_size_ = {{panel_data["image_size"][0], panel_data["image_size"][1]}};
    trusted_range_ = {{panel_data["trusted_range"][0], panel_data["trusted_range"][1]}};
    type_ = panel_data["type"];
    name_ = panel_data["name"];
    thickness_ = panel_data["thickness"];
    mu_ = panel_data["mu"];
    raw_image_offset_ = {{panel_data["raw_image_offset"][0], panel_data["raw_image_offset"][1]}};
    gain_ = panel_data["gain"];
    pedestal_ = panel_data["pedestal"];
    pixel_to_mm_strategy_ = panel_data["px_mm_strategy"]["type"];
    if (pixel_to_mm_strategy_ != std::string("SimplePxMmStrategy")){
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
    panel_data["mask"] = std::array<int, 0> {};
    panel_data["identifier"]= "";
    panel_data["gain"] = gain_;
    panel_data["pedestal"] = pedestal_,
    panel_data["px_mm_strategy"] = {{"type", "ParallaxCorrectedPxMmStrategy"}};
    return panel_data;
}

Matrix3d Panel::get_d_matrix() const {
    return d_;
}

std::array<double, 2> Panel::px_to_mm(double x, double y) const {
  double x1 = x*pixel_size_[0];
  double x2 = y*pixel_size_[1];
  if (!parallax_correction_){
    return std::array<double, 2>{x1,x2};
  }
  Vector3d fast = d_.col(0);
  Vector3d slow = d_.col(1);
  Vector3d origin = d_.col(2);
  Vector3d s1 = origin + x1*fast + x2*slow;
  s1.normalize();
  double o = attenuation_length(mu_, thickness_, s1, fast, slow, origin);
  double c1 = x1 - (s1.dot(fast))*o;
  double c2 = x2 - (s1.dot(slow))*o;
  return std::array<double, 2>{c1,c2};
}

#endif // DX2_MODEL_DETECTOR_H