
// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "SWE.h"

#include "exahype2/RefinementControl.h"

tarch::logging::Log tests::exahype2::aderdg::SWE::_log(
  "tests::exahype2::aderdg::SWE"
);

::exahype2::RefinementCommand tests::exahype2::aderdg::SWE::refinementCriterion(
  const double* __restrict__ Q, // Q[4+0]
  const tarch::la::Vector<Dimensions, double>& x,
  const tarch::la::Vector<Dimensions, double>& h,
  double                                       t
) {
  ::exahype2::RefinementCommand result = ::exahype2::RefinementCommand::Keep;

  // result = ::exahype2::RefinementCommand::Refine;
  // if(t>0.02){// && h[0]<0.03 && x[0]>0.3){
  //   result = ::exahype2::RefinementCommand::Erase;
  // }

  // if(x[0]<0.3 && t>0.02){
  //   // std::cout << "Call refine\n";
  //    result = ::exahype2::RefinementCommand::Refine;
  // }

  if (x[0] < 0.3) {
    result = ::exahype2::RefinementCommand::Refine;
  }

  return result;
}

void tests::exahype2::aderdg::SWE::initialCondition(
  double* __restrict__ Q, // Q[3+1]
  const tarch::la::Vector<Dimensions, double>& x,
  const tarch::la::Vector<Dimensions, double>& h,
  // const tarch::la::Vector<Dimensions,int>&     point,
  bool gridIsConstructed
) {

  double r = std::sqrt(
    (x[0] - 0.5) * (x[0] - 0.5) + (x[1] - 0.5) * (x[1] - 0.5)
  );

  Q[0] = 1.;

  Q[1] = 0.;
  Q[2] = 0.;

  Q[3] = r < 0.1 ? 0.0 : 0.05;
}

void tests::exahype2::aderdg::SWE::boundaryConditions(
  const double* __restrict__ Qinside, // Qinside[3+1]
  double* __restrict__ Qoutside,      // Qoutside[3+1]
  const tarch::la::Vector<Dimensions, double>& x,
  const tarch::la::Vector<Dimensions, double>& volumeH,
  double                                       t,
  int                                          normal
) {
  // double r = std::sqrt((x[0]-0.5)*(x[0]-0.5)+(x[1]-0.5)*(x[1]-0.5));

  // Qoutside[0] = 1. + r;
  // Qoutside[1] = 0.;
  // Qoutside[2] = 0.;
  // Qoutside[3] = 0.;

  initialCondition(Qoutside, x, volumeH, true);
}

double tests::exahype2::aderdg::SWE::maxEigenvalue(
  const double* __restrict__ Q, // Q[3+1]
  const tarch::la::Vector<Dimensions, double>& volumeCentre,
  const tarch::la::Vector<Dimensions, double>& volumeSize,
  double                                       t,
  double                                       dt,
  int                                          normal
) {
  constexpr double grav = 9.81;

  const double u = Q[1 + normal] / Q[0];
  const double c = std::sqrt(grav * Q[0]);

  return std::max(std::abs(u + c), std::abs(u - c));

  // constexpr double c = std::sqrt(4.);
  // return c;
}

void tests::exahype2::aderdg::SWE::flux(
  const double* __restrict__ Q, // Q[3+1]
  const tarch::la::Vector<Dimensions, double>& volumeX,
  const tarch::la::Vector<Dimensions, double>& volumeH,
  double                                       t,
  double                                       dt,
  int                                          normal,
  double* __restrict__ F // F[3]
) {

  double ih = 1.0 / Q[0];

  F[0] = Q[1 + normal];
  F[1] = Q[1 + normal] * Q[1] * ih;
  F[2] = Q[1 + normal] * Q[2] * ih;
  F[3] = 0.0;

  // F[0] = 0.;
  // F[1] = 0.;
  // F[2] = 0.;
  // F[3] = 0.;
}

void tests::exahype2::aderdg::SWE::nonconservativeProduct(
  const double* __restrict__ Q,      // Q[3+1]
  const double* __restrict__ deltaQ, // deltaQ[3+1]
  const tarch::la::Vector<Dimensions, double>& volumeX,
  const tarch::la::Vector<Dimensions, double>& volumeH,
  double                                       t,
  double                                       dt,
  int                                          normal,
  double* __restrict__ BTimesDeltaQ // BTimesDeltaQ[3]
) {
  constexpr double grav = 9.81;
  BTimesDeltaQ[0]       = 0.0;
  switch (normal) {
  case 0:
    BTimesDeltaQ[1] = grav * Q[0] * (deltaQ[0] + deltaQ[3]);
    BTimesDeltaQ[2] = 0.0;
    break;
  case 1:
    BTimesDeltaQ[1] = 0.0;
    BTimesDeltaQ[2] = grav * Q[0] * (deltaQ[0] + deltaQ[3]);
    break;
  }
  BTimesDeltaQ[3] = 0.0;

  // BTimesDeltaQ[0] = 0.;
  // BTimesDeltaQ[1] = 0.;
  // BTimesDeltaQ[2] = 0.;
  // BTimesDeltaQ[3] = 0.;
}
