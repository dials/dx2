#include <dx2/beam.hpp>
#include <dx2/beam_ops.hpp>
#include <gtest/gtest.h>

using namespace beam_ops;

// ------------------------------------------------------------
// require_monochromatic (non-const)
// ------------------------------------------------------------

TEST(BeamOpsTest, RequireMonochromatic_Succeeds) {
  Beam beam = MonochromaticBeam(1.0);

  auto &b = require_monochromatic(beam);

  EXPECT_NEAR(b.get_wavelength(), 1.0, 1e-10);
}

TEST(BeamOpsTest, RequireMonochromatic_ReturnsReference) {
  Beam beam = MonochromaticBeam(1.0);

  auto &b = require_monochromatic(beam);
  b.set_wavelength(2.0);

  // underlying variant should reflect change
  auto &inner = std::get<MonochromaticBeam>(beam);
  EXPECT_NEAR(inner.get_wavelength(), 2.0, 1e-10);
}

TEST(BeamOpsTest, RequireMonochromatic_ThrowsForWrongType) {
  Beam beam = PolychromaticBeam(std::array<double, 2>{1.0, 2.0});

  EXPECT_THROW(require_monochromatic(beam), std::invalid_argument);
}

// ------------------------------------------------------------
// require_monochromatic (const)
// ------------------------------------------------------------

TEST(BeamOpsTest, RequireMonochromaticConst_Succeeds) {
  const Beam beam = MonochromaticBeam(1.5);

  const auto &b = require_monochromatic(beam);

  EXPECT_NEAR(b.get_wavelength(), 1.5, 1e-10);
}

TEST(BeamOpsTest, RequireMonochromaticConst_Throws) {
  const Beam beam = PolychromaticBeam(std::array<double, 2>{1.0, 2.0});

  EXPECT_THROW(require_monochromatic(beam), std::invalid_argument);
}

// ------------------------------------------------------------
// require_polychromatic (non-const)
// ------------------------------------------------------------

TEST(BeamOpsTest, RequirePolychromatic_Succeeds) {
  Beam beam = PolychromaticBeam(std::array<double, 2>{1.0, 3.0});

  auto &b = require_polychromatic(beam);

  auto r = b.get_wavelength_range();
  EXPECT_NEAR(r[0], 1.0, 1e-10);
  EXPECT_NEAR(r[1], 3.0, 1e-10);
}

TEST(BeamOpsTest, RequirePolychromatic_ReturnsReference) {
  Beam beam = PolychromaticBeam(std::array<double, 2>{1.0, 2.0});

  auto &b = require_polychromatic(beam);
  b.set_wavelength_range({2.0, 4.0});

  auto &inner = std::get<PolychromaticBeam>(beam);
  auto r = inner.get_wavelength_range();

  EXPECT_NEAR(r[0], 2.0, 1e-10);
  EXPECT_NEAR(r[1], 4.0, 1e-10);
}

TEST(BeamOpsTest, RequirePolychromatic_ThrowsForWrongType) {
  Beam beam = MonochromaticBeam(1.0);

  EXPECT_THROW(require_polychromatic(beam), std::invalid_argument);
}

// ------------------------------------------------------------
// require_polychromatic (const)
// ------------------------------------------------------------

TEST(BeamOpsTest, RequirePolychromaticConst_Succeeds) {
  const Beam beam = PolychromaticBeam(std::array<double, 2>{1.5, 2.5});

  const auto &b = require_polychromatic(beam);

  auto r = b.get_wavelength_range();
  EXPECT_NEAR(r[0], 1.5, 1e-10);
  EXPECT_NEAR(r[1], 2.5, 1e-10);
}

TEST(BeamOpsTest, RequirePolychromaticConst_Throws) {
  const Beam beam = MonochromaticBeam(1.0);

  EXPECT_THROW(require_polychromatic(beam), std::invalid_argument);
}

// ------------------------------------------------------------
// Cross-check: variant integrity
// ------------------------------------------------------------

TEST(BeamOpsTest, VariantTypeUnchangedAfterAccess) {
  Beam beam = MonochromaticBeam(1.0);

  auto &b = require_monochromatic(beam);
  b.set_wavelength(3.0);

  EXPECT_TRUE(std::holds_alternative<MonochromaticBeam>(beam));
}

// ------------------------------------------------------------
// Const correctness enforcement (compile-time intent)
// ------------------------------------------------------------

TEST(BeamOpsTest, ConstReferenceDoesNotAllowMutation) {
  const Beam beam = MonochromaticBeam(1.0);

  const auto &b = require_monochromatic(beam);

  // This test is mostly semantic — compilation ensures correctness.
  EXPECT_NEAR(b.get_wavelength(), 1.0, 1e-10);
}
