// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "exahype2/ErrorMeasurement.h"
#include "exahype2/dg/Basis.h"
#include "peano4/Constants.h"
#include "peano4/Partition.h"
#include "tarch/CompilerSpecificSettings.h"
#include "tarch/Span.h"
#include "tarch/accelerator/ChunkedMemoryPool.h"
#include "tarch/accelerator/CompileTarget.h"
#include "tarch/la/Vector.h"

/**
 * @file
 * The kernels of the discontinuous Galerkin solver families, where Derived is the generated
 * solver class. A face stores one state half and one flux half per side, the lower side first,
 * and the cell on each side writes its own half. The Riemann solver overwrites both flux halves
 * with the numerical flux minus the half's own flux, times the sign of the side's outward normal.
 */
TARGET_DECLARE
namespace exahype2::dg {
  struct MaxEigenvalueStatistics;

  [[nodiscard]] INLINE_METHOD GPU_CALLABLE_METHOD int getOwnHalf(int faceNumber);

  [[nodiscard]] INLINE_METHOD GPU_CALLABLE_METHOD int getCorrectionHalf(
    const peano4::Partition& partition,
    int                      face,
    int                      faceNumber
  );

  [[nodiscard]] INLINE_METHOD GPU_CALLABLE_METHOD int getBoundaryFaceNumber(
    const peano4::Partition& partition,
    int                      face
  );

  template <typename Derived, typename StoragePrecision, typename QuadraturePrecision>
  INLINE_METHOD void initialiseNodes(
    const tarch::la::Vector<DIMENSIONS, double>& centre,
    const tarch::la::Vector<DIMENSIONS, double>& width,
    const QuadraturePrecision*                   quadraturePoints,
    const QuadraturePrecision*                   quadratureWeights,
    StoragePrecision*                            nodes
  );

  template <typename Derived, typename CellPrecision, typename FacePrecision>
  INLINE_METHOD GPU_CALLABLE_METHOD void extrapolateStateToFaces(
    FacePrecision* const NOALIAS faceStates[peano4::FacesPerCell],
    const CellPrecision* NOALIAS cellState,
    const FacePrecision* NOALIAS leftFaceCoefficients,
    const FacePrecision* NOALIAS rightFaceCoefficients
  );

  template <typename Derived, typename ComputePrecision>
  INLINE_METHOD GPU_CALLABLE_METHOD void extrapolateFluxToFaces(
    ComputePrecision* const NOALIAS              faceFluxes[peano4::FacesPerCell],
    const ComputePrecision* NOALIAS              cellState,
    const ComputePrecision* NOALIAS              leftFaceCoefficients,
    const ComputePrecision* NOALIAS              rightFaceCoefficients,
    const ComputePrecision* NOALIAS              quadratureNodes,
    const ComputePrecision* NOALIAS              derivativeOperator,
    const tarch::la::Vector<DIMENSIONS, double>& cellCentre,
    const tarch::la::Vector<DIMENSIONS, double>& cellSize,
    double                                       timeStamp,
    double                                       timeStepSize
  );

  template <typename Derived, typename StoragePrecision, typename ComputePrecision>
  struct ProjectSolutionOntoFaces;

  template <typename Derived, typename StoragePrecision, typename ComputePrecision>
  struct ProjectSolutionOntoPlane;

  template <typename Derived, typename ComputePrecision>
  struct InterpolateFaceStateHalves;

  template <typename Derived, typename ComputePrecision>
  struct InterpolateFaceHalves;

  template <typename Derived, typename ComputePrecision>
  INLINE_METHOD GPU_CALLABLE_METHOD void applyBoundaryConditions(
    ComputePrecision* NOALIAS                    faceState,
    const ComputePrecision* NOALIAS              quadratureNodes,
    const tarch::la::Vector<DIMENSIONS, double>& faceCentre,
    const tarch::la::Vector<DIMENSIONS, double>& cellSize,
    double                                       timeStamp,
    int                                          faceNumber
  );

