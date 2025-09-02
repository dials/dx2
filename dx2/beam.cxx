/**
 * @file beam.cxx
 * @brief Implementation file for Beam classes.
 */

#include "dx2/beam.hpp"
#include <stdexcept>
#include <string>
#include <vector>

// BeamBase definitions

// full constructor
BeamBase::BeamBase(Vector3d direction, double divergence,
                   double sigma_divergence, Vector3d polarization_normal,
                   double polarization_fraction, double flux,
                   double transmission, double sample_to_source_distance)
    : sample_to_source_direction_{direction}, divergence_{divergence},
      sigma_divergence_{sigma_divergence},
      polarization_normal_{polarization_normal},
      polarization_fraction_{polarization_fraction}, flux_{flux},
      transmission_{transmission},
      sample_to_source_distance_{sample_to_source_distance} {}

void BeamBase::init_from_json(json beam_data) {
  // Load values from a json object.
  // Allow these to not be present so we can load a simple json dict without all
  // these items.
  if (beam_data.find("direction") != beam_data.end()) {
    Vector3d direction{{beam_data["direction"][0], beam_data["direction"][1],
                        beam_data["direction"][2]}};
    sample_to_source_direction_ = direction;
  }
  if (beam_data.find("divergence") != beam_data.end()) {
    divergence_ = beam_data["divergence"];
  }
  if (beam_data.find("sigma_divergence") != beam_data.end()) {
    sigma_divergence_ = beam_data["sigma_divergence"];
  }
  if (beam_data.find("polarization_normal") != beam_data.end()) {
    Vector3d pn{{beam_data["polarization_normal"][0],
                 beam_data["polarization_normal"][1],
                 beam_data["polarization_normal"][2]}};
    polarization_normal_ = pn;
  }
  if (beam_data.find("polarization_fraction") != beam_data.end()) {
    polarization_fraction_ = beam_data["polarization_fraction"];
  }
  if (beam_data.find("flux") != beam_data.end()) {
    flux_ = beam_data["flux"];
  }
  if (beam_data.find("transmission") != beam_data.end()) {
    transmission_ = beam_data["transmission"];
  }
  if (beam_data.find("sample_to_source_distance") != beam_data.end()) {
    sample_to_source_distance_ = beam_data["sample_to_source_distance"];
  }
}

void BeamBase::add_to_json(json &beam_data) const {
  // Add the members to the json object to prepare for serialization.
  beam_data["direction"] = sample_to_source_direction_;
  beam_data["divergence"] = divergence_;
  beam_data["sigma_divergence"] = sigma_divergence_;
  beam_data["polarization_normal"] = polarization_normal_;
  beam_data["polarization_fraction"] = polarization_fraction_;
  beam_data["flux"] = flux_;
  beam_data["transmission"] = transmission_;
  beam_data["sample_to_source_distance"] = sample_to_source_distance_;
}

Vector3d BeamBase::get_polarization_normal() const {
  return polarization_normal_;
}

Vector3d BeamBase::get_sample_to_source_direction() const {
  return sample_to_source_direction_;
}

// MonochromaticBeam definitions

// full constructor
MonochromaticBeam::MonochromaticBeam(double wavelength, Vector3d direction,
                                     double divergence, double sigma_divergence,
                                     Vector3d polarization_normal,
                                     double polarization_fraction, double flux,
                                     double transmission,
                                     double sample_to_source_distance)
    : BeamBase{direction,
               divergence,
               sigma_divergence,
               polarization_normal,
               polarization_fraction,
               flux,
               transmission,
               sample_to_source_distance},
      wavelength_{wavelength} {}

MonochromaticBeam::MonochromaticBeam(double wavelength)
    : wavelength_{wavelength} {}

MonochromaticBeam::MonochromaticBeam(Vector3d s0) {
  double len = s0.norm();
  wavelength_ = 1.0 / len;
  sample_to_source_direction_ = -1.0 * s0 / len;
}

