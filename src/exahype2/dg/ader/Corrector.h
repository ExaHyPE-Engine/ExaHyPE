// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "exahype2/dg/Basis.h"
#include "exahype2/dg/Kernels.h"
#include "peano4/CellConnectivity.h"
#include "peano4/Constants.h"
#include "tarch/CompilerSpecificSettings.h"
#include "tarch/Span.h"
#include "tarch/accelerator/ChunkedMemoryPool.h"
#include "tarch/accelerator/CompileTarget.h"
#include "tarch/accelerator/ParallelFor.h"
#include "tarch/la/Vector.h"

namespace exahype2::dg::ader {
  template <typename Derived, typename StoragePrecision, typename ComputePrecision>
  class Corrector;

  template <typename Derived, bool ReportEigenvalues, typename StoragePrecision, typename ComputePrecision>
  struct CorrectCells;
} // namespace exahype2::dg::ader

TARGET_DECLARE
/**
 * The corrector phase of an ADER-DG solver. Every thread of a team corrects its own nodes.
 */
template <typename Derived, typename StoragePrecision, typename ComputePrecision>
class exahype2::dg::ader::Corrector final {
public:
  using CellLayout = exahype2::dg::PaddedLayout<Derived>;

  [[nodiscard]] static INLINE_METHOD int getTeamThreads();
  [[nodiscard]] static INLINE_METHOD int getNumberOfTeams(int cells);

  static INLINE_METHOD GPU_CALLABLE_METHOD void correctNode(
    StoragePrecision* NOALIAS       cellValues,
    int                             node,
    const ComputePrecision* const   faceFluxes[peano4::FacesPerCell],
    const ComputePrecision* NOALIAS faceQuadratureWeights,
    const ComputePrecision* NOALIAS inverseCellQuadratureWeights,
    const ComputePrecision* NOALIAS leftFaceCoefficients,
    const ComputePrecision* NOALIAS rightFaceCoefficients,
    const double                    inverseCellSize[DIMENSIONS],
    double                          timeStepSize
  );
};

/**
 * Corrects every listed cell and reduces the wave-speed maxima of the updated solution when
 * ReportEigenvalues is true. Team entry corrects the cells entry, entry + numberOfTeams and so on.
 * Its threads divide the nodes of these cells among themselves.
 */
template <typename Derived, bool ReportEigenvalues, typename StoragePrecision, typename ComputePrecision>
struct exahype2::dg::ader::CorrectCells final {
  using Kernel     = exahype2::dg::ader::Corrector<Derived, StoragePrecision, ComputePrecision>;
  using CellLayout = typename Kernel::CellLayout;

  using CellPool = tarch::accelerator::ChunkedMemoryPool<StoragePrecision>;
  using FluxPool = tarch::accelerator::ChunkedMemoryPool<ComputePrecision>;

  peano4::CellConnectivity            cellConnectivity{};
  tarch::Span<const int>              cells{};
  int                                 numberOfTeams{1};
  typename CellPool::View             solution{};
  typename FluxPool::ConstView        faceFluxes{};
  const ComputePrecision*             faceQuadratureWeights{nullptr};
  const ComputePrecision*             inverseCellQuadratureWeights{nullptr};
  const ComputePrecision*             leftFaceCoefficients{nullptr};
  const ComputePrecision*             rightFaceCoefficients{nullptr};
  const StoragePrecision*             stateQuadraturePoints{nullptr};
  double                              timeStamp{0.0};
  double                              timeStepSize{0.0};
  typename Derived::RuntimeParameters parameters{};

  INLINE_METHOD GPU_CALLABLE_METHOD void operator()(
    int                                    entry,
    tarch::accelerator::TeamHandle         team,
    exahype2::dg::MaxEigenvalueStatistics& accumulator
  ) const;
};
TARGET_DECLARE_END

#include "exahype2/dg/ader/Corrector.cpph"
