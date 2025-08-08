#pragma once
#include "dx2/utils.hpp"
#include <Eigen/Dense>
#include <gemmi/cellred.hpp>
#include <gemmi/math.hpp>
#include <gemmi/symmetry.hpp>
#include <gemmi/unitcell.hpp>
#include <math.h>
#include <nlohmann/json.hpp>

using json = nlohmann::json;
using Eigen::Matrix3d;
using Eigen::Vector3d;

Matrix3d Matrix3d_from_gemmi_cb(gemmi::Op cb);

class Crystal {
public:
  Crystal() = default;
  Crystal(Vector3d a, Vector3d b, Vector3d c, gemmi::SpaceGroup space_group);
  Crystal(json crystal_data);
  gemmi::UnitCell get_unit_cell() const;
  gemmi::SpaceGroup get_space_group() const;
  Matrix3d get_A_matrix() const;
  Matrix3d get_U_matrix() const;
  Matrix3d get_B_matrix() const;
  void niggli_reduce();
  void set_A_matrix(Matrix3d A);
  json to_json() const;

protected:
  void init_from_abc(Vector3d a, Vector3d b, Vector3d c);
  gemmi::SpaceGroup space_group_;
  gemmi::UnitCell unit_cell_;
  Matrix3d B_;
  Matrix3d A_;
  Matrix3d U_;
};
