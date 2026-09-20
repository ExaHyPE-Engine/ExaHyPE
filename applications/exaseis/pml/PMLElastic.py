# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
def initial():
    return """
  auto dmp_pml_width_x = PmlCellWidth * h[0];
  auto dmp_pml_width_y = PmlCellWidth * h[1];
  auto dmp_pml_width_z = PmlCellWidth * h[2];

  auto dmp_pml_right_x = DomainOffset[0] + DomainSize[0] - dmp_pml_width_x;
  auto dmp_pml_right_y = DomainOffset[1] + DomainSize[1] - dmp_pml_width_y;
  auto dmp_pml_right_z = DomainOffset[2] + DomainSize[2] - dmp_pml_width_z;

  auto dmp_pml_left_x = DomainOffset[0] + dmp_pml_width_x;
  auto dmp_pml_left_y = DomainOffset[1] + dmp_pml_width_y;
  auto dmp_pml_left_z = DomainOffset[2] + dmp_pml_width_z;

  constexpr double AmplitudeNumerator = 6.0;

  auto dmp_amplitude_x = (PmlPower + 1) * AmplitudeNumerator / (2 * dmp_pml_width_x) * log(1 / PmlRelativeError);
  auto dmp_amplitude_y = (PmlPower + 1) * AmplitudeNumerator / (2 * dmp_pml_width_y) * log(1 / PmlRelativeError);
  auto dmp_amplitude_z = (PmlPower + 1) * AmplitudeNumerator / (2 * dmp_pml_width_z) * log(1 / PmlRelativeError);

  double offset_x = x[0] - h[0] * 0.5;
  double offset_y = x[1] - h[1] * 0.5;
  double offset_z = x[2] - h[2] * 0.5;

  kernels::idx4 idx(DGOrder + 1, DGOrder + 1, DGOrder + 1, NumberOfUnknowns + NumberOfAuxiliaryVariables);

  for (int k = 0; k < DGOrder + 1; k++) {
    double computational_z = (offset_z + h[2] * kernels::{{SOLVER_NAME}}::Quadrature<{{SOLUTION_STORAGE_PRECISION}}>::QuadraturePoints1D[k]);
    for (int j = 0; j < DGOrder + 1; j++) {
      double computational_y = (offset_y + h[1] * kernels::{{SOLVER_NAME}}::Quadrature<{{SOLUTION_STORAGE_PRECISION}}>::QuadraturePoints1D[j]);
      for (int i = 0; i < DGOrder + 1; i++) {
        double computational_x = (offset_x + h[0] * kernels::{{SOLVER_NAME}}::Quadrature<{{SOLUTION_STORAGE_PRECISION}}>::QuadraturePoints1D[i]);

        if (PmlTop != 0) { // No PML on surface
          if (computational_x < dmp_pml_left_x) {
            Q[idx(
              k, j, i, Shortcuts::dmp_pml + 0
            )] = dmp_amplitude_x * pow((dmp_pml_left_x - computational_x) / dmp_pml_width_x, PmlPower);
          }
        }
        if (PmlTop != 1) { // No PML on surface
          if (computational_y <= dmp_pml_left_y) {
            Q[idx(
              k, j, i, Shortcuts::dmp_pml + 1
            )] = dmp_amplitude_y * pow((dmp_pml_left_y - computational_y) / dmp_pml_width_y, PmlPower);
          }
        }
        if (PmlTop != 2) { // No PML on surface
          if (computational_z <= dmp_pml_left_z) {
            Q[idx(
              k, j, i, Shortcuts::dmp_pml + 2
            )] = dmp_amplitude_z * pow((dmp_pml_left_z - computational_z) / dmp_pml_width_z, PmlPower);
          }
        }
        if (computational_x >= dmp_pml_right_x) {
          Q[idx(
            k, j, i, Shortcuts::dmp_pml + 0
          )] = dmp_amplitude_x * pow((computational_x - dmp_pml_right_x) / dmp_pml_width_x, PmlPower);
        }
        if (computational_y >= dmp_pml_right_y) {
          Q[idx(
            k, j, i, Shortcuts::dmp_pml + 1
          )] = dmp_amplitude_y * pow((computational_y - dmp_pml_right_y) / dmp_pml_width_y, PmlPower);
        }
        if (computational_z >= dmp_pml_right_z) {
          Q[idx(
            k, j, i, Shortcuts::dmp_pml + 2
          )] = dmp_amplitude_z * pow((computational_z - dmp_pml_right_z) / dmp_pml_width_z, PmlPower);
        }
      }
    }
  }
"""


