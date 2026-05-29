#include <gtest/gtest.h>
#include <dx2/beam.hpp>
#include <cmath>

using Eigen::Vector3d;

constexpr double EPS = 1e-10;

// ------------------------------------------------------------
// MonochromaticBeam basic construction
// ------------------------------------------------------------

TEST(MonochromaticBeamTest, ConstructFromWavelength)
{
    MonochromaticBeam b(1.0);
    EXPECT_NEAR(b.get_wavelength(), 1.0, EPS);
}

TEST(MonochromaticBeamTest, ConstructFromS0)
{
    Vector3d s0(0, 0, -2.0);
    MonochromaticBeam b(s0);

    EXPECT_NEAR(b.get_wavelength(), 0.5, EPS);

    Vector3d s0_out = b.get_s0();
    EXPECT_NEAR(s0_out.norm(), 2.0, EPS);
}

// ------------------------------------------------------------
// MonochromaticBeam setters
// ------------------------------------------------------------

TEST(MonochromaticBeamTest, SetWavelength)
{
    MonochromaticBeam b;
    b.set_wavelength(2.0);

    EXPECT_NEAR(b.get_wavelength(), 2.0, EPS);
}

TEST(MonochromaticBeamTest, SetS0Consistency)
{
    MonochromaticBeam b;

    Vector3d s0(0, 0, -4.0);
    b.set_s0(s0);

    EXPECT_NEAR(b.get_wavelength(), 0.25, EPS);

    Vector3d s0_out = b.get_s0();
    EXPECT_NEAR(s0_out.norm(), 4.0, EPS);
}

// ------------------------------------------------------------
// JSON serialization / deserialization
// ------------------------------------------------------------

TEST(MonochromaticBeamTest, JsonRoundTrip)
{
    MonochromaticBeam b(1.5);

    json j = b.to_json();
    MonochromaticBeam b2(j);

    EXPECT_NEAR(b2.get_wavelength(), 1.5, EPS);
}

TEST(PolychromaticBeamTest, JsonRoundTrip)
{
    std::array<double, 2> range = {1.0, 2.0};
    PolychromaticBeam b(range);

    json j = b.to_json();
    PolychromaticBeam b2(j);

    auto r = b2.get_wavelength_range();
    EXPECT_NEAR(r[0], 1.0, EPS);
    EXPECT_NEAR(r[1], 2.0, EPS);
}

// ------------------------------------------------------------
// PolychromaticBeam basic tests
// ------------------------------------------------------------

TEST(PolychromaticBeamTest, Construct)
{
    std::array<double, 2> range = {1.0, 3.0};
    PolychromaticBeam b(range);

    auto r = b.get_wavelength_range();
    EXPECT_EQ(r[0], 1.0);
    EXPECT_EQ(r[1], 3.0);
}

TEST(PolychromaticBeamTest, SetRange)
{
    PolychromaticBeam b;

    std::array<double, 2> range = {0.5, 2.5};
    b.set_wavelength_range(range);

    auto r = b.get_wavelength_range();
    EXPECT_EQ(r[0], 0.5);
    EXPECT_EQ(r[1], 2.5);
}

// ------------------------------------------------------------
// Derived classes (probe behaviour)
// ------------------------------------------------------------

TEST(MonoXrayBeamTest, ToJsonSetsProbe)
{
    MonoXrayBeam b(1.0);

    json j = b.to_json();
    EXPECT_EQ(j["probe"], "x-ray");
}

TEST(MonoElectronBeamTest, ToJsonSetsProbe)
{
    MonoElectronBeam b(1.0);

    json j = b.to_json();
    EXPECT_EQ(j["probe"], "electron");
}

// ------------------------------------------------------------
// BeamBase shared behaviour
// ------------------------------------------------------------

TEST(BeamBaseTest, BaseFieldsPreserved)
{
    Vector3d dir(1, 0, 0);
    Vector3d pol(0, 1, 0);

    MonochromaticBeam b(
        1.0,
        dir,
        0.1,
        0.2,
        pol,
        0.9,
        100.0,
        0.8,
        10.0
    );

    Vector3d out_dir = b.get_sample_to_source_direction();
    EXPECT_NEAR(out_dir[0], 1.0, EPS);
}

// ------------------------------------------------------------
// Error handling
// ------------------------------------------------------------

TEST(MonochromaticBeamTest, MissingJsonKeyThrows)
{
    json j;
    j["__id__"] = "monochromatic";

    EXPECT_THROW(MonochromaticBeam b(j), std::invalid_argument);
}

TEST(PolychromaticBeamTest, MissingJsonKeyThrows)
{
    json j;
    j["__id__"] = "polychromatic";

    EXPECT_THROW(PolychromaticBeam b(j), std::invalid_argument);
}
