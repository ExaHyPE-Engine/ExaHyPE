// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "exahype2/dg/Basis.h"
#include "exahype2/dg/Kernels.h"
#include "peano4/Constants.h"
#include "peano4/Partition.h"
#include "tarch/CompilerSpecificSettings.h"
#include "tarch/Span.h"
#include "tarch/accelerator/ChunkedMemoryPool.h"
#include "tarch/accelerator/CompileTarget.h"
#include "tarch/la/Vector.h"

namespace exahype2::dg::ader {
  template <typename Derived, typename ComputePrecision>
  class Corrector;

  template <typename Derived, bool ReportEigenvalues, typename StoragePrecision, typename ComputePrecision>
  struct CorrectCells;
} // namespace exahype2::dg::ader

TARGET_DECLARE
/**
 * The corrector phase of one cell of an ADER-DG solver.
 */
template <typename Derived, typename ComputePrecision>
class exahype2::dg::ader::Corrector final {
public:
  using CellLayout = exahype2::dg::PaddedLayout<Derived>;

  template <bool ReportEigenvalues, typename StoragePrecision, typename Postprocessing>
  static INLINE_METHOD GPU_CALLABLE_METHOD exahype2::dg::MaxEigenvalueStatistics correct(
    const tarch::la::Vector<DIMENSIONS, double>& cellCentre,
    const tarch::la::Vector<DIMENSIONS, double>& cellSize,
    const ComputePrecision* const                faceFluxes[peano4::FacesPerCell],
    StoragePrecision* NOALIAS                    cellState,
    const ComputePrecision* NOALIAS              faceQuadratureWeights,
    const ComputePrecision* NOALIAS              inverseCellQuadratureWeights,
    const ComputePrecision* NOALIAS              leftFaceCoefficients,
    const ComputePrecision* NOALIAS              rightFaceCoefficients,
    const StoragePrecision* NOALIAS              stateQuadraturePoints,
    double                                       timeStamp,
    double                                       timeStepSize,
    Postprocessing&&                             postprocessCell
  );
};

/**
 * Corrects every listed cell and reduces the wave-speed maxima of the updated solution when
 * ReportEigenvalues is true.
 */
template <typename Derived, bool ReportEigenvalues, typename StoragePrecision, typename ComputePrecision>
struct exahype2::dg::ader::CorrectCells final {
  using CellLayout = exahype2::dg::PaddedLayout<Derived>;

  using CellPool = tarch::accelerator::ChunkedMemoryPool<StoragePrecision>;
  using FluxPool = tarch::accelerator::ChunkedMemoryPool<ComputePrecision>;

  peano4::Partition            partition{};
  tarch::Span<const int>       cells{};
  typename CellPool::View      solution{};
  typename FluxPool::ConstView faceFluxes{};
  const ComputePrecision*      faceQuadratureWeights{nullptr};
  const ComputePrecision*      inverseCellQuadratureWeights{nullptr};
  const ComputePrecision*      leftFaceCoefficients{nullptr};
  const ComputePrecision*      rightFaceCoefficients{nullptr};
  const StoragePrecision*      stateQuadraturePoints{nullptr};
  double                       timeStamp{0.0};
  double                       timeStepSize{0.0};

  INLINE_METHOD GPU_CALLABLE_METHOD void operator()(
    int                                    entry,
    exahype2::dg::MaxEigenvalueStatistics& accumulator
  ) const;
};
TARGET_DECLARE_END

#include "exahype2/dg/ader/Corrector.cpph"
