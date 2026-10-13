// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "peano4/CellConnectivity.h"
#include "peano4/Constants.h"
#include "tarch/CompilerSpecificSettings.h"
#include "tarch/accelerator/CompileTarget.h"
#include "tarch/la/Vector.h"

/**
 * @file
 * The nodal Lagrange basis of a discontinuous Galerkin cell. The nodes of a cell lie at the
 * tensor-product quadrature points and are linearised with the first axis fastest. Every node
 * stores Values consecutive values.
 */
TARGET_DECLARE
namespace exahype2::dg {
  template <typename Derived>
  struct PaddedLayout;

  template <typename Precision>
  struct BasisTables;

  constexpr int getNodesPerFace(const int nodesPerAxis) { return peano4::power(nodesPerAxis, DIMENSIONS - 1); }

  constexpr int getNodesPerCell(const int nodesPerAxis) { return peano4::power(nodesPerAxis, DIMENSIONS); }

  template <typename QuadraturePrecision>
  INLINE_METHOD GPU_CALLABLE_METHOD tarch::la::Vector<DIMENSIONS, double> getQuadraturePoint(
    const tarch::la::Vector<DIMENSIONS, double>& cellCentre,
    const tarch::la::Vector<DIMENSIONS, double>& cellSize,
    const tarch::la::Vector<DIMENSIONS, int>&    index,
    const QuadraturePrecision* NOALIAS           quadraturePoints
  );

  template <int NodesPerAxis, int Values, typename Precision, typename QuadraturePrecision, typename Functor>
  INLINE_METHOD constexpr void forEachNode(
    const tarch::la::Vector<DIMENSIONS, double>& cellCentre,
    const tarch::la::Vector<DIMENSIONS, double>& cellSize,
    const QuadraturePrecision*                   quadraturePoints,
    const QuadraturePrecision*                   quadratureWeights,
    Precision*                                   values,
    Functor&&                                    functor
  );

  template <typename StoragePrecision>
  INLINE_METHOD GPU_CALLABLE_METHOD StoragePrecision evaluatePolynomial(
    const tarch::la::Vector<DIMENSIONS, double>& cellCentre,
    const tarch::la::Vector<DIMENSIONS, double>& cellSize,
    int                                          order,
    const double* NOALIAS                        quadratureNodes,
    int                                          unknownsPerNode,
    const StoragePrecision* NOALIAS              values,
    const tarch::la::Vector<DIMENSIONS, double>& position,
    int                                          unknown
  );

  [[nodiscard]] INLINE_METHOD GPU_CALLABLE_METHOD double orderInvariantProduct(
    double firstFactor,
    double secondFactor,
    double thirdFactor
  );

  template <int Values, typename T>
  INLINE_METHOD GPU_CALLABLE_METHOD void takeSmallestValues(
    double (&reference)[Values],
    const T* NOALIAS nodes,
    int              numberOfNodes
  );

  template <
    int  NumberOfEntries,
    int  NumberOfTargetNodes,
    int  NumberOfSourceNodes,
    int  ValueStride,
    int  OperatorStride,
    int  ResultStride,
    bool OverwriteResult,
    typename ResultPrecision,
    typename ComputePrecision,
    typename... Scalings
  >
  INLINE_METHOD GPU_CALLABLE_METHOD void applyOperatorAlongAxis(
    ResultPrecision* NOALIAS        result,
    const ComputePrecision* NOALIAS values,
    const ComputePrecision* NOALIAS operatorMatrix,
    const Scalings... scalings
  );

  template <
    int  NumberOfEntries,
    int  NumberOfNodes,
    int  ValueStride,
    int  OperatorStride,
    int  ResultStride,
    bool OverwriteResult,
    typename ResultPrecision,
    typename ComputePrecision,
    typename... Scalings
  >
  INLINE_METHOD GPU_CALLABLE_METHOD void differentiateAlongAxis(
    ResultPrecision* NOALIAS        result,
    const ComputePrecision* NOALIAS values,
    const ComputePrecision* NOALIAS derivativeOperator,
    const Scalings... scalings
  );

  template <int NodesPerAxis, int Values, typename StoragePrecision, typename ComputePrecision>
  INLINE_METHOD GPU_CALLABLE_METHOD void computeNodalGradients(
    ComputePrecision* NOALIAS                    gradients,
    const StoragePrecision* NOALIAS              cellState,
    const ComputePrecision* NOALIAS              derivativeOperator,
    const tarch::la::Vector<DIMENSIONS, double>& cellSize
  );

  template <
    int NodesPerAxis,
    int NumberOfEntries,
    int NodeStride,
    typename FieldPrecision,
    typename FacePrecision,
    typename CoefficientPrecision
  >
  INLINE_METHOD GPU_CALLABLE_METHOD void extrapolateNodalFieldToFacePair(
    FacePrecision* NOALIAS              lowerFace,
    FacePrecision* NOALIAS              upperFace,
    const FieldPrecision* NOALIAS       field,
    const CoefficientPrecision* NOALIAS leftFaceCoefficients,
    const CoefficientPrecision* NOALIAS rightFaceCoefficients,
    int                                 direction
  );

  template <int Dimensions, int InputDofs, int OutputDofs, int ValuesPerDof, typename Value>
  INLINE_METHOD GPU_CALLABLE_METHOD void projectTensorProduct(const Value* input, Value* output, const Value* matrix);

