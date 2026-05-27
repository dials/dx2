#include <dx2/experiment.hpp>

Beam make_beam_from_json(const json& beam_data)
{
    std::string type = beam_data["__id__"];

    if (type == "monochromatic") {
        return MonochromaticBeam(beam_data);
    }
    else if (type == "polychromatic") {
        return PolychromaticBeam(beam_data);
    }
    else {
        throw std::runtime_error("Unknown beam id: " + type);
    }
}

Experiment::Experiment(json experiment_data) {
  std::string identifier = experiment_data["experiment"][0]["identifier"];
  json beam_data = experiment_data["beam"][0];
  Beam beam = make_beam_from_json(beam_data);
  json scan_data = experiment_data["scan"][0];
  Scan scan(scan_data);
  json gonio_data = experiment_data["goniometer"][0];
  Goniometer gonio(gonio_data);
  json detector_data = experiment_data["detector"][0];
  Detector detector(detector_data);
  this->_identifier = identifier;
  this->_beam = std::move(beam);
  this->_scan = std::move(scan);
  this->_goniometer = std::move(gonio);
  this->_detector = std::move(detector);
  // Save the imageset json to propagate when saving to file.
  json imagesequence_data = experiment_data["imageset"][0];
  ImageSequence imagesequence(imagesequence_data);
  this->_imagesequence = imagesequence;
  try { // We don't always have a crystal model e.g. before indexing.
    json crystal_data = experiment_data["crystal"][0];
    Crystal crystal(crystal_data);
    this->_crystal = crystal;
  } catch (...) {
    ;
  }
}


json beam_to_json(const Beam& beam)
{
    return std::visit([](const auto& b) {
        return b.to_json();
    }, beam);
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
  elist_out["scan"] = std::array<json, 1>{_scan.to_json()};
  elist_out["goniometer"] = std::array<json, 1>{_goniometer.to_json()};
  elist_out["beam"] = std::array<json, 1>{beam_to_json(_beam)};
  elist_out["detector"] = std::array<json, 1>{_detector.to_json()};
  elist_out["imageset"] = std::array<json, 1>{_imagesequence.to_json()};

  if (_crystal.get_U_matrix().determinant()) {
    expt_out["crystal"] = 0;
    elist_out["crystal"] = std::array<json, 1>{_crystal.to_json()};
  } else {
    elist_out["crystal"] = std::array<json, 0>{};
  }

  elist_out["experiment"] = std::array<json, 1>{expt_out};
  return elist_out;
}

Scan &Experiment::scan() { return _scan; }

const Goniometer &Experiment::goniometer() const {
  return _goniometer;
}

Detector &Experiment::detector() {
  return _detector;
}

Crystal &Experiment::crystal() {
  return _crystal;
}

ImageSequence &Experiment::imagesequence() {
  return _imagesequence;
}


void Experiment::set_crystal(Crystal crystal) {
  _crystal = crystal;
}

void Experiment::set_beam(Beam beam) {
  _beam = beam;
}

Beam& Experiment::beam() {
  return _beam;
}

void Experiment::set_scan(Scan scan) {
  _scan = scan;
}


void Experiment::set_detector(Detector detector) {
  _detector = detector;
}

void Experiment::set_goniometer(Goniometer goniometer) {
  _goniometer = goniometer;
}

void Experiment::set_imagesequence(ImageSequence imagesequence) {
  _imagesequence = imagesequence;
}

const std::string &Experiment::identifier() const {
  return _identifier;
}

void Experiment::set_identifier(std::string identifier) {
  _identifier = identifier;
}

void Experiment::generate_identifier() {
  _identifier = ersatz_uuid4();
}

MonochromaticBeam& require_monochromatic(Beam& beam)
{
    if (auto* b = std::get_if<MonochromaticBeam>(&beam)) {
        return *b;
    }
    throw std::invalid_argument("Beam must be monochromatic");
}

MonochromaticBeam& Experiment::monochromatic_beam()
{
    return require_monochromatic(_beam);
}
