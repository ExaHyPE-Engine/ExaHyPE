

def initial(pointwise_initial_conditions):
  return """
  std::fill_n(Q, (NumberOfAuxiliaryVariables + NumberOfUnknowns) * (Order + 1) * (Order + 1) * (Order + 1), 0);

  context->initUnknownsPatch(Q, x, h, 0.0, 0.0,
  [&](
      const {{SOLUTION_STORAGE_PRECISION}}* const x,
      const tarch::la::Vector<Dimensions,double>& center,
      const double t,
      const double dt, 
      {{SOLUTION_STORAGE_PRECISION}}* Q
    ) -> void { """ + pointwise_initial_conditions + """
    }
  );

  // Initialisation of PML parameters
  auto dmp_pml_width_x = pml_cell_width * h[0];
  auto dmp_pml_width_y = pml_cell_width * h[1];
  auto dmp_pml_width_z = pml_cell_width * h[2];

  auto dmp_pml_right_x = DomainOffset[0] + DomainSize[0] - dmp_pml_width_x;
  auto dmp_pml_right_y = DomainOffset[1] + DomainSize[1] - dmp_pml_width_y;
  auto dmp_pml_right_z = DomainOffset[2] + DomainSize[2] - dmp_pml_width_z;

  auto dmp_pml_left_x = DomainOffset[0] + dmp_pml_width_x;
  auto dmp_pml_left_y = DomainOffset[1] + dmp_pml_width_y;
  auto dmp_pml_left_z = DomainOffset[2] + dmp_pml_width_z;

  constexpr double amplitude_nominator = 6.0;

  auto dmp_amplitude_x = (pml_power + 1) * amplitude_nominator / (2 * dmp_pml_width_x) * log(1.0 / pml_rel_error);
  auto dmp_amplitude_y = (pml_power + 1) * amplitude_nominator / (2 * dmp_pml_width_y) * log(1.0 / pml_rel_error);
  auto dmp_amplitude_z = (pml_power + 1) * amplitude_nominator / (2 * dmp_pml_width_z) * log(1.0 / pml_rel_error);

  double offset_x = x[0] - h[0] * 0.5;
  double offset_y = x[1] - h[1] * 0.5;
  double offset_z = x[2] - h[2] * 0.5;

  idx4 idx(Order + 1, Order + 1, Order + 1, NumberOfUnknowns + NumberOfAuxiliaryVariables);

  for (int k = 0; k < Order + 1; k++) { 
    double computational_z = (offset_z + h[2] * kernels::{{SOLVER_NAME}}::Quadrature<{{SOLUTION_STORAGE_PRECISION}}>::nodes[k]);
    for (int j = 0; j < Order + 1; j++) {
      double computational_y = (offset_y + h[1] * kernels::{{SOLVER_NAME}}::Quadrature<{{SOLUTION_STORAGE_PRECISION}}>::nodes[j]);
      for (int i = 0; i < Order + 1; i++) {
        double computational_x = (offset_x + h[0] * kernels::{{SOLVER_NAME}}::Quadrature<{{SOLUTION_STORAGE_PRECISION}}>::nodes[i]);

        if (_TOP != 0) { //  No PML on surface
          if (computational_x < dmp_pml_left_x) {
            Q[idx(
              k, j, i, s::dmp_pml + 0
            )] = dmp_amplitude_x * pow((dmp_pml_left_x - computational_x) / dmp_pml_width_x, pml_power);
          }
        }
        if (_TOP != 1) { //  No PML on surface
          if (computational_y <= dmp_pml_left_y) {
            Q[idx(
              k, j, i, s::dmp_pml + 1
            )] = dmp_amplitude_y * pow((dmp_pml_left_y - computational_y) / dmp_pml_width_y, pml_power);
          }
        }
        if (_TOP != 2) { //  No PML on surface
          if (computational_z <= dmp_pml_left_z) {
            Q[idx(
              k, j, i, s::dmp_pml + 2
            )] = dmp_amplitude_z * pow((dmp_pml_left_z - computational_z) / dmp_pml_width_z, pml_power);
          }
        }
        if (computational_x >= dmp_pml_right_x) {
          Q[idx(
            k, j, i, s::dmp_pml + 0
          )] = dmp_amplitude_x * pow((computational_x - dmp_pml_right_x) / dmp_pml_width_x, pml_power);
        }
        if (computational_y >= dmp_pml_right_y) {
          Q[idx(
            k, j, i, s::dmp_pml + 1
          )] = dmp_amplitude_y * pow((computational_y - dmp_pml_right_y) / dmp_pml_width_y, pml_power);
        }
        if (computational_z >= dmp_pml_right_z) {
          Q[idx(
            k, j, i, s::dmp_pml + 2
          )] = dmp_amplitude_z * pow((computational_z - dmp_pml_right_z) / dmp_pml_width_z, pml_power);
        }
      }
    }
  }
"""