// constructor from json data
MonochromaticBeam::MonochromaticBeam(json beam_data) {
  // minimal required keys
  std::vector<std::string> required_keys = {"wavelength"};
  for (const auto &key : required_keys) {
    if (beam_data.find(key) == beam_data.end()) {
      throw std::invalid_argument("Key " + key +
                                  " is missing from the input beam JSON");
    }
  }
  wavelength_ = beam_data["wavelength"];
  init_from_json(beam_data);
}

// serialize to json format
json MonochromaticBeam::to_json(std::string probe) const {
  // create a json object that conforms to a dials model serialization.
  json beam_data = {{"__id__", "monochromatic"}, {"probe", probe}};
  beam_data["wavelength"] = wavelength_;
  add_to_json(beam_data);
  return beam_data;
}

// Serialize to json format (default probe)
json MonochromaticBeam::to_json() const {
  // Delegate to the overload with the default probe name
  return to_json("x-ray");
}

double MonochromaticBeam::get_wavelength() const { return wavelength_; }

void MonochromaticBeam::set_wavelength(double wavelength) {
  wavelength_ = wavelength;
}

Vector3d MonochromaticBeam::get_s0() const {
  return -sample_to_source_direction_ / wavelength_;
}

void MonochromaticBeam::set_s0(const Vector3d s0) {
  double len = s0.norm();
  wavelength_ = 1.0 / len;
  sample_to_source_direction_ = -1.0 * s0 / len;
}

// PolychromaticBeam definitions

// full constructor
PolychromaticBeam::PolychromaticBeam(std::array<double, 2> wavelength_range,
                                     Vector3d direction, double divergence,
                                     double sigma_divergence,
                                     Vector3d polarization_normal,
                                     double polarization_fraction, double flux,
                                     double transmission,
                                     double sample_to_source_distance)
    : BeamBase{direction,
               divergence,
               sigma_divergence,
               polarization_normal,
               polarization_fraction,
               flux,
               transmission,
               sample_to_source_distance},
      wavelength_range_{wavelength_range} {}

PolychromaticBeam::PolychromaticBeam(std::array<double, 2> wavelength_range)
    : wavelength_range_{wavelength_range} {}

PolychromaticBeam::PolychromaticBeam(std::array<double, 2> wavelength_range,
                                     Vector3d direction)
    : wavelength_range_{wavelength_range} {
  sample_to_source_direction_ = direction / direction.norm();
}

// constructor from json data
PolychromaticBeam::PolychromaticBeam(json beam_data) {
  // minimal required keys
  std::vector<std::string> required_keys = {"wavelength_range"};
  for (const auto &key : required_keys) {
    if (beam_data.find(key) == beam_data.end()) {
      throw std::invalid_argument("Key " + key +
                                  " is missing from the input beam JSON");
    }
  }
  wavelength_range_ = beam_data["wavelength_range"];
  init_from_json(beam_data);
}

// serialize to json format
json PolychromaticBeam::to_json(std::string probe) const {
  // create a json object that conforms to a dials model serialization.
  json beam_data = {{"__id__", "polychromatic"}, {"probe", probe}};
  beam_data["wavelength_range"] = wavelength_range_;
  add_to_json(beam_data);
  return beam_data;
}

// Serialize to json format (default probe)
json PolychromaticBeam::to_json() const {
  // Delegate to the overload with the default probe name
  return to_json("x-ray");
}

std::array<double, 2> PolychromaticBeam::get_wavelength_range() const {
  return wavelength_range_;
}

void PolychromaticBeam::set_wavelength_range(
    std::array<double, 2> wavelength_range) {
  wavelength_range_ = wavelength_range;
}

// MonoXrayBeam definitions

json MonoXrayBeam::to_json() const {
  // call the parent function with the correct probe name (which is the same as
  // the default in this case)
  return MonochromaticBeam::to_json("x-ray");
}

// MonoElectronBeam definitions

json MonoElectronBeam::to_json() const {
  // call the parent function with the correct probe name
  return MonochromaticBeam::to_json("electron");
}
