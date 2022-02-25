#include "testUtils.h"

void exahype2::dg::tests::eulerFlux(
        const double * __restrict__                  Q,
        const tarch::la::Vector<Dimensions,double>&  x,
        double                                       t,
        int                                          normal,
        double * __restrict__                        F
      ){
  constexpr double gamma = 1.4;
  const double irho = 1./Q[0];
  const double p = (gamma-1) * (Q[4] - 0.5*irho*(Q[1]*Q[1]+Q[2]*Q[2]));

  const double velocity = irho*Q[normal+1];
  F[0] = velocity*Q[0];
  F[1] = velocity*Q[1];
  F[2] = velocity*Q[2];
  F[3] = velocity*Q[3];
  F[normal+1] += p;
  F[4] = velocity*(Q[4]+p);
}

void exahype2::dg::tests::eulerSource(
      const double * __restrict__                  Q,
      const tarch::la::Vector<Dimensions,double>&  x,
      double                                       t,
      double * __restrict__                        S
    ){
  S[0] = 0.0;
  S[1] = 0.0;
  S[2] = 0.0;
  S[3] = 0.0;
  S[4] = 0.0;
}

double exahype2::dg::tests::eulerEigenvalue(
      const double * __restrict__                 Q,
      const tarch::la::Vector<Dimensions,double>& x,
      double                                      t,
      int                                          normal
      ){

  const double irho = 1.0/Q[0];

  // based on the assumption that the fluid is an ideal gas, gamma chosen for dry air
  const double gamma = 1.4;
  const double p = (gamma-1) * (Q[3] - 0.5*irho*(Q[1]*Q[1]+Q[2]*Q[2]));

  const double c   = std::sqrt(gamma * p * irho);

  double result = 1.0;
  switch(normal){
  case 0: //x
    result = std::max( std::abs(Q[1] * irho - c), std::abs(Q[1] * irho + c) );
    break;
  case 1: //y
    result = std::max( std::abs(Q[2] * irho - c), std::abs(Q[2] * irho + c) );
    break;
  }
  return result;

}


void exahype2::dg::tests::eulerBoundaryConditions(
        const double * __restrict__                  Qinside,
        double * __restrict__                         Qoutside,
        const tarch::la::Vector<Dimensions,double>&  x,
        double                                       t,
        int                                          normal
      ){
  Qoutside[0] = Qinside[0];
  Qoutside[1] = Qinside[1];
  Qoutside[2] = Qinside[2];
  Qoutside[3] = Qinside[3];
  Qoutside[4] = Qinside[4];
}
