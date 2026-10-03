// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "exahype2/dg/Kernels.h"
#include "exahype2/dg/limiting/SubcellScheme.h"
#include "peano4/Constants.h"
#include "peano4/Partition.h"
#include "tarch/CompilerSpecificSettings.h"
#include "tarch/Span.h"
#include "tarch/accelerator/ChunkedMemoryPool.h"
#include "tarch/accelerator/CompileTarget.h"
#include "tarch/la/Vector.h"

namespace exahype2::dg::limiting {
  template <typename Derived, typename ComputePrecision>
  class Detector;

  template <typename Derived, typename StoragePrecision, typename ComputePrecision>
  struct OpenInitialStatuses;

  template <typename Derived, typename StoragePrecision, typename ComputePrecision>
  struct ClearFaces;

  template <typename Derived, typename StoragePrecision, typename ComputePrecision>
  struct OpenStep;

  template <typename Derived, typename StoragePrecision, typename ComputePrecision>
  struct ReduceReferenceExtrema;

  template <typename Derived, typename StoragePrecision, typename ComputePrecision>
  struct WriteExtremaOntoFaces;

  template <typename Derived, typename StoragePrecision, typename ComputePrecision>
  struct MergeNeighbourExtrema;

  template <typename Derived, typename StoragePrecision, typename ComputePrecision>
  struct JudgeCandidates;

  template <typename Derived, typename ComputePrecision>
  struct SpreadStatuses;

  struct SubcellSchemeCounts;

  struct SubcellSchemeCountsReduction;

  INLINE_METHOD GPU_CALLABLE_METHOD int getFirstCellOfBlock(int block, int numberOfBlocks, int numberOfCells);

  struct CountCellsOnSubcellScheme;

  struct ListCellsOnSubcellScheme;

  struct InterpolateStatusHalves;

  struct RestrictStatusHalves;

  template <typename Derived, typename StoragePrecision, typename ComputePrecision>
  struct InterpolateExtremaHalves;

  template <typename Derived, typename StoragePrecision, typename ComputePrecision>
  struct RestrictExtremaHalves;

  struct StatusTransfer;
} // namespace exahype2::dg::limiting

namespace exahype2::dg::limiting::tests {
  class DetectorTest;
} // namespace exahype2::dg::limiting::tests

TARGET_DECLARE
/**
 * The troubled-cell detector of one cell of Derived. The extrema of a cell are the minima of its
 * NumberOfObservables observables followed by their maxima.
 */
template <typename Derived, typename ComputePrecision>
class exahype2::dg::limiting::Detector final {
public:
  using Scheme = exahype2::dg::limiting::SubcellScheme<Derived, ComputePrecision>;

  static constexpr int  NumberOfObservables  = Derived::NumberOfObservables;
  static constexpr bool UsesMaximumPrinciple = NumberOfObservables > 0;
  static constexpr int  ExtremaPerCell       = UsesMaximumPrinciple ? 2 * NumberOfObservables : 2;
  static constexpr int  ExtremaValuesPerFace = 2 * ExtremaPerCell;
  static constexpr int  StatusValuesPerFace  = 2;

  template <typename StoragePrecision>
  static INLINE_METHOD GPU_CALLABLE_METHOD void openInitialStatus(
    int* NOALIAS                    status,
    const StoragePrecision* NOALIAS nodes,
    const StoragePrecision* NOALIAS subcells,
    const StoragePrecision* NOALIAS nodesToLobatto,
    const StoragePrecision* NOALIAS nodesToSubcells,
    StoragePrecision                relaxationParameter,
    StoragePrecision                differencesScaling
  );

  template <typename StoragePrecision>
  static INLINE_METHOD GPU_CALLABLE_METHOD void openStep(
    int* NOALIAS                    status,
    const StoragePrecision* NOALIAS solution,
    StoragePrecision* NOALIAS       previousSolution
  );

  template <typename StoragePrecision>
  static INLINE_METHOD GPU_CALLABLE_METHOD void reduceExtrema(
    StoragePrecision* NOALIAS       extrema,
    const StoragePrecision* NOALIAS nodes,
    const StoragePrecision* NOALIAS nodesToLobatto,
    const StoragePrecision* NOALIAS nodesToSubcells
  );

  template <typename StoragePrecision>
  static INLINE_METHOD GPU_CALLABLE_METHOD void clearExtrema(StoragePrecision* NOALIAS extrema);