def flux():
    return """
  auto sigma_xx = Q[Shortcuts::sigma + 0];
  auto sigma_yy = Q[Shortcuts::sigma + 1];
  auto sigma_zz = Q[Shortcuts::sigma + 2];
  auto sigma_xy = Q[Shortcuts::sigma + 3];
  auto sigma_xz = Q[Shortcuts::sigma + 4];
  auto sigma_yz = Q[Shortcuts::sigma + 5];

  auto jacobian = Q[Shortcuts::jacobian];

  std::fill_n(F, NumberOfUnknowns, 0);

  switch (normal) {
    case 0: {
      auto q_x = Q[Shortcuts::metric_derivative + 0];
      auto q_y = Q[Shortcuts::metric_derivative + 1];
      auto q_z = Q[Shortcuts::metric_derivative + 2];
      F[Shortcuts::v+0] = -jacobian * (q_x * sigma_xx + q_y * sigma_xy + q_z * sigma_xz);
      F[Shortcuts::v+1] = -jacobian * (q_x * sigma_xy + q_y * sigma_yy + q_z * sigma_yz);
      F[Shortcuts::v+2] = -jacobian * (q_x * sigma_xz + q_y * sigma_yz + q_z * sigma_zz);
      F[Shortcuts::pml + 9*0 + 0] = F[Shortcuts::v + 0];
      F[Shortcuts::pml + 9*0 + 1] = F[Shortcuts::v + 1];
      F[Shortcuts::pml + 9*0 + 2] = F[Shortcuts::v + 2];
    } break;
    case 1: {
      auto r_x = Q[Shortcuts::metric_derivative + 3];
      auto r_y = Q[Shortcuts::metric_derivative + 4];
      auto r_z = Q[Shortcuts::metric_derivative + 5];
      F[Shortcuts::v+0] = -jacobian * (r_x * sigma_xx + r_y * sigma_xy + r_z * sigma_xz);
      F[Shortcuts::v+1] = -jacobian * (r_x * sigma_xy + r_y * sigma_yy + r_z * sigma_yz);
      F[Shortcuts::v+2] = -jacobian * (r_x * sigma_xz + r_y * sigma_yz + r_z * sigma_zz);
      F[Shortcuts::pml + 9*1 + 0] = F[Shortcuts::v + 0];
      F[Shortcuts::pml + 9*1 + 1] = F[Shortcuts::v + 1];
      F[Shortcuts::pml + 9*1 + 2] = F[Shortcuts::v + 2];
    } break;
    case 2: {
      auto s_x = Q[Shortcuts::metric_derivative + 6];
      auto s_y = Q[Shortcuts::metric_derivative + 7];
      auto s_z = Q[Shortcuts::metric_derivative + 8];
      F[Shortcuts::v+0] = -jacobian * (s_x * sigma_xx + s_y * sigma_xy + s_z * sigma_xz);
      F[Shortcuts::v+1] = -jacobian * (s_x * sigma_xy + s_y * sigma_yy + s_z * sigma_yz);
      F[Shortcuts::v+2] = -jacobian * (s_x * sigma_xz + s_y * sigma_yz + s_z * sigma_zz);
      F[Shortcuts::pml + 9*2 + 0] = F[Shortcuts::v + 0];
      F[Shortcuts::pml + 9*2 + 1] = F[Shortcuts::v + 1];
      F[Shortcuts::pml + 9*2 + 2] = F[Shortcuts::v + 2];
    }
  }
"""