  template <typename Derived, typename ComputePrecision>
  INLINE_METHOD GPU_CALLABLE_METHOD void evaluateFluxOnFace(
    ComputePrecision* NOALIAS                    faceFlux,
    const ComputePrecision* NOALIAS              leftState,
    const ComputePrecision* NOALIAS              rightState,
    int                                          side,
    const ComputePrecision* NOALIAS              quadratureNodes,
    const tarch::la::Vector<DIMENSIONS, double>& faceCentre,
    const tarch::la::Vector<DIMENSIONS, double>& cellSize,
    double                                       timeStamp,
    double                                       timeStepSize,
    int                                          direction
  );

  template <typename Derived, typename ComputePrecision>
  INLINE_METHOD GPU_CALLABLE_METHOD void solveRiemannProblem(
    ComputePrecision* NOALIAS                    leftFlux,
    ComputePrecision* NOALIAS                    rightFlux,
    const ComputePrecision* NOALIAS              leftState,
    const ComputePrecision* NOALIAS              rightState,
    const ComputePrecision* NOALIAS              faceQuadratureWeights,
    const tarch::la::Vector<DIMENSIONS, double>& faceCentre,
    const tarch::la::Vector<DIMENSIONS, double>& cellSize,
    double                                       timeStamp,
    double                                       timeStepSize,
    int                                          direction
  );

  template <typename Derived, typename ComputePrecision>
  INLINE_METHOD GPU_CALLABLE_METHOD void solveLinearRiemannProblem(
    ComputePrecision* NOALIAS                    leftFlux,
    ComputePrecision* NOALIAS                    rightFlux,
    const ComputePrecision* NOALIAS              leftState,
    const ComputePrecision* NOALIAS              rightState,
    const ComputePrecision* NOALIAS              faceQuadratureWeights,
    const tarch::la::Vector<DIMENSIONS, double>& faceCentre,
    const tarch::la::Vector<DIMENSIONS, double>& cellSize,
    double                                       timeStamp,
    double                                       timeStepSize,
    int                                          direction
  );

  template <typename Derived, typename ComputePrecision>
  INLINE_METHOD GPU_CALLABLE_METHOD void solveRiemannProblemOnHalves(
    ComputePrecision* NOALIAS                    leftFlux,
    ComputePrecision* NOALIAS                    rightFlux,
    const ComputePrecision* NOALIAS              leftState,
    const ComputePrecision* NOALIAS              rightState,
    const ComputePrecision* NOALIAS              faceQuadratureWeights,
    const tarch::la::Vector<DIMENSIONS, double>& faceCentre,
    const tarch::la::Vector<DIMENSIONS, double>& cellSize,
    double                                       timeStamp,
    double                                       timeStepSize,
    int                                          direction,
    bool                                         isBoundary,
    int                                          faceNumber
  );

  template <typename Derived, typename ComputePrecision>
  INLINE_METHOD GPU_CALLABLE_METHOD void solveRiemannProblemOnFace(
    ComputePrecision* NOALIAS                    faceState,
    ComputePrecision* NOALIAS                    faceFlux,
    const ComputePrecision* NOALIAS              quadraturePoints,
    const ComputePrecision* NOALIAS              faceQuadratureWeights,
    const tarch::la::Vector<DIMENSIONS, double>& faceCentre,
    const tarch::la::Vector<DIMENSIONS, double>& cellSize,
    double                                       timeStamp,
    double                                       timeStepSize,
    int                                          direction,
    int                                          boundaryFaceNumber
  );

  template <typename Derived, typename ComputePrecision>
  struct ApplyBoundaryConditions;

  template <typename Derived, typename ComputePrecision>
  struct SolveRiemannProblems;

  template <typename Derived, typename ComputePrecision>
  struct RestrictFluxHalves;

  template <typename Derived, typename ComputePrecision>
  struct RestrictStateHalves;

  template <typename Operation>
  struct ForLevelJumpOnPlane;

