#include "SWESolver.h"

#include <cmath>
#include <iostream>


tarch::logging::Log applications::exahype2::avalanche::SWESolver::_log("applications::exahype2::avalanche::SWESolver");

void applications::exahype2::avalanche::SWESolver::initialCondition(
  double* __restrict__ Q,
  const tarch::la::Vector<Dimensions, double>& volumeCentre,
  const tarch::la::Vector<Dimensions, double>& volumeH,
  bool                                         gridIsConstructred
) {
  const bool nearCentre = tarch::la::norm2(volumeCentre) < 0.40;

  Q[0] = 1.0; // Water height (h)
  Q[1] = 0.2; // x-momentum (hu)
  Q[2] = 0.3; // y-momentum (hv)
  Q[3] = (nearCentre ? -1.0 : -1.5);
}

void applications::exahype2::avalanche::SWESolver::boundaryConditions(
  const double* __restrict__ Qinside, // Qinside[3+1]
  double* __restrict__ Qoutside,      // Qoutside[3+1]
  const tarch::la::Vector<Dimensions, double>& faceCentre,
  const tarch::la::Vector<Dimensions, double>& volumeH,
  double                                       t,
  int                                          normal
) {
  // Reflecting boundary conditions
  Qoutside[0] = Qinside[0];
  Qoutside[1] = -Qinside[1];
  Qoutside[2] = -Qinside[2];
  Qoutside[3] = Qinside[3];
}

double applications::exahype2::avalanche::SWESolver::maxEigenvalue(
  const double* __restrict__ Q, // Q[3+1],
  const tarch::la::Vector<Dimensions, double>& faceCentre,
  const tarch::la::Vector<Dimensions, double>& volumeH,
  double                                       t,
  double                                       dt,
  int                                          normal
) {
  assertion(Q[0] > 0);
  double       result = 1.0;
  const double ih     = 1.0 / Q[0]; // Inverse water height
  const double c      = std::sqrt(Gravity * cos(Zeta) * Q[0]);
  double       u      = 0.0;

  switch (normal) {
  case 0: // x
          // Q[1] is the momentum in x-direction but we need the velocity
          // here. Therefore, multiply by the inverse water height (i.e., we
          // divide by the height).
    u      = ih * Q[1];
    result = std::max(std::abs(u - c), std::abs(u + c));
    break;
  case 1: // y
    u      = ih * Q[2];
    result = std::max(std::abs(u - c), std::abs(u + c));
    break;
  }

  return result;
}

void applications::exahype2::avalanche::SWESolver::flux(
  const double* __restrict__ Q, // Q[3+1],
  const tarch::la::Vector<Dimensions, double>& faceCentre,
  const tarch::la::Vector<Dimensions, double>& volumeH,
  double                                       t,
  double                                       dt,
  int                                          normal,
  double* __restrict__ F // F[3]
) {
  const double ih = 1.0 / Q[0];

  switch (normal) {
  case 0:
    F[0] = Q[1];                                                       // hu_1
    F[1] = ih * Q[1] * Q[1] + 0.5 * Gravity * cos(Zeta) * Q[0] * Q[0]; // h(u_1)^2 + 0.5*g*h**2
    F[2] = ih * Q[1] * Q[2];                                           // hu_1u_2
    break;
  case 1:
    F[0] = Q[2];                                                       // hu_2
    F[1] = ih * Q[2] * Q[1];                                           // hu_1u_2
    F[2] = ih * Q[2] * Q[2] + 0.5 * Gravity * cos(Zeta) * Q[0] * Q[0]; // h(u_2)^2 + 0.5*g*h**2
    break;
  }
}

