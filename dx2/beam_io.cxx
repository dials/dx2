
#include <dx2/beam_io.hpp>

namespace beam_io {

json to_json(const Beam& beam)
{
    return std::visit([](const auto& b) {
        return b.to_json();
    }, beam);
}

Beam from_json(const json& beam_data)
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

} // namespace beam_io
