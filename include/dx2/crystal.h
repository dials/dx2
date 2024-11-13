#ifndef DX2_MODEL_CRYSTAL_H
#define DX2_MODEL_CRYSTAL_H
#include <math.h>
#include <Eigen/Dense>
#include <nlohmann/json.hpp>
#include "gemmi/unitcell.hpp"
#include "gemmi/cellred.hpp"
#include "gemmi/symmetry.hpp"
#include <iostream>
#include "utils.h"

using json = nlohmann::json;
using Eigen::Matrix3d;
using Eigen::Vector3d;

Matrix3d Matrix3d_from_gemmi_cb(gemmi::Op cb){
    std::array<std::array<int, 3>, 3> rot = cb.rot;
    std::cout << "ROT: " << rot[0][0]/cb.DEN << " " << rot[1][0]/cb.DEN << " "<< rot[2][0]/cb.DEN << std::endl;
    std::cout << rot[0][1]/cb.DEN << " " << rot[1][1]/cb.DEN << " "<< rot[2][1]/cb.DEN << std::endl;
    std::cout << rot[0][2]/cb.DEN << " " << rot[1][2]/cb.DEN << " "<< rot[2][2]/cb.DEN << std::endl;
    return Matrix3d{
      {(double)(rot[0][0]/cb.DEN), (double)(rot[1][0]/cb.DEN), (double)(rot[2][0]/cb.DEN)},
      {(double)(rot[0][1]/cb.DEN), (double)(rot[1][1]/cb.DEN), (double)(rot[2][1]/cb.DEN)},
      {(double)(rot[0][2]/cb.DEN), (double)(rot[1][2]/cb.DEN),(double)(rot[2][2]/cb.DEN)}};
}



class Crystal {
    public:
        Crystal(Vector3d a, Vector3d b, Vector3d c);
        gemmi::UnitCell get_unit_cell() const;
        Matrix3d get_A_matrix() const;
        void niggli_reduce();

    protected:
        gemmi::UnitCell unit_cell_;
        Matrix3d B_;
        Matrix3d A_;
        Matrix3d U_;

};

Crystal::Crystal(Vector3d a, Vector3d b, Vector3d c){
    // calculate B matrix, A matrix, set the unut cell values
    Matrix3d A{{a[0], a[1], a[2]}, {b[0], b[1], b[2]}, {c[0], c[1], c[2]}};
    A_ = A.inverse();
    std::cout << A_ << std::endl;
    double real_space_a = a.norm();
    double real_space_b = b.norm();
    double real_space_c = c.norm();
    double alpha = angle_between_vectors_degrees(b, c);
    double beta = angle_between_vectors_degrees(c, a);
    double gamma = angle_between_vectors_degrees(a, b);
    unit_cell_ = {real_space_a,real_space_b,real_space_c,alpha,beta,gamma};
}

void Crystal::niggli_reduce() {
    char centering{'P'};
    gemmi::GruberVector gv(unit_cell_, centering, true);
    gv.niggli_reduce();
    gemmi::UnitCell niggli_cell = gv.get_cell();
    std::cout << "Input cell" << std::endl;
    std::cout << unit_cell_.a << " " << unit_cell_.b << " " << unit_cell_.c << " " << unit_cell_.alpha << " " << unit_cell_.beta << " " << unit_cell_.gamma << std::endl;
    std::cout << "Reduced" << std::endl;
    std::cout << niggli_cell.a << " " << niggli_cell.b << " " << niggli_cell.c << " " << niggli_cell.alpha << " " << niggli_cell.beta << " " << niggli_cell.gamma << std::endl;
    unit_cell_ = niggli_cell;
    gemmi::Op cb = *gv.change_of_basis;
    
    Matrix3d cb_op = Matrix3d_from_gemmi_cb(cb);
    A_ = A_*cb_op.inverse();
    std::cout << A_ << std::endl;
}

gemmi::UnitCell Crystal::get_unit_cell() const {
    return unit_cell_;
}

Matrix3d Crystal::get_A_matrix() const {
    return A_;
}

#endif // DX2_MODEL_CRYSTAL_H