def ncp():
    return """
  auto        rho     = Q[Shortcuts::rho];
  auto        cp      = Q[Shortcuts::cp];
  auto        cs      = Q[Shortcuts::cs];
  auto        dmp_pml = Q[Shortcuts::dmp_pml + normal];
  auto        mu      = rho * cs * cs;
  auto        lambda  = rho * cp * cp - 2 * mu;

  std::fill_n(BTimesDeltaQ, NumberOfUnknowns, 0);

  switch (normal) {
    case 0: {
      auto u_q = deltaQ[Shortcuts::v + 0];
      auto v_q = deltaQ[Shortcuts::v + 1];
      auto w_q = deltaQ[Shortcuts::v + 2];
      auto q_x = Q[Shortcuts::metric_derivative + 0];
      auto q_y = Q[Shortcuts::metric_derivative + 1];
      auto q_z = Q[Shortcuts::metric_derivative + 2];
      auto lam_temp = lambda * (-q_x * u_q - q_y * v_q - q_z * w_q);
      BTimesDeltaQ[Shortcuts::sigma + 0] = -2 * mu * q_x * u_q + lam_temp;
      BTimesDeltaQ[Shortcuts::sigma + 1] = -2 * mu * q_y * v_q + lam_temp;
      BTimesDeltaQ[Shortcuts::sigma + 2] = -2 * mu * q_z * w_q + lam_temp;
      BTimesDeltaQ[Shortcuts::sigma + 3] = -mu * (q_y * u_q + q_x * v_q); // sigma_xy
      BTimesDeltaQ[Shortcuts::sigma + 4] = -mu * (q_z * u_q + q_x * w_q); // sigma_xz
      BTimesDeltaQ[Shortcuts::sigma + 5] = -mu * (q_z * v_q + q_y * w_q); // sigma_yz
      BTimesDeltaQ[Shortcuts::pml + Shortcuts::sigma + 9*0 + 0] = -dmp_pml * q_x * u_q;
      BTimesDeltaQ[Shortcuts::pml + Shortcuts::sigma + 9*0 + 1] = -dmp_pml * q_y * v_q;
      BTimesDeltaQ[Shortcuts::pml + Shortcuts::sigma + 9*0 + 2] = -dmp_pml * q_z * w_q;
      BTimesDeltaQ[Shortcuts::pml + Shortcuts::sigma + 9*0 + 3] = -dmp_pml * (q_y * u_q + q_x * v_q);
      BTimesDeltaQ[Shortcuts::pml + Shortcuts::sigma + 9*0 + 4] = -dmp_pml * (q_z * u_q + q_x * w_q);
      BTimesDeltaQ[Shortcuts::pml + Shortcuts::sigma + 9*0 + 5] = -dmp_pml * (q_z * v_q + q_y * w_q);
    } break;
    case 1: {
      auto u_r = deltaQ[Shortcuts::v + 0];
      auto v_r = deltaQ[Shortcuts::v + 1];
      auto w_r = deltaQ[Shortcuts::v + 2];
      auto r_x = Q[Shortcuts::metric_derivative + 3];
      auto r_y = Q[Shortcuts::metric_derivative + 4];
      auto r_z = Q[Shortcuts::metric_derivative + 5];
      auto lam_temp = lambda * (-r_x * u_r - r_y * v_r - r_z * w_r);
      BTimesDeltaQ[Shortcuts::sigma + 0] = -2 * mu * r_x * u_r + lam_temp;
      BTimesDeltaQ[Shortcuts::sigma + 1] = -2 * mu * r_y * v_r + lam_temp;
      BTimesDeltaQ[Shortcuts::sigma + 2] = -2 * mu * r_z * w_r + lam_temp;
      BTimesDeltaQ[Shortcuts::sigma + 3] = -mu * (r_y * u_r + r_x * v_r); // sigma_xy
      BTimesDeltaQ[Shortcuts::sigma + 4] = -mu * (r_z * u_r + r_x * w_r); // sigma_xz
      BTimesDeltaQ[Shortcuts::sigma + 5] = -mu * (r_z * v_r + r_y * w_r); // sigma_yz
      BTimesDeltaQ[Shortcuts::pml + Shortcuts::sigma + 9*1 + 0] = -dmp_pml * r_x * u_r;
      BTimesDeltaQ[Shortcuts::pml + Shortcuts::sigma + 9*1 + 1] = -dmp_pml * r_y * v_r;
      BTimesDeltaQ[Shortcuts::pml + Shortcuts::sigma + 9*1 + 2] = -dmp_pml * r_z * w_r;
      BTimesDeltaQ[Shortcuts::pml + Shortcuts::sigma + 9*1 + 3] = -dmp_pml * (r_y * u_r + r_x * v_r);
      BTimesDeltaQ[Shortcuts::pml + Shortcuts::sigma + 9*1 + 4] = -dmp_pml * (r_z * u_r + r_x * w_r);
      BTimesDeltaQ[Shortcuts::pml + Shortcuts::sigma + 9*1 + 5] = -dmp_pml * (r_z * v_r + r_y * w_r);
    } break;
    case 2: {
      auto u_s = deltaQ[Shortcuts::v + 0];
      auto v_s = deltaQ[Shortcuts::v + 1];
      auto w_s = deltaQ[Shortcuts::v + 2];
      auto s_x = Q[Shortcuts::metric_derivative + 6];
      auto s_y = Q[Shortcuts::metric_derivative + 7];
      auto s_z = Q[Shortcuts::metric_derivative + 8];
      auto lam_temp = lambda * (-s_x * u_s - s_y * v_s - s_z * w_s);
      BTimesDeltaQ[Shortcuts::sigma + 0] = -2 * mu * s_x * u_s + lam_temp;
      BTimesDeltaQ[Shortcuts::sigma + 1] = -2 * mu * s_y * v_s + lam_temp;
      BTimesDeltaQ[Shortcuts::sigma + 2] = -2 * mu * s_z * w_s + lam_temp;
      BTimesDeltaQ[Shortcuts::sigma + 3] = -mu * (s_y * u_s + s_x * v_s); // sigma_xy
      BTimesDeltaQ[Shortcuts::sigma + 4] = -mu * (s_z * u_s + s_x * w_s); // sigma_xz
      BTimesDeltaQ[Shortcuts::sigma + 5] = -mu * (s_z * v_s + s_y * w_s); // sigma_yz
      BTimesDeltaQ[Shortcuts::pml + Shortcuts::sigma + 9*2 + 0] = -dmp_pml * s_x * u_s;
      BTimesDeltaQ[Shortcuts::pml + Shortcuts::sigma + 9*2 + 1] = -dmp_pml * s_y * v_s;
      BTimesDeltaQ[Shortcuts::pml + Shortcuts::sigma + 9*2 + 2] = -dmp_pml * s_z * w_s;
      BTimesDeltaQ[Shortcuts::pml + Shortcuts::sigma + 9*2 + 3] = -dmp_pml * (s_y * u_s + s_x * v_s);
      BTimesDeltaQ[Shortcuts::pml + Shortcuts::sigma + 9*2 + 4] = -dmp_pml * (s_z * u_s + s_x * w_s);
      BTimesDeltaQ[Shortcuts::pml + Shortcuts::sigma + 9*2 + 5] = -dmp_pml * (s_z * v_s + s_y * w_s);
    }
  }
"""