def boundary():
  return """
  for (int i = 0; i < NumberOfUnknowns; i++) {
    Qoutside[i] = Qinside[i];
  }
"""

def eigenvalue():
  return """
  // Check for NaN in any of the velocities
  assert(("Check for Nans in velocity", !(std::isnan(Q[s::v + 0]) || std::isnan(Q[s::v + 1]) || std::isnan(Q[s::v + 2]))));

  auto cp  = Q[s::cp];
  auto cs  = Q[s::cs];
  auto q_x = Q[s::metric_derivative + 0];
  auto q_y = Q[s::metric_derivative + 1];
  auto q_z = Q[s::metric_derivative + 2];
  auto r_x = Q[s::metric_derivative + 3];
  auto r_y = Q[s::metric_derivative + 4];
  auto r_z = Q[s::metric_derivative + 5];
  auto s_x = Q[s::metric_derivative + 6];
  auto s_y = Q[s::metric_derivative + 7];
  auto s_z = Q[s::metric_derivative + 8];

  double lambda[10]{1.0};

  lambda[0] = std::sqrt(q_x * q_x + q_y * q_y + q_z * q_z) * cp;
  lambda[1] = std::sqrt(q_x * q_x + q_y * q_y + q_z * q_z) * cs;
  lambda[2] = std::sqrt(q_x * q_x + q_y * q_y + q_z * q_z) * cs;

  lambda[3] = std::sqrt(r_x * r_x + r_y * r_y + r_z * r_z) * cp;
  lambda[4] = std::sqrt(r_x * r_x + r_y * r_y + r_z * r_z) * cs;
  lambda[5] = std::sqrt(r_x * r_x + r_y * r_y + r_z * r_z) * cs;

  lambda[6] = std::sqrt(s_x * s_x + s_y * s_y + s_z * s_z) * cp;
  lambda[7] = std::sqrt(s_x * s_x + s_y * s_y + s_z * s_z) * cs;
  lambda[8] = std::sqrt(s_x * s_x + s_y * s_y + s_z * s_z) * cs;
  lambda[9] = 1.0;

  return *std::max_element(lambda, lambda + 10);
"""

