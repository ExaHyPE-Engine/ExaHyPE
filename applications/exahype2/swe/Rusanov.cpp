// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "Rusanov.h"

#include "Constants.h"
#include "tarch/Assertions.h"
#include "tarch/NonCriticalAssertions.h"
#include "VariableShortcuts.h"

using s = applications::exahype2::swe::VariableShortcuts;

#if defined(GPUOffloadingOMP)
#pragma omp declare target
#endif
double applications::exahype2::swe::rusanov(
  const double* __restrict__ QR,
  const double* __restrict__ QL,
  const double* __restrict__ FR,
  const double* __restrict__ FL,
  const double* __restrict__ LR,
  const double* __restrict__ LL,
  const ::tarch::la::Vector<Dimensions, double>& xR,
  const ::tarch::la::Vector<Dimensions, double>& xL,
  const ::tarch::la::Vector<Dimensions, double>& h,
  double                                         t,
  double                                         dt,
  int                                            normal,
  double* __restrict__ APDQ,
  double* __restrict__ AMDQ
) {
  double fluxLeft[3]{0.0};
  double fluxRight[3]{0.0};
  double lambdaLeft[3]{0.0};
  double lambdaRight[3]{0.0};

  bool leftDry  = !tarch::la::greater(QL[s::h], DRY_TOLERANCE);
  bool rightDry = !tarch::la::greater(QR[s::h], DRY_TOLERANCE);

  double h_l  = QL[s::h];
  double hu_l = QL[s::hu + 0];
  double hv_l = QL[s::hu + 1];
  double b_l  = QL[s::b];

  double h_r  = QR[s::h];
  double hu_r = QR[s::hu + 0];
  double hv_r = QR[s::hu + 1];
  double b_r  = QR[s::b];

  for (int i = 0; i < 3; i++) {
    AMDQ[i] = 0.0;
    APDQ[i] = 0.0;
  }

  // Dry land
  if (leftDry or rightDry) {
    return 0.0;
  } else if (rightDry and !leftDry) {
    b_r  = b_l;
    h_r  = h_l;
    hu_r = -hu_l;
    hv_r = -hv_l;
    for (int i = 0; i < 3; i++) {
      lambdaLeft[i]  = LL[i];
      lambdaRight[i] = -lambdaLeft[i];
      fluxLeft[i]    = FL[i];
      fluxRight[i]   = fluxLeft[i];
    }
    fluxRight[0] *= -1.0;
  } else if (leftDry and !rightDry) {
    b_l  = b_r;
    h_l  = h_r;
    hu_l = -hu_r;
    hv_l = -hv_r;
    for (int i = 0; i < 3; i++) {
      lambdaRight[i] = LR[i];
      lambdaLeft[i]  = -lambdaRight[i];
      fluxRight[i]   = FR[i];
      fluxLeft[i]    = fluxRight[i];
    }
    fluxLeft[0] *= -1.0;
  } else {
    for (int i = 0; i < 3; i++) {
      lambdaRight[i] = LR[i];
      lambdaLeft[i]  = LL[i];
      fluxRight[i]   = FR[i];
      fluxLeft[i]    = FL[i];
    }
  }

  double smax = 0.0;
  {
    double maxAbsLambda = 0.0;
    for (int i = 0; i < 3; i++) {
      maxAbsLambda = std::max(
        std::abs(lambdaLeft[i]),
        std::abs(lambdaRight[i])
      );
      smax = std::max(smax, maxAbsLambda);
    }
  }

  double flux[3] = {0.0};
  flux[0] = 0.5 * (fluxLeft[0] + fluxRight[0]) - 0.5 * smax * (h_r - h_l);
  flux[1] = 0.5 * (fluxLeft[1] + fluxRight[1]) - 0.5 * smax * (hu_r - hu_l);
  flux[2] = 0.5 * (fluxLeft[2] + fluxRight[2]) - 0.5 * smax * (hv_r - hv_l);

  for (int i = 0; i < 3; i++) {
    APDQ[i] = !rightDry * flux[i];
    AMDQ[i] = !leftDry * flux[i];
  }

  APDQ[normal + 1] -= !rightDry * 0.25 * g * (h_l + h_r) * (b_r - b_l);
  AMDQ[normal + 1] += !leftDry * 0.25 * g * (h_l + h_r) * (b_r - b_l);

  return smax;
}
#if defined(GPUOffloadingOMP)
#pragma omp end declare target
#endif
