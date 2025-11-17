#include "dx2/detector_attenuations.hpp"
#include <algorithm>
#include <array>
#include <cmath>
#include <stdexcept>
#include <string>
#include <vector>

// Just implement the required corrections for Si and CdTe detectors for now
// using NIST data.

constexpr double factor_kev_angstrom = 6.62607015 * 2.99792458 / 1.602176634;
constexpr double factor_ev_angstrom = factor_kev_angstrom * 1000.0;

// Silicon data
constexpr double silicon_density = 2.33;
constexpr std::array<double, 38> silicon_energy_data = {
    0.001, 0.0015, 0.0018389, 0.0018389, 0.002, 0.003, 0.004, 0.005,
    0.006, 0.008,  0.01,      0.015,     0.02,  0.03,  0.04,  0.05,
    0.06,  0.08,   0.1,       0.15,      0.2,   0.3,   0.4,   0.5,
    0.6,   0.8,    1.0,       1.25,      1.5,   2.0,   3.0,   4.0,
    5.0,   6.0,    8.0,       10.0,      15.0,  20.0};
constexpr std::array<double, 38> silicon_mu_rho_data = {
    1570.0,  535.5,   309.2,   3192.0,  2777.0,  978.4,  452.9,   245.0,
    147.0,   64.68,   33.89,   10.34,   4.464,   1.436,  0.7012,  0.4385,
    0.3207,  0.2228,  0.1835,  0.1448,  0.1275,  0.1082, 0.09614, 0.08748,
    0.08077, 0.07082, 0.06361, 0.05688, 0.05183, 0.0448, 0.03678, 0.0324,
    0.02967, 0.02788, 0.02574, 0.02462, 0.02352, 0.02338};

// CdTe data
constexpr double CdTe_density = 6.2;
constexpr std::array<double, 59> CdTe_energy_data = {
    0.0010,   0.001003, 0.001006, 0.001006, 0.00150,  0.0020,   0.0030,
    0.003537, 0.003537, 0.003631, 0.003727, 0.003727, 0.0040,   0.004018,
    0.004018, 0.004177, 0.004341, 0.004341, 0.004475, 0.004612, 0.004612,
    0.004773, 0.004939, 0.004939, 0.0050,   0.0060,   0.0080,   0.010,
    0.0150,   0.020,    0.026711, 0.026711, 0.030,    0.031814, 0.031814,
    0.040,    0.050,    0.060,    0.080,    0.10,     0.150,    0.20,
    0.30,     0.40,     0.50,     0.60,     0.80,     1.0,      1.250,
    1.50,     2.0,      3.0,      4.0,      5.0,      6.0,      8.0,
    10.0,     15.0,     20.0};
constexpr std::array<double, 59> CdTe_mu_rho_data = {
    7927.0,   7875.0,   7824.0,   8014.0,   3291.0,   1664.0,   614.60,
    406.40,   778.70,   730.0,    684.0,    860.10,   723.0,    715.10,
    793.40,   722.10,   656.20,   932.80,   873.90,   813.50,   943.80,
    870.20,   799.90,   865.30,   839.20,   528.60,   249.20,   138.10,
    46.570,   21.440,   9.8340,   29.430,   21.820,   18.730,   34.920,
    19.30,    10.670,   6.5420,   3.0190,   1.6710,   0.60710,  0.32460,
    0.16280,  0.11470,  0.092910, 0.080420, 0.0660,   0.057420, 0.050430,
    0.045910, 0.04070,  0.036490, 0.035250, 0.035130, 0.035480, 0.036870,
    0.038570, 0.042730, 0.046160};

template <std::size_t N>
double mu_rho_at_ev(double energy_ev, const std::array<double, N> &energy_data,
                    const std::array<double, N> &mu_rho_data) {
  double energy = energy_ev / 1e6;

  auto it = std::lower_bound(energy_data.begin(), energy_data.end(), energy);
  int index = static_cast<int>(std::distance(energy_data.begin(), it)) - 1;

  if (index < 0 || index + 1 >= static_cast<int>(energy_data.size())) {
    throw std::out_of_range("Energy value out of interpolation range");
  }
  // Interpolate
  double x0 = std::log(energy_data[index]);
  double x1 = std::log(energy_data[index + 1]);
  double y0 = std::log(mu_rho_data[index]);
  double y1 = std::log(mu_rho_data[index + 1]);

  double x = std::log(energy);
  return std::exp(y0 + (y1 - y0) * (x - x0) / (x1 - x0));
}

double calculate_CdTe_mu_from_wavelength(double wavelength) {
  double ev = factor_ev_angstrom / wavelength;
  return mu_rho_at_ev(ev, CdTe_energy_data, CdTe_mu_rho_data) * CdTe_density /
         10.0;
}

double calculate_silicon_mu_from_wavelength(double wavelength) {
  double ev = factor_ev_angstrom / wavelength;
  return mu_rho_at_ev(ev, silicon_energy_data, silicon_mu_rho_data) *
         silicon_density / 10.0;
}

// Calculate mu (the material absorption coefficient) in mm^-1 at the wavelength
// (in angstrom).
double calculate_mu_for_material_at_wavelength(const std::string &material,
                                               double wavelength) {
  if (material == "Si") {
    return calculate_silicon_mu_from_wavelength(wavelength);
  } else if (material == "CdTe") {
    return calculate_CdTe_mu_from_wavelength(wavelength);
  } else {
    throw std::invalid_argument("Only Silicon or CdTe detector absorption "
                                "coefficients currently implemented");
  }
}
