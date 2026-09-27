// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "exahype2/dg/Basis.h"
#include "exahype2/dg/ader/LinearPredictor.h"
#include "exahype2/dg/ader/NonlinearPredictor.h"
#include "peano4/Partition.h"
#include "tarch/CompilerSpecificSettings.h"
#include "tarch/Span.h"
#include "tarch/accelerator/ChunkedMemoryPool.h"
#include "tarch/accelerator/CompileTarget.h"
#include "tarch/la/Vector.h"

namespace exahype2::dg::ader {
  template <typename Derived, typename ComputePrecision>
  class Predictor;

  template <typename Derived, typename StoragePrecision, typename ComputePrecision, typename DivergenceMarks>
  struct PredictCells;
} // namespace exahype2::dg::ader

namespace exahype2::dg::ader::tests {
  class PredictorTest;
} // namespace exahype2::dg::ader::tests

TARGET_DECLARE
/**
 * The predictor phase of one cell of an ADER-DG solver. predict() runs the space-time predictor
 * of the system of Derived, NonlinearPredictor or LinearPredictor, writes the time-averaged face
 * states and face fluxes onto the halves of the faces the cell owns and subtracts the volume
 * integral of the time averages from the solution. Derived is the generated solver class, whose
 * IsLinear selects the predictor.
 */
template <typename Derived, typename ComputePrecision>
class exahype2::dg::ader::Predictor final {
public:
  using CellLayout = exahype2::dg::PaddedLayout<Derived>;
  using Tables     = exahype2::dg::BasisTables<ComputePrecision>;
  using Linear     = exahype2::dg::ader::LinearPredictor<Derived, ComputePrecision>;
  using Nonlinear  = exahype2::dg::ader::NonlinearPredictor<Derived, ComputePrecision>;

  /**
   * Values of the scratch block of one cell: The block of the linear predictor, or the
   * time-averaged block of the nonlinear one.
   */
  static constexpr int ScratchSize = Derived::IsLinear ? Linear::Scratch::Size : Nonlinear::Scratch::TimeAveragedSize;

  /** Values of the Picard block of one cell; zero for a linear Derived. */
  static constexpr int PicardScratchSize = Derived::IsLinear ? 0 : Nonlinear::Scratch::PicardSize;

  template <typename StoragePrecision>
  static INLINE_METHOD GPU_CALLABLE_METHOD int predict(
    const tarch::la::Vector<DIMENSIONS, double>& cellCentre,
    const tarch::la::Vector<DIMENSIONS, double>& cellSize,
    ComputePrecision* const*                     faceStates,
    ComputePrecision* const*                     faceFluxes,
    StoragePrecision* NOALIAS                    cellState,
    const double* NOALIAS                        pointSourceLocations,
    ComputePrecision* NOALIAS                    scratch,
    ComputePrecision* NOALIAS                    picardScratch,
    const Tables&                                tables,
    double                                       timeStamp,
    double                                       timeStepSize
  );

private:
  friend class exahype2::dg::ader::tests::PredictorTest;

  template <bool AccumulatesFlux, bool AccumulatesSource, typename StoragePrecision>
  static INLINE_METHOD GPU_CALLABLE_METHOD void subtractTimeAveragedVolumeIntegral(
    StoragePrecision* NOALIAS       cellState,
    const ComputePrecision* NOALIAS timeAveragedFlux,
    const ComputePrecision* NOALIAS timeAveragedSource,
    double                          timeStepSize
  );
};

/**
 * Runs the predictor of every listed cell: The solution takes its volume integral, and the cell's
 * own half of every face takes the time-averaged face states and face fluxes. DivergenceMarks is
 * the status view of a limiter, in which a cell with a diverged Picard iteration marks itself, or
 * an empty type, and then the reduction counts those cells.
 */
template <typename Derived, typename StoragePrecision, typename ComputePrecision, typename DivergenceMarks>
struct exahype2::dg::ader::PredictCells final {
  using CellLayout = exahype2::dg::PaddedLayout<Derived>;

  static constexpr bool HasLimiter = not std::is_empty_v<DivergenceMarks>;

  using CellPool  = tarch::accelerator::ChunkedMemoryPool<StoragePrecision>;
  using StatePool = tarch::accelerator::ChunkedMemoryPool<ComputePrecision>;
  using FluxPool  = tarch::accelerator::ChunkedMemoryPool<ComputePrecision>;

  peano4::Partition                           partition{};
  tarch::Span<const int>                      cells{};
  typename CellPool::View                     solution{};
  typename StatePool::View                    faceStates{};
  typename FluxPool::View                     faceFluxes{};
  exahype2::dg::BasisTables<ComputePrecision> tables{};
  const double*                               pointSourceLocations{nullptr};
  ComputePrecision*                           predictorScratch{nullptr};
  int                                         predictorScratchSize{0};
  ComputePrecision*                           picardScratch{nullptr};
  int                                         picardScratchSize{0};
  DivergenceMarks                             divergenceMarks{};
  double                                      timeStamp{0.0};
  double                                      timeStepSize{0.0};

  INLINE_METHOD GPU_CALLABLE_METHOD void operator()(int entry, int& accumulator) const;
};
TARGET_DECLARE_END

#include "exahype2/dg/ader/Predictor.cpph"
