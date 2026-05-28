
#include <variant>
#include <stdexcept>
#include <dx2/beam.hpp>
#include <dx2/beam_ops.hpp>

namespace beam_ops {

MonochromaticBeam& require_monochromatic(Beam& beam)
{
    if (auto* b = std::get_if<MonochromaticBeam>(&beam)) {
        return *b;
    }
    throw std::invalid_argument("Beam must be monochromatic");
}

const MonochromaticBeam& require_monochromatic(const Beam& beam)
{
    if (auto* b = std::get_if<MonochromaticBeam>(&beam)) {
        return *b;
    }
    throw std::invalid_argument("Beam must be monochromatic");
}

} // namespace beam_ops