  template <typename StoragePrecision>
  static INLINE_METHOD GPU_CALLABLE_METHOD void widenExtrema(
    StoragePrecision* NOALIAS       extrema,
    const StoragePrecision* NOALIAS other
  );

  template <typename StoragePrecision>
  static INLINE_METHOD GPU_CALLABLE_METHOD exahype2::dg::MaxEigenvalueStatistics judgeCandidate(
    int* NOALIAS                                 status,
    int* const                                   ownStatusHalves[peano4::FacesPerCell],
    const StoragePrecision* NOALIAS              candidate,
    const StoragePrecision* NOALIAS              referenceExtrema,
    const StoragePrecision* NOALIAS              nodesToLobatto,
    const StoragePrecision* NOALIAS              nodesToSubcells,
    const StoragePrecision* NOALIAS              quadraturePoints,
    StoragePrecision                             relaxationParameter,
    StoragePrecision                             differencesScaling,
    const tarch::la::Vector<DIMENSIONS, double>& cellCentre,
    const tarch::la::Vector<DIMENSIONS, double>& cellSize,
    double                                       timeStamp,
    double                                       timeStepSize
  );

  static INLINE_METHOD GPU_CALLABLE_METHOD void spreadStatus(
    int* NOALIAS     status,
    const int* const neighbourStatusHalves[peano4::FacesPerCell],
    int* const       ownStatusHalves[peano4::FacesPerCell]
  );

private:
  friend class exahype2::dg::limiting::tests::DetectorTest;

  template <typename StoragePrecision>
  [[nodiscard]] static INLINE_METHOD GPU_CALLABLE_METHOD bool interpolantOscillates(
    const StoragePrecision* NOALIAS nodes,
    const StoragePrecision* NOALIAS subcells,
    const StoragePrecision* NOALIAS nodesToLobatto,
    const StoragePrecision* NOALIAS nodesToSubcells,
    StoragePrecision                relaxationParameter,
    StoragePrecision                differencesScaling
  );

  template <typename StoragePrecision>
  static INLINE_METHOD GPU_CALLABLE_METHOD void reduceObservableExtrema(
    StoragePrecision* NOALIAS       extrema,
    const StoragePrecision* NOALIAS states,
    int                             numberOfStates
  );

  template <int SamplesPerAxis, typename StoragePrecision>
  static INLINE_METHOD GPU_CALLABLE_METHOD void reduceProjectedObservableExtrema(
    StoragePrecision* NOALIAS       extrema,
    const StoragePrecision* NOALIAS nodes,
    const StoragePrecision* NOALIAS samplingMatrix
  );

  template <typename StoragePrecision>
  [[nodiscard]] static INLINE_METHOD GPU_CALLABLE_METHOD StoragePrecision mergeMinimum(
    StoragePrecision left,
    StoragePrecision right
  );

  template <typename StoragePrecision>
  [[nodiscard]] static INLINE_METHOD GPU_CALLABLE_METHOD StoragePrecision mergeMaximum(
    StoragePrecision left,
    StoragePrecision right
  );

  template <typename StoragePrecision>
  [[nodiscard]] static INLINE_METHOD GPU_CALLABLE_METHOD bool isPhysicallyAdmissible(
    const StoragePrecision* NOALIAS              nodes,
    const StoragePrecision* NOALIAS              subcells,
    const tarch::la::Vector<DIMENSIONS, double>& cellCentre,
    const tarch::la::Vector<DIMENSIONS, double>& cellSize
  );

  template <typename StoragePrecision>
  [[nodiscard]] static INLINE_METHOD GPU_CALLABLE_METHOD bool satisfiesMaximumPrinciple(
    const StoragePrecision* NOALIAS candidate,
    const StoragePrecision* NOALIAS referenceExtrema,
    const StoragePrecision* NOALIAS nodesToLobatto,
    const StoragePrecision* NOALIAS nodesToSubcells,
    StoragePrecision                relaxationParameter,
    StoragePrecision                differencesScaling
  );

  template <typename StoragePrecision>
  [[nodiscard]] static INLINE_METHOD GPU_CALLABLE_METHOD bool satisfiesRelaxedMaximumPrinciple(
    const StoragePrecision* NOALIAS candidateExtrema,
    const StoragePrecision* NOALIAS referenceExtrema,
    StoragePrecision                relaxationParameter,
    StoragePrecision                differencesScaling
  );

  template <typename StoragePrecision>
  [[nodiscard]] static INLINE_METHOD GPU_CALLABLE_METHOD StoragePrecision getRelaxationParameterOfObservable(
    int              observable,
    StoragePrecision relaxationParameter
  );

