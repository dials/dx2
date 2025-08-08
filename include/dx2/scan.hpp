#pragma once

#include <nlohmann/json.hpp>
using json = nlohmann::json;

class Scan {
  // A class to represent the physical measurement, consisting of the number of
  // images, starting oscillation and a constant oscillation width between
  // sequential images. This class MUST NOT be modified during processing or
  // used to track additional metadata.
public:
  Scan() = default;
  Scan(std::array<int, 2> image_range, std::array<double, 2> oscillation);
  Scan(json scan_data);
  std::array<int, 2> get_image_range() const;
  std::array<double, 2> get_oscillation() const;
  json to_json() const;

protected:
  std::array<int, 2> image_range_{{0, 0}};
  int num_images_{0};
  double oscillation_width_{0.0};
  double oscillation_start_{0.0};
};
