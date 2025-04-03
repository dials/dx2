#ifndef DX2_MODEL_BEAM_H
#define DX2_MODEL_BEAM_H
#include <Eigen/Dense>
#include <memory>
#include <nlohmann/json.hpp>
using Eigen::Vector3d;
using json = nlohmann::json;

/*
This file defines a set of Beam classes.

BeamBase defines most of the class attributes except the wavelength.

The two subclasses are MonochromaticBeam and PolychromaticBeam,
which define a single wavelength or wavelength range respectively.

*/

enum Probe { xray = 1, electron = 2, neutron = 3 };

class BeamBase {
  // A base class for beam objects
public:
  virtual ~BeamBase() = default;
  BeamBase() = default;
  BeamBase(Vector3d direction, double divergence, double sigma_divergence,
           Vector3d polarization_normal, double polarization_fraction,
           double flux, double transmission, Probe probe,
           double sample_to_source_distance);
  Probe get_probe() const;
  std::string get_probe_name() const;

protected:
  void init_from_json(json beam_data);
  void add_to_json(json &beam_data) const;
  Vector3d sample_to_source_direction_{0.0, 0.0,
                                       1.0}; // called direction_ in dxtbx
  double divergence_{0.0}; // "beam divergence - be more specific with name?"
  double sigma_divergence_{0.0}; // standard deviation of the beam divergence
  Vector3d polarization_normal_{0.0, 1.0, 0.0};
  double polarization_fraction_{0.999};
  double flux_{0.0};
  double transmission_{1.0};
  Probe probe_{xray};
  double sample_to_source_distance_{0.0}; // FIXME is this really needed?
};

class MonochromaticBeam : public BeamBase {
  // A monochromatic beam (i.e. single wavelength value)
public:
  MonochromaticBeam() = default;
  MonochromaticBeam(double wavelength);
  MonochromaticBeam(Vector3d s0);
  MonochromaticBeam(double wavelength, Vector3d direction, double divergence,
                    double sigma_divergence, Vector3d polarization_normal,
                    double polarization_fraction, double flux,
                    double transmission, Probe probe,
                    double sample_to_source_distance);
  MonochromaticBeam(json beam_data);
  json to_json() const;
  double get_wavelength() const;
  void set_wavelength(double wavelength);
  Vector3d get_s0() const;
  void set_s0(Vector3d s0);

protected:
  double wavelength_{0.0};
};

class PolychromaticBeam : public BeamBase {
  // A polychromatic beam (i.e. a wavelength range)
public:
  PolychromaticBeam() = default;
  PolychromaticBeam(std::array<double, 2> wavelength_range);
  PolychromaticBeam(std::array<double, 2> wavelength_range, Vector3d direction);
  PolychromaticBeam(std::array<double, 2> wavelength_range, Vector3d direction,
                    double divergence, double sigma_divergence,
                    Vector3d polarization_normal, double polarization_fraction,
                    double flux, double transmission, Probe probe,
                    double sample_to_source_distance);
  PolychromaticBeam(json beam_data);
  json to_json() const;
  std::array<double, 2> get_wavelength_range() const;
  void set_wavelength_range(std::array<double, 2> wavelength_range);

protected:
  std::array<double, 2> wavelength_range_{{0.0, 0.0}};
};

// BeamBase definitions

// full constructor
BeamBase::BeamBase(Vector3d direction, double divergence,
                   double sigma_divergence, Vector3d polarization_normal,
                   double polarization_fraction, double flux,
                   double transmission, Probe probe,
                   double sample_to_source_distance)
    : sample_to_source_direction_{direction}, divergence_{divergence},
      sigma_divergence_{sigma_divergence},
      polarization_normal_{polarization_normal},
      polarization_fraction_{polarization_fraction}, flux_{flux},
      transmission_{transmission}, probe_{probe},
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
  if (beam_data.find("probe") != beam_data.end()) {
    std::string probe = beam_data["probe"];
    if (probe == "x-ray") {
      probe_ = Probe::xray;
    } else if (probe == "neutron") {
      probe_ = Probe::neutron;
    } else if (probe == "electron") {
      probe_ = Probe::electron;
    }
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
  beam_data["probe"] = "x-ray"; // FIXME get_probe_name() not working?
  beam_data["sample_to_source_distance"] = sample_to_source_distance_;
}

