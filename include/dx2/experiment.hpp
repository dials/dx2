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

class ExperimentList; // forward declaration

class Experiment {
  friend class ExperimentList;
public:

  struct Models {
    Models(std::shared_ptr<Beam> beam,
          std::shared_ptr<Detector> detector,
          std::shared_ptr<Scan> scan,
          std::shared_ptr<Goniometer> goniometer,
          std::shared_ptr<ImageSequence> imagesequence,
          std::shared_ptr<Crystal> crystal = nullptr) // Optional argument
        : beam(std::move(beam)),
          detector(std::move(detector)),
          scan(std::move(scan)),
          goniometer(std::move(goniometer)),
          imagesequence(std::move(imagesequence)),
          crystal(std::move(crystal)) {}

    std::shared_ptr<Beam> beam;
    std::shared_ptr<Detector> detector;
    std::shared_ptr<Scan> scan;
    std::shared_ptr<Goniometer> goniometer;
    std::shared_ptr<ImageSequence> imagesequence;
    std::shared_ptr<Crystal> crystal;
  };

  explicit Experiment(Models models, std::string identifier={});

  // These models should always exist as they are defined by the experiment metadata

  Beam& beam() { return *_models.beam; }
  const Beam& beam() const { return *_models.beam; }

  Detector& detector() { return *_models.detector; }
  const Detector& detector() const { return *_models.detector; }

  Scan& scan() { return *_models.scan; }
  const Scan& scan() const { return *_models.scan; }

  Goniometer& goniometer() { return *_models.goniometer; }
  const Goniometer& goniometer() const { return *_models.goniometer; }

  ImageSequence& imagesequence() { return *_models.imagesequence; }
  const ImageSequence& imagesequence() const { return *_models.imagesequence; }

  // These models are discovered during processing and so are optional
  Crystal& crystal();
  const Crystal& crystal() const;
  bool has_crystal() const { return static_cast<bool>(_models.crystal);}
  
  void set_crystal(Crystal crystal);
  void set_crystal(std::shared_ptr<Crystal> crystal);
  
  const std::string& identifier() const { return _identifier; }
  void set_identifier(std::string identifier) { _identifier = identifier; }
  
  void generate_identifier();

private:
  Models _models;
  std::string _identifier{};
};
