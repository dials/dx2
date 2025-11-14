/**
 * @file crystal.cxx
 * @brief Implementation file for Crystal class.
 */

#include "dx2/crystal.hpp"

Matrix3d Matrix3d_from_gemmi_cb(gemmi::Op cb) {
  std::array<std::array<int, 3>, 3> rot = cb.rot;
  return Matrix3d{{(double)(rot[0][0] / cb.DEN), (double)(rot[1][0] / cb.DEN),
                   (double)(rot[2][0] / cb.DEN)},
                  {(double)(rot[0][1] / cb.DEN), (double)(rot[1][1] / cb.DEN),
                   (double)(rot[2][1] / cb.DEN)},
                  {(double)(rot[0][2] / cb.DEN), (double)(rot[1][2] / cb.DEN),
                   (double)(rot[2][2] / cb.DEN)}};
}

void Crystal::init_from_abc(Vector3d a, Vector3d b, Vector3d c) {
  // calculate B matrix, A matrix, set the input cell values
  Matrix3d A{{a[0], a[1], a[2]}, {b[0], b[1], b[2]}, {c[0], c[1], c[2]}};
  A_ = A.inverse();
  double real_space_a = a.norm();
  double real_space_b = b.norm();
  double real_space_c = c.norm();
  double alpha = angle_between_vectors_degrees(b, c);
  double beta = angle_between_vectors_degrees(c, a);
  double gamma = angle_between_vectors_degrees(a, b);
  unit_cell_ = {real_space_a, real_space_b, real_space_c, alpha, beta, gamma};
  gemmi::Mat33 B = unit_cell_.frac.mat;
  B_ << B.a[0][0], B.a[1][0], B.a[2][0], B.a[0][1], B.a[1][1], B.a[2][1],
      B.a[0][2], B.a[1][2],
      B.a[2][2]; // Transpose due to different definition of B form
  U_ = A_ * B_.inverse();
}

Crystal::Crystal(Vector3d a, Vector3d b, Vector3d c,
                 gemmi::SpaceGroup space_group)
    : space_group_(space_group) {
  init_from_abc(a, b, c);
}

Crystal::Crystal(json crystal_data) {
  std::vector<std::string> required_keys = {"real_space_a", "real_space_b",
                                            "real_space_c",
                                            "space_group_hall_symbol"};
  for (const auto &key : required_keys) {
    if (crystal_data.find(key) == crystal_data.end()) {
      throw std::invalid_argument("Key " + key +
                                  " is missing from the input crystal JSON");
    }
  }
  Vector3d rsa{{crystal_data["real_space_a"][0],
                crystal_data["real_space_a"][1],
                crystal_data["real_space_a"][2]}};
  Vector3d rsb{{crystal_data["real_space_b"][0],
                crystal_data["real_space_b"][1],
                crystal_data["real_space_b"][2]}};
  Vector3d rsc{{crystal_data["real_space_c"][0],
                crystal_data["real_space_c"][1],
                crystal_data["real_space_c"][2]}};
  std::string hall_symbol = crystal_data["space_group_hall_symbol"];
  gemmi::GroupOps ops = gemmi::symops_from_hall(hall_symbol.c_str());
  space_group_ = *gemmi::find_spacegroup_by_ops(ops);
  init_from_abc(rsa, rsb, rsc);
}

void Crystal::niggli_reduce() {
  char centering{'P'};
  gemmi::GruberVector gv(unit_cell_, centering, true);
  gv.niggli_reduce();
  unit_cell_ = gv.get_cell(); // The Niggli cell
  gemmi::Op cb = *gv.change_of_basis;

  Matrix3d cb_op = Matrix3d_from_gemmi_cb(cb);
  A_ = A_ * cb_op.inverse();
  gemmi::Mat33 B = unit_cell_.frac.mat;
  B_ << B.a[0][0], B.a[1][0], B.a[2][0], B.a[0][1], B.a[1][1], B.a[2][1],
      B.a[0][2], B.a[1][2],
      B.a[2][2]; // Transpose due to different definition of B form
  U_ = A_ * B_.inverse();
}

void Crystal::set_A_matrix(Matrix3d A) {
  // input in reciprocal units
  A_ = A;
  Matrix3d Areal = A.inverse();
  Vector3d a{Areal(0, 0), Areal(0, 1), Areal(0, 2)};
  Vector3d b{Areal(1, 0), Areal(1, 1), Areal(1, 2)};
  Vector3d c{Areal(2, 0), Areal(2, 1), Areal(2, 2)};
  double real_space_a = a.norm();
  double real_space_b = b.norm();
  double real_space_c = c.norm();
  double alpha = angle_between_vectors_degrees(b, c);
  double beta = angle_between_vectors_degrees(c, a);
  double gamma = angle_between_vectors_degrees(a, b);
  unit_cell_ = {real_space_a, real_space_b, real_space_c, alpha, beta, gamma};
  gemmi::Mat33 B = unit_cell_.frac.mat;
  B_ << B.a[0][0], B.a[1][0], B.a[2][0], B.a[0][1], B.a[1][1], B.a[2][1],
      B.a[0][2], B.a[1][2],
      B.a[2][2]; // Transpose due to different definition of B form
  U_ = A_ * B_.inverse();
}

gemmi::UnitCell Crystal::get_unit_cell() const { return unit_cell_; }

gemmi::SpaceGroup Crystal::get_space_group() const { return space_group_; }

Matrix3d Crystal::get_A_matrix() const { return A_; }

Matrix3d Crystal::get_B_matrix() const { return B_; }

Matrix3d Crystal::get_U_matrix() const { return U_; }

json Crystal::to_json() const {
  json crystal_data;
  crystal_data["__id__"] = "crystal";
  Matrix3d A_inv = A_.inverse();
  Vector3d rsa{{A_inv(0, 0), A_inv(0, 1), A_inv(0, 2)}};
  Vector3d rsb{{A_inv(1, 0), A_inv(1, 1), A_inv(1, 2)}};
  Vector3d rsc{{A_inv(2, 0), A_inv(2, 1), A_inv(2, 2)}};
  crystal_data["real_space_a"] = rsa;
  crystal_data["real_space_b"] = rsb;
  crystal_data["real_space_c"] = rsc;
  crystal_data["space_group_hall_symbol"] = space_group_.hall;
  return crystal_data;
}
