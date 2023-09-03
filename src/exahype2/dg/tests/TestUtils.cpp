#include "TestUtils.h"

void exahype2::dg::tests::eulerFlux(
  [[maybe_unused]] const double * __restrict__                  Q,
  [[maybe_unused]] const tarch::la::Vector<Dimensions,double>&  x,
  [[maybe_unused]] double                                       t,
  [[maybe_unused]] double                                       dt,
  [[maybe_unused]] int                                          normal,
  [[maybe_unused]] double * __restrict__                        F
) {
  constexpr double gamma = 1.4;
  const double irho = 1./Q[0];
  const double p = (gamma-1) * (Q[4] - 0.5*irho*(Q[1]*Q[1]+Q[2]*Q[2]+Q[3]*Q[3]));

  const double velocity = irho*Q[normal+1];
  F[0] = velocity*Q[0];
  F[1] = velocity*Q[1];
  F[2] = velocity*Q[2];
  F[3] = velocity*Q[3];
  F[normal+1] += p;
  F[4] = velocity*(Q[4]+p);
}

void exahype2::dg::tests::eulerSource(
  [[maybe_unused]] const double * __restrict__                  Q,
  [[maybe_unused]] const tarch::la::Vector<Dimensions,double>&  x,
  [[maybe_unused]] double                                       t,
  [[maybe_unused]] double                                       dt,
  [[maybe_unused]] double * __restrict__                        S
) {
  S[0] = 0.0;
  S[1] = 0.0;
  S[2] = 0.0;
  S[3] = 0.0;
  S[4] = 0.0;
}

double exahype2::dg::tests::eulerEigenvalue(
  [[maybe_unused]] const double * __restrict__                 Q,
  [[maybe_unused]] const tarch::la::Vector<Dimensions,double>& x,
  [[maybe_unused]] double                                      t,
  [[maybe_unused]] double                                       dt,
  [[maybe_unused]] int                                          normal
) {
  assertion8( Q[0]>0.0, Q[0], Q[1], Q[2], Q[3], Q[4], x, t, normal );
  const double irho = 1.0/Q[0];

  // based on the assumption that the fluid is an ideal gas, gamma chosen for dry air
  const double gamma = 1.4;
  const double p = (gamma-1) * (Q[4] - 0.5*irho*(Q[1]*Q[1]+Q[2]*Q[2]+Q[3]*Q[3]));

  assertion8( p>=0.0, Q[0], Q[1], Q[2], Q[3], Q[4], x, t, normal );
  const double c   = std::sqrt(gamma * p * irho);

  double result = 1.0;
  switch(normal){
  case 0: //x
    result = std::max( std::abs(Q[1] * irho - c), std::abs(Q[1] * irho + c) );
    break;
  case 1: //y
    result = std::max( std::abs(Q[2] * irho - c), std::abs(Q[2] * irho + c) );
    break;
  case 2: //z
    result = std::max( std::abs(Q[3] * irho - c), std::abs(Q[3] * irho + c) );
  }
  return result;
}

void exahype2::dg::tests::eulerBoundaryConditions(
  [[maybe_unused]] const double * __restrict__                  Qinside,
  [[maybe_unused]] double * __restrict__                         Qoutside,
  [[maybe_unused]] const tarch::la::Vector<Dimensions,double>&  x,
  [[maybe_unused]] double                                       t,
  [[maybe_unused]] double                                       dt,
  [[maybe_unused]] int                                          normal
) {
  Qoutside[0] = Qinside[0];
  Qoutside[1] = Qinside[1];
  Qoutside[2] = Qinside[2];
  Qoutside[3] = Qinside[3];
  Qoutside[4] = Qinside[4];
}

void exahype2::dg::tests::eulerInitial(
  [[maybe_unused]] double* __restrict__  Q,
  [[maybe_unused]] int                   node
) {
  Q[0] = 1.00000000000000;
  Q[1] = 0.100000000000000;
  Q[2] = 0.200000000000000;
  Q[3] = 0.300000000000000;
  Q[4] = 2.57000000000000;
}

void exahype2::dg::tests::elasticInitial(double * __restrict__ Q) {
  for(int i=0; i<9; i++){
    Q[i] = 1.0; //6 stresses, 3 velocities
  }
  //auxiliary variables
  Q[9]  = 4.0; //rho
  Q[10] = 2.0; //cp
  Q[11] = 2.6; //cs
}

void exahype2::dg::tests::elasticFlux(
  [[maybe_unused]] const double * __restrict__                  Q,
  [[maybe_unused]] const tarch::la::Vector<Dimensions,double>&  x,
  [[maybe_unused]] double                                       t,
  [[maybe_unused]] double                                       dt,
  [[maybe_unused]] int                                          normal,
  [[maybe_unused]] double * __restrict__                        F
) {
  //stresses
  for(int i=0; i<6; i++){
    F[i] = 0;
  }

  //velocities
  switch(normal){
  case 0: //x
    F[6] = Q[0]; //xx
    F[7] = Q[3]; //xy
    F[8] = Q[4]; //xz
    break;
  case 1: //y
    F[6] = Q[3]; //xy
    F[7] = Q[1]; //yy
    F[8] = Q[5]; //yz
    break;
  case 2: //z
    F[6] = Q[4]; //xz
    F[7] = Q[5]; //yz
    F[8] = Q[2]; //zz
    break;
  }
}

