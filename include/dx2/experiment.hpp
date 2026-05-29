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
  Experiment() = default;
  Experiment(const json& experiment_data);
  json to_json() const;

  Beam& beam();
  const Beam& beam() const;

  const std::string &identifier() const;
  const Goniometer &goniometer() const;
  Scan &scan();
  Detector &detector();
  Crystal &crystal();
  ImageSequence &imagesequence();
  void set_crystal(Crystal crystal);
  void set_beam(Beam beam);
  void set_scan(Scan scan);
  void set_detector(Detector detector);
  void set_goniometer(Goniometer goniometer);
  void set_imagesequence(ImageSequence imagesequence);
  void set_identifier(std::string identifier);
  void generate_identifier();

protected:
  Beam _beam{};
  Scan _scan{};
  Goniometer _goniometer{};
  Detector _detector{};
  Crystal _crystal{};
  ImageSequence _imagesequence{};
  std::string _identifier{};
};
