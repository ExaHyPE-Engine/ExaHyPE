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

  template <int NodesPerAxis, typename ComputePrecision>
  struct DerivativeWeightsOfNodePairs;

  template <int InputDofs, int OutputDofs, typename Value>
  struct ProjectionWeightsOfDofPairs;

  constexpr int getNodesPerFace(const int nodesPerAxis) { return peano4::power(nodesPerAxis, DIMENSIONS - 1); }

  constexpr int getNodesPerCell(const int nodesPerAxis) { return peano4::power(nodesPerAxis, DIMENSIONS); }

  constexpr int getNodeStrideOfAxis(const int nodesPerAxis, const int axis) {
    return peano4::power(nodesPerAxis, axis);
  }

  /**
   * Returns the first node of line among the lines of nodes along axis in the linearised node order
   * of a cell. Line i of an axis runs through face node i of a face normal to the axis.
   */
  constexpr int getFirstNodeOfLine(const int nodesPerAxis, const int axis, const int line) {
    const int stride = getNodeStrideOfAxis(nodesPerAxis, axis);
    return (line / stride) * stride * nodesPerAxis + line % stride;
  }

  constexpr int getPaddedCountOf(const int count) {
    return ((count + PEANO_WITH_SIMDLEN - 1) / PEANO_WITH_SIMDLEN) * PEANO_WITH_SIMDLEN;
  }

  template <typename QuadraturePrecision>
  INLINE_METHOD GPU_CALLABLE_METHOD tarch::la::Vector<DIMENSIONS, double> getQuadraturePoint(
    const tarch::la::Vector<DIMENSIONS, double>& cellCentre,
    const tarch::la::Vector<DIMENSIONS, double>& cellSize,
    const tarch::la::Vector<DIMENSIONS, int>&    index,
    const QuadraturePrecision* NOALIAS           quadraturePoints
  );

  template <int NodesPerAxis, typename NodePrecision>
  INLINE_METHOD GPU_CALLABLE_METHOD constexpr double getLagrangeBasisFunction1D(
    const NodePrecision* nodes,
    int                  basisFunction,
    double               offsetFromCentre
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

  template <int Values, typename T>
  INLINE_METHOD GPU_CALLABLE_METHOD void computeReferencesAndBoundsOfTransferTerms(
    double (&reference)[Values],
    double (&bound)[Values],
    bool (&varies)[Values],
    const T* const NOALIAS arrays[],
    int                    numberOfArrays,
    int                    nodesPerArray,
    double                 largestWeight
  );

  INLINE_METHOD GPU_CALLABLE_METHOD void computeReferenceAndBound(
    double  smallest,
    double  largest,
    double  largestWeight,
    double& reference,
    double& bound,
    bool&   varies
  );

  template <int Factors>
  INLINE_METHOD GPU_CALLABLE_METHOD double getLargestProductOfEntries(
    const double* NOALIAS matrix,
    int                   numberOfEntries
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

  template <int NodesPerAxis, typename ComputePrecision>
  INLINE_METHOD GPU_CALLABLE_METHOD DerivativeWeightsOfNodePairs<NodesPerAxis, ComputePrecision>
  getDerivativeWeightsOfNodePairs(const ComputePrecision* NOALIAS derivativeOperator);

  template <int InputDofs, int OutputDofs, typename Value>
  INLINE_METHOD GPU_CALLABLE_METHOD ProjectionWeightsOfDofPairs<InputDofs, OutputDofs, Value>
  getProjectionWeightsOfDofPairs(const Value* NOALIAS matrix);

  template <
    int  NumberOfEntries,
    int  NumberOfNodes,
    bool OverwriteResult,
    typename ResultPrecision,
    typename ComputePrecision,
    typename... Scalings
  >
  INLINE_METHOD GPU_CALLABLE_METHOD void differentiateAlongAxis(
    ResultPrecision*                                                     result,
    int                                                                  resultStride,
    const ComputePrecision*                                              values,
    int                                                                  valueStride,
    const DerivativeWeightsOfNodePairs<NumberOfNodes, ComputePrecision>& derivativeWeights,
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
  INLINE_METHOD GPU_CALLABLE_METHOD void extrapolateLineToFacePair(
    FacePrecision* NOALIAS              lowerFace,
    FacePrecision* NOALIAS              upperFace,
    const FieldPrecision* NOALIAS       field,
    const CoefficientPrecision* NOALIAS leftFaceCoefficients,
    const CoefficientPrecision* NOALIAS rightFaceCoefficients,
    int                                 direction,
    int                                 faceNode
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

  template <
    int NodesPerAxis,
    int NumberOfEntries,
    int NodeStride,
    int DirectionStride,
    typename FieldPrecision,
    typename FacePrecision,
    typename CoefficientPrecision
  >
  INLINE_METHOD GPU_CALLABLE_METHOD void extrapolateNodalFieldToFaces(
    FacePrecision* const NOALIAS        faces[peano4::FacesPerCell],
    const FieldPrecision* NOALIAS       field,
    const CoefficientPrecision* NOALIAS leftFaceCoefficients,
    const CoefficientPrecision* NOALIAS rightFaceCoefficients,
    int                                 firstItem,
    int                                 itemStride
  );

  /**
   * PairedProducts rounds the product of every degree of freedom of a line on its own. An identity
   * matrix then copies the values bitwise. Butterfly multiplies the sums and the differences of
   * mirrored degrees of freedom with half the products of PairedProducts. Butterfly requires a
   * matrix that is its own mirror image.
   */
  enum class ProjectionRule { PairedProducts, Butterfly };

  /**
   * Line makes every line of a projection one work item. ValueOfLine makes every value of every
   * line one work item. Both return the same bits.
   */
  enum class ProjectionItem { Line, ValueOfLine };

  template <
    int            Dimensions,
    int            InputDofs,
    int            OutputDofs,
    int            ValuesPerDof,
    ProjectionRule Rule,
    ProjectionItem Item,
    int            ProjectedAxes,
    int            Axis,
    typename Value,
    typename Combine
  >
  INLINE_METHOD GPU_CALLABLE_METHOD void projectLinesAlongAxis(
    const Value* NOALIAS                                             source,
    const Value* NOALIAS                                             matrix,
    const ProjectionWeightsOfDofPairs<InputDofs, OutputDofs, Value>& weights,
    const Combine&                                                   combine,
    int                                                              firstItem,
    int                                                              itemStride
  );

  template <int Dimensions, int InputDofs, int OutputDofs, int ValuesPerDof>
  INLINE_METHOD constexpr int getScratchValuesOfTensorProduct();

  template <int Dimensions>
  INLINE_METHOD constexpr int getStagesOfTensorProduct();

  template <
    int            Dimensions,
    int            InputDofs,
    int            OutputDofs,
    int            ValuesPerDof,
    ProjectionRule Rule,
    ProjectionItem Item,
    typename Value
  >
  INLINE_METHOD GPU_CALLABLE_METHOD void projectTensorProductStage(
    int                                                              stage,
    const Value* NOALIAS                                             input,
    Value* NOALIAS                                                   output,
    const Value* NOALIAS                                             matrix,
    const ProjectionWeightsOfDofPairs<InputDofs, OutputDofs, Value>& weights,
    Value* NOALIAS                                                   scratch,
    int                                                              firstItem,
    int                                                              itemStride
  );

  template <int Dimensions, int InputDofs, int OutputDofs, int ValuesPerDof, ProjectionRule Rule, typename Value>
  INLINE_METHOD GPU_CALLABLE_METHOD void projectTensorProduct(const Value* input, Value* output, const Value* matrix);

  template <typename Derived, typename ComputePrecision>
  INLINE_METHOD GPU_CALLABLE_METHOD void differentiateFluxInPlace(
    ComputePrecision* NOALIAS                              flux,
    const ComputePrecision* NOALIAS                        derivativeOperator,
    const tarch::la::Vector<DIMENSIONS, ComputePrecision>& inverseCellSize,
    int                                                    firstItem  = 0,
    int                                                    itemStride = 1
  );
} // namespace exahype2::dg

/**
 * The node counts of a cell of Derived and its node strides padded to the SIMD width. Every
 * padding lane is zero.
 */
template <typename Derived>
struct exahype2::dg::PaddedLayout final {
  static constexpr int NodesPerAxis           = Derived::DGOrder + 1;
  static constexpr int NumberOfUnknowns       = Derived::NumberOfUnknowns;
  static constexpr int NumberOfData           = NumberOfUnknowns + Derived::NumberOfAuxiliaryVariables;
  static constexpr int PaddedNumberOfUnknowns = exahype2::dg::getPaddedCountOf(NumberOfUnknowns);
  static constexpr int PaddedNumberOfData     = exahype2::dg::getPaddedCountOf(NumberOfData);
  static constexpr int NodesPerAxisIn3D       = (DIMENSIONS == 3) ? NodesPerAxis : 1;
  static constexpr int NodesPerFace           = NodesPerAxis * NodesPerAxisIn3D;
  static constexpr int NodesPerCell           = NodesPerFace * NodesPerAxis;
  static constexpr int SpaceTimeNodes         = NodesPerCell * NodesPerAxis;
};

/**
 * Stores the derivative operator of a line in the sums and differences of its mirror node pairs.
 * Entry [i][p] of sumWeights is half the sum of the weights of the nodes p and NodesPerAxis - 1 - p
 * in row i of the operator. Entry [i][p] of differenceWeights is half their difference.
 */
template <int NodesPerAxis, typename ComputePrecision>
struct exahype2::dg::DerivativeWeightsOfNodePairs final {
  static constexpr int Pairs     = NodesPerAxis / 2;
  static constexpr int PairSlots = Pairs > 0 ? Pairs : 1;

  ComputePrecision sumWeights[NodesPerAxis][PairSlots]{};
  ComputePrecision differenceWeights[NodesPerAxis][PairSlots]{};
};

/**
 * Stores a projection matrix in the sums and differences of its mirror input pairs. Entry [p][d] of
 * sumWeights is half the sum of the entries of the inputs p and InputDofs - 1 - p for the output d
 * of the lower half. Entry [p][d] of differenceWeights is half their difference.
 */
template <int InputDofs, int OutputDofs, typename Value>
struct exahype2::dg::ProjectionWeightsOfDofPairs final {
  static constexpr int Pairs     = InputDofs / 2;
  static constexpr int PairSlots = Pairs > 0 ? Pairs : 1;
  static constexpr int LowerDofs = (OutputDofs + 1) / 2;

  Value sumWeights[PairSlots][LowerDofs]{};
  Value differenceWeights[PairSlots][LowerDofs]{};
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

  template <int Values>
  inline constexpr int ReferencesAndBoundsEntries = 3 * Values;

  template <int Values, typename T>
  INLINE_METHOD GPU_CALLABLE_METHOD void storeReferenceAndBoundOfTransferTerm(
    const T* const NOALIAS arrays[],
    int                    numberOfArrays,
    int                    nodesPerArray,
    double                 largestWeight,
    int                    value,
    double* NOALIAS        referencesAndBounds
  );

  template <int NodesPerAxis, int Values, typename T>
  INLINE_METHOD GPU_CALLABLE_METHOD void computeReferenceAndBoundOfParent(
    const T* NOALIAS      coarseCell,
    const double* NOALIAS prolongationMatrix1D,
    int                   value,
    double* NOALIAS       referencesAndBounds
  );

  template <int NodesPerAxis, int Values, typename T>
  INLINE_METHOD GPU_CALLABLE_METHOD void computeReferenceAndBoundOfChildren(
    const T* const NOALIAS fineCells[peano4::ChildrenPerCell],
    const double* NOALIAS  restrictionMatrix1D,
    int                    value,
    double* NOALIAS        referencesAndBounds
  );

  template <int NodesPerAxis, int Values, typename T>
  INLINE_METHOD GPU_CALLABLE_METHOD void interpolateNode(
    const T* NOALIAS      coarseCell,
    T* NOALIAS            fineCell,
    const double* NOALIAS prolongationMatrix1D,
    const double* NOALIAS referencesAndBounds,
    int                   childPosition,
    int                   fineNode
  );

  template <int NodesPerAxis, int Values, typename T>
  INLINE_METHOD GPU_CALLABLE_METHOD void restrictFace(
    const T* const NOALIAS fineFaces[peano4::SubfacesPerFace],
    T* NOALIAS             coarseFace,
    const double* NOALIAS  restrictionMatrix1D
  );

  template <int NodesPerAxis, int Values, typename T>
  INLINE_METHOD GPU_CALLABLE_METHOD void restrictNode(
    const T* const NOALIAS fineCells[peano4::ChildrenPerCell],
    T* NOALIAS             coarseCell,
    const double* NOALIAS  restrictionMatrix1D,
    const double* NOALIAS  referencesAndBounds,
    int                    coarseNode
  );
} // namespace exahype2::dg
TARGET_DECLARE_END

#include "exahype2/dg/Basis.cpph"