def flux():
  return """
  auto sigma_xx = Q[s::sigma + 0];
  auto sigma_yy = Q[s::sigma + 1];
  auto sigma_zz = Q[s::sigma + 2];
  auto sigma_xy = Q[s::sigma + 3];
  auto sigma_xz = Q[s::sigma + 4];
  auto sigma_yz = Q[s::sigma + 5];

  auto jacobian = Q[s::jacobian];

  auto rho     = Q[s::rho];
  auto rho_inv = 1.0 / rho;

  std::fill_n(F, NumberOfUnknowns, 0.0);

  idx2 idx_pml(Dimensions, 9);

  switch (normal) {
    case 0: {
      auto q_x                     = Q[s::metric_derivative + 0];
      auto q_y                     = Q[s::metric_derivative + 1];
      auto q_z                     = Q[s::metric_derivative + 2];
      F[s::v + 0]                     = -jacobian * (q_x * sigma_xx + q_y * sigma_xy + q_z * sigma_xz);
      F[s::v + 1]                     = -jacobian * (q_x * sigma_xy + q_y * sigma_yy + q_z * sigma_yz);
      F[s::v + 2]                     = -jacobian * (q_x * sigma_xz + q_y * sigma_yz + q_z * sigma_zz);
      F[s::pml + idx_pml(0, s::v + 0)] = F[s::v + 0];
      F[s::pml + idx_pml(0, s::v + 1)] = F[s::v + 1];
      F[s::pml + idx_pml(0, s::v + 2)] = F[s::v + 2];
    } break;
    case 1: {
      auto r_x                     = Q[s::metric_derivative + 3];
      auto r_y                     = Q[s::metric_derivative + 4];
      auto r_z                     = Q[s::metric_derivative + 5];
      F[s::v + 0]                     = -jacobian * (r_x * sigma_xx + r_y * sigma_xy + r_z * sigma_xz);
      F[s::v + 1]                     = -jacobian * (r_x * sigma_xy + r_y * sigma_yy + r_z * sigma_yz);
      F[s::v + 2]                     = -jacobian * (r_x * sigma_xz + r_y * sigma_yz + r_z * sigma_zz);
      F[s::pml + idx_pml(1, s::v + 0)] = F[s::v + 0];
      F[s::pml + idx_pml(1, s::v + 1)] = F[s::v + 1];
      F[s::pml + idx_pml(1, s::v + 2)] = F[s::v + 2];
    } break;
    case 2: {
      auto s_x                     = Q[s::metric_derivative + 6];
      auto s_y                     = Q[s::metric_derivative + 7];
      auto s_z                     = Q[s::metric_derivative + 8];
      F[s::v + 0]                     = -jacobian * (s_x * sigma_xx + s_y * sigma_xy + s_z * sigma_xz);
      F[s::v + 1]                     = -jacobian * (s_x * sigma_xy + s_y * sigma_yy + s_z * sigma_yz);
      F[s::v + 2]                     = -jacobian * (s_x * sigma_xz + s_y * sigma_yz + s_z * sigma_zz);
      F[s::pml + idx_pml(2, s::v + 0)] = F[s::v + 0];
      F[s::pml + idx_pml(2, s::v + 1)] = F[s::v + 1];
      F[s::pml + idx_pml(2, s::v + 2)] = F[s::v + 2];
    }
  }
"""