  template <typename Derived, typename StoragePrecision, typename ComputePrecision>
  INLINE_METHOD GPU_CALLABLE_METHOD void addFaceIntegral(
    StoragePrecision* NOALIAS       nodeUpdate,
    int                             cellNode,
    const ComputePrecision* NOALIAS faceFlux,
    const ComputePrecision* NOALIAS faceQuadratureWeights,
    const ComputePrecision* NOALIAS inverseCellQuadratureWeights,
    const ComputePrecision* NOALIAS leftFaceCoefficients,
    const ComputePrecision* NOALIAS rightFaceCoefficients,
    int                             direction,
    int                             orientation,
    double                          inverseCellSizeAlongNormal,
    double                          timeStepSize
  );

  template <typename Derived, int NumberOfValuesPerNode, typename StoragePrecision, typename ComputePrecision>
  INLINE_METHOD GPU_CALLABLE_METHOD void addFaceIntegrals(
    StoragePrecision* NOALIAS                    cellValues,
    const ComputePrecision* const                faceFluxes[peano4::FacesPerCell],
    const ComputePrecision* NOALIAS              faceQuadratureWeights,
    const ComputePrecision* NOALIAS              inverseCellQuadratureWeights,
    const ComputePrecision* NOALIAS              leftFaceCoefficients,
    const ComputePrecision* NOALIAS              rightFaceCoefficients,
    const tarch::la::Vector<DIMENSIONS, double>& cellSize,
    double                                       timeStepSize
  );

  template <typename Derived, typename Precision>
  INLINE_METHOD GPU_CALLABLE_METHOD MaxEigenvalueStatistics reduceMaxEigenvalues(
    const Precision* NOALIAS                     cellState,
    const Precision* NOALIAS                     quadratureNodes,
    const tarch::la::Vector<DIMENSIONS, double>& cellCentre,
    const tarch::la::Vector<DIMENSIONS, double>& cellSize,
    double                                       timeStamp,
    double                                       timeStepSize
  );

  template <typename Derived, bool ReportEigenvalues, typename StoragePrecision>
  [[nodiscard]] INLINE_METHOD GPU_CALLABLE_METHOD MaxEigenvalueStatistics finishCell(
    StoragePrecision*                            updated,
    const StoragePrecision*                      quadraturePoints,
    const tarch::la::Vector<DIMENSIONS, double>& centre,
    const tarch::la::Vector<DIMENSIONS, double>& width,
    double                                       timeStamp,
    double                                       timeStepSize
  );

  template <typename Derived, typename StoragePrecision>
  struct MeasureNodeEigenvalues;

  template <typename Derived, typename StoragePrecision>
  struct MeasureNodeErrors;

  template <typename Derived, typename StoragePrecision>
  struct RegridCells;

  template <typename Derived, typename PartnerDerived, typename PartnerPrecision, typename ComputePrecision>
  struct CopyPartnerFaceHalves;
} // namespace exahype2::dg

/**
 * The largest wave speed and the largest sum over the axes of the wave speed divided by the cell
 * extent. combine() keeps a wave speed that is negative or not finite.
 */
struct exahype2::dg::MaxEigenvalueStatistics final {
  using ValueType = exahype2::dg::MaxEigenvalueStatistics;

  double maxPhysicalEigenvalue{0.0};
  double maxScaledEigenvalue{0.0};

  [[nodiscard]] static INLINE_METHOD GPU_CALLABLE_METHOD ValueType identity();
  [[nodiscard]] static INLINE_METHOD GPU_CALLABLE_METHOD ValueType combine(ValueType left, ValueType right);

  template <int Dimensions, typename EigenvalueEvaluator>
  INLINE_METHOD GPU_CALLABLE_METHOD void accumulate(
    const tarch::la::Vector<Dimensions, double>& cellSize,
    EigenvalueEvaluator&&                        evaluateEigenvalue
  );
};

/**
 * Writes the solution of every listed cell into the own state half of each of its faces.
 */
template <typename Derived, typename StoragePrecision, typename ComputePrecision>
struct exahype2::dg::ProjectSolutionOntoFaces final {
  static constexpr int NodesPerAxis = Derived::DGOrder + 1;
  static constexpr int NumberOfData = Derived::NumberOfUnknowns + Derived::NumberOfAuxiliaryVariables;