  template <typename StoragePrecision>
  [[nodiscard]] static INLINE_METHOD GPU_CALLABLE_METHOD bool satisfiesDiscreteMaximumPrinciple(
    StoragePrecision candidateMinimum,
    StoragePrecision candidateMaximum,
    StoragePrecision referenceMinimum,
    StoragePrecision referenceMaximum,
    StoragePrecision relaxationParameter,
    StoragePrecision differencesScaling
  );

  template <typename StoragePrecision>
  [[nodiscard]] static INLINE_METHOD GPU_CALLABLE_METHOD StoragePrecision quantiseObservableExtremum(
    StoragePrecision extremum
  );

  template <typename StoragePrecision>
  [[nodiscard]] static INLINE_METHOD GPU_CALLABLE_METHOD exahype2::dg::MaxEigenvalueStatistics candidateEigenvalues(
    const StoragePrecision* NOALIAS              candidate,
    const StoragePrecision* NOALIAS              candidateSubcells,
    const StoragePrecision* NOALIAS              quadraturePoints,
    const tarch::la::Vector<DIMENSIONS, double>& cellCentre,
    const tarch::la::Vector<DIMENSIONS, double>& cellSize,
    double                                       timeStamp,
    double                                       timeStepSize
  );
};

/**
 * Opens every cell on the Galerkin scheme. With a discrete maximum principle, a cell whose nodal
 * interpolant oscillates beyond its subcells starts on the subcell scheme.
 */
template <typename Derived, typename StoragePrecision, typename ComputePrecision>
struct exahype2::dg::limiting::OpenInitialStatuses final {
  using Kernel     = exahype2::dg::limiting::Detector<Derived, ComputePrecision>;
  using CellPool   = tarch::accelerator::ChunkedMemoryPool<StoragePrecision>;
  using StatusPool = tarch::accelerator::ChunkedMemoryPool<int>;

  peano4::Partition            partition{};
  typename CellPool::ConstView nodes{};
  typename CellPool::ConstView subcells{};
  typename StatusPool::View    statuses{};
  const StoragePrecision*      nodesToLobatto{nullptr};
  const StoragePrecision*      nodesToSubcells{nullptr};
  StoragePrecision             relaxationParameter{0};
  StoragePrecision             differencesScaling{0};

  INLINE_METHOD GPU_CALLABLE_METHOD void operator()(int entry) const;
};

/**
 * Resets both halves of every face to the status Regular and empty extrema, so the outer half of a
 * boundary face adds nothing. The subcell halo of the face becomes zero.
 */
template <typename Derived, typename StoragePrecision, typename ComputePrecision>
struct exahype2::dg::limiting::ClearFaces final {
  using Kernel     = exahype2::dg::limiting::Detector<Derived, ComputePrecision>;
  using CellPool   = tarch::accelerator::ChunkedMemoryPool<StoragePrecision>;
  using StatusPool = tarch::accelerator::ChunkedMemoryPool<int>;

  typename StatusPool::View faceStatuses{};
  typename CellPool::View   extremaFaces{};
  typename CellPool::View   subcellHalo{};

  INLINE_METHOD GPU_CALLABLE_METHOD void operator()(int face) const;
};

/**
 * Copies the solution of every cell into its previous solution and opens its status for the step.
 */
template <typename Derived, typename StoragePrecision, typename ComputePrecision>
struct exahype2::dg::limiting::OpenStep final {
  using Kernel     = exahype2::dg::limiting::Detector<Derived, ComputePrecision>;
  using CellPool   = tarch::accelerator::ChunkedMemoryPool<StoragePrecision>;
  using StatusPool = tarch::accelerator::ChunkedMemoryPool<int>;

  peano4::Partition            partition{};
  typename CellPool::ConstView solution{};
  typename CellPool::View      previousSolution{};
  typename StatusPool::View    statuses{};

  INLINE_METHOD GPU_CALLABLE_METHOD void operator()(int entry) const;
};

/**
 * Writes the observable extrema of the previous solution of every cell into its reference extrema.
 */
template <typename Derived, typename StoragePrecision, typename ComputePrecision>
struct exahype2::dg::limiting::ReduceReferenceExtrema final {
  using Kernel   = exahype2::dg::limiting::Detector<Derived, ComputePrecision>;
  using CellPool = tarch::accelerator::ChunkedMemoryPool<StoragePrecision>;

