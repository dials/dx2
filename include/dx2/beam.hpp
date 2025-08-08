#pragma once
#include <Eigen/Dense>
#include <nlohmann/json.hpp>

using Eigen::Vector3d;
using json = nlohmann::json;

/*
This file defines a set of Beam classes.

BeamBase defines most of the class attributes except the wavelength.

The two subclasses are MonochromaticBeam and PolychromaticBeam,
which define a single wavelength or wavelength range respectively.

MonochromaticBeam is subclassed further into MonoXrayBeam and
MonoElectronBeam, these simply set the correct probe name
when serializing/deserializing to/from json.
*/

class BeamBase {
  // A base class for beam objects
public:
  BeamBase() = default;
  BeamBase(Vector3d direction, double divergence, double sigma_divergence,
           Vector3d polarization_normal, double polarization_fraction,
           double flux, double transmission, double sample_to_source_distance);
  Vector3d get_polarization_normal() const;
  Vector3d get_sample_to_source_direction() const;

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
                    double transmission, double sample_to_source_distance);
  MonochromaticBeam(json beam_data);
  json to_json(std::string probe) const;
  double get_wavelength() const;
  void set_wavelength(double wavelength);
  Vector3d get_s0() const;
  void set_s0(Vector3d const s0);

protected:
  double wavelength_{0.0};
};

class MonoXrayBeam : public MonochromaticBeam {
  // Same as the parent class, except explicitly set a probe type when calling
  // to_json
  using MonochromaticBeam::MonochromaticBeam;

public:
  json to_json() const;
};

class MonoElectronBeam : public MonochromaticBeam {
  // Same as the parent class, except explicitly set a probe type when calling
  // to_json
  using MonochromaticBeam::MonochromaticBeam;

public:
  json to_json() const;
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
                    double flux, double transmission,
                    double sample_to_source_distance);
  PolychromaticBeam(json beam_data);
  json to_json(std::string probe) const;
  std::array<double, 2> get_wavelength_range() const;
  void set_wavelength_range(std::array<double, 2> wavelength_range);

protected:
  std::array<double, 2> wavelength_range_{{0.0, 0.0}};
};
