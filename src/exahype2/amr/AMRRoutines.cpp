// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "AMRRoutines.h"

/**
 * The transfer routines are defined in the .cpph, so this file names every specialisation
 * and compiles it along with the library, for the two scalar types a caller holds its
 * patches in.
 *
 * interpolateCell(), restrictCell() and restrictHaloLayerFromStagedSubfaces() are overloaded
 * on their matrix arguments, which an address alone cannot tell apart, so those pairs are
 * instantiated by signature.
 */
template void exahype2::amr::interpolateCell<
  double>(const peano4::datamanagement::CellMarker&, const double* NOALIAS, double* NOALIAS, const int, const int);
template void exahype2::amr::interpolateCell<
  float>(const peano4::datamanagement::CellMarker&, const float* NOALIAS, float* NOALIAS, const int, const int);

template void exahype2::amr::interpolateCell<double>(
  const peano4::datamanagement::CellMarker&,
  const double* NOALIAS,
  double* NOALIAS,
  const double* NOALIAS,
  const int,
  const int
);
template void exahype2::amr::interpolateCell<float>(
  const peano4::datamanagement::CellMarker&,
  const float* NOALIAS,
  float* NOALIAS,
  const double* NOALIAS,
  const int,
  const int
);

template void exahype2::amr::restrictCell<
  double>(const peano4::datamanagement::CellMarker&, double* NOALIAS, const double* NOALIAS, const int, const int);
template void exahype2::amr::restrictCell<
  float>(const peano4::datamanagement::CellMarker&, float* NOALIAS, const float* NOALIAS, const int, const int);

template void exahype2::amr::restrictCell<double>(
  const peano4::datamanagement::CellMarker&,
  double* NOALIAS,
  const double* NOALIAS,
  const double* NOALIAS,
  const double* NOALIAS,
  const int,
  const int
);
template void exahype2::amr::restrictCell<float>(
  const peano4::datamanagement::CellMarker&,
  float* NOALIAS,
  const float* NOALIAS,
  const double* NOALIAS,
  const double* NOALIAS,
  const int,
  const int
);

template void exahype2::amr::restrictHaloLayerFromStagedSubfaces<double>(
  exahype2::amr::ChildDataStage<double>&,
  const void*,
  double* NOALIAS,
  const int,
  const int,
  const int
);
template void exahype2::amr::restrictHaloLayerFromStagedSubfaces<float>(
  exahype2::amr::ChildDataStage<float>&,
  const void*,
  float* NOALIAS,
  const int,
  const int,
  const int
);

template void exahype2::amr::restrictHaloLayerFromStagedSubfaces<double>(
  exahype2::amr::ChildDataStage<double>&,
  const void*,
  double* NOALIAS,
  const double* NOALIAS,
  const double* NOALIAS,
  const int,
  const int,
  const int
);
template void exahype2::amr::restrictHaloLayerFromStagedSubfaces<float>(
  exahype2::amr::ChildDataStage<float>&,
  const void*,
  float* NOALIAS,
  const double* NOALIAS,
  const double* NOALIAS,
  const int,
  const int,
  const int
);

template void exahype2::amr::restrictCellFromChildren<double>(
  double* NOALIAS,
  const double* const* NOALIAS,
  const double* const* NOALIAS,
  const int,
  const int
);
template void exahype2::amr::restrictCellFromChildren<float>(
  float* NOALIAS,
  const float* const* NOALIAS,
  const double* const* NOALIAS,
  const int,
  const int
);

template void exahype2::amr::restrictCellFromChildren<double>(
  double* NOALIAS,
  const double* const* NOALIAS,
  const int,
  const int
);
template void exahype2::amr::restrictCellFromChildren<float>(
  float* NOALIAS,
  const float* const* NOALIAS,
  const int,
  const int
);

namespace {
  [[maybe_unused]] void instantiateTemplatesForCompileCheck() {
    [[maybe_unused]] auto* interpolateHaloInDouble = &exahype2::amr::interpolateHaloLayer<double>;
    [[maybe_unused]] auto* interpolateHaloInFloat  = &exahype2::amr::interpolateHaloLayer<float>;

    [[maybe_unused]] auto* interpolateSlopeInDouble = &exahype2::amr::interpolateHaloLayerWithTangentialSlope<double>;
    [[maybe_unused]] auto* interpolateSlopeInFloat  = &exahype2::amr::interpolateHaloLayerWithTangentialSlope<float>;

    [[maybe_unused]] auto* interpolateFromCellInDouble = &exahype2::amr::interpolateHaloLayerFromCoarseCell<double>;
    [[maybe_unused]] auto* interpolateFromCellInFloat  = &exahype2::amr::interpolateHaloLayerFromCoarseCell<float>;

    [[maybe_unused]] auto* restrictHaloInDouble = &exahype2::amr::restrictHaloLayer<double>;
    [[maybe_unused]] auto* restrictHaloInFloat  = &exahype2::amr::restrictHaloLayer<float>;

    [[maybe_unused]] auto* stageHaloInDouble = &exahype2::amr::stageHaloLayerForGatheredRestriction<double>;
    [[maybe_unused]] auto* stageHaloInFloat  = &exahype2::amr::stageHaloLayerForGatheredRestriction<float>;

    [[maybe_unused]] auto* applyTensorProductInDouble =
      &exahype2::amr::applyTensorProductOrderInvariant<double>;
    [[maybe_unused]] auto* applyTensorProductInFloat =
      &exahype2::amr::applyTensorProductOrderInvariant<float>;

    [[maybe_unused]] auto* interpolateFaceInDouble = &exahype2::amr::interpolateFaceFromCoarse<double>;
    [[maybe_unused]] auto* interpolateFaceInFloat  = &exahype2::amr::interpolateFaceFromCoarse<float>;

    [[maybe_unused]] auto* restrictFaceInDouble = &exahype2::amr::restrictFaceFromChildren<double>;
    [[maybe_unused]] auto* restrictFaceInFloat  = &exahype2::amr::restrictFaceFromChildren<float>;
  }
} // namespace