  peano4::Partition            partition{};
  typename CellPool::ConstView previousSolution{};
  typename CellPool::View      referenceExtrema{};
  const StoragePrecision*      nodesToLobatto{nullptr};
  const StoragePrecision*      nodesToSubcells{nullptr};

  INLINE_METHOD GPU_CALLABLE_METHOD void operator()(int entry) const;
};

/**
 * Copies the reference extrema of every cell into its own halves of its two faces along axis.
 */
template <typename Derived, typename StoragePrecision, typename ComputePrecision>
struct exahype2::dg::limiting::WriteExtremaOntoFaces final {
  using Kernel   = exahype2::dg::limiting::Detector<Derived, ComputePrecision>;
  using CellPool = tarch::accelerator::ChunkedMemoryPool<StoragePrecision>;

  peano4::Partition            partition{};
  typename CellPool::ConstView referenceExtrema{};
  typename CellPool::View      extremaFaces{};
  int                          axis{0};

  INLINE_METHOD GPU_CALLABLE_METHOD void operator()(int entry) const;
};

/**
 * Widens the reference extrema of every listed cell by the extrema its two face neighbours along axis wrote.
 */
template <typename Derived, typename StoragePrecision, typename ComputePrecision>
struct exahype2::dg::limiting::MergeNeighbourExtrema final {
  using Kernel   = exahype2::dg::limiting::Detector<Derived, ComputePrecision>;
  using CellPool = tarch::accelerator::ChunkedMemoryPool<StoragePrecision>;

  peano4::Partition            partition{};
  tarch::Span<const int>       cells{};
  typename CellPool::View      referenceExtrema{};
  typename CellPool::ConstView extremaFaces{};
  int                          axis{0};

  INLINE_METHOD GPU_CALLABLE_METHOD void operator()(int entry) const;
};

/**
 * Checks the candidate of every cell and writes the resulting status into the cell and onto its
 * faces. The wave speeds of the accepted candidates whose cells were not troubled reduce into the
 * accumulator.
 */
template <typename Derived, typename StoragePrecision, typename ComputePrecision>
struct exahype2::dg::limiting::JudgeCandidates final {
  using Kernel     = exahype2::dg::limiting::Detector<Derived, ComputePrecision>;
  using CellPool   = tarch::accelerator::ChunkedMemoryPool<StoragePrecision>;
  using StatusPool = tarch::accelerator::ChunkedMemoryPool<int>;

  peano4::Partition            partition{};
  typename CellPool::ConstView candidates{};
  typename StatusPool::View    statuses{};
  typename StatusPool::View    faceStatuses{};
  typename CellPool::ConstView referenceExtrema{};
  const StoragePrecision*      nodesToLobatto{nullptr};
  const StoragePrecision*      nodesToSubcells{nullptr};
  const StoragePrecision*      quadraturePoints{nullptr};
  StoragePrecision             relaxationParameter{0};
  StoragePrecision             differencesScaling{0};
  double                       timeStamp{0.0};
  double                       timeStepSize{0.0};

  INLINE_METHOD GPU_CALLABLE_METHOD void operator()(
    int                                    entry,
    exahype2::dg::MaxEigenvalueStatistics& accumulator
  ) const;
};

/**
 * Raises the status of every listed cell by the statuses of its face neighbours and writes the
 * result onto its faces.
 */
template <typename Derived, typename ComputePrecision>
struct exahype2::dg::limiting::SpreadStatuses final {
  using Kernel     = exahype2::dg::limiting::Detector<Derived, ComputePrecision>;
  using StatusPool = tarch::accelerator::ChunkedMemoryPool<int>;

  peano4::Partition         partition{};
  tarch::Span<const int>    cells{};
  typename StatusPool::View statuses{};
  typename StatusPool::View faceStatuses{};

  INLINE_METHOD GPU_CALLABLE_METHOD void operator()(int entry) const;
};

/**
 * The numbers of cells whose status is not Regular, of recomputed cells, of recomputed local cells
 * and of troubled cells.
 */
struct exahype2::dg::limiting::SubcellSchemeCounts final {
  int cellsOnScheme{0};
  int recomputedCells{0};
  int localRecomputedCells{0};
  int troubledCells{0};
};

/**
 * Adds the cell counts of two work items.
 */
struct exahype2::dg::limiting::SubcellSchemeCountsReduction final {
  using ValueType = exahype2::dg::limiting::SubcellSchemeCounts;

  static INLINE_METHOD GPU_CALLABLE_METHOD ValueType identity();
  static INLINE_METHOD GPU_CALLABLE_METHOD ValueType combine(ValueType left, ValueType right);
};

