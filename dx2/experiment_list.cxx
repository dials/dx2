#include <dx2/beam.hpp>
#include <dx2/beam_io.hpp>
#include <dx2/crystal.hpp>
#include <dx2/detector.hpp>
#include <dx2/experiment_list.hpp>
#include <dx2/goniometer.hpp>
#include <dx2/imagesequence.hpp>
#include <dx2/scan.hpp>
#include <dx2/utils.hpp>
#include <unordered_set>

ExperimentList::ExperimentList(const json &experiment_data) {

  // Phase 1: deserialize all models

  std::vector<std::shared_ptr<Beam>> beams;
  for (const auto &beam_json : experiment_data["beam"]) {
    beams.push_back(std::make_shared<Beam>(beam_io::from_json(beam_json)));
  }

  std::vector<std::shared_ptr<Detector>> detectors;
  for (const auto &detector_json : experiment_data["detector"]) {
    detectors.push_back(std::make_shared<Detector>(Detector(detector_json)));
  }

  std::vector<std::shared_ptr<Scan>> scans;
  for (const auto &scan_json : experiment_data["scan"]) {
    scans.push_back(std::make_shared<Scan>(Scan(scan_json)));
  }

  std::vector<std::shared_ptr<Goniometer>> goniometers;
  for (const auto &gonio_json : experiment_data["goniometer"]) {
    goniometers.push_back(std::make_shared<Goniometer>(Goniometer(gonio_json)));
  }

  std::vector<std::shared_ptr<ImageSequence>> imagesets;
  for (const auto &imageset_json : experiment_data["imageset"]) {
    imagesets.push_back(
        std::make_shared<ImageSequence>(ImageSequence(imageset_json)));
  }

  std::vector<std::shared_ptr<Crystal>> crystals;
  if (experiment_data.contains("crystal")) {
    for (const auto &crystal_json : experiment_data["crystal"]) {
      crystals.push_back(std::make_shared<Crystal>(Crystal(crystal_json)));
    }
  }

  // Phase 2: construct experiments

  for (const auto &expt_json : experiment_data["experiment"]) {

    Experiment::Models models{
        beams.at(expt_json["beam"]), detectors.at(expt_json["detector"]),
        scans.at(expt_json["scan"]), goniometers.at(expt_json["goniometer"]),
        imagesets.at(expt_json["imageset"])};

    if (expt_json.contains("crystal")) {
      models.crystal = crystals.at(expt_json["crystal"]);
    }

    _experiments.emplace_back(std::move(models), expt_json["identifier"]);
  }
}

json ExperimentList::to_json() const {

  json result;
  result["__id__"] = "ExperimentList";

  result["experiment"] = json::array();
  result["beam"] = json::array();
  result["detector"] = json::array();
  result["scan"] = json::array();
  result["goniometer"] = json::array();
  result["imageset"] = json::array();
  result["crystal"] = json::array();

  std::unordered_map<const Beam *, std::size_t> beam_indices;
  std::unordered_map<const Detector *, std::size_t> detector_indices;
  std::unordered_map<const Scan *, std::size_t> scan_indices;
  std::unordered_map<const Goniometer *, std::size_t> goniometer_indices;
  std::unordered_map<const ImageSequence *, std::size_t> imageset_indices;
  std::unordered_map<const Crystal *, std::size_t> crystal_indices;

  for (const auto &expt : _experiments) {

    json expt_json;
    expt_json["__id__"] = "Experiment";
    expt_json["identifier"] = expt.identifier();

    //
    // Beam
    //
    const Beam *beam_ptr = expt._models.beam.get();
    auto [beam_it, beam_inserted] =
        beam_indices.emplace(beam_ptr, beam_indices.size());

    if (beam_inserted) {
      result["beam"].push_back(beam_io::to_json(*expt._models.beam));
    }

    expt_json["beam"] = beam_it->second;

    //
    // Detector
    //
    const Detector *detector_ptr = expt._models.detector.get();
    auto [detector_it, detector_inserted] =
        detector_indices.emplace(detector_ptr, detector_indices.size());

    if (detector_inserted) {
      result["detector"].push_back(expt._models.detector->to_json());
    }

    expt_json["detector"] = detector_it->second;

    //
    // Scan
    //
    const Scan *scan_ptr = expt._models.scan.get();
    auto [scan_it, scan_inserted] =
        scan_indices.emplace(scan_ptr, scan_indices.size());

    if (scan_inserted) {
      result["scan"].push_back(expt._models.scan->to_json());
    }

    expt_json["scan"] = scan_it->second;

    //
    // Goniometer
    //
    const Goniometer *gonio_ptr = expt._models.goniometer.get();

    auto [gonio_it, gonio_inserted] =
        goniometer_indices.emplace(gonio_ptr, goniometer_indices.size());

    if (gonio_inserted) {
      result["goniometer"].push_back(expt._models.goniometer->to_json());
    }

    expt_json["goniometer"] = gonio_it->second;

    //
    // Imageset
    //
    const ImageSequence *imageset_ptr = expt._models.imagesequence.get();

    auto [imageset_it, imageset_inserted] =
        imageset_indices.emplace(imageset_ptr, imageset_indices.size());

    if (imageset_inserted) {
      result["imageset"].push_back(expt._models.imagesequence->to_json());
    }

    expt_json["imageset"] = imageset_it->second;

    //
    // Optional crystal
    //
    if (expt.has_crystal()) {

      const Crystal *crystal_ptr = expt._models.crystal.get();

      auto [crystal_it, crystal_inserted] =
          crystal_indices.emplace(crystal_ptr, crystal_indices.size());

      if (crystal_inserted) {
        result["crystal"].push_back(expt._models.crystal->to_json());
      }

      expt_json["crystal"] = crystal_it->second;
    }

    result["experiment"].push_back(std::move(expt_json));
  }

  return result;
}

ExperimentList
ExperimentList::select(const std::vector<std::string> &identifiers) const {

  // Implement a simple linear scan and lookup for now.
  std::unordered_set<std::string> wanted(identifiers.begin(),
                                         identifiers.end());

  ExperimentList result;

  for (const auto &expt : _experiments) {
    if (wanted.contains(expt.identifier())) {
      result.append(expt);
    }
  }

  return result;
}

ExperimentList
ExperimentList::select(const std::vector<std::size_t> &indices) const {

  ExperimentList result;

  for (std::size_t i : indices) {
    if (i >= _experiments.size()) {
      throw std::out_of_range("Experiment index out of range");
    }

    result.append(_experiments[i]);
  }

  return result;
}

void ExperimentList::append(Experiment experiment) {
  _experiments.push_back(std::move(experiment));
}
