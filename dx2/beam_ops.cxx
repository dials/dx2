
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

PolychromaticBeam& require_polychromatic(Beam& beam)
{
    if (auto* b = std::get_if<PolychromaticBeam>(&beam)) {
        return *b;
    }
    throw std::invalid_argument("Beam must be polychromatic");
}

const PolychromaticBeam& require_polychromatic(const Beam& beam)
{
    if (auto* b = std::get_if<PolychromaticBeam>(&beam)) {
        return *b;
    }
    throw std::invalid_argument("Beam must be polychromatic");
}

} // namespace beam_ops