Probe BeamBase::get_probe() const { return probe_; }

std::string BeamBase::get_probe_name() const {
  // Return a name that matches NeXus definitions from
  // https://manual.nexusformat.org/classes/base_classes/NXsource.html
  switch (probe_) {
  case xray:
    return std::string("x-ray");
  case electron:
    return std::string("electron");
  case neutron:
    return std::string("neutron");
  default:
    throw std::runtime_error("Unknown probe type");
  }
}

// MonochromaticBeam definitions

// full constructor
MonochromaticBeam::MonochromaticBeam(double wavelength, Vector3d direction,
                                     double divergence, double sigma_divergence,
                                     Vector3d polarization_normal,
                                     double polarization_fraction, double flux,
                                     double transmission, Probe probe,
                                     double sample_to_source_distance)
    : BeamBase{direction,
               divergence,
               sigma_divergence,
               polarization_normal,
               polarization_fraction,
               flux,
               transmission,
               probe,
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
json MonochromaticBeam::to_json() const {
  // create a json object that conforms to a dials model serialization.
  json beam_data = {{"__id__", "monochromatic"}};
  beam_data["wavelength"] = wavelength_;
  add_to_json(beam_data);
  return beam_data;
}

double MonochromaticBeam::get_wavelength() const { return wavelength_; }
void MonochromaticBeam::set_wavelength(double wavelength) {
  wavelength_ = wavelength;
}

Vector3d MonochromaticBeam::get_s0() const {
  return -sample_to_source_direction_ / wavelength_;
}
void MonochromaticBeam::set_s0(Vector3d s0) {
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
                                     double transmission, Probe probe,
                                     double sample_to_source_distance)
    : BeamBase{direction,
               divergence,
               sigma_divergence,
               polarization_normal,
               polarization_fraction,
               flux,
               transmission,
               probe,
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
json PolychromaticBeam::to_json() const {
  // create a json object that conforms to a dials model serialization.
  json beam_data = {{"__id__", "polychromatic"}};
  beam_data["wavelength_range"] = wavelength_range_;
  add_to_json(beam_data);
  return beam_data;
}

std::array<double, 2> PolychromaticBeam::get_wavelength_range() const {
  return wavelength_range_;
}
void PolychromaticBeam::set_wavelength_range(
    std::array<double, 2> wavelength_range) {
  wavelength_range_ = wavelength_range;
}

// Functions to handle run-time polymorphism for json
// serialization/deserialization.

std::shared_ptr<BeamBase> make_beam(json beam_data) {
  if (beam_data.find("wavelength") != beam_data.end()) {
    return std::make_shared<MonochromaticBeam>(beam_data);
  } else if (beam_data.find("wavelength_range") != beam_data.end()) {
    return std::make_shared<PolychromaticBeam>(beam_data);
  } else {
    throw std::runtime_error("Unrecognised beam type in json data");
  }
}

json make_beam_json(std::shared_ptr<BeamBase> beamptr) {
  // first try to cast to mono
  MonochromaticBeam *monobeam =
      dynamic_cast<MonochromaticBeam *>(beamptr.get());
  if (monobeam != nullptr) {
    return (*monobeam).to_json();
  }
  PolychromaticBeam *polybeam =
      dynamic_cast<PolychromaticBeam *>(beamptr.get());
  if (polybeam != nullptr) {
    return (*polybeam).to_json();
  }
  throw std::runtime_error(
      "Unable to cast base beam pointer for json creation");
}

#endif // DX2_MODEL_BEAM_H