  using CellPool  = tarch::accelerator::ChunkedMemoryPool<StoragePrecision>;
  using StatePool = tarch::accelerator::ChunkedMemoryPool<ComputePrecision>;

  peano4::Partition            partition{};
  tarch::Span<const int>       cells{};
  typename CellPool::ConstView solution{};
  typename StatePool::View     faceStates{};
  const ComputePrecision*      leftFaceCoefficients{nullptr};
  const ComputePrecision*      rightFaceCoefficients{nullptr};

  INLINE_METHOD GPU_CALLABLE_METHOD void operator()(int entry) const;
};

/**
 * Writes the solution of every listed cell that touches the plane at position into the own state
 * halves of its two faces along normal.
 */
template <typename Derived, typename StoragePrecision, typename ComputePrecision>
struct exahype2::dg::ProjectSolutionOntoPlane final {
  static constexpr int NodesPerAxis = Derived::DGOrder + 1;
  static constexpr int NumberOfData = Derived::NumberOfUnknowns + Derived::NumberOfAuxiliaryVariables;

  using CellPool  = tarch::accelerator::ChunkedMemoryPool<StoragePrecision>;
  using StatePool = tarch::accelerator::ChunkedMemoryPool<ComputePrecision>;

  peano4::Partition            partition{};
  tarch::Span<const int>       cells{};
  typename CellPool::ConstView solution{};
  typename StatePool::View     faceStates{};
  const ComputePrecision*      leftFaceCoefficients{nullptr};
  const ComputePrecision*      rightFaceCoefficients{nullptr};
  int                          normal{0};
  double                       position{0.0};

  INLINE_METHOD GPU_CALLABLE_METHOD void operator()(int entry) const;
};

/**
 * Interpolates the coarse-side state half of the coarse face into the same half of every fine
 * face of a level jump.
 */
template <typename Derived, typename ComputePrecision>
struct exahype2::dg::InterpolateFaceStateHalves final {
  static constexpr int NodesPerAxis = Derived::DGOrder + 1;
  static constexpr int NumberOfData = Derived::NumberOfUnknowns + Derived::NumberOfAuxiliaryVariables;

  using StatePool = tarch::accelerator::ChunkedMemoryPool<ComputePrecision>;

  typename StatePool::View faceStates{};
  const double*            prolongationMatrix{nullptr};

  INLINE_METHOD GPU_CALLABLE_METHOD void operator()(const peano4::LevelJump& levelJump) const;
};

/**
 * Interpolates the coarse-side state and flux halves of the coarse face into the same halves of
 * every fine face of a level jump.
 */
template <typename Derived, typename ComputePrecision>
struct exahype2::dg::InterpolateFaceHalves final {
  static constexpr int NodesPerAxis     = Derived::DGOrder + 1;
  static constexpr int NumberOfUnknowns = Derived::NumberOfUnknowns;
  static constexpr int NumberOfData     = NumberOfUnknowns + Derived::NumberOfAuxiliaryVariables;

  using StatePool = tarch::accelerator::ChunkedMemoryPool<ComputePrecision>;
  using FluxPool  = tarch::accelerator::ChunkedMemoryPool<ComputePrecision>;

  typename StatePool::View faceStates{};
  typename FluxPool::View  faceFluxes{};
  const double*            prolongationMatrix{nullptr};

  INLINE_METHOD GPU_CALLABLE_METHOD void operator()(const peano4::LevelJump& levelJump) const;
};

/**
 * Writes the boundary state of the inner half of every listed boundary face at timeStamp into
 * its outer half. Every other listed face stays as it is.
 */
template <typename Derived, typename ComputePrecision>
struct exahype2::dg::ApplyBoundaryConditions final {
  using StatePool = tarch::accelerator::ChunkedMemoryPool<ComputePrecision>;

  peano4::Partition        partition{};
  tarch::Span<const int>   faces{};
  typename StatePool::View faceStates{};
  const ComputePrecision*  quadraturePoints{nullptr};
  double                   timeStamp{0.0};

