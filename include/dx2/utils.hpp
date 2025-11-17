#pragma once
#include <Eigen/Dense>

using Eigen::Vector3d;

double angle_between_vectors_degrees(Vector3d v1, Vector3d v2);

std::string ersatz_uuid4();