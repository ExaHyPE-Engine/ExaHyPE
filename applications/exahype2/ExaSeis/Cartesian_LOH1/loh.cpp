#include "loh.h"

#include "exahype2/RefinementControl.h"

tarch::logging::Log examples::exahype2::elastic::loh::_log("examples::exahype2::elastic::loh");

/*
 * Enables the usage of shortcuts to access variables, e.g. use Q[s.v] instead of Q[0]
 */
examples::exahype2::elastic::VariableShortcuts s;

::exahype2::RefinementCommand examples::exahype2::elastic::loh::refinementCriterion(
  const double* __restrict__ Q, // Q[9+3]
  const tarch::la::Vector<Dimensions, double>& x,
  const tarch::la::Vector<Dimensions, double>& h,
  double                                       t
) {

  ::exahype2::RefinementCommand result = ::exahype2::RefinementCommand::Keep;
  return result;
}

void examples::exahype2::elastic::loh::initialCondition(
  double* __restrict__ Q, // Q[9+3]
  const tarch::la::Vector<Dimensions, double>& x,
  const tarch::la::Vector<Dimensions, double>& h,
  const tarch::la::Vector<Dimensions, int>&    point,
  bool                                         gridIsConstructed
) {

  Q[0] = 0.0;
  Q[1] = 0.0;
  Q[2] = 0.0;

  // stresses
  Q[3] = 0.0;
  Q[4] = 0.0;
  Q[5] = 0.0;
  Q[6] = 0.0;
  Q[7] = 0.0;
  Q[8] = 0.0;

  // material parameters
  Q[9]  = x[1] < 1.0 ? 2.6 : 2.7;   // rho
  Q[10] = x[1] < 1.0 ? 4.0 : 6.0;   // cp
  Q[11] = x[1] < 1.0 ? 2.0 : 3.343; // cs
}

double ::examples::exahype2::elastic::loh::maxEigenvalue(
  const double* __restrict__ Q, // Q[9+3]
  const tarch::la::Vector<Dimensions, double>& volumeCentre,
  const tarch::la::Vector<Dimensions, double>& volumeSize,
  double                                       t,
  double                                       dt,
  int                                          normal
) {

  return std::max(std::abs(Q[s.cp]), std::abs(Q[s.cs]));
}

void ::examples::exahype2::elastic::loh::flux(
  const double* __restrict__ Q, // Q[9+3]
  const tarch::la::Vector<Dimensions, double>& volumeX,
  const tarch::la::Vector<Dimensions, double>& volumeH,
  double                                       t,
  double                                       dt,
  int                                          normal,
  double* __restrict__ F // F[9]
) {

  for (int i = 0; i < 6; i++) {
    F[s.sigma + i] = 0.0;
  }

  switch (normal) {
  case 0:
    F[s.v + 0] = -Q[s.sigma + 0]; // sigma_xx
    F[s.v + 1] = -Q[s.sigma + 3]; // sigma_xy
    F[s.v + 2] = -Q[s.sigma + 4]; // sigma_xz
    break;
  case 1:
    F[s.v + 0] = -Q[s.sigma + 3]; // sigma_xy
    F[s.v + 1] = -Q[s.sigma + 1]; // sigma_yy
    F[s.v + 2] = -Q[s.sigma + 5]; // sigma_yz
    break;
  case 2:
    F[s.v + 0] = -Q[s.sigma + 4]; // sigma_xz
    F[s.v + 1] = -Q[s.sigma + 5]; // sigma_yz
    F[s.v + 2] = -Q[s.sigma + 2]; // sigma_zz
  }
}

void ::examples::exahype2::elastic::loh::nonconservativeProduct(
  const double* __restrict__ Q,      // Q[9+3]
  const double* __restrict__ deltaQ, // deltaQ[9+3]
  const tarch::la::Vector<Dimensions, double>& volumeX,
  const tarch::la::Vector<Dimensions, double>& volumeH,
  double                                       t,
  double                                       dt,
  int                                          normal,
  double* __restrict__ BgradQ // BgradQ[9]
) {

  // Lamé parameters
  double mu     = Q[s.rho] * Q[s.cs] * Q[s.cs];          // rho*cs^2
  double lambda = Q[s.rho] * Q[s.cp] * Q[s.cp] - 2 * mu; // rho*cp^2 - 2 mu

  switch (normal) {
  case 0:
    BgradQ[s.sigma + 0] = -deltaQ[s.v + 0]; // xx
    BgradQ[s.sigma + 1] = 0.0;              // yy
    BgradQ[s.sigma + 2] = 0.0;              // zz
    BgradQ[s.sigma + 3] = -deltaQ[s.v + 1]; // xy
    BgradQ[s.sigma + 4] = -deltaQ[s.v + 2]; // xz
    BgradQ[s.sigma + 5] = 0.0;              // yz
    break;
  case 1:
    BgradQ[s.sigma + 0] = 0.0;              // xx
    BgradQ[s.sigma + 1] = -deltaQ[s.v + 1]; // yy
    BgradQ[s.sigma + 2] = 0.0;              // zz
    BgradQ[s.sigma + 3] = -deltaQ[s.v + 0]; // xy
    BgradQ[s.sigma + 4] = 0.0;              // xz
    BgradQ[s.sigma + 5] = -deltaQ[s.v + 2]; // yz
    break;
  case 2:
    BgradQ[s.sigma + 0] = 0.0;              // xx
    BgradQ[s.sigma + 1] = 0.0;              // yy
    BgradQ[s.sigma + 2] = -deltaQ[s.v + 2]; // zz
    BgradQ[s.sigma + 3] = 0.0;              // xy
    BgradQ[s.sigma + 4] = -deltaQ[s.v + 0]; // xz
    BgradQ[s.sigma + 5] = -deltaQ[s.v + 1]; // yz
  }

  for (int i = 0; i < 3; i++) {
    BgradQ[s.v + i] = 0.0;
  }
}