void exahype2::dg::tests::elasticSource(
  [[maybe_unused]] const double * __restrict__                  Q,
  [[maybe_unused]] const tarch::la::Vector<Dimensions,double>&  x,
  [[maybe_unused]] double                                       t,
  [[maybe_unused]] double                                       dt,
  [[maybe_unused]] double * __restrict__                        S
) {
  S[0] = 1.0;
  S[1] = 1.0;
  S[2] = 1.0;
  S[3] = 1.0;
  S[4] = 1.0;
  S[5] = 1.0;
  S[6] = 1.0;
  S[7] = 1.0;
  S[8] = 1.0;
}

double exahype2::dg::tests::elasticEigenvalue(
  [[maybe_unused]] const double * __restrict__                 Q,
  [[maybe_unused]] const tarch::la::Vector<Dimensions,double>& x,
  [[maybe_unused]] double                                      t,
  [[maybe_unused]] double                                       dt,
  [[maybe_unused]] int                                          normal
) {
  double result = std::max(std::abs(Q[10]), std::abs(Q[11]));
  return result;
}

void exahype2::dg::tests::elasticBoundaryConditions(
  [[maybe_unused]] const double * __restrict__                  Qinside,
  [[maybe_unused]] double * __restrict__                        Qoutside,
  [[maybe_unused]] const tarch::la::Vector<Dimensions,double>&  x,
  [[maybe_unused]] double                                       t,
  [[maybe_unused]] double                                       dt,
  [[maybe_unused]] int                                          normal
) {
  Qoutside[0] = Qinside[0];
  Qoutside[1] = Qinside[1];
  Qoutside[2] = Qinside[2];
  Qoutside[3] = Qinside[3];
  Qoutside[4] = Qinside[4];
  Qoutside[5] = Qinside[5];
  Qoutside[6] = Qinside[6];
  Qoutside[7] = Qinside[7];
  Qoutside[8] = Qinside[8];
  Qoutside[9] = Qinside[9];
  Qoutside[10] = Qinside[10];
  Qoutside[11] = Qinside[11];
}

void exahype2::dg::tests::elasticNonConservativeProduct(
  [[maybe_unused]] const double * __restrict__                  Q,
  [[maybe_unused]] const double * __restrict__                  deltaQ,
  [[maybe_unused]] const tarch::la::Vector<Dimensions,double>&  faceCentre,
  [[maybe_unused]] double                                       t,
  [[maybe_unused]] double                                       dt,
  [[maybe_unused]] int                                          normal,
  [[maybe_unused]] double * __restrict__                        BgradQ // BgradQ[9]
) {
  //Lamé parameters
  double mu = Q[9] * Q[11] * Q[11]; //rho*cs^2
  double lambda = Q[9] * Q[10] * Q[10] - 2 * mu; //rho*cp^2 - 2 mu

  //stresses
  switch(normal){
  case 0: //x
    BgradQ[0] = (lambda + 2*mu) * deltaQ[6]; //xx
    BgradQ[1] =  lambda * deltaQ[6];         //yy
    BgradQ[2] =  lambda * deltaQ[6];       //zz
    BgradQ[3] =  mu * deltaQ[7];         //xy
    BgradQ[4] =  mu * deltaQ[8];         //xz
    BgradQ[5] =  0;              //yz
    break;
  case 1: //y
    BgradQ[0] =  lambda * deltaQ[7];         //xx
    BgradQ[1] = (lambda + 2*mu) * deltaQ[7]; //yy
    BgradQ[2] =  lambda * deltaQ[7];       //zz
    BgradQ[3] =  mu * deltaQ[6];         //xy
    BgradQ[4] =  0;              //xz
    BgradQ[5] =  mu * deltaQ[8];         //yz
    break;
  case 2:
    BgradQ[0] =  lambda * deltaQ[8];         //xx
    BgradQ[1] =  lambda * deltaQ[8];       //yy
    BgradQ[2] = (lambda + 2*mu) * deltaQ[8]; //zz
    BgradQ[3] =  0;              //xy
    BgradQ[4] =  mu * deltaQ[6];         //xz
    BgradQ[5] =  mu * deltaQ[7];         //yz
    break;
  }

  //velocities
  BgradQ[6] = 0;
  BgradQ[7] = 0;
  BgradQ[8] = 0;
}

void exahype2::dg::tests::testBoundaryConditions(
  [[maybe_unused]] const double * __restrict__                  Qinside,
  [[maybe_unused]] double * __restrict__                        Qoutside,
  [[maybe_unused]] const tarch::la::Vector<Dimensions,double>&  x,
  [[maybe_unused]] double                                       t,
  [[maybe_unused]] double                                       dt,
  [[maybe_unused]] int                                          normal
) {
  Qoutside[0] = Qinside[0];
  Qoutside[1] = x[0];
  Qoutside[2] = t;
  Qoutside[3] = normal;
}

double exahype2::dg::tests::testEigenvalue(
  [[maybe_unused]] const double * __restrict__                 Q,
  [[maybe_unused]] const tarch::la::Vector<Dimensions,double>& x,
  [[maybe_unused]] double                                      t,
  [[maybe_unused]] double                                      dt,
  [[maybe_unused]] int                                         normal
) {
  double result = Q[0]*normal;
  return result;
}

void exahype2::dg::tests::secondTestBoundaryConditions(
  [[maybe_unused]] const double * __restrict__                  Qinside,
  [[maybe_unused]] double * __restrict__                        Qoutside,
  [[maybe_unused]] const tarch::la::Vector<Dimensions,double>&  x,
  [[maybe_unused]] double                                       t,
  [[maybe_unused]] double                                       dt,
  [[maybe_unused]] int                                          normal
) {
  Qoutside[0] = 1.0;
  Qoutside[1] = 2.0;
  Qoutside[2] = 3.0;
  Qoutside[3] = 4.0;
}
