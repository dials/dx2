#pragma once
#include <dx2/experiment.hpp>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

class ExperimentList {
public:
  ExperimentList() = default;
  ExperimentList(const json &experiment_data);
  json to_json() const;

  std::size_t size() const { return _experiments.size(); }

  Experiment &operator[](std::size_t i) { return _experiments[i]; }

  const Experiment &operator[](std::size_t i) const { return _experiments[i]; }

  auto begin() { return _experiments.begin(); }
  auto end() { return _experiments.end(); }

  auto begin() const { return _experiments.begin(); }
  auto end() const { return _experiments.end(); }

  ExperimentList select(const std::vector<std::string> &identifiers)
      const; // Select on identifiers

  ExperimentList
  select(const std::vector<std::size_t> &indices) const; // Select on indices

  void append(Experiment experiment);

private:
  std::vector<Experiment> _experiments;
};