  INLINE_METHOD GPU_CALLABLE_METHOD void operator()(int entry) const;
};

/**
 * Solves the Riemann problem of every listed face. On a boundary face, the boundary conditions
 * first overwrite the outer state half. With appliedFluxes, every half of a face there also takes
 * the numerical flux that the cell on its side applies, along the positive normal.
 */
template <typename Derived, typename ComputePrecision>
struct exahype2::dg::SolveRiemannProblems final {
  using StatePool = tarch::accelerator::ChunkedMemoryPool<ComputePrecision>;
  using FluxPool  = tarch::accelerator::ChunkedMemoryPool<ComputePrecision>;

  peano4::Partition        partition{};
  tarch::Span<const int>   faces{};
  typename StatePool::View faceStates{};
  typename FluxPool::View  faceFluxes{};
  const ComputePrecision*  quadraturePoints{nullptr};
  const ComputePrecision*  faceQuadratureWeights{nullptr};
  double                   timeStamp{0.0};
  double                   timeStepSize{0.0};
  typename FluxPool::View  appliedFluxes{};

  INLINE_METHOD GPU_CALLABLE_METHOD void operator()(int entry) const;
};

/**
 * Restricts the fluxes of the fine faces of a level jump into the fine-side flux half of the
 * coarse face. A hanging fine face contributes its coarse-side half, every other fine face the
 * restricted flux in its fine-side half.
 */
template <typename Derived, typename ComputePrecision>
struct exahype2::dg::RestrictFluxHalves final {
  static constexpr int NodesPerAxis     = Derived::DGOrder + 1;
  static constexpr int NumberOfUnknowns = Derived::NumberOfUnknowns;

  using FluxPool = tarch::accelerator::ChunkedMemoryPool<ComputePrecision>;

  peano4::Partition       partition{};
  typename FluxPool::View faceFluxes{};
  const double*           restrictionMatrix{nullptr};

  INLINE_METHOD GPU_CALLABLE_METHOD void operator()(const peano4::LevelJump& levelJump) const;
};

/**
 * Restricts the fine-side state halves of the fine faces of a level jump into the fine-side half
 * of the coarse face.
 */
template <typename Derived, typename ComputePrecision>
struct exahype2::dg::RestrictStateHalves final {
  static constexpr int NodesPerAxis = Derived::DGOrder + 1;
  static constexpr int NumberOfData = Derived::NumberOfUnknowns + Derived::NumberOfAuxiliaryVariables;

  using StatePool = tarch::accelerator::ChunkedMemoryPool<ComputePrecision>;

  typename StatePool::View faceStates{};
  const double*            restrictionMatrix{nullptr};

  INLINE_METHOD GPU_CALLABLE_METHOD void operator()(const peano4::LevelJump& levelJump) const;
};

/**
 * Calls operation for a level jump whose coarse face lies on the plane at position along normal.
 */
template <typename Operation>
struct exahype2::dg::ForLevelJumpOnPlane final {
  peano4::Partition partition{};
  int               normal{0};
  double            position{0.0};
  Operation         operation{};

  INLINE_METHOD GPU_CALLABLE_METHOD void operator()(const peano4::LevelJump& levelJump) const;
};

template <typename Derived, typename StoragePrecision>
struct exahype2::dg::MeasureNodeEigenvalues final {
  using CellPool = tarch::accelerator::ChunkedMemoryPool<StoragePrecision>;

  peano4::Partition            partition{};
  tarch::Span<const int>       cells{};
  typename CellPool::ConstView solution{};
  const StoragePrecision*      quadraturePoints{nullptr};
  double                       timeStamp{0.0};

  INLINE_METHOD GPU_CALLABLE_METHOD void operator()(
    int                                    entry,
    exahype2::dg::MaxEigenvalueStatistics& accumulator
  ) const;
};

/**
 * Writes the error norms of every listed cell into errorsPerCell, indexed by its position in cells.
 */
