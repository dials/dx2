#pragma once
#include "dx2/beam.hpp"

namespace beam_ops {

// constraints
MonochromaticBeam& require_monochromatic(Beam&);
const MonochromaticBeam& require_monochromatic(const Beam&);
PolychromaticBeam& require_polychromatic(Beam&);
const PolychromaticBeam& require_polychromatic(const Beam&);

// generic operations
// e.g. anything that acts on all beam types.

// transformations between types
// might not be relevant for beam, but will be for scans e.g. static->varying

// validations
}