void ::examples::exahype2::elastic::loh::initPointSourceLocations(
  double sourceLocation[NumberOfPointSources][Dimensions]
) {

  sourceLocation[0][0] = 0.0;
  sourceLocation[0][1] = 2.0;
#if Dimensions == 3
  sourceLocation[0][2] = 0.0;
#endif
}

void ::examples::exahype2::elastic::loh::pointSource(
  const double* const Q, // Q[9+3]
  const double* const x,
  const double        t,
  const double        dt,
  double* const       forceVector, // Q[9
  int                 n
) {

  for (int i = 0; i < s.SizeVariables; i++) {
    forceVector[i] = 0.0;
  }

  constexpr double t0 = 0.1;
  constexpr double M0 = 1000.0;
  double           f  = M0 * t / (t0 * t0) * std::exp(-t / t0);

  forceVector[7] = f;
}

void examples::exahype2::elastic::loh::boundaryConditions(
  const double* __restrict__ Qinside, // Qinside[9+3]
  double* __restrict__ Qoutside,      // Qoutside[9+3]
  const tarch::la::Vector<Dimensions, double>& faceCentre,
  const tarch::la::Vector<Dimensions, double>& volumeH,
  double                                       t,
  int                                          normal
) {

  for (int i = 0; i < s.Size; i++) {
    Qoutside[i] = Qinside[i];
  }

  // switch(normal){
  //   case 0:
  //     Qoutside[s.sigma+0] = -Qinside[s.sigma+0]; //xx
  //     Qoutside[s.sigma+3] = -Qinside[s.sigma+3]; //xy
  //     Qoutside[s.sigma+4] = -Qinside[s.sigma+4]; //xz
  //     Qoutside[s.v+0] = Qinside[s.v+0];
  //     Qoutside[s.v+1] = Qinside[s.v+1];
  //    Qoutside[s.v+2] = Qinside[s.v+2];
  //     break;
  //   case 1:
  //     Qoutside[s.sigma+1] = -Qinside[s.sigma+1]; //yy
  //     Qoutside[s.sigma+3] = -Qinside[s.sigma+3]; //xy
  //     Qoutside[s.sigma+5] = -Qinside[s.sigma+5]; //yz
  //     Qoutside[s.v+0] = Qinside[s.v+0];
  //     Qoutside[s.v+1] = Qinside[s.v+1];
  //     Qoutside[s.v+2] = Qinside[s.v+2];
  //     break;
  //   case 2:
  //     Qoutside[s.sigma+2] = -Qinside[s.sigma+2]; //zz
  //     Qoutside[s.sigma+4] = -Qinside[s.sigma+4]; //xz
  //     Qoutside[s.sigma+5] = -Qinside[s.sigma+5]; //yz
  //     Qoutside[s.v+0] = Qinside[s.v+0];
  //     Qoutside[s.v+1] = Qinside[s.v+1];
  //     Qoutside[s.v+2] = Qinside[s.v+2];
  // }

  // //auxiliary variables
  // Qoutside[s.rho]     = Qinside[s.rho];
  // Qoutside[s.cp]      = Qinside[s.cp];
  // Qoutside[s.cs]      = Qinside[s.cs];
}

void examples::exahype2::elastic::loh::riemannSolver(
  double* const                                FL, // FL[9
  double* const                                FR, // FR[9
  const double* const                          QL, // QL[9+3]
  const double* const                          QR, // QR[9+3]
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
    std::copy_n(FIn, s.Size, FOut);
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

  // if(isBoundaryFace){
  //   std::cout << "RiemannSolver at boundary, position: " << x << ", faceIndex: " << faceIndex << ", myFaceIndex: " <<
  //   myFaceIndex << std::endl;;
  // }

  ::loh::riemannSolver::riemannSolver(FL, FR, QL, QR, t, dt, h, direction, isBoundaryFace, myFaceIndex);
}

void ::examples::exahype2::elastic::loh::multiplyMaterialParameterMatrix(
  const double* const                          Q,
  const tarch::la::Vector<Dimensions, double>& volumeX,
  const tarch::la::Vector<Dimensions, double>& volumeH,
  double                                       t,
  double                                       dt,
  int                                          normal,
  double* const                                rhs
) {

  double rho     = Q[9];
  double c_p     = Q[10];
  double c_s     = Q[11];
  double mu      = rho * c_s * c_s;
  double lambda  = rho * c_p * c_p - 2.0 * mu;
  double rho_inv = 1.0 / rho;

  rhs[0] = rho_inv * rhs[0];
  rhs[1] = rho_inv * rhs[1];
  rhs[2] = rho_inv * rhs[2];

  double lam_temp = lambda * (rhs[3] + rhs[4] + rhs[5]);
  rhs[3]          = (2 * mu) * rhs[3] + lam_temp;
  rhs[4]          = (2 * mu) * rhs[4] + lam_temp;
  rhs[5]          = (2 * mu) * rhs[5] + lam_temp;

  rhs[6] = mu * rhs[6];
  rhs[7] = mu * rhs[7];
  rhs[8] = mu * rhs[8];
}
