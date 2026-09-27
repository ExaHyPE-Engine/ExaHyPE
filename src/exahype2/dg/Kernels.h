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
 * The kernels of the discontinuous Galerkin solver families. A cell of Derived, the generated
 * solver class, holds (DGOrder + 1)^DIMENSIONS nodes with one value per unknown and auxiliary
 * variable. A face holds two halves of face states and two halves of face fluxes, the half of
 * the lower side first, and the cell on a side writes its own half. The Riemann problem of a face
 * replaces both flux halves by the difference between the numerical flux and the half's own flux
 * with the sign of that side's outward normal.
 */
TARGET_DECLARE
namespace exahype2::dg {
  struct MaxEigenvalueStatistics;

  /** Returns the number of values the nodes of one cell carry. */
  constexpr int getValuesPerCell(const int nodesPerAxis, const int numberOfData) {
    return exahype2::dg::getNodesPerCell(nodesPerAxis) * numberOfData;
  }

  /** Returns the number of values the face states of both halves of one face carry. */
  constexpr int getStateValuesPerFace(const int nodesPerAxis, const int numberOfData) {
    return 2 * exahype2::dg::getNodesPerFace(nodesPerAxis) * numberOfData;
  }

  /** Returns the number of values the face fluxes of both halves of one face carry. */
  constexpr int getFluxValuesPerFace(const int nodesPerAxis, const int unknowns) {
    return 2 * exahype2::dg::getNodesPerFace(nodesPerAxis) * unknowns;
  }

  [[nodiscard]] INLINE_METHOD GPU_CALLABLE_METHOD int getOwnHalf(int faceNumber);

  [[nodiscard]] INLINE_METHOD GPU_CALLABLE_METHOD int getCorrectionHalf(
    const peano4::Partition& partition,
    int                      face,
    int                      faceNumber
  );

  template <typename Derived, typename StoragePrecision, typename QuadraturePrecision>
  INLINE_METHOD GPU_CALLABLE_METHOD void initialiseNodes(
    const tarch::la::Vector<DIMENSIONS, double>& centre,
    const tarch::la::Vector<DIMENSIONS, double>& width,
    const QuadraturePrecision*                   quadraturePoints,
    const QuadraturePrecision*                   quadratureWeights,
    StoragePrecision*                            nodes
  );

  template <typename Derived, typename StoragePrecision>
  struct InitialiseCells;

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
  struct SolveRiemannProblems;

  template <typename Derived, typename ComputePrecision>
  struct RestrictFluxHalves;

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
 * extent. identity() and combine() reduce the statistics of many cells through
 * exahype2::MaxEigenvalueReduction, so a wave speed that is negative or not finite survives.
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

/** Writes the initial condition of Derived into the nodes of every cell. */
template <typename Derived, typename StoragePrecision>
struct exahype2::dg::InitialiseCells final {
  using CellPool = tarch::accelerator::ChunkedMemoryPool<StoragePrecision>;

  peano4::Partition       partition{};
  typename CellPool::View solution{};
  const StoragePrecision* quadraturePoints{nullptr};
  const StoragePrecision* quadratureWeights{nullptr};

  INLINE_METHOD GPU_CALLABLE_METHOD void operator()(int entry) const;
};

/** Overwrites the halves of its faces that every listed cell owns with the face states of its solution. */
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
 * Overwrites the halves of its two faces along normal that a listed cell with a face on the
 * plane at position owns with the face states of its solution; a cell away from the plane writes
 * nothing.
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
 * Fills the coarse-side halves of the face states and the face fluxes of the fine faces of a
 * level jump from the same halves of its coarse face.
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
 * Solves the Riemann problem of every listed face and writes the numerical flux into both flux
 * halves. A boundary face first takes the boundary state of its cell into its outer half.
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

  INLINE_METHOD GPU_CALLABLE_METHOD void operator()(int entry) const;
};

/**
 * Fills the fine-side half of the coarse face of a level jump from the numerical fluxes of its
 * fine faces: A hanging face contributes the flux its coarse-side half holds, an intermediate
 * face the restricted flux its fine-side half holds.
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

/** Reduces the wave-speed maxima of Derived over the nodes of every listed cell. */
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

/** Writes the error norms of the nodes of every listed cell into the NumberOfNorms entries of the cell. */
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
 * Fills the nodes of a cell of the new partition in a regrid from the cells of the old
 * partition: copy() keeps a cell, interpolateFromParent() evaluates the polynomial of the parent
 * at the nodes of a new child with the prolongation matrix, and restrictFromChildren() projects
 * the nodes of the erased children onto their parent with the restriction matrix. Every function
 * reads the source pool and writes one cell into target.
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
 * Overwrites the partner's half of every coupling face with the face states and the face fluxes
 * the partner solver wrote, mapped from the face nodes of PartnerDerived onto the face nodes and
 * the compute precision of this solver. couplingFaces holds two entries per face: The face and
 * the side of the cell this solver owns.
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