/**
 * Writes the counts of one block of consecutive cells into countsOfBlocks and adds them to counts.
 * The first numberOfLocalCells cells of the partition are the local ones.
 */
struct exahype2::dg::limiting::CountCellsOnSubcellScheme final {
  using StatusPool = tarch::accelerator::ChunkedMemoryPool<int>;

  peano4::Partition                            partition{};
  StatusPool::ConstView                        statuses{};
  int                                          numberOfBlocks{0};
  int                                          numberOfLocalCells{0};
  exahype2::dg::limiting::SubcellSchemeCounts* countsOfBlocks{nullptr};

  INLINE_METHOD GPU_CALLABLE_METHOD void operator()(
    int                                          block,
    exahype2::dg::limiting::SubcellSchemeCounts& counts
  ) const;
};

/**
 * Writes the slots of the cells of one block that are not Regular into cellsOnScheme and of those
 * that recompute into recomputedCells, behind the entries of the earlier blocks. Both lists keep
 * the order of the partition, the local cells first.
 */
struct exahype2::dg::limiting::ListCellsOnSubcellScheme final {
  using StatusPool = tarch::accelerator::ChunkedMemoryPool<int>;

  peano4::Partition                                  partition{};
  StatusPool::ConstView                              statuses{};
  int                                                numberOfBlocks{0};
  const exahype2::dg::limiting::SubcellSchemeCounts* countsOfBlocks{nullptr};
  int*                                               cellsOnScheme{nullptr};
  int*                                               recomputedCells{nullptr};

  INLINE_METHOD GPU_CALLABLE_METHOD void operator()(int block) const;
};

/**
 * Copies the coarse-side status half of the coarse face of a level jump onto its fine faces.
 */
struct exahype2::dg::limiting::InterpolateStatusHalves final {
  using StatusPool = tarch::accelerator::ChunkedMemoryPool<int>;

  StatusPool::View faceStatuses{};

  INLINE_METHOD GPU_CALLABLE_METHOD void operator()(const peano4::LevelJump& levelJump) const;
};

/**
 * Writes the most limited fine-side status of the fine faces of a level jump onto its coarse face.
 */
struct exahype2::dg::limiting::RestrictStatusHalves final {
  using StatusPool = tarch::accelerator::ChunkedMemoryPool<int>;

  StatusPool::View faceStatuses{};

  INLINE_METHOD GPU_CALLABLE_METHOD void operator()(const peano4::LevelJump& levelJump) const;
};

/**
 * Copies the coarse-side extrema half of the coarse face of a level jump onto its fine faces.
 */
template <typename Derived, typename StoragePrecision, typename ComputePrecision>
struct exahype2::dg::limiting::InterpolateExtremaHalves final {
  using Kernel   = exahype2::dg::limiting::Detector<Derived, ComputePrecision>;
  using CellPool = tarch::accelerator::ChunkedMemoryPool<StoragePrecision>;

  typename CellPool::View extremaFaces{};

  INLINE_METHOD GPU_CALLABLE_METHOD void operator()(const peano4::LevelJump& levelJump) const;
};

/**
 * Writes the union of the fine-side extrema of the fine faces of a level jump onto its coarse face.
 */
template <typename Derived, typename StoragePrecision, typename ComputePrecision>
struct exahype2::dg::limiting::RestrictExtremaHalves final {
  using Kernel   = exahype2::dg::limiting::Detector<Derived, ComputePrecision>;
  using CellPool = tarch::accelerator::ChunkedMemoryPool<StoragePrecision>;

  typename CellPool::View extremaFaces{};

  INLINE_METHOD GPU_CALLABLE_METHOD void operator()(const peano4::LevelJump& levelJump) const;
};

/**
 * Fills the status of a new cell of a regrid. A child copies the status of its parent, and a parent
 * takes the entry-wise maximum of its children.
 */
struct exahype2::dg::limiting::StatusTransfer final {
  using StatusPool = tarch::accelerator::ChunkedMemoryPool<int>;

  StatusPool::ConstView statuses{};

  INLINE_METHOD GPU_CALLABLE_METHOD void copy(int slot, int* target) const;
  INLINE_METHOD GPU_CALLABLE_METHOD void interpolateFromParent(int parentSlot, int* target, int childPosition) const;
  INLINE_METHOD GPU_CALLABLE_METHOD void restrictFromChildren(const int* childSlots, int* target) const;
};
TARGET_DECLARE_END

#include "exahype2/dg/limiting/Detector.cpph"