def ncp():
  return """
  idx2 idx_pml(Dimensions, 9);
  auto        rho       = Q[s::rho];
  auto        cp        = Q[s::cp];
  auto        cs        = Q[s::cs];
  auto        jacobian  = Q[s::jacobian];
  auto        dmp_pml_x = Q[s::dmp_pml + 0];
  auto        dmp_pml_y = Q[s::dmp_pml + 1];
  auto        dmp_pml_z = Q[s::dmp_pml + 2];
  auto        mu        = rho * cs * cs;
  auto        lambda    = rho * cp * cp - 2 * mu;
  auto        rho_inv   = 1.0 / (rho * jacobian);

  std::fill_n(BTimesDeltaQ, NumberOfUnknowns, 0.0);

  switch (normal) {
    case 0: {
      auto u_q                              = deltaQ[s::v + 0];
      auto v_q                              = deltaQ[s::v + 1];
      auto w_q                              = deltaQ[s::v + 2];
      auto q_x                              = Q[s::metric_derivative + 0];
      auto q_y                              = Q[s::metric_derivative + 1];
      auto q_z                              = Q[s::metric_derivative + 2];
      auto lam_temp                         = lambda * (-q_x * u_q - q_y * v_q - q_z * w_q);
      BTimesDeltaQ[s::sigma + 0]                     = -2 * mu * q_x * u_q + lam_temp;
      BTimesDeltaQ[s::sigma + 1]                     = -2 * mu * q_y * v_q + lam_temp;
      BTimesDeltaQ[s::sigma + 2]                     = -2 * mu * q_z * w_q + lam_temp;
      BTimesDeltaQ[s::sigma + 3]                     = -mu * (q_y * u_q + q_x * v_q); // sigma_xy
      BTimesDeltaQ[s::sigma + 4]                     = -mu * (q_z * u_q + q_x * w_q); // sigma_xz
      BTimesDeltaQ[s::sigma + 5]                     = -mu * (q_z * v_q + q_y * w_q); // sigma_yz
      BTimesDeltaQ[s::pml + idx_pml(0, s::sigma + 0)] = -dmp_pml_x * q_x * u_q;
      BTimesDeltaQ[s::pml + idx_pml(0, s::sigma + 1)] = -dmp_pml_x * q_y * v_q;
      BTimesDeltaQ[s::pml + idx_pml(0, s::sigma + 2)] = -dmp_pml_x * q_z * w_q;
      BTimesDeltaQ[s::pml + idx_pml(0, s::sigma + 3)] = -dmp_pml_x * (q_y * u_q + q_x * v_q);
      BTimesDeltaQ[s::pml + idx_pml(0, s::sigma + 4)] = -dmp_pml_x * (q_z * u_q + q_x * w_q);
      BTimesDeltaQ[s::pml + idx_pml(0, s::sigma + 5)] = -dmp_pml_x * (q_z * v_q + q_y * w_q);
    } break;
    case 1: {
      auto u_r                              = deltaQ[s::v + 0];
      auto v_r                              = deltaQ[s::v + 1];
      auto w_r                              = deltaQ[s::v + 2];
      auto r_x                              = Q[s::metric_derivative + 3];
      auto r_y                              = Q[s::metric_derivative + 4];
      auto r_z                              = Q[s::metric_derivative + 5];
      auto lam_temp                         = lambda * (-r_x * u_r - r_y * v_r - r_z * w_r);
      BTimesDeltaQ[s::sigma + 0]                     = -2 * mu * r_x * u_r + lam_temp;
      BTimesDeltaQ[s::sigma + 1]                     = -2 * mu * r_y * v_r + lam_temp;
      BTimesDeltaQ[s::sigma + 2]                     = -2 * mu * r_z * w_r + lam_temp;
      BTimesDeltaQ[s::sigma + 3]                     = -mu * (r_y * u_r + r_x * v_r); // sigma_xy
      BTimesDeltaQ[s::sigma + 4]                     = -mu * (r_z * u_r + r_x * w_r); // sigma_xz
      BTimesDeltaQ[s::sigma + 5]                     = -mu * (r_z * v_r + r_y * w_r); // sigma_yz
      BTimesDeltaQ[s::pml + idx_pml(1, s::sigma + 0)] = -dmp_pml_y * r_x * u_r;
      BTimesDeltaQ[s::pml + idx_pml(1, s::sigma + 1)] = -dmp_pml_y * r_y * v_r;
      BTimesDeltaQ[s::pml + idx_pml(1, s::sigma + 2)] = -dmp_pml_y * r_z * w_r;
      BTimesDeltaQ[s::pml + idx_pml(1, s::sigma + 3)] = -dmp_pml_y * (r_y * u_r + r_x * v_r);
      BTimesDeltaQ[s::pml + idx_pml(1, s::sigma + 4)] = -dmp_pml_y * (r_z * u_r + r_x * w_r);
      BTimesDeltaQ[s::pml + idx_pml(1, s::sigma + 5)] = -dmp_pml_y * (r_z * v_r + r_y * w_r);
    } break;
    case 2: {
      auto u_s                              = deltaQ[s::v + 0];
      auto v_s                              = deltaQ[s::v + 1];
      auto w_s                              = deltaQ[s::v + 2];
      auto s_x                              = Q[s::metric_derivative + 6];
      auto s_y                              = Q[s::metric_derivative + 7];
      auto s_z                              = Q[s::metric_derivative + 8];
      auto lam_temp                         = lambda * (-s_x * u_s - s_y * v_s - s_z * w_s);
      BTimesDeltaQ[s::sigma + 0]                     = -2 * mu * s_x * u_s + lam_temp;
      BTimesDeltaQ[s::sigma + 1]                     = -2 * mu * s_y * v_s + lam_temp;
      BTimesDeltaQ[s::sigma + 2]                     = -2 * mu * s_z * w_s + lam_temp;
      BTimesDeltaQ[s::sigma + 3]                     = -mu * (s_y * u_s + s_x * v_s); // sigma_xy
      BTimesDeltaQ[s::sigma + 4]                     = -mu * (s_z * u_s + s_x * w_s); // sigma_xz
      BTimesDeltaQ[s::sigma + 5]                     = -mu * (s_z * v_s + s_y * w_s); // sigma_yz
      BTimesDeltaQ[s::pml + idx_pml(2, s::sigma + 0)] = -dmp_pml_z * s_x * u_s;
      BTimesDeltaQ[s::pml + idx_pml(2, s::sigma + 1)] = -dmp_pml_z * s_y * v_s;
      BTimesDeltaQ[s::pml + idx_pml(2, s::sigma + 2)] = -dmp_pml_z * s_z * w_s;
      BTimesDeltaQ[s::pml + idx_pml(2, s::sigma + 3)] = -dmp_pml_z * (s_y * u_s + s_x * v_s);
      BTimesDeltaQ[s::pml + idx_pml(2, s::sigma + 4)] = -dmp_pml_z * (s_z * u_s + s_x * w_s);
      BTimesDeltaQ[s::pml + idx_pml(2, s::sigma + 5)] = -dmp_pml_z * (s_z * v_s + s_y * w_s);
    }
  }
"""

