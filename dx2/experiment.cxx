#include <dx2/beam_io.hpp>
#include <dx2/experiment.hpp>

Experiment::Experiment(const json &experiment_data) {
  std::string identifier = experiment_data["experiment"][0]["identifier"];
  json beam_data = experiment_data["beam"][0];
  Beam beam = beam_io::from_json(beam_data);
  json scan_data = experiment_data["scan"][0];
  Scan scan(scan_data);
  json gonio_data = experiment_data["goniometer"][0];
  Goniometer gonio(gonio_data);
  json detector_data = experiment_data["detector"][0];
  Detector detector(detector_data);
  this->_identifier = identifier;
  _beam = std::make_shared<Beam>(beam);
  _scan = std::make_shared<Scan>(scan);
  _goniometer = std::make_shared<Goniometer>(gonio);
  >_detector = std::make_shared<Detector>(detector);
  // Save the imageset json to propagate when saving to file.
  json imagesequence_data = experiment_data["imageset"][0];
  ImageSequence imagesequence(imagesequence_data);
  _imagesequence = std::make_shared<ImageSequence>(imagesequence);
  if (experiment_data.contains("crystal") &&
      experiment_data["crystal"].is_array() &&
      !experiment_data["crystal"].empty()) {
    const json &crystal_data = experiment_data["crystal"][0];
    Crystal crystal(crystal_data);
    set_crystal(crystal);
  }
}

json Experiment::to_json() const {
  // save this experiment as an example experiment list
  json elist_out; // a list of potentially multiple experiments
  elist_out["__id__"] = "ExperimentList";
  json expt_out; // our single experiment
  expt_out["__id__"] = "Experiment";
  expt_out["identifier"] = _identifier;
  expt_out["beam"] =
      0; // the indices of the models that will correspond to our experiment
  expt_out["detector"] = 0;
  expt_out["goniometer"] = 0;
  expt_out["scan"] = 0;
  expt_out["imageset"] = 0;

  // add the the actual models
  elist_out["scan"] = std::array<json, 1>{_scan->to_json()};
  elist_out["goniometer"] = std::array<json, 1>{_goniometer->to_json()};
  elist_out["beam"] = std::array<json, 1>{beam_io::to_json(*_beam)};
  elist_out["detector"] = std::array<json, 1>{_detector->to_json()};
  elist_out["imageset"] = std::array<json, 1>{_imagesequence->to_json()};

  if (_crystal) {
    expt_out["crystal"] = 0;
    elist_out["crystal"] = std::array<json, 1>{_crystal->to_json()};
  } else {
    elist_out["crystal"] = std::array<json, 0>{};
  }

  elist_out["experiment"] = std::array<json, 1>{expt_out};
  return elist_out;
}

void Experiment::set_crystal(Crystal crystal) { _crystal = std::make_shared<Crystal>(std::move(crystal)); }

void Experiment::set_crystal(std::shared_ptr<Crystal> crystal){_crystal = std::move(crystal);}

void Experiment::set_beam(std::shared_ptr<Beam> beam) { _beam = std::move(beam); }

void Experiment::set_scan(std::shared_ptr<Scan> scan) { _scan = std::move(scan); }

void Experiment::set_detector(std::shared_ptr<Detector> detector) { _detector = std::move(detector); }

void Experiment::set_goniometer(std::shared_ptr<Goniometer> goniometer) {
  _goniometer = std::move(goniometer);
}

void Experiment::set_imagesequence(std::shared_ptr<ImageSequence> imagesequence) {
  _imagesequence = std::move(imagesequence);
}

const std::string &Experiment::identifier() const { return _identifier; }

void Experiment::set_identifier(std::string identifier) {
  _identifier = identifier;
}

void Experiment::generate_identifier() { _identifier = ersatz_uuid4(); }
