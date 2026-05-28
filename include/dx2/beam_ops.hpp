#include "dx2/beam.hpp"

namespace beam_ops {

// constraints
MonochromaticBeam& require_monochromatic(Beam&);
const MonochromaticBeam& require_monochromatic(const Beam&);
// similarly for other types in the variant

// generic operations
// e.g. anything that acts on all beam types.

// transformations between types
// might not be relevant for beam, but will be for scans e.g. static->varying

// validations
}