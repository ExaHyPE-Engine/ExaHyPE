// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "exahype2/dg/Basis.h"
#include "exahype2/dg/ader/LinearPredictor.h"
#include "exahype2/dg/ader/NonlinearPredictor.h"
#include "exahype2/dg/ader/TeamWorkers.h"
#include "peano4/CellConnectivity.h"
#include "tarch/CompilerSpecificSettings.h"
#include "tarch/Span.h"
#include "tarch/accelerator/ChunkedMemoryPool.h"
#include "tarch/accelerator/CompileTarget.h"
#include "tarch/accelerator/ParallelFor.h"
#include "tarch/la/Vector.h"

namespace exahype2::dg::ader {
  template <typename Derived, typename PredictorPrecision, typename TimeAveragePrecision>
  class Predictor;

  struct PredictionCounts;

  template <typename Derived, typename StoragePrecision, typename PredictorPrecision, typename TimeAveragePrecision, typename CorrectorPrecision, typename DivergenceMarks>
  struct PredictCells;
} // namespace exahype2::dg::ader

namespace exahype2::dg::ader::tests {
  class PredictorTest;
} // namespace exahype2::dg::ader::tests

TARGET_DECLARE
/**
 * The predictor phase of one cell of an ADER-DG solver. Derived::IsLinear selects LinearPredictor
 * or NonlinearPredictor.
 */
template <typename Derived, typename PredictorPrecision, typename TimeAveragePrecision>
class exahype2::dg::ader::Predictor final {
public:
  using CellLayout = exahype2::dg::PaddedLayout<Derived>;
  using Linear     = exahype2::dg::ader::LinearPredictor<Derived, PredictorPrecision>;
  using Nonlinear  = exahype2::dg::ader::NonlinearPredictor<Derived, PredictorPrecision, TimeAveragePrecision>;

  static constexpr int ScratchSize = Derived::IsLinear ? Linear::Scratch::Size : Nonlinear::Scratch::PicardSize;
  static constexpr int TimeAveragedScratchSize = Derived::IsLinear ? 0 : Nonlinear::Scratch::TimeAveragedSize;

  template <typename StoragePrecision, typename CorrectorPrecision>
  static INLINE_METHOD GPU_CALLABLE_METHOD int predict(
    const tarch::la::Vector<DIMENSIONS, double>&            cellCentre,
    const tarch::la::Vector<DIMENSIONS, double>&            cellSize,
    CorrectorPrecision* const*                              faceStates,
    CorrectorPrecision* const*                              faceFluxes,
    StoragePrecision* NOALIAS                               cellState,
    const double* NOALIAS                                   pointSourceLocations,
    PredictorPrecision* NOALIAS                             scratch,
    TimeAveragePrecision* NOALIAS                           timeAveragedScratch,
    const exahype2::dg::BasisTables<PredictorPrecision>&    tables,
    const exahype2::dg::BasisTables<TimeAveragePrecision>&  timeAveragedTables,
    double                                                  timeStamp,
    double                                                  timeStepSize,
    const TeamWorkers&                                      workers = TeamWorkers{}
  );

  [[nodiscard]] static INLINE_METHOD GPU_CALLABLE_METHOD int getTeamScratchBytes(int teamThreads);

private:
  friend class exahype2::dg::ader::tests::PredictorTest;

  template <bool AccumulatesFlux, bool AccumulatesSource, typename StoragePrecision, typename SourceOfNode>
  static INLINE_METHOD GPU_CALLABLE_METHOD void subtractTimeAveragedVolumeIntegral(
    StoragePrecision* NOALIAS           cellState,
    const TimeAveragePrecision* NOALIAS timeAveragedFlux,
    const SourceOfNode&                 sourceOfNode,
    double                              timeStepSize,
    const TeamWorkers&                  workers = TeamWorkers{}
  );
};

/**
 * The numbers of cells of one predictor sweep whose Picard iteration diverged or stopped at its
 * sweep limit.
 */
struct exahype2::dg::ader::PredictionCounts final {
  using ValueType = exahype2::dg::ader::PredictionCounts;

  int divergedCells{0};
  int unconvergedCells{0};

  [[nodiscard]] static INLINE_METHOD GPU_CALLABLE_METHOD ValueType identity();
  [[nodiscard]] static INLINE_METHOD GPU_CALLABLE_METHOD ValueType combine(ValueType left, ValueType right);
};

/**
 * Runs the predictor of every listed cell on teams with threadsPerCell threads per cell. With a
 * limiter, DivergenceMarks is its status view and a cell whose Picard iteration diverged or
 * stopped at its sweep limit marks itself there. Without a limiter, DivergenceMarks is an empty
 * type and the reduction counts those cells.
 */
template <typename Derived, typename StoragePrecision, typename PredictorPrecision, typename TimeAveragePrecision, typename CorrectorPrecision, typename DivergenceMarks>
struct exahype2::dg::ader::PredictCells final {
  using CellLayout = exahype2::dg::PaddedLayout<Derived>;

  static constexpr bool HasLimiter = not std::is_empty_v<DivergenceMarks>;

  using CellPool  = tarch::accelerator::ChunkedMemoryPool<StoragePrecision>;
  using StatePool = tarch::accelerator::ChunkedMemoryPool<CorrectorPrecision>;
  using FluxPool  = tarch::accelerator::ChunkedMemoryPool<CorrectorPrecision>;

  peano4::CellConnectivity                        cellConnectivity{};
  tarch::Span<const int>                          cells{};
  int                                             numberOfTeams{0};
  int                                             threadsPerCell{1};
  typename CellPool::View                         solution{};
  typename StatePool::View                        faceStates{};
  typename FluxPool::View                         faceFluxes{};
  exahype2::dg::BasisTables<PredictorPrecision>   tables{};
  exahype2::dg::BasisTables<TimeAveragePrecision> timeAveragedTables{};
  const double*                                   pointSourceLocations{nullptr};
  PredictorPrecision*                             predictorScratch{nullptr};
  TimeAveragePrecision*                           predictorTimeAveragedScratch{nullptr};
  int                                             predictorScratchSize{0};
  int                                             predictorTimeAveragedScratchSize{0};
  int                                             predictorScratchChunks{0};
  DivergenceMarks                                 divergenceMarks{};
  double                                          timeStamp{0.0};
  double                                          timeStepSize{0.0};

  INLINE_METHOD GPU_CALLABLE_METHOD void operator()(
    int                            entry,
    tarch::accelerator::TeamHandle team,
    PredictionCounts&              accumulator
  ) const;
};
TARGET_DECLARE_END

#include "exahype2/dg/ader/Predictor.cpph"
