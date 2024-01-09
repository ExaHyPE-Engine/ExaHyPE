// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "LOH1Solver.h"

tarch::logging::Log applications::exahype2::exaseis::LOH1Solver::_log("applications::exahype2::exaseis::LOH1Solver");

using Vars = applications::exahype2::exaseis::VariableShortcuts;


void applications::exahype2::exaseis::LOH1Solver::initialCondition(
  double* __restrict__ Q,
  const tarch::la::Vector<Dimensions, double>& x,
  const tarch::la::Vector<Dimensions, double>& h,
  const tarch::la::Vector<Dimensions, int>&    index,
  bool                                         gridIsConstructed
) {
  for (int i = 0; i < NumberOfUnknowns; i++) {
    Q[i] = 0.0;
  }

  // Isotropic
  Q[Vars::rho] = x[1] < 1.0 ? 2.6 : RHO;
  Q[Vars::cp]  = x[1] < 1.0 ? 4.0 : P_WAVE_SPEED;
  Q[Vars::cs]  = x[1] < 1.0 ? 2.0 : S_WAVE_SPEED;
}


void applications::exahype2::exaseis::LOH1Solver::boundaryConditions(
  const double* __restrict__ Qinside,
  double* __restrict__ Qoutside,
  const tarch::la::Vector<Dimensions, double>& x,
  const tarch::la::Vector<Dimensions, double>& h,
  double                                       t,
  int                                          normal
) {
  for (int i = 0; i < NumberOfUnknowns + NumberOfAuxiliaryVariables; i++) {
    Qoutside[i] = Qinside[i];
  }
}


#ifdef USE_POINT_SOURCES
void applications::exahype2::exaseis::LOH1Solver::pointSource(const double* const Q, const double* const x, const double t, const double dt, double* const forceVector, int n) {
  for (int i = 0; i < NumberOfUnknowns; i++) {
    forceVector[i] = 0.0;
  }

  forceVector[Vars::sxz] = SOURCE_M0 * t / (SOURCE_t0 * SOURCE_t0) * std::exp(-t / SOURCE_t0);
}


void applications::exahype2::exaseis::LOH1Solver::initPointSourceLocations(double sourceLocations[NumberOfPointSources][Dimensions]) {
  sourceLocations[0][0] = SOURCE_LOCATION_X;
  sourceLocations[0][1] = SOURCE_LOCATION_Y;
  sourceLocations[0][2] = SOURCE_LOCATION_Z;
}
#endif


::exahype2::RefinementCommand applications::exahype2::exaseis::LOH1Solver::refinementCriterion(
  const double* __restrict__ Q, const tarch::la::Vector<Dimensions, double>& x, const tarch::la::Vector<Dimensions, double>& h, double t
) {
  auto result = ::exahype2::RefinementCommand::Keep;

  if (tarch::la::equals(t, 0.0)) {
    tarch::la::Vector<Dimensions, double> sourceLocation({SOURCE_LOCATION_X, SOURCE_LOCATION_Y, SOURCE_LOCATION_Z});
    if (tarch::la::norm2(x - sourceLocation) < SOURCE_RADIUS) {
      result = ::exahype2::RefinementCommand::Refine;
    }
  }

  return result;
}


#ifdef USE_RIEMANN
void applications::exahype2::exaseis::LOH1Solver::riemannSolver(
  double* const                                FL,
  double* const                                FR,
  const double* const                          QL,
  const double* const                          QR,
  const double                                 t,
  const double                                 dt,
  const tarch::la::Vector<Dimensions, double>& x,
  const tarch::la::Vector<Dimensions, double>& h,
  const int                                    direction,
  bool                                         isBoundaryFace,
  int                                          faceIndex
) {
  // Local lax-friedrich
  // generated::kernels::AderDG::linear::riemannSolver(
  //   *this,
  //   FL,
  //   FR,
  //   QL,
  //   QR,
  //   t,
  //   dt,
  //   x,
  //   h,
  //   direction
  // );

  if (isBoundaryFace) {
    double* FIn  = faceIndex < Dimensions ? FR : FL;
    double* FOut = faceIndex < Dimensions ? FL : FR;
    std::copy_n(FIn, NumberOfUnknowns, FOut);
  }

  int myFaceIndex = 0;
  switch (faceIndex) {
  case 0:
    myFaceIndex = 0;
    break;
  case 1:
    myFaceIndex = 2;
    break;
  case 2:
    myFaceIndex = 4;
    break;
  case 3:
    myFaceIndex = 1;
    break;
  case 4:
    myFaceIndex = 3;
    break;
  case 5:
    myFaceIndex = 5;
    break;
  }

  // if (isBoundaryFace) {
  //   std::cout << "RiemannSolver at boundary, position: " << x << ", faceIndex: " << faceIndex << ", myFaceIndex: " << myFaceIndex << std::endl;
  // }

  RiemannSolver<double, Order>::solve(FL, FR, QL, QR, t, dt, h, direction, isBoundaryFace, myFaceIndex);
}
#endif


#ifdef USE_MATERIAL_PARAMETERS
void applications::exahype2::exaseis::LOH1Solver::multiplyMaterialParameterMatrix(
  const double* const Q, const tarch::la::Vector<Dimensions, double>& x, const tarch::la::Vector<Dimensions, double>& h, double t, double dt, int normal, double* const rhs
) {
  const double mu     = Q[Vars::rho] * Q[Vars::cs] * Q[Vars::cs];
  const double lambda = Q[Vars::rho] * Q[Vars::cp] * Q[Vars::cp] - 2.0 * mu;
  const double rhoInv = 1.0 / Q[Vars::rho];

  rhs[Vars::vx] = rhoInv * rhs[Vars::vx];
  rhs[Vars::vy] = rhoInv * rhs[Vars::vy];
  rhs[Vars::vz] = rhoInv * rhs[Vars::vz];

  const double lamTemp = lambda * (rhs[Vars::sxx] + rhs[Vars::syy] + rhs[Vars::szz]);
  rhs[Vars::sxx]       = (2.0 * mu) * rhs[Vars::sxx] + lamTemp;
  rhs[Vars::syy]       = (2.0 * mu) * rhs[Vars::syy] + lamTemp;
  rhs[Vars::szz]       = (2.0 * mu) * rhs[Vars::szz] + lamTemp;

  rhs[Vars::sxy] = mu * rhs[Vars::sxy];
  rhs[Vars::sxz] = mu * rhs[Vars::sxz];
  rhs[Vars::syz] = mu * rhs[Vars::syz];
}
#endif