def algebraic_source():
  return """
  auto rho = Q[s::rho];
  auto cp  = Q[s::cp];
  auto cs  = Q[s::cs];

  auto dmp_pml_x = Q[s::dmp_pml + 0];
  auto dmp_pml_y = Q[s::dmp_pml + 1];
  auto dmp_pml_z = Q[s::dmp_pml + 2];

  auto mu      = rho * cs * cs;
  auto lambda  = rho * cp * cp - 2 * mu;
  auto rho_inv = 1.0 / rho;

  auto alpha_x = (pml_alpha_const + pml_alpha_scalar * dmp_pml_x);
  auto alpha_y = (pml_alpha_const + pml_alpha_scalar * dmp_pml_y);
  auto alpha_z = (pml_alpha_const + pml_alpha_scalar * dmp_pml_z);

  idx2 idx_pml(Dimensions, 9);

  const auto* Q_pml = Q + s::pml;
  auto*       S_pml = S + s::pml;

  S[0] = rho_inv * (Q_pml[idx_pml(0, 0)] + Q_pml[idx_pml(1, 0)] + Q_pml[idx_pml(2, 0)]);
  S[1] = rho_inv * (Q_pml[idx_pml(0, 1)] + Q_pml[idx_pml(1, 1)] + Q_pml[idx_pml(2, 1)]);
  S[2] = rho_inv * (Q_pml[idx_pml(0, 2)] + Q_pml[idx_pml(1, 2)] + Q_pml[idx_pml(2, 2)]);

  S[3] = (2 * mu + lambda) * Q_pml[idx_pml(0, 3)] + lambda * (Q_pml[idx_pml(0, 4)] + Q_pml[idx_pml(0, 5)])
         + (2 * mu + lambda) * Q_pml[idx_pml(1, 3)] + lambda * (Q_pml[idx_pml(1, 4)] + Q_pml[idx_pml(1, 5)])
         + (2 * mu + lambda) * Q_pml[idx_pml(2, 3)] + lambda * (Q_pml[idx_pml(2, 4)] + Q_pml[idx_pml(2, 5)]);

  S[4] = (2 * mu + lambda) * Q_pml[idx_pml(0, 4)] + lambda * (Q_pml[idx_pml(0, 3)] + Q_pml[idx_pml(0, 5)])
         + (2 * mu + lambda) * Q_pml[idx_pml(1, 4)] + lambda * (Q_pml[idx_pml(1, 3)] + Q_pml[idx_pml(1, 5)])
         + (2 * mu + lambda) * Q_pml[idx_pml(2, 4)] + lambda * (Q_pml[idx_pml(2, 3)] + Q_pml[idx_pml(2, 5)]);

  S[5] = (2 * mu + lambda) * Q_pml[idx_pml(0, 5)] + lambda * (Q_pml[idx_pml(0, 3)] + Q_pml[idx_pml(0, 4)])
         + (2 * mu + lambda) * Q_pml[idx_pml(1, 5)] + lambda * (Q_pml[idx_pml(1, 3)] + Q_pml[idx_pml(1, 4)])
         + (2 * mu + lambda) * Q_pml[idx_pml(2, 5)] + lambda * (Q_pml[idx_pml(2, 3)] + Q_pml[idx_pml(2, 4)]);

  S[6] = mu * (Q_pml[idx_pml(0, 6)] + Q_pml[idx_pml(1, 6)] + Q_pml[idx_pml(2, 6)]);
  S[7] = mu * (Q_pml[idx_pml(0, 7)] + Q_pml[idx_pml(1, 7)] + Q_pml[idx_pml(2, 7)]);
  S[8] = mu * (Q_pml[idx_pml(0, 8)] + Q_pml[idx_pml(1, 8)] + Q_pml[idx_pml(2, 8)]);

  for (int j = 0; j < 9; j++) {
    S_pml[idx_pml(0, j)] = (dmp_pml_x + alpha_x) * Q_pml[idx_pml(0, j)];
    S_pml[idx_pml(1, j)] = (dmp_pml_y + alpha_y) * Q_pml[idx_pml(1, j)];
    S_pml[idx_pml(2, j)] = (dmp_pml_z + alpha_z) * Q_pml[idx_pml(2, j)];
  }
"""

def multiplyMaterialParameterMatrix():
  return """
  auto rho = Q[s::rho];
  auto cp  = Q[s::cp];
  auto cs  = Q[s::cs];

  auto jacobian = Q[s::jacobian];

  auto dmp_pml_x = Q[s::dmp_pml + 0];
  auto dmp_pml_y = Q[s::dmp_pml + 1];
  auto dmp_pml_z = Q[s::dmp_pml + 2];

  auto mu      = rho * cs * cs;
  auto lambda  = rho * cp * cp - 2 * mu;
  auto rho_inv = 1.0 / (rho * jacobian);

  idx2 idx_pml(Dimensions, 9);

  // Rhs uses the same formula regardless of dimension, hence no switch necessary

  rhs[s::v + 0] = rho_inv * rhs[s::v + 0];
  rhs[s::v + 1] = rho_inv * rhs[s::v + 1];
  rhs[s::v + 2] = rho_inv * rhs[s::v + 2];

  for (int j = 0; j < 3; j++) {
    rhs[s::pml + idx_pml(0, j)] = dmp_pml_x / jacobian * rhs[s::pml + idx_pml(0, j)];
    rhs[s::pml + idx_pml(1, j)] = dmp_pml_y / jacobian * rhs[s::pml + idx_pml(1, j)];
    rhs[s::pml + idx_pml(2, j)] = dmp_pml_z / jacobian * rhs[s::pml + idx_pml(2, j)];
  }
"""

def refinement_criterion():
  return """
  auto result = ::exahype2::RefinementCommand::Keep;
  return result;
"""

