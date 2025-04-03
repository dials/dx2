#ifndef DX2_MODEL_EXPERIMENT_H
#define DX2_MODEL_EXPERIMENT_H
#include <Eigen/Dense>
#include <dx2/beam.h>
#include <dx2/crystal.h>
#include <dx2/detector.h>
#include <dx2/goniometer.h>
#include <dx2/scan.h>
#include <memory>
#include <nlohmann/json.hpp>
using Eigen::Vector3d;
using json = nlohmann::json;

class Experiment {
public:
  Experiment() = default;
  Experiment(json experiment_data);
  Experiment(std::shared_ptr<BeamBase> beam, Scan scan, Goniometer gonio, Detector detector);
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

Experiment::Experiment(std::shared_ptr<BeamBase> beam,
                       Scan scan,
                       Goniometer gonio,
                       Detector detector){
    this->_beam = beam;
    this->_scan = scan;
    this->_goniometer = gonio;
    this->_detector = detector;
  }

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

std::shared_ptr<BeamBase> Experiment::beam() const { return _beam; }

Scan Experiment::scan() const { return _scan; }

Goniometer Experiment::goniometer() const { return _goniometer; }

Detector Experiment::detector() const { return _detector; }

Crystal Experiment::crystal() const { return _crystal; }

void Experiment::set_crystal(Crystal crystal) { _crystal = crystal; }


template<typename BeamType> class TypedExperiment {
  public:
    TypedExperiment(BeamType& beam, Scan scan, Goniometer gonio, Detector detector);
    BeamType& get_beam() const;
    Scan scan() const;
    Detector detector() const;
    Goniometer goniometer() const;
    Crystal crystal() const;
    void set_crystal(Crystal crystal);
    json to_json() const;

  private:
    BeamType& _beam;
    Scan _scan;
    Goniometer _goniometer;
    Detector _detector;
    Crystal _crystal;
};

template<typename BeamType>
TypedExperiment<BeamType>::TypedExperiment(BeamType& beam, Scan scan, Goniometer gonio, Detector detector) : _beam(beam), _scan(scan), _goniometer(gonio), _detector(detector){}

template<typename BeamType>
BeamType& TypedExperiment<BeamType>::get_beam() const {
  return _beam;
}

template<typename BeamType>
Scan TypedExperiment<BeamType>::scan() const { return _scan; }

template<typename BeamType>
Goniometer TypedExperiment<BeamType>::goniometer() const { return _goniometer; }

template<typename BeamType>
Detector TypedExperiment<BeamType>::detector() const { return _detector; }

template<typename BeamType>
Crystal TypedExperiment<BeamType>::crystal() const { return _crystal; }

template<typename BeamType>
void TypedExperiment<BeamType>::set_crystal(Crystal crystal) { _crystal = crystal; }

template<typename BeamType>
json TypedExperiment<BeamType>::to_json() const {
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
  elist_out["beam"] = std::array<json, 1>{_beam.to_json()};
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



// factory function to make a typed experiment
template<typename BeamType>
TypedExperiment<BeamType> make_typed_experiment(json experiment_data){ // can add other types e.g. detector, scan
  json beam_data = experiment_data["beam"][0];
  std::shared_ptr<BeamBase> beamptr = make_beam(beam_data); // may raise std::runtime_error

  // Make sure we can cast to the requested types.
  BeamType* converted = dynamic_cast<BeamType*>(beamptr.get());
  if (converted == nullptr){
    throw std::runtime_error("Unable to make requested beam type");
  }
  json scan_data = experiment_data["scan"][0];
  Scan scan(scan_data);
  json gonio_data = experiment_data["goniometer"][0];
  Goniometer gonio(gonio_data);
  json detector_data = experiment_data["detector"][0];
  Detector detector(detector_data);
  return TypedExperiment<BeamType>(*converted, scan, gonio, detector);
}

#endif // DX2_MODEL_EXPERIMENT_H