#pragma once
#include <dx2/beam.hpp>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

namespace beam_io {

Beam from_json(const json& beam_data);
json to_json(const Beam& beam);

}