  template <typename Derived, typename ComputePrecision>
  INLINE_METHOD GPU_CALLABLE_METHOD void differentiateFluxInPlace(
    ComputePrecision* NOALIAS                              flux,
    const ComputePrecision* NOALIAS                        derivativeOperator,
    const tarch::la::Vector<DIMENSIONS, ComputePrecision>& inverseCellSize,
    int                                                    firstLine  = 0,
    int                                                    lineStride = 1
  );
} // namespace exahype2::dg

/**
 * The node counts of a cell of Derived and its node strides padded to the SIMD width. Every
 * padding lane is zero.
 */
template <typename Derived>
struct exahype2::dg::PaddedLayout final {
  static constexpr int NodesPerAxis     = Derived::DGOrder + 1;
  static constexpr int NumberOfUnknowns = Derived::NumberOfUnknowns;
  static constexpr int NumberOfData     = NumberOfUnknowns + Derived::NumberOfAuxiliaryVariables;
  static constexpr int PaddedNumberOfUnknowns =
    ((NumberOfUnknowns + PEANO_WITH_SIMDLEN - 1) / PEANO_WITH_SIMDLEN) * PEANO_WITH_SIMDLEN;
  static constexpr int PaddedNumberOfData =
    ((NumberOfData + PEANO_WITH_SIMDLEN - 1) / PEANO_WITH_SIMDLEN) * PEANO_WITH_SIMDLEN;
  static constexpr int NodesPerAxisIn3D = (DIMENSIONS == 3) ? NodesPerAxis : 1;
  static constexpr int NodesPerFace     = NodesPerAxis * NodesPerAxisIn3D;
  static constexpr int NodesPerCell     = NodesPerFace * NodesPerAxis;
  static constexpr int SpaceTimeNodes   = NodesPerCell * NodesPerAxis;
};

/**
 * The operator tables of one Lagrange basis. Every matrix is stored row major with NodesPerAxis
 * columns. The quadrature points are also the time nodes of the space-time predictor.
 */
template <typename Precision>
struct exahype2::dg::BasisTables final {
  const Precision* quadraturePoints{nullptr};
  const Precision* quadratureWeights{nullptr};
  const Precision* inverseQuadratureWeights{nullptr};
  const Precision* cellQuadratureWeights{nullptr};
  const Precision* inverseCellQuadratureWeights{nullptr};
  const Precision* stiffnessOperator{nullptr};
  const Precision* stiffnessOperatorTransposed{nullptr};
  const Precision* derivativeOperator{nullptr};
  const Precision* inversePicardOperator{nullptr};
  const Precision* leftFaceCoefficients{nullptr};
  const Precision* rightFaceCoefficients{nullptr};
};
TARGET_DECLARE_END

namespace exahype2::dg {
  /**
   * The 1D transfer matrices of the child intervals, stored as [child][row][column]. The transfer
   * operator between a cell and its children is a tensor product of them.
   */
  template <int NodesPerAxis>
  using AMRMatrices1D =
    std::array<double, static_cast<std::size_t>(peano4::SubdivisionFactor * NodesPerAxis * NodesPerAxis)>;

  template <int NodesPerAxis>
  INLINE_METHOD constexpr double getLagrangeBasisFunction1D(const double* nodes, int basisFunction, double x);

  template <int NodesPerAxis>
  INLINE_METHOD constexpr AMRMatrices1D<NodesPerAxis> getInterpolationMatrix1D(const double* nodes);

  template <int NodesPerAxis>
  INLINE_METHOD constexpr AMRMatrices1D<NodesPerAxis> getRestrictionMatrix1D(const double* nodes);

  template <int NodesPerAxis>
  INLINE_METHOD constexpr AMRMatrices1D<NodesPerAxis> getWeightedRestrictionMatrix1D(
    const double* nodes,
    const double* weights
  );

  template <int NodesPerAxis>
  INLINE_METHOD constexpr void averageMirrorPairs(AMRMatrices1D<NodesPerAxis>& matrices);

  template <int NodesPerAxis>
  INLINE_METHOD constexpr AMRMatrices1D<NodesPerAxis> transposePerChild(const AMRMatrices1D<NodesPerAxis>& matrices);
} // namespace exahype2::dg

TARGET_DECLARE
namespace exahype2::dg {
  template <int NodesPerAxis, int Values, typename T>
  INLINE_METHOD GPU_CALLABLE_METHOD void interpolateFace(
    const T* NOALIAS      coarseFace,
    T* NOALIAS            fineFace,
    const double* NOALIAS prolongationMatrix1D,
    int                   subface
  );

  template <int NodesPerAxis, int Values, typename T>
  INLINE_METHOD GPU_CALLABLE_METHOD void interpolateCell(
    const T* NOALIAS      coarseCell,
    T* NOALIAS            fineCell,
    const double* NOALIAS prolongationMatrix1D,
    int                   childPosition
  );

  template <int NodesPerAxis, int Values, typename T>
  INLINE_METHOD GPU_CALLABLE_METHOD void restrictFace(
    const T* const NOALIAS fineFaces[peano4::SubfacesPerFace],
    T* NOALIAS             coarseFace,
    const double* NOALIAS  restrictionMatrix1D
  );

  template <int NodesPerAxis, int Values, typename T>
  INLINE_METHOD GPU_CALLABLE_METHOD void restrictCell(
    const T* const NOALIAS fineCells[peano4::ChildrenPerCell],
    T* NOALIAS             coarseCell,
    const double* NOALIAS  restrictionMatrix1D
  );
} // namespace exahype2::dg
TARGET_DECLARE_END

#include "exahype2/dg/Basis.cpph"
