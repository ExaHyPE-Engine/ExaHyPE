#include "SWESolver.h"

#include <cmath>
#include <iostream>

#include "exahype2/fv/PatchAccess.h"

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
  ::exahype2::fv::PatchAccess access(
    Q,                                    // make it initially point to input
    NumberOfFiniteVolumesPerAxisPerPatch, // defined in superclass
    1,                                    // halo size, which you know as user
    NumberOfUnknowns,                     // defined in superclass
    NumberOfAuxiliaryVariables            // defined in superclass
  );

  const int voxel_size = NumberOfUnknowns + NumberOfAuxiliaryVariables;
  std::cout<<"coords: ("<<faceCentre(0)<<", "<<faceCentre(1)<<")"<<std::endl;
  std::cout<<"\n";

  // Values of unkowns in central volume(current volume)
  const double ih_centre = 1.0 / access.centre(0);
  const double h_centre  = access.centre(0);
  const double u_centre  = ih_centre * access.centre(1);
  const double v_centre  = ih_centre * access.centre(2);

  std::cout<<"ih_centre "<<ih_centre<<std::endl;
  std::cout<<"h_centre "<<h_centre<<std::endl;
  // std::cout<<"u_centre "<<u_centre<<std::endl;
  // std::cout<<"v_centre "<<v_centre<<std::endl;
  std::cout<<"\n";

  // Values of unkowns in the left volume
  const double ih_left = 1.0 / access.left(0, 0);
  const double h_left  = access.left(0, 0);
  const double u_left  = ih_left * access.left(0, 1);
  const double v_left  = ih_left * access.left(0, 2);
  
  std::cout<<"ih_left "<<ih_left<<std::endl;
  std::cout<<"h_left "<<h_left<<std::endl;
  // std::cout<<"u_left "<<u_left<<std::endl;
  // std::cout<<"v_left "<<v_left<<std::endl;
  std::cout<<"\n";
  

  // Values of unkowns in the right volume
  const double ih_right = 1.0 / access.right(0, 0);
  const double h_right  = access.right(0, 0);
  const double u_right  = ih_right * access.right(0, 1);
  const double v_right  = ih_right * access.right(0, 2);

  std::cout<<"ih_right "<<ih_right<<std::endl;
  std::cout<<"h_right "<<h_right<<std::endl;
  // std::cout<<"u_right "<<u_right<<std::endl;
  // std::cout<<"v_right "<<v_right<<std::endl;
  std::cout<<"\n";

  // Values of unkowns in the lower volume
  const double ih_down = 1.0 / access.left(1, 0);
  const double h_down  = access.left(1, 0);
  const double u_down  = ih_down * access.left(1, 1);
  const double v_down  = ih_down * access.left(1, 2);

  std::cout<<"ih_down "<<ih_down<<std::endl;
  std::cout<<"h_down "<<h_down<<std::endl;
  // std::cout<<"u_down "<<u_down<<std::endl;
  // std::cout<<"v_down "<<v_down<<std::endl;
  std::cout<<"\n";

  // Values of unkowns in the upper volume
  const double ih_up = 1.0 / access.right(1, 0);
  const double h_up  = access.right(1, 0);
  const double u_up  = ih_up * access.right(1, 1);
  const double v_up  = ih_up * access.right(1, 2);

  std::cout<<"ih_up "<<ih_up<<std::endl;
  std::cout<<"h_up "<<h_up<<std::endl;
  // std::cout<<"u_up "<<u_up<<std::endl;
  // std::cout<<"v_up "<<v_up<<std::endl;
  std::cout<<"\n";

  // Values of unkowns in the lower-left volume 
  const double ih_lower_left = (1.0 / *(access.left(1)-voxel_size+0));
  const double h_lower_left  = *(access.left(1)-voxel_size+0);
  const double u_lower_left  = (ih_lower_left * *(access.left(1)-voxel_size+1));
  const double v_lower_left  = (ih_lower_left * *(access.left(1)-voxel_size+2));

  std::cout<<"ih_lower_left "<<ih_lower_left<<std::endl;
  std::cout<<"h_lower_left "<<h_lower_left<<std::endl;
  // std::cout<<"u_lower_left "<<u_lower_left<<std::endl;
  // std::cout<<"v_lower_left "<<v_lower_left<<std::endl;
  std::cout<<"\n";
 
  // Values of unkowns in the lower-right volume 
  const double ih_lower_right = (1.0 / *(access.left(1)+voxel_size+0));
  const double h_lower_right  = *(access.left(1)+voxel_size+0);
  const double u_lower_right  = (ih_lower_right * *(access.left(1)+voxel_size+1));
  const double v_lower_right  = (ih_lower_right * *(access.left(1)+voxel_size+2));

  std::cout<<"ih_lower_right "<<ih_lower_right<<std::endl;
  std::cout<<"h_lower_right "<<h_lower_right<<std::endl;
  // std::cout<<"u_lower_right "<<u_lower_right<<std::endl;
  // std::cout<<"v_lower_right "<<v_lower_right<<std::endl;
  std::cout<<"\n";
 
  // Values of unkowns in the upper-left volume 
  const double ih_upper_left = (1.0 / *(access.right(1)-voxel_size+0));
  const double h_upper_left  = *(access.right(1)-voxel_size+0);
  const double u_upper_left  = (ih_upper_left * *(access.right(1)-voxel_size+1));
  const double v_upper_left  = (ih_upper_left * *(access.right(1)-voxel_size+2));

  std::cout<<"ih_upper_left "<<ih_upper_left<<std::endl;
  std::cout<<"h_upper_left "<<h_upper_left<<std::endl;
  // std::cout<<"u_upper_left "<<u_upper_left<<std::endl;
  // std::cout<<"v_upper_left "<<v_upper_left<<std::endl;
  std::cout<<"\n";
 
  // Values of unkowns in the upper-right volume 
  const double ih_upper_right = (1.0 / *(access.right(1)+voxel_size+0));
  const double h_upper_right  = *(access.right(1)+voxel_size+0);
  const double u_upper_right  = (ih_upper_right * *(access.right(1)+voxel_size+1));
  const double v_upper_right  = (ih_upper_right * *(access.right(1)+voxel_size+2));

  std::cout<<"ih_upper_right "<<ih_upper_right<<std::endl;
  std::cout<<"h_upper_right "<<h_upper_right<<std::endl;
  // std::cout<<"u_upper_right "<<u_upper_right<<std::endl;
  // std::cout<<"v_upper_right "<<v_upper_right<<std::endl;
  std::cout<<"\n";

  // Partial Derivatives of unknowns

  // u
  const double u_x  = (u_right - u_left) / (2 * volumeH(0));
  const double u_y  = (u_up - u_down) / (2 * volumeH(1));
  const double u_xy = (u_upper_right - u_upper_left - u_lower_right + u_lower_left) / (4 * volumeH(0) * volumeH(1));
  const double u_xx = (u_right - 2 * u_centre + u_left) / (volumeH(0) * volumeH(0));
  const double u_yy = (u_up - 2 * u_centre + u_down) / (volumeH(1) * volumeH(1));

  std::cout<<"u_x "<<u_x<<std::endl;
  std::cout<<"u_y "<<u_y<<std::endl;
  std::cout<<"u_xx "<<u_xx<<std::endl;
  std::cout<<"u_xy "<<u_xy<<std::endl;
  std::cout<<"u_yy "<<u_yy<<std::endl;
  std::cout<<"\n";

  // v
  const double v_x  = (v_right - v_left) / (2 * volumeH(0));
  const double v_y  = (v_up - v_down) / (2 * volumeH(1));
  const double v_xy = (v_upper_right - v_upper_left - v_lower_right + v_lower_left) / (4 * volumeH(0) * volumeH(1));
  const double v_xx = (v_right - 2 * v_centre + v_left) / (volumeH(0) * volumeH(0));
  const double v_yy = (v_up - 2 * v_centre + v_down) / (volumeH(1) * volumeH(1));
  std::cout<<"v_x "<<v_x<<std::endl;
  std::cout<<"v_y "<<v_y<<std::endl;
  std::cout<<"v_xx "<<v_xx<<std::endl;
  std::cout<<"v_xy "<<v_xy<<std::endl;
  std::cout<<"v_yy "<<v_yy<<std::endl;
  std::cout<<"\n";
  std::cout<<"\n";
  std::cout<<"\n";


  switch (normal) {
  case 0: // x
    BTimesDeltaQ[0] = 0.0;
    BTimesDeltaQ[1] = Nu * sqrt(h_centre) * (1.5 * deltaQ[0] * u_x + h_centre * u_xx);
    BTimesDeltaQ[2] = 0.5 * Nu * sqrt(h_centre) * (1.5 * deltaQ[0] * (u_y + v_x) + h_centre * (u_xy + v_xx));
    break;
  case 1: // y
    BTimesDeltaQ[0] = 0.0;
    BTimesDeltaQ[1] = 0.5 * Nu * sqrt(h_centre) * (1.5 * deltaQ[0] * (u_y + v_x) + h_centre * (u_yy + v_xy));
    BTimesDeltaQ[2] = Nu * sqrt(h_centre) * (1.5 * deltaQ[0] * v_y + h_centre * v_yy);
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
