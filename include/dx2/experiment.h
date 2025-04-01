#ifndef DX2_MODEL_EXPERIMENT_H
#define DX2_MODEL_EXPERIMENT_H
#include <Eigen/Dense>
#include <dx2/beam.h>
#include <dx2/crystal.h>
#include <dx2/detector.h>
#include <dx2/goniometer.h>
#include <dx2/scan.h>
#include <nlohmann/json.hpp>
#include <memory>
using Eigen::Vector3d;
using json = nlohmann::json;

class Experiment {
public:
  Experiment() = default;
  Experiment(json experiment_data);
  json to_json() const;
  Goniometer goniometer() const;
  std::shared_ptr<BeamBase> beam() const;
  Scan scan() const;
  Detector detector() const;
  Crystal crystal() const;
  void set_crystal(Crystal crystal);

protected:
  std::shared_ptr<BeamBase> _beam{};
  Scan _scan{};
  Goniometer _goniometer{};
  Detector _detector{};
  Crystal _crystal{};
};


Experiment::Experiment(json experiment_data) {
  json beam_data = experiment_data["beam"][0];
  std::shared_ptr<BeamBase> beam = make_beam(beam_data);
  json scan_data = experiment_data["scan"][0];
  Scan scan(scan_data);
  json gonio_data = experiment_data["goniometer"][0];
  Goniometer gonio(gonio_data);
  json detector_data = experiment_data["detector"][0];
  Detector detector(detector_data);
  this->_beam = beam;
  this->_scan = scan;
  this->_goniometer = gonio;
  this->_detector = detector;
  try { // We don't always have a crystal model e.g. before indexing.
    json crystal_data = experiment_data["crystal"][0];
    Crystal crystal(crystal_data);
    this->_crystal = crystal;
  } catch (...) {
    ;
  }
}

json Experiment::to_json() const {
  // save this experiment as an example experiment list
  json elist_out; // a list of potentially multiple experiments
  elist_out["__id__"] = "ExperimentList";
  json expt_out; // our single experiment
  // no imageset (for now?).
  expt_out["__id__"] = "Experiment";
  expt_out["identifier"] = "test";
  expt_out["beam"] =
      0; // the indices of the models that will correspond to our experiment
  expt_out["detector"] = 0;
  expt_out["goniometer"] = 0;
  expt_out["scan"] = 0;

  // add the the actual models
  elist_out["scan"] = std::array<json, 1>{_scan.to_json()};
  elist_out["goniometer"] = std::array<json, 1>{_goniometer.to_json()};
  elist_out["beam"] = std::array<json, 1>{make_beam_json(_beam)};
  elist_out["detector"] = std::array<json, 1>{_detector.to_json()};

  if (_crystal.get_U_matrix().determinant()) {
    expt_out["crystal"] = 0;
    elist_out["crystal"] = std::array<json, 1>{_crystal.to_json()};
  } else {
    elist_out["crystal"] = std::array<json, 0>{};
  }

  elist_out["experiment"] = std::array<json, 1>{expt_out};
  return elist_out;
}

Scan Experiment::scan() const {
  return _scan;
}

Goniometer Experiment::goniometer() const {
  return _goniometer;
}

Detector Experiment::detector() const {
  return _detector;
}

Crystal Experiment::crystal() const {
  return _crystal;
}

void Experiment::set_crystal(Crystal crystal) {
  _crystal = crystal;
}

std::shared_ptr<BeamBase> Experiment::beam() const {
  return _beam;
}

#endif // DX2_MODEL_EXPERIMENT_H