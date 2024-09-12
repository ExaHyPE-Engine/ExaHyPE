// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "HLLEM.h"

#include "Constants.h"
#include "tarch/Assertions.h"
#include "tarch/NonCriticalAssertions.h"
#include "VariableShortcuts.h"

using s = applications::exahype2::swe::VariableShortcuts;

#if defined(GPUOffloadingOMP)
#pragma omp declare target
#endif
double applications::exahype2::swe::hllem(
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
  const bool leftWet  = tarch::la::greater(QL[s::h], DRY_TOLERANCE);
  const bool rightWet = tarch::la::greater(QR[s::h], DRY_TOLERANCE);
  const bool leftDry  = !leftWet;
  const bool rightDry = !rightWet;

  for (int i = 0; i < 3; i++) {
    AMDQ[i] = 0.0;
    APDQ[i] = 0.0;
  }

  // Dry land
  if (leftDry or rightDry) {
    return 0.0;
  }

  const double m_h_l = leftWet ? QL[s::h] : QR[s::h];
  const double m_h_r = rightWet ? QR[s::h] : QL[s::h];

  const double m_hu_l = leftWet ? QL[s::hu + 0] : -1 * QR[s::hu + 0];
  const double m_hu_r = rightWet ? QR[s::hu + 0] : -1 * QL[s::hu + 0];

  const double m_hv_l = leftWet ? QL[s::hu + 1] : -1 * QR[s::hu + 1];
  const double m_hv_r = rightWet ? QR[s::hu + 1] : -1 * QL[s::hu + 1];

  const double bat_l = leftWet ? QL[s::b] : QR[s::b];
  const double bat_r = rightWet ? QR[s::b] : QL[s::b];

  double smax = 0.0;
  {
    double maxAbsLambda = 0.0;
    if (leftWet and rightWet) {
      for (int i = 0; i < 3; i++) {
        maxAbsLambda = std::max(std::abs(LL[i]), std::abs(LR[i]));
        smax         = std::max(smax, maxAbsLambda);
      }
    }
  }

  const double bm   = std::max(bat_l, bat_r);
  const double hRoe = 0.5 * (m_h_l + m_h_r);
  const double DEta = std::max(m_h_r + bat_r - bm, 0.0)
                      - std::max(m_h_l + bat_l - bm, 0.0);

  double flux[3] = {0.0};
  flux[0]        = 0.5 * (FL[s::h] + FR[s::h]) - 0.5 * smax * DEta;
  flux[1]        = 0.5 * (FL[s::hu + 0] + FR[s::hu + 0])
            - 0.5 * smax * (m_hu_r - m_hu_l);
  flux[2] = 0.5 * (FL[s::hu + 1] + FR[s::hu + 1])
            - 0.5 * smax * (m_hv_r - m_hv_l);

  APDQ[s::h] = flux[s::h];
  AMDQ[s::h] = flux[s::h];

  APDQ[s::hu + 0] = flux[s::hu + 0];
  AMDQ[s::hu + 0] = flux[s::hu + 0];

  APDQ[s::hu + 1] = flux[s::hu + 1];
  AMDQ[s::hu + 1] = flux[s::hu + 1];

  APDQ[normal + 1] -= 0.5 * g * hRoe * DEta;
  AMDQ[normal + 1] += 0.5 * g * hRoe * DEta;

  return smax;
}
#if defined(GPUOffloadingOMP)
#pragma omp end declare target
#endif
