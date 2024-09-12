// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "FWave.h"

#include "Constants.h"
#include "tarch/Assertions.h"
#include "tarch/NonCriticalAssertions.h"
#include "VariableShortcuts.h"

using s = applications::exahype2::swe::VariableShortcuts;

#if defined(GPUOffloadingOMP)
#pragma omp declare target
#endif
double applications::exahype2::swe::fwave(
  const double* __restrict__ QR,
  const double* __restrict__ QL,
  const double* __restrict__ FR,
  const double* __restrict__ FL,
  const double* __restrict__ LR,
  const double* __restrict__ LL,
  const ::tarch::la::Vector<Dimensions, double>& xR,
  const ::tarch::la::Vector<Dimensions, double>& xL,
  const ::tarch::la::Vector<Dimensions, double>& h,
  double                                         t,
  double                                         dt,
  int                                            normal,
  double* __restrict__ APDQ,
  double* __restrict__ AMDQ
) {
  const bool leftWet  = tarch::la::greater(QL[s::h], DRY_TOLERANCE);
  const bool rightWet = tarch::la::greater(QR[s::h], DRY_TOLERANCE);
  const bool leftDry  = !leftWet;
  const bool rightDry = !rightWet;

  for (int i = 0; i < 3; i++) {
    AMDQ[i] = 0.0;
    APDQ[i] = 0.0;
  }

  // Dry land
  if (leftDry or rightDry) {
    return 0.0;
  }

  const double m_h_l = leftWet * QL[s::h] + leftDry * QR[s::h];
  const double m_h_r = rightWet * QR[s::h] + rightDry * QL[s::h];

  const double m_hu_l = leftWet * QL[normal + 1] - leftDry * QR[normal + 1];
  const double m_hu_r = rightWet * QR[normal + 1] - rightDry * QL[normal + 1];

  const double bat_l = leftWet * QL[s::b] + leftDry * QR[s::b];
  const double bat_r = rightWet * QR[s::b] + rightDry * QL[s::b];

  const double u[2]{m_hu_l / m_h_l, m_hu_r / m_h_r};
  const double h_roe = (m_h_l + m_h_r) / 2;
  const double u_roe = (u[0] * std::sqrt(m_h_l) + u[1] * std::sqrt(m_h_r))
                       / (std::sqrt(m_h_l) + std::sqrt(m_h_r));
  const double
    lambda_roe[2]{u_roe - std::sqrt(g * h_roe), u_roe + std::sqrt(g * h_roe)};

  // const double f_q_l[2]{leftWet * FL[0] - leftDry * FR[0], leftWet *
  // FL[normal+1] + leftDry * FR[normal+1]}; const double f_q_r[2]{rightWet *
  // FR[0] - rightDry * FL[0], rightWet * FR[normal+1] + rightDry *
  // FL[normal+1]};

  double f_q_l[2];
  if (leftWet) {
    f_q_l[0] = FL[s::h];
    f_q_l[1] = FL[normal + 1];
  } else {
    f_q_l[0] = FR[s::h];
    f_q_l[1] = FR[normal + 1];
  }

  double f_q_r[2];
  if (rightWet) {
    f_q_r[0] = FR[s::h];
    f_q_r[1] = FR[normal + 1];
  } else {
    f_q_r[0] = FL[s::h];
    f_q_r[1] = FL[normal + 1];
  }

  const double delta_x_psi = -g * (bat_r - bat_l) * h_roe;
  const double
    delta_f[2]{f_q_r[0] - f_q_l[0], f_q_r[1] - f_q_l[1] - delta_x_psi};

  const double determinant{1 / (lambda_roe[1] - lambda_roe[0])};
  const double deter_mul_del_f_1 = determinant * delta_f[1];
  const double deter_mul_del_f_0 = determinant * delta_f[0];
  const double alpha[]{
    lambda_roe[1] * deter_mul_del_f_0 - deter_mul_del_f_1,
    -(lambda_roe[0]) * deter_mul_del_f_0 + deter_mul_del_f_1};

  const double z_1[2]{alpha[0], alpha[0] * lambda_roe[0]};
  const double z_2[2]{alpha[1], alpha[1] * lambda_roe[1]};

  /*
      APDQ[0] = rightWet * -((tarch::la::greater(lambda_roe[0], 0.0) ? z_1[0] :
     0) + (tarch::la::greater(lambda_roe[1], 0) ? z_2[0] : 0)); APDQ[1] = 0.0;
      APDQ[2] = 0.0;
      APDQ[normal+1] = rightWet * -((tarch::la::greater(lambda_roe[0], 0) ?
     z_1[1] : 0) + (tarch::la::greater(lambda_roe[1], 0) ? z_2[1] : 0));

      AMDQ[0] = leftWet * (tarch::la::smaller(lambda_roe[0], 0) ? z_1[0] : 0) +
     (tarch::la::smaller(lambda_roe[1], 0) ? z_2[0] : 0); AMDQ[1] = 0.0; AMDQ[2]
     = 0.0; AMDQ[normal+1] = leftWet * (tarch::la::smaller(lambda_roe[0], 0) ?
     z_1[1] : 0) + (tarch::la::smaller(lambda_roe[1], 0) ? z_2[1] : 0);
  */

  if (lambda_roe[0] > 0 && rightWet) {
    APDQ[s::h] -= z_1[0];
    APDQ[normal + 1] -= z_1[1];
  } else if (lambda_roe[0] < 0 && leftWet) {
    AMDQ[s::h] += z_1[0];
    AMDQ[normal + 1] += z_1[1];
  }

  if (lambda_roe[1] > 0 && rightWet) {
    APDQ[s::h] -= z_2[0];
    APDQ[normal + 1] -= z_2[1];
  } else if (lambda_roe[0] < 0 && leftWet) {
    AMDQ[s::h] += z_2[0];
    AMDQ[normal + 1] += z_2[1];
  }

  return std::max(std::abs(lambda_roe[0]), std::abs(lambda_roe[1]));
}
#if defined(GPUOffloadingOMP)
#pragma omp end declare target
#endif
