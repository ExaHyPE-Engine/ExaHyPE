// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "fvSWE.h"
#include "exahype2/RefinementControl.h"

tarch::logging::Log   tests::exahype2::fortran::fvSWE::_log( "tests::exahype2::fortran::fvSWE" );


double tests::exahype2::fortran::fvSWE::maxEigenvalue(
  [[maybe_unused]] const double* const NOALIAS             Q, // Q[3+1]
  [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& x,
  [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& h,
  [[maybe_unused]] const double                                 t,
  [[maybe_unused]] const double                                 dt,
  [[maybe_unused]] const int                                    normal
) {
  static constexpr double g = 9.81;
  const double u = Q[0] > 1e-5 ? Q[1 + normal] / Q[0] : 0.0;
  const double c = Q[0] > 1e-5 ? sqrt(g * Q[0]) : 0.0;
  return std::max(std::abs(u - c), std::abs(u + c));
}

void tests::exahype2::fortran::fvSWE::flux(
  [[maybe_unused]] const double* const NOALIAS             Q, // Q[3+1]
  [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& x,
  [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& h_,
  [[maybe_unused]] const double                                 t,
  [[maybe_unused]] const double                                 dt,
  [[maybe_unused]] const int                                    normal,
  [[maybe_unused]] double* const NOALIAS                   F  // F[3]
) {
  static constexpr double g = 9.81;

  const double ih = Q[0] > 1e-5 ? 1.0 / Q[0] : 0.0;
  F[0]         = Q[1 + normal];
  F[1 + 0]    = ih * Q[1 + normal] * Q[1 + 0];
  F[1 + 1]    = ih * Q[1 + normal] * Q[1 + 1];
  F[1 + normal] += 0.5 * g * Q[0] * Q[0];

}

extern "C" {
  double rpn2_augmented_wrapper(
    const double* NOALIAS ql,
    const double* NOALIAS qr,
    double* NOALIAS amdq,
    double* NOALIAS apdq, const int* normal);
}

double tests::exahype2::fortran::fvSWE::solveRiemannProblem(
  [[maybe_unused]] const double* const NOALIAS             QL, // QL[3+1]
  [[maybe_unused]] const double* const NOALIAS             QR, // QR[3+1]
  [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& x,
  [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& h,
  [[maybe_unused]] const double                                 t,
  [[maybe_unused]] const double                                 dt,
  [[maybe_unused]] const int                                    normal,
  [[maybe_unused]] double* const NOALIAS                   FL, // FL[3]
  [[maybe_unused]] double* const NOALIAS                   FR  // FR[3]
) {
#if 0
  double smax = rpn2_augmented_wrapper(QL, QR, FL, FR, &normal);
#else
  const double cL = std::sqrt(9.81 * QL[0]);
  double uL = QL[normal + 1] * QL[0] * std::sqrt(2)/std::sqrt(std::pow(QL[0], 4) + std::pow(std::max(QL[0], 1e-5), 4));

  double smaxL = std::max(std::abs(uL - cL), std::abs(uL + cL));

  const double cR = std::sqrt(9.81 * QR[0]);
  double uR = QR[normal + 1] * QR[0] * std::sqrt(2)/std::sqrt(std::pow(QR[0], 4) + std::pow(std::max(QR[0], 1e-5), 4));

  double smaxR = std::max(std::abs(uR - cR), std::abs(uR + cR));

  double smax = std::max(smaxL, smaxR);

  // double smax = std::max(smax, maxEigenvalue(QL, x, h, t, dt, normal));
  //        smax = std::max(smax, maxEigenvalue(QR, x, h, t, dt, normal));

  flux(QL, x, h, t, dt, normal, FL);
  flux(QR, x, h, t, dt, normal, FR);

  // h gets added term from bathimetry, u and v are regular Rusanov, b does not change
  FL[0] = 0.5*(FL[0]+FR[0] + smax*(QL[0]+QL[3]-QR[0]-QR[3]) );
  FL[1] = 0.5*(FL[1]+FR[1] + smax*(QL[1]-QR[1]) );
  FL[2] = 0.5*(FL[2]+FR[2] + smax*(QL[2]-QR[2]) );
  // FL[3] = 0.0;

  FR[0] = FL[0];
  FR[1] = FL[1];
  FR[2] = FL[2];
  // FR[3] = 0.0;

  //Contribution from NCP
  FL[normal+1] += 0.5*9.81*0.5*(QL[0]+QR[0])*(QR[3]-QL[3]);
  FR[normal+1] -= 0.5*9.81*0.5*(QL[0]+QR[0])*(QR[3]-QL[3]);
#endif

 return smax;
}