template <typename Derived, typename StoragePrecision>
struct exahype2::dg::MeasureNodeErrors final {
  static constexpr int NodesPerAxis               = Derived::DGOrder + 1;
  static constexpr int NumberOfUnknowns           = Derived::NumberOfUnknowns;
  static constexpr int NumberOfAuxiliaryVariables = Derived::NumberOfAuxiliaryVariables;
  static constexpr int NumberOfData               = NumberOfUnknowns + NumberOfAuxiliaryVariables;
  static constexpr int NumberOfNorms              = exahype2::ErrorMeasurement::NumberOfNorms;

  using CellPool = tarch::accelerator::ChunkedMemoryPool<StoragePrecision>;

  peano4::Partition            partition{};
  tarch::Span<const int>       cells{};
  typename CellPool::ConstView solution{};
  const StoragePrecision*      quadraturePoints{nullptr};
  const StoragePrecision*      quadratureWeights{nullptr};
  double*                      errorsPerCell{nullptr};
  double                       timeStamp{0.0};
  double                       timeStepSize{0.0};

  INLINE_METHOD GPU_CALLABLE_METHOD void operator()(int entry) const;
};

/**
 * Fills one cell of the partition after a regrid from the cells in source. The slots of source
 * belong to the partition before the regrid.
 */
template <typename Derived, typename StoragePrecision>
struct exahype2::dg::RegridCells final {
  static constexpr int NodesPerAxis  = Derived::DGOrder + 1;
  static constexpr int NumberOfData  = Derived::NumberOfUnknowns + Derived::NumberOfAuxiliaryVariables;
  static constexpr int ValuesPerCell = exahype2::dg::getNodesPerCell(NodesPerAxis) * NumberOfData;

  typename tarch::accelerator::ChunkedMemoryPool<StoragePrecision>::ConstView source{};
  const double*                                                               prolongationMatrix{nullptr};
  const double*                                                               restrictionMatrix{nullptr};

  INLINE_METHOD GPU_CALLABLE_METHOD void copy(int slot, StoragePrecision* target) const;
  INLINE_METHOD GPU_CALLABLE_METHOD void interpolateFromParent(
    int               parentSlot,
    StoragePrecision* target,
    int               childPosition
  ) const;
  INLINE_METHOD GPU_CALLABLE_METHOD void restrictFromChildren(const int* childSlots, StoragePrecision* target) const;
};

/**
 * Copies the partner's state and flux halves of every coupling face into the same halves of this
 * solver, projected onto its face nodes. couplingFaces stores two entries per face: The face and
 * the side of this solver's cell.
 */
template <typename Derived, typename PartnerDerived, typename PartnerPrecision, typename ComputePrecision>
struct exahype2::dg::CopyPartnerFaceHalves final {
  static constexpr int PartnerNodesPerAxis = PartnerDerived::DGOrder + 1;
  static constexpr int NodesPerAxis        = Derived::DGOrder + 1;
  static constexpr int NumberOfUnknowns    = Derived::NumberOfUnknowns;
  static constexpr int NumberOfData        = NumberOfUnknowns + Derived::NumberOfAuxiliaryVariables;

  using PartnerStatePool = tarch::accelerator::ChunkedMemoryPool<PartnerPrecision>;
  using PartnerFluxPool  = tarch::accelerator::ChunkedMemoryPool<PartnerPrecision>;
  using StatePool        = tarch::accelerator::ChunkedMemoryPool<ComputePrecision>;
  using FluxPool         = tarch::accelerator::ChunkedMemoryPool<ComputePrecision>;

  const int*                           couplingFaces{nullptr};
  typename PartnerStatePool::ConstView partnerStates{};
  typename PartnerFluxPool::ConstView  partnerFluxes{};
  typename StatePool::View             states{};
  typename FluxPool::View              fluxes{};
  const ComputePrecision*              matrix{nullptr};

  INLINE_METHOD GPU_CALLABLE_METHOD void operator()(int entry) const;
};
TARGET_DECLARE_END

#include "exahype2/dg/Kernels.cpph"
