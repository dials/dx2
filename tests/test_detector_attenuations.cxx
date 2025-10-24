#include <dx2/detector_attenuations.hpp>
#include <gtest/gtest.h>
#include <string>

TEST(ModelTests, CrystalTest) {
  std::string silicon = "Si";
  std::string cdte = "CdTe";
  double mu_si = calculate_mu_for_material_at_wavelength(silicon, 0.976254);
  EXPECT_NEAR(mu_si, 3.9220836, 1E-6);

  double mu_cdte = calculate_mu_for_material_at_wavelength(cdte, 0.4959);
  EXPECT_NEAR(mu_cdte, 7.2858499, 1E-6);
}