def algebraic_source():
    return """
  auto rho = Q[Shortcuts::rho];
  auto cp  = Q[Shortcuts::cp];
  auto cs  = Q[Shortcuts::cs];

  auto dmp_pml_x = Q[Shortcuts::dmp_pml + 0];
  auto dmp_pml_y = Q[Shortcuts::dmp_pml + 1];
  auto dmp_pml_z = Q[Shortcuts::dmp_pml + 2];

  auto mu      = rho * cs * cs;
  auto lambda  = rho * cp * cp - 2 * mu;
  auto rho_inv = 1 / rho;

  auto alpha_x = (PmlAlphaConstant + PmlAlphaScalar * dmp_pml_x);
  auto alpha_y = (PmlAlphaConstant + PmlAlphaScalar * dmp_pml_y);
  auto alpha_z = (PmlAlphaConstant + PmlAlphaScalar * dmp_pml_z);

  S[Shortcuts::v + 0] = -rho_inv * (Q[Shortcuts::pml + 9*0 + 0] + Q[Shortcuts::pml + 9*1 + 0] + Q[Shortcuts::pml + 9*2 + 0]);
  S[Shortcuts::v + 1] = -rho_inv * (Q[Shortcuts::pml + 9*0 + 1] + Q[Shortcuts::pml + 9*1 + 1] + Q[Shortcuts::pml + 9*2 + 1]);
  S[Shortcuts::v + 2] = -rho_inv * (Q[Shortcuts::pml + 9*0 + 2] + Q[Shortcuts::pml + 9*1 + 2] + Q[Shortcuts::pml + 9*2 + 2]);

  S[Shortcuts::sigma + 0] = -((2 * mu + lambda) * Q[Shortcuts::pml + 9*0 + 3] + lambda * (Q[Shortcuts::pml + 9*0 + 4] + Q[Shortcuts::pml + 9*0 + 5])
                            + (2 * mu + lambda) * Q[Shortcuts::pml + 9*1 + 3] + lambda * (Q[Shortcuts::pml + 9*1 + 4] + Q[Shortcuts::pml + 9*1 + 5])
                            + (2 * mu + lambda) * Q[Shortcuts::pml + 9*2 + 3] + lambda * (Q[Shortcuts::pml + 9*2 + 4] + Q[Shortcuts::pml + 9*2 + 5]));

  S[Shortcuts::sigma + 1] = -((2 * mu + lambda) * Q[Shortcuts::pml + 9*0 + 4] + lambda * (Q[Shortcuts::pml + 9*0 + 3] + Q[Shortcuts::pml + 9*0 + 5])
                            + (2 * mu + lambda) * Q[Shortcuts::pml + 9*1 + 4] + lambda * (Q[Shortcuts::pml + 9*1 + 3] + Q[Shortcuts::pml + 9*1 + 5])
                            + (2 * mu + lambda) * Q[Shortcuts::pml + 9*2 + 4] + lambda * (Q[Shortcuts::pml + 9*2 + 3] + Q[Shortcuts::pml + 9*2 + 5]));

  S[Shortcuts::sigma + 2] = -((2 * mu + lambda) * Q[Shortcuts::pml + 9*0 + 5] + lambda * (Q[Shortcuts::pml + 9*0 + 3] + Q[Shortcuts::pml + 9*0 + 4])
                            + (2 * mu + lambda) * Q[Shortcuts::pml + 9*1 + 5] + lambda * (Q[Shortcuts::pml + 9*1 + 3] + Q[Shortcuts::pml + 9*1 + 4])
                            + (2 * mu + lambda) * Q[Shortcuts::pml + 9*2 + 5] + lambda * (Q[Shortcuts::pml + 9*2 + 3] + Q[Shortcuts::pml + 9*2 + 4]));

  S[Shortcuts::sigma + 3] = -mu * (Q[Shortcuts::pml + 9*0 + 6] + Q[Shortcuts::pml + 9*1 + 6] + Q[Shortcuts::pml + 9*2 + 6]);
  S[Shortcuts::sigma + 4] = -mu * (Q[Shortcuts::pml + 9*0 + 7] + Q[Shortcuts::pml + 9*1 + 7] + Q[Shortcuts::pml + 9*2 + 7]);
  S[Shortcuts::sigma + 5] = -mu * (Q[Shortcuts::pml + 9*0 + 8] + Q[Shortcuts::pml + 9*1 + 8] + Q[Shortcuts::pml + 9*2 + 8]);

  for (int j = 0; j < 9; j++) {
    S[Shortcuts::pml + 9*0 + j] = -(dmp_pml_x + alpha_x) * Q[Shortcuts::pml + 9*0 + j];
    S[Shortcuts::pml + 9*1 + j] = -(dmp_pml_y + alpha_y) * Q[Shortcuts::pml + 9*1 + j];
    S[Shortcuts::pml + 9*2 + j] = -(dmp_pml_z + alpha_z) * Q[Shortcuts::pml + 9*2 + j];
  }
"""


def multiplyMaterialParameterMatrix():
    return """
  auto rho = Q[Shortcuts::rho];
  auto jacobian = Q[Shortcuts::jacobian];

  auto rho_inv = 1 / (rho * jacobian);

  auto dmp_pml_x = Q[Shortcuts::dmp_pml + 0];
  auto dmp_pml_y = Q[Shortcuts::dmp_pml + 1];
  auto dmp_pml_z = Q[Shortcuts::dmp_pml + 2];

  rhs[Shortcuts::v + 0] = rho_inv * rhs[Shortcuts::v + 0];
  rhs[Shortcuts::v + 1] = rho_inv * rhs[Shortcuts::v + 1];
  rhs[Shortcuts::v + 2] = rho_inv * rhs[Shortcuts::v + 2];

  for (int j = 0; j < 3; j++) {
    rhs[Shortcuts::pml + 9*0 + j] = dmp_pml_x / jacobian * rhs[Shortcuts::pml + 9*0 + j];
    rhs[Shortcuts::pml + 9*1 + j] = dmp_pml_y / jacobian * rhs[Shortcuts::pml + 9*1 + j];
    rhs[Shortcuts::pml + 9*2 + j] = dmp_pml_z / jacobian * rhs[Shortcuts::pml + 9*2 + j];
  }
"""


def riemann_solver():
    return """
  applications::exahype2::exaseis::riemannSolverPML<VariableShortcuts{{SOLVER_NAME}}, {{CORRECTOR_COMPUTATION_PRECISION}}, DGOrder, NumberOfUnknowns, NumberOfAuxiliaryVariables, 1>(
    FL, FR, QL, QR, dt, direction, isBoundaryFace, faceIndex
  );
"""


def userIncludes():
    return [
        "../riemannSolvers/riemannsolverPML.h",
        "exahype2/Curvi.h",
        "exahype2/dg/DGUtils.h",
        "tarch/reader/NetCDFFieldSampler.h",
    ]


def abstractDeclarations():
    return """
protected:
  static constexpr int     PmlCellWidth     = 1;
  static constexpr double  PmlAlphaConstant = 1.5;
  static constexpr double  PmlAlphaScalar   = 0.0;
  static constexpr double  PmlRelativeError = 0.001;
  static constexpr int     PmlPower         = 1;
"""
