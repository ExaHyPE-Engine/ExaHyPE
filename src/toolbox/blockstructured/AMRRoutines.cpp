// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "AMRRoutines.h"

/**
 * The transfer routines are defined in the .cpph, so nothing in the core library
 * instantiates them and a mistake in a body would first surface when a solver calls it.
 * Naming the specialisations here compiles them along with the library, for the two widths
 * a solver may hold its patches in.
 *
 * interpolateCell() and restrictCell() are overloaded on their matrix arguments, which an
 * address alone cannot tell apart, so those two pairs are instantiated by signature.
 */
template void toolbox::blockstructured::interpolateCell<
  double>(const peano4::datamanagement::CellMarker&, const double* NOALIAS, double* NOALIAS, const int, const int);
template void toolbox::blockstructured::interpolateCell<
  float>(const peano4::datamanagement::CellMarker&, const float* NOALIAS, float* NOALIAS, const int, const int);

template void toolbox::blockstructured::interpolateCell<double>(
  const peano4::datamanagement::CellMarker&,
  const double* NOALIAS,
  double*       NOALIAS,
  const double* NOALIAS,
  const int,
  const int
);
template void toolbox::blockstructured::interpolateCell<float>(
  const peano4::datamanagement::CellMarker&,
  const float*  NOALIAS,
  float*        NOALIAS,
  const double* NOALIAS,
  const int,
  const int
);

template void toolbox::blockstructured::restrictCell<
  double>(const peano4::datamanagement::CellMarker&, double* NOALIAS, const double* NOALIAS, const int, const int);
template void toolbox::blockstructured::restrictCell<
  float>(const peano4::datamanagement::CellMarker&, float* NOALIAS, const float* NOALIAS, const int, const int);

template void toolbox::blockstructured::restrictCell<double>(
  const peano4::datamanagement::CellMarker&,
  double*       NOALIAS,
  const double* NOALIAS,
  const double* NOALIAS,
  const double* NOALIAS,
  const int,
  const int
);
template void toolbox::blockstructured::restrictCell<float>(
  const peano4::datamanagement::CellMarker&,
  float*        NOALIAS,
  const float*  NOALIAS,
  const double* NOALIAS,
  const double* NOALIAS,
  const int,
  const int
);

namespace {
  [[maybe_unused]] void instantiateTemplatesForCompileCheck() {
    [[maybe_unused]] auto* interpolateHaloInDouble = &toolbox::blockstructured::interpolateHaloLayer<double>;
    [[maybe_unused]] auto* interpolateHaloInFloat  = &toolbox::blockstructured::interpolateHaloLayer<float>;

    [[maybe_unused]] auto* interpolateSlopeInDouble =
      &toolbox::blockstructured::interpolateHaloLayerWithTangentialSlope<double>;
    [[maybe_unused]] auto* interpolateSlopeInFloat =
      &toolbox::blockstructured::interpolateHaloLayerWithTangentialSlope<float>;

    [[maybe_unused]] auto* interpolateFromCellInDouble =
      &toolbox::blockstructured::interpolateHaloLayerFromCoarseCell<double>;
    [[maybe_unused]] auto* interpolateFromCellInFloat =
      &toolbox::blockstructured::interpolateHaloLayerFromCoarseCell<float>;

    [[maybe_unused]] auto* restrictHaloInDouble = &toolbox::blockstructured::restrictHaloLayer<double>;
    [[maybe_unused]] auto* restrictHaloInFloat  = &toolbox::blockstructured::restrictHaloLayer<float>;
  }
} // namespace
