#pragma once
#include <Eigen/Dense>
#include <dx2/beam.hpp>
#include <dx2/crystal.hpp>
#include <dx2/detector.hpp>
#include <dx2/goniometer.hpp>
#include <dx2/imagesequence.hpp>
#include <dx2/scan.hpp>
#include <dx2/utils.hpp>
#include <nlohmann/json.hpp>

using Eigen::Vector3d;
using json = nlohmann::json;

class Experiment {
public:
  Experiment(const json &experiment_data);
  Experiment(std::shared_ptr<Beam> beam,
             std::shared_ptr<Detector> detector
             std::shared_ptr<Scan> _scan,
             std::shared_ptr<Goniometer> _goniometer,
             std::shared_ptr<ImageSequence> _imagesequence,
             std::shared_ptr<Crystal> _crystal);
  json to_json() const;

  // These models should always exist as they are defined by the experiment metadata

  Beam& beam() { return *_beam; }
  const Beam& beam() const { return *_beam; }

  Detector& detector() { return *_detector; }
  const Detector& detector() const { return *_detector; }

  Scan& scan() { return *_scan; }
  const Scan& scan() const { return *_scan; }

  Goniometer& goniometer() { return *_goniometer; }
  const Goniometer& goniometer() const { return *_goniometer; }

  ImageSequence& imagesequence() { return *_imagesequence; }
  const ImageSequence& imagesequence() const { return *_imagesequence; }

  // These models are discovered during processing and so are optional

  Crystal& crystal() {
    if (!_crystal) {
      throw std::runtime_error("Experiment has no crystal");
    }
    return *_crystal;
  }
  const Crystal& crystal() const {
    if (!_crystal) {
      throw std::runtime_error("Experiment has no crystal");
    }
    return *_crystal;
  }

  bool has_crystal() const {
    return static_cast<bool>(_crystal);
  }

  const std::string &identifier() const;

  void set_beam(std::shared_ptr<Beam> beam);
  void set_detector(std::shared_ptr<Detector> detector);
  void set_scan(std::shared_ptr<Scan> scan);
  void set_goniometer(std::shared_ptr<Goniometer> goniometer);
  void set_imagesequence(std::shared_ptr<ImageSequence> imagesequence);
  void set_crystal(std::shared_ptr<Crystal> crystal);
  void set_crystal(Crystal crystal);
  void set_identifier(std::string identifier);
  void generate_identifier();

private:
  std::shared_ptr<Beam> _beam;
  std::shared_ptr<Detector> _detector;
  std::shared_ptr<Scan> _scan;
  std::shared_ptr<Goniometer> _goniometer;
  std::shared_ptr<ImageSequence> _imagesequence;
  std::shared_ptr<Crystal> _crystal;
  std::string _identifier{};
};
