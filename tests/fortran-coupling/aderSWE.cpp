// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "aderSWE.h"
#include "exahype2/RefinementControl.h"

tarch::logging::Log   tests::exahype2::fortran::aderSWE::_log( "tests::exahype2::fortran::aderSWE" );


double tests::exahype2::fortran::aderSWE::maxEigenvalue(
  [[maybe_unused]] const double* NOALIAS Q, // Q[4+0]
  [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>&      x,
  [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>&      h,
  [[maybe_unused]] double                                            t,
  [[maybe_unused]] double                                            dt,
  [[maybe_unused]] int                                               normal
) {

  static constexpr double g = 9.81;
  static constexpr double zeroFlux = 1e-3;

  if(Q[0] < zeroFlux)
  {
      return 0;
  }

  double u = Q[normal+1] / Q[0];
  double c = std::sqrt(g * Q[0]);

  return std::max(std::abs(u - c), std::abs(u + c));

}

void tests::exahype2::fortran::aderSWE::flux(
  [[maybe_unused]] const double* NOALIAS Q, // Q[4+0]
  [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>&      x,
  [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>&      h_,
  [[maybe_unused]] double                                            t,
  [[maybe_unused]] double                                            dt,
  [[maybe_unused]] int                                               normal,
  [[maybe_unused]] double* NOALIAS       F // F[4]
) {

  static constexpr double g = 9.81;
  static constexpr double zeroFlux = 1e-3;

  double h = Q[0];

  if(h < zeroFlux)
  {
      F[0] = 0;
      F[1] = 0;
      F[2] = 0;
      F[3] = 0;
      return;
  }

  double hu = Q[1];
  double hv = Q[2];
  double u = Q[1] / h;
  double v = Q[2] / h;

  if (normal == 0) {
      F[0] = hu;
      F[1] = h * u * u + 0.5 * g * h * h;
      F[2] = h * u * v;
      F[3] = 0;
  } else {
      F[0] = hv;
      F[1] = h * u * v;
      F[2] = h * v * v + 0.5 * g * h * h;
      F[3] = 0;
  }

}

void tests::exahype2::fortran::aderSWE::nonconservativeProduct(
  [[maybe_unused]] const double* NOALIAS Q, // Q[4+0]
  [[maybe_unused]] const double* NOALIAS deltaQ, // deltaQ[4+0]
  [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>&      x,
  [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>&      h,
  [[maybe_unused]] double                                            t,
  [[maybe_unused]] double                                            dt,
  [[maybe_unused]] int                                               normal,
  [[maybe_unused]] double* NOALIAS       BTimesDeltaQ // BTimesDeltaQ[4]
) {

  static constexpr double g = 9.81;
  static constexpr double zeroFlux = 1e-3;

  if(Q[0] < zeroFlux)
  {
      BTimesDeltaQ[0] = 0.0;
      BTimesDeltaQ[1] = 0.0;
      BTimesDeltaQ[2] = 0.0;
      BTimesDeltaQ[3] = 0.0;
      return;
  }

  switch (normal) {
      case 0: //x
          BTimesDeltaQ[0] = 0.0;
          BTimesDeltaQ[1] = g * Q[0] * (deltaQ[3]);
          BTimesDeltaQ[2] = 0.0;
          BTimesDeltaQ[3] = 0.0;
          break;
      case 1: //y
          BTimesDeltaQ[0] = 0.0;
          BTimesDeltaQ[1] = 0.0;
          BTimesDeltaQ[2] = g * Q[0] * (deltaQ[3]);
          BTimesDeltaQ[3] = 0.0;
          break;
  }

}

void tests::exahype2::fortran::aderSWE::riemannSolver(
  double* const FL, // FL[4
  double* const FR, // FR[4
  const double* const QL, // QL[4+0]
  const double* const QR, // QR[4+0]
  const double t,
  const double dt,
  const tarch::la::Vector<DIMENSIONS, double>& x,
  const tarch::la::Vector<DIMENSIONS, double>& h,
  const int direction,
  bool isBoundaryFace,
  int faceIndex
) {

  //Compute max eigenvalue over face
  double smax = 0.0;
  for(int xy=0; xy<Order+1; xy++){
    smax = std::max(smax, maxEigenvalue(&QL[xy*4], x, h, t, dt, direction));
    smax = std::max(smax, maxEigenvalue(&QR[xy*4], x, h, t, dt, direction));

  }

  for(int xy=0; xy<Order+1; xy++){

    // h gets added term from bathimetry, u and v are regular Rusanov, b does not change
    FL[xy*4+0] = 0.5*(FL[xy*4+0]+FR[xy*4+0] + smax*(QL[xy*4+0]+QL[xy*4+3]-QR[xy*4+0]-QR[xy*4+3]) );
    FL[xy*4+1] = 0.5*(FL[xy*4+1]+FR[xy*4+1] + smax*(QL[xy*4+1]-QR[xy*4+1]) );
    FL[xy*4+2] = 0.5*(FL[xy*4+2]+FR[xy*4+2] + smax*(QL[xy*4+2]-QR[xy*4+2]) );
    FL[xy*4+3] = 0.0;

    FR[xy*4+0] = FL[xy*4+0];
    FR[xy*4+1] = FL[xy*4+1];
    FR[xy*4+2] = FL[xy*4+2];
    FR[xy*4+3] = 0.0;

    //Contribution from NCP
    FL[xy*4+direction+1] += 0.5*9.81*0.5*(QL[xy*4+0]+QR[xy*4+0])*(QR[xy*4+3]-QL[xy*4+3]);
    FR[xy*4+direction+1] -= 0.5*9.81*0.5*(QL[xy*4+0]+QR[xy*4+0])*(QR[xy*4+3]-QL[xy*4+3]);

  }

}