void applications::exahype2::avalanche::SWESolver::nonconservativeProduct(
  const double* __restrict__ Q,      // Q[3+1],
  const double* __restrict__ deltaQ, // deltaQ[3+1]
  const tarch::la::Vector<Dimensions, double>& faceCentre,
  const tarch::la::Vector<Dimensions, double>& volumeH,
  double                                       t,
  double                                       dt,
  int                                          normal,
  double* __restrict__ BTimesDeltaQ // BTimesDeltaQ[3]
) {
  std::cout<<"t = "<<t<<std::endl;
  std::cout<<"dt = "<<dt<<std::endl;
  std::cout<<"coordinates: "<<faceCentre<<std::endl;
  enum derivatives{u_x=1,u_y,u_xx,u_yy,u_xy,v_x,v_y,v_xx,v_yy,v_xy};
  const double h_centre = Q[0];



  // Partial Derivatives of unknowns

  // u
  const double ux  = (Q[NumberOfUnknowns + u_x] - 0.5*deltaQ[NumberOfUnknowns + u_x]) / (2 * volumeH(0));
  const double uy  = (Q[NumberOfUnknowns + u_y] - 0.5*deltaQ[NumberOfUnknowns + u_y]) / (2 * volumeH(1));
  const double uxx = (Q[NumberOfUnknowns + u_xx] - 0.5*deltaQ[NumberOfUnknowns + u_xx]) / (volumeH(0) * volumeH(0));
  const double uyy = (Q[NumberOfUnknowns + u_yy] - 0.5*deltaQ[NumberOfUnknowns + u_yy]) / (volumeH(1) * volumeH(1));
  const double uxy = (Q[NumberOfUnknowns + u_xy] - 0.5*deltaQ[NumberOfUnknowns + u_xy]) / (4 * volumeH(0) * volumeH(1));

  std::cout<<"u_x "<<ux<<std::endl;
  std::cout<<"u_y "<<uy<<std::endl;
  std::cout<<"u_xx "<<uxx<<std::endl;
  std::cout<<"u_xy "<<uxy<<std::endl;
  std::cout<<"u_yy "<<uyy<<std::endl;
  std::cout<<"\n";

  // v
  const double vx  = (Q[NumberOfUnknowns + v_x] - 0.5*deltaQ[NumberOfUnknowns + v_x]) / (2 * volumeH(0));
  const double vy  = (Q[NumberOfUnknowns + v_y] - 0.5*deltaQ[NumberOfUnknowns + v_y]) / (2 * volumeH(1));
  const double vxx = (Q[NumberOfUnknowns + v_xx] - 0.5*deltaQ[NumberOfUnknowns + v_xx]) / (volumeH(0) * volumeH(0));
  const double vyy = (Q[NumberOfUnknowns + v_yy] - 0.5*deltaQ[NumberOfUnknowns + v_yy]) / (volumeH(1) * volumeH(1));
  const double vxy = (Q[NumberOfUnknowns + v_xy] - 0.5*deltaQ[NumberOfUnknowns + v_xy]) / (4 * volumeH(0) * volumeH(1));
  std::cout<<"v_x "<<vx<<std::endl;
  std::cout<<"v_y "<<vy<<std::endl;
  std::cout<<"v_xx "<<vxx<<std::endl;
  std::cout<<"v_xy "<<vxy<<std::endl;
  std::cout<<"v_yy "<<vyy<<std::endl;
  std::cout<<"\n";
  std::cout<<"\n";
  std::cout<<"\n";


  switch (normal) {
  case 0: // x
    BTimesDeltaQ[0] = volumeH(0) * 0.0;
    BTimesDeltaQ[1] = volumeH(0) * Nu * sqrt(h_centre) * (1.5 * deltaQ[0] * u_x + h_centre * u_xx);
    BTimesDeltaQ[2] = volumeH(0) * 0.5 * Nu * sqrt(h_centre) * (1.5 * deltaQ[0] * (u_y + v_x) + h_centre * (u_xy + v_xx));
    break;
  case 1: // y
    BTimesDeltaQ[0] = volumeH(1) * 0.0;
    BTimesDeltaQ[1] = volumeH(1) * 0.5 * Nu * sqrt(h_centre) * (1.5 * deltaQ[0] * (u_y + v_x) + h_centre * (u_yy + v_xy));
    BTimesDeltaQ[2] = volumeH(1) * Nu * sqrt(h_centre) * (1.5 * deltaQ[0] * v_y + h_centre * v_yy);
    break;
  }
}

void applications::exahype2::avalanche::SWESolver::sourceTerm(
  const double* __restrict__ Q,
  const tarch::la::Vector<Dimensions, double>& volumeCentre,
  const tarch::la::Vector<Dimensions, double>& volumeH,
  double                                       t,
  double                                       dt,
  double* __restrict__ S
) {
  const double ih = 1.0 / Q[0];
  const double u  = ih * Q[1];
  const double v  = ih * Q[2];
  // const double ubar = sqrt(Q[1] * Q[1] + Q[2] * Q[2]) * ih;
  const double ubar = sqrt(Q[1] * Q[1] + Q[2] * Q[2]) * ih;

  const double Fr       = ubar / sqrt(Gravity * Q[0] * cos(Zeta));
  const double Tau      = Beta * Q[0] / Hstop - Fr;
  const double Betastar = Beta * (Mu2 + Mu3 - 2 * Mu1) / ((Mu2 - Mu3) * (Mu2 - Mu3)) - Tau;
  const double Hstar    = (Betastar + Tau) * Hstop / Beta;
  double       Mu       = 1.0;

  if (Fr > Betastar) {
    const double vstop  = (Q[0] * Beta) / (Fr + Tau);
    const double Mustop = Mu1 + (Mu1 - Mu2) / (1 + vstop / FrictionLengthScale);
    Mu                  = Mustop;
  }

  else if (Fr > 0 && Fr <= Betastar) {
    const double vstart  = Q[0];
    const double vstop   = Q[0] * Beta / (Betastar + Tau);
    const double Mustart = Mu3 + (Mu1 - Mu2) / (1 + vstart / FrictionLengthScale);
    const double Mustop  = Mu1 + (Mu1 - Mu2) / (1 + vstop / FrictionLengthScale);
    const double T       = Mustart + (Mustop - Mustart) * (Fr / Betastar);
    Mu                   = T;
  }

  else if (Fr == 0) {
    const double vstart  = Q[0];
    const double Mustart = Mu3 + (Mu1 - Mu2) / (1 + vstart / FrictionLengthScale);
    Mu                   = Mustart;
  }

  S[0] = 0.0;
  S[1] = Q[0] * Gravity * sin(Zeta) - Mu * (u / ubar) * Q[0] * Gravity * cos(Zeta);
  S[2] = -Mu * (v / ubar) * Q[0] * Gravity * cos(Zeta);
}

::exahype2::RefinementCommand applications::exahype2::avalanche::SWESolver::refinementCriterion(
  const double* __restrict__ Q, // Q[3+1]
  const tarch::la::Vector<Dimensions, double>& volumeX,
  const tarch::la::Vector<Dimensions, double>& volumeH,
  double                                       t
) {
  ::exahype2::RefinementCommand result = ::exahype2::RefinementCommand::Keep;
  // result = volumeX[0] < 0.0 ? ::exahype2::RefinementCommand::Refine : result;
  result = ::exahype2::RefinementCommand::Keep;
  return result;
}