def riemann_solver():
  return """
  Numerics::riemannSolver<VariableShortcuts{{SOLVER_NAME}}, {{CORRECTOR_COMPUTATION_PRECISION}}, Order, NumberOfUnknowns, NumberOfAuxiliaryVariables, 1>(
    FL, FR, QL, QR, dt, direction, isBoundaryFace, faceIndex
  );
"""

def userIncludes():
  return """
#define _CUSTOM_COORDINATES
#define ASAGI_NOMPI
#define _TOP 1

#include "../ExaSeis_core/Curvilinear/ContextCurvilinear.h"
#include "../ExaSeis_core/Numerics/riemannsolver_pml.h"
"""

def abstractUserDefinitions():
  return """
ContextCurvilinear<{{NAMESPACE | join("::")}}::VariableShortcuts{{SOLVER_NAME}},
  {{NAMESPACE | join("::")}}::{{CLASSNAME}}::Order + 1,
  ({{NAMESPACE | join("::")}}::{{CLASSNAME}}::NumberOfUnknowns + {{NAMESPACE | join("::")}}::{{CLASSNAME}}::NumberOfAuxiliaryVariables),
  {{SOLUTION_STORAGE_PRECISION}}>* {{NAMESPACE | join("::")}}::{{CLASSNAME}}::context;
"""

def abstractDeclarations():
  return """
public:
  double QuadraturePoints1d[Order+1];
  
protected:
  static ContextCurvilinear<VariableShortcuts{{SOLVER_NAME}}, Order + 1, (NumberOfAuxiliaryVariables + NumberOfUnknowns), {{SOLUTION_STORAGE_PRECISION}}>* context;

  static constexpr int     pml_cell_width    = 1;
  static constexpr double  pml_alpha_const   = 1.5;
  static constexpr double  pml_alpha_scalar  = 0.0;
  static constexpr double  pml_rel_error     = 0.001;
  static constexpr int     pml_power         = 1;
"""

def init_grid_step_implementation(scenario_string):
  return """
  std::copy_n(
    kernels::{{SOLVER_NAME}}::Quadrature<{{SOLUTION_STORAGE_PRECISION}}>::nodes,
    (Order+1),
    QuadraturePoints1d    
  );

  std::string topography_string = \"""" + scenario_string + """.yaml";

  //setting coarsestMeshLevel and maxAdaptiveMeshLevel
  double maxDSize = std::numeric_limits<double>::min();
  double minDSize = std::numeric_limits<double>::max();
  tarch::la::Vector<Dimensions, double> sizes = DomainSize;
  
  for(int i=0; i<Dimensions; i++){
    if(sizes[i]>maxDSize){
      maxDSize = sizes[i];
    }
    if(sizes[i]<minDSize){
      minDSize = sizes[i];
    }    
  }
  
  int depth = 0;
  
  //finding coarsest possible Mesh level
  while(maxDSize>MaxAdmissibleCellH){
    depth += 1;
    maxDSize /= 3.0;
  }
  
  double realCoarsestMeshSize = maxDSize;
  double coarsestMeshLevel = depth;  
  
  depth = 0;
  while(minDSize>MinAdmissibleCellH){
    depth += 1;
    minDSize /= 3.0;
  }

  context = new ContextCurvilinear<VariableShortcuts{{SOLVER_NAME}}, Order + 1, (NumberOfAuxiliaryVariables + NumberOfUnknowns), {{SOLUTION_STORAGE_PRECISION}}>(
    topography_string,
    coarsestMeshLevel,
    realCoarsestMeshSize,
    depth,
    DomainOffset, DomainSize,
    kernels::{{SOLVER_NAME}}::Quadrature<{{SOLUTION_STORAGE_PRECISION}}>::nodes,
    kernels::{{SOLVER_NAME}}::DGMatrices<{{SOLUTION_STORAGE_PRECISION}}>::dudx
  );

  std::cout << "PML is using " << pml_cell_width << " elements" << std::endl;
  std::cout << "Freesurface set at " << _TOP * 2 << std::endl;
"""

def abstractDestructor():
  return """
  delete context;
"""
