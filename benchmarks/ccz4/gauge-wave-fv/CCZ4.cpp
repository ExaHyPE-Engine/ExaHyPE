// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "CCZ4.h"

#include "Constants.h"
#include "exahype2/RefinementControl.h"
#include "tarch/NonCriticalAssertions.h"

#include <algorithm>
#include <cstring>
#include <limits>
#include <stdio.h>
#include <string.h>

tarch::logging::Log benchmarks::exahype2::ccz4::CCZ4::_log("benchmarks::exahype2::ccz4::CCZ4");

void benchmarks::exahype2::ccz4::CCZ4::initialCondition(
  double* NOALIAS                              Q,
  const tarch::la::Vector<DIMENSIONS, double>& volumeX,
  const tarch::la::Vector<DIMENSIONS, double>& volumeH
) {
  constexpr int    nVars       = 59;
  constexpr double pi          = M_PI;
  constexpr double peak_number = 2.0;
  constexpr double ICA         = 0.1; ///< Amplitude of the wave
  double           HH          = 1.0 - ICA * sin(peak_number * pi * (volumeX[0] - 0));
  double           dxH         = -peak_number * pi * ICA * cos(peak_number * pi * (volumeX[0] - 0));
  double           dxphi       = -pow(HH, (-7.0 / 6.0)) * dxH / 6.0;
  double           phi         = pow((1.0 / HH), (1.0 / 6.0));
  double           Kxx         = -0.5 * peak_number * pi * ICA * cos(peak_number * pi * (volumeX[0] - 0))
               / sqrt(1.0 - ICA * sin(peak_number * pi * (volumeX[0] - 0)));
  double traceK = Kxx / HH;
  std::memset(Q, .0, nVars * sizeof(double));
  Q[0]  = phi * phi * HH;                               //\tilde(\gamma)_xx
  Q[3]  = phi * phi;                                    //\tilde(\gamma)_yy
  Q[5]  = phi * phi;                                    //\tilde(\gamma)_zz
  Q[6]  = phi * phi * (Kxx - 1.0 / 3.0 * traceK * HH);  //\tilde(A)_xx
  Q[9]  = phi * phi * (0.0 - 1.0 / 3.0 * traceK * 1.0); //\tilde(A)_yy
  Q[11] = phi * phi * (0.0 - 1.0 / 3.0 * traceK * 1.0); //\tilde(A)_zz
  Q[16] = sqrt(HH);                                     //\alpha
  Q[13] = 2.0 / (3.0 * pow(HH, (5.0 / 3.0))) * dxH;     //\hat(\Gamma)^x
  Q[23] = 1.0 / (2.0) * dxH * pow(HH, (-1.0 / 2.0));    // A_x
  Q[35] = pow(HH, (-1.0 / 3.0)) * dxH / 3.0;            // D_xxx
  Q[38] = phi * dxphi;                                  // D_xyy
  Q[40] = phi * dxphi;                                  // D_xzz
  Q[53] = traceK;                                       // K
  Q[54] = phi;                                          //\phi
  Q[55] = dxphi;                                        // P_x

  for (int i = 0; i < NumberOfUnknowns; i++) {
    assertion2(std::isfinite(Q[i]), volumeX, i);
  }
}

void benchmarks::exahype2::ccz4::CCZ4::boundaryConditions(
  const double* NOALIAS                        Qinside,  // Qinside[59+0]
  double* NOALIAS                              Qoutside, // Qoutside[59+0]
  const tarch::la::Vector<DIMENSIONS, double>& faceCentre,
  const tarch::la::Vector<DIMENSIONS, double>& volumeH,
  double                                       t,
  int                                          normal
) {
  for (int i = 0; i < NumberOfUnknowns + NumberOfAuxiliaryVariables; i++) {
    assertion4(Qinside[i] == Qinside[i], faceCentre, t, normal, i);
    Qoutside[i] = Qinside[i];
  }
}

::exahype2::RefinementCommand benchmarks::exahype2::ccz4::CCZ4::refinementCriterion(
  const double* NOALIAS                        Q,
  const tarch::la::Vector<DIMENSIONS, double>& volumeCentre,
  const tarch::la::Vector<DIMENSIONS, double>& volumeH,
  double                                       t
) {
  ::exahype2::RefinementCommand result = ::exahype2::RefinementCommand::Keep;

  const double  radius                   = tarch::la::norm2(volumeCentre);
  constexpr int NumberOfRefinementLayers = 3;

  const double radii[NumberOfRefinementLayers] = {40, 15, 6};
  const double maxH[NumberOfRefinementLayers]  = {0.32, 0.12, 0.04};

  result = ::exahype2::RefinementCommand::Keep;
  for (int i = 0; i < NumberOfRefinementLayers; i++) {
    if (radius < radii[i] and tarch::la::max(volumeH) > maxH[i]) {
      result = ::exahype2::RefinementCommand::Refine;
    }
  }

  return result;
}
