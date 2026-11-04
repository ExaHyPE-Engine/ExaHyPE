// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "BasisTest.h"

#include "exahype2/dg/Basis.h"
#include "exahype2/dg/tests/TransferTestData.h"
#include "peano4/CellConnectivity.h"
#include "peano4/Constants.h"
#include "peano4/Loop.h"
#include "tarch/la/Vector.h"

namespace {
  constexpr int NodesPerAxis               = 3;
  constexpr int NumberOfUnknowns           = 2;
  constexpr int NumberOfAuxiliaryVariables = 1;
  constexpr int NumberOfData               = NumberOfUnknowns + NumberOfAuxiliaryVariables;
  constexpr int FaceNodes                  = exahype2::dg::getNodesPerFace(NodesPerAxis);
  constexpr int CellNodes                  = exahype2::dg::getNodesPerCell(NodesPerAxis);
  constexpr int EntriesPerAxis             = CellNodes * NumberOfData;
  constexpr int PaddedNodeStride           = NumberOfData + 2;

  struct Order2Layout {
    static constexpr int DGOrder                    = NodesPerAxis - 1;
    static constexpr int NumberOfUnknowns           = ::NumberOfUnknowns;
    static constexpr int NumberOfAuxiliaryVariables = ::NumberOfAuxiliaryVariables;
  };

  /**
   * A solver layout whose unknowns fill the vector width exactly.
   */
  struct ExactWidthLayout {
    static constexpr int DGOrder                    = NodesPerAxis - 1;
    static constexpr int NumberOfUnknowns           = PEANO_WITH_SIMDLEN;
    static constexpr int NumberOfAuxiliaryVariables = 0;
  };

  /**
   * A solver layout whose unknowns exceed the vector width by one.
   */
  struct OneAboveWidthLayout {
    static constexpr int DGOrder                    = NodesPerAxis - 1;
    static constexpr int NumberOfUnknowns           = PEANO_WITH_SIMDLEN + 1;
    static constexpr int NumberOfAuxiliaryVariables = 0;
  };

  using PaddedCell = exahype2::dg::PaddedLayout<Order2Layout>;

  constexpr int FluxAxisStride = CellNodes * PaddedCell::PaddedNumberOfUnknowns;

  // The nodes of the three-point Gauss-Legendre rule on the unit interval.
  const double QuadratureNodes[NodesPerAxis] = {0.5 * (1.0 - std::sqrt(0.6)), 0.5, 0.5 * (1.0 + std::sqrt(0.6))};

  constexpr double QuadratureNodes1DP1[] = {0.21132486540518713, 0.7886751345948129};
  constexpr double QuadratureNodes1DP2[] = {0.1127016653792583, 0.5, 0.8872983346207417};

  double getBasisValue(const double x, const int node) {
    double result = 1.0;
    for (int other = 0; other < NodesPerAxis; other++) {
      if (other != node) {
        result *= (x - QuadratureNodes[other]) / (QuadratureNodes[node] - QuadratureNodes[other]);
      }
    }
    return result;
  }

  double getBasisDerivative(const double x, const int node) {
    double result = 0.0;
    for (int omitted = 0; omitted < NodesPerAxis; omitted++) {
      if (omitted != node) {
        double term = 1.0 / (QuadratureNodes[node] - QuadratureNodes[omitted]);
        for (int other = 0; other < NodesPerAxis; other++) {
          if (other != node and other != omitted) {
            term *= (x - QuadratureNodes[other]) / (QuadratureNodes[node] - QuadratureNodes[other]);
          }
        }
        result += term;
      }
    }
    return result;
  }

  /**
   * The entry (i, j) is the derivative of basis function j at node i.
   */
  struct DerivativeOperator final {
    double entries[NodesPerAxis * NodesPerAxis]{};

    DerivativeOperator() {
      for (int row = 0; row < NodesPerAxis; row++) {
        for (int column = 0; column < NodesPerAxis; column++) {
          entries[row * NodesPerAxis + column] = getBasisDerivative(QuadratureNodes[row], column);
        }
      }
    }
  };

  /**
   * The values of the basis functions at the two ends of the unit interval.
   */
  struct EndCoefficients final {
    double left[NodesPerAxis]{};
    double right[NodesPerAxis]{};

    EndCoefficients() {
      for (int node = 0; node < NodesPerAxis; node++) {
        left[node]  = getBasisValue(0.0, node);
        right[node] = getBasisValue(1.0, node);
      }
    }
  };

  /**
   * Returns the cell node of face node faceNode at position normalIndex along direction.
   */
  int getCellNodeInAxisOrder(const int direction, const int faceNode, const int normalIndex) {
    auto nodeIndex       = peano4::dDelinearisedExclusive(faceNode, NodesPerAxis, direction);
    nodeIndex(direction) = normalIndex;
    return peano4::dLinearised(nodeIndex, NodesPerAxis);
  }

  tarch::la::Vector<DIMENSIONS, double> getCellSize() {
    tarch::la::Vector<DIMENSIONS, double> cellSize;
    for (int axis = 0; axis < DIMENSIONS; axis++) {
      cellSize(axis) = 0.5 + 0.25 * axis;
    }
    return cellSize;
  }

  tarch::la::Vector<DIMENSIONS, double> getRampOverAxes(const double first, const double increment) {
    tarch::la::Vector<DIMENSIONS, double> result;
    for (int axis = 0; axis < DIMENSIONS; axis++) {
      result(axis) = first + increment * axis;
    }
    return result;
  }

  double getAffineField(const tarch::la::Vector<DIMENSIONS, double>& position) {
    double result = 1.0;
    for (int axis = 0; axis < DIMENSIONS; axis++) {
      result += (1.0 + axis) * position(axis);
    }
    return result;
  }

  /**
   * Writes the affine field at the nodes of the cell into values. An interpolant of order one or
   * higher reproduces it everywhere.
   */
  void fillAffineNodalValues(
    const tarch::la::Vector<DIMENSIONS, double>& cellCentre,
    const tarch::la::Vector<DIMENSIONS, double>& cellSize,
    const int                                    order,
    const double* NOALIAS const                  nodes,
    double* NOALIAS const                        values
  ) {
    peano4::dForEach(order + 1, [&](const tarch::la::Vector<DIMENSIONS, int>& node) {
      tarch::la::Vector<DIMENSIONS, double> position;
      for (int axis = 0; axis < DIMENSIONS; axis++) {
        position(axis) = cellCentre(axis) - 0.5 * cellSize(axis) + nodes[node(axis)] * cellSize(axis);
      }
      values[peano4::dLinearised(node, order + 1)] = getAffineField(position);
    });
  }

  constexpr int    WalkNodesPerAxis = 2;
  constexpr int    WalkValues       = 3;
  constexpr int    WalkNodes        = exahype2::dg::getNodesPerCell(WalkNodesPerAxis);
  constexpr auto   WalkEntries      = static_cast<std::size_t>(WalkNodes) * WalkValues;
  constexpr double WalkCellWidth    = 2.0;
  constexpr double WalkPoints[]     = {0.25, 0.75};
  constexpr double WalkWeights[]    = {0.5, 0.5};

  constexpr int NumberOfEntries     = PEANO_WITH_SIMDLEN;
  constexpr int NumberOfTargetNodes = 4;
  constexpr int NumberOfSourceNodes = 2;

  // The entries are distinct and non-symmetric. An index mix-up then changes the result.
  void fillOperands(
    double (&values)[NumberOfSourceNodes * NumberOfEntries],
    double (&operatorMatrix)[NumberOfTargetNodes * NumberOfSourceNodes]
  ) {
    for (int entry = 0; entry < NumberOfSourceNodes * NumberOfEntries; entry++) {
      values[entry] = 1.0 + 0.25 * entry;
    }
    for (int entry = 0; entry < NumberOfTargetNodes * NumberOfSourceNodes; entry++) {
      operatorMatrix[entry] = 2.0 - 0.125 * entry;
    }
  }

  constexpr int    SubdivisionFactor  = peano4::SubdivisionFactor;
  constexpr int    MatrixNodesPerAxis = 4;
  constexpr double MatrixNodes[MatrixNodesPerAxis]{
    0.06943184420297371,
    0.33000947820757187,
    0.6699905217924281,
    0.9305681557970262
  };
  constexpr double MatrixWeights[MatrixNodesPerAxis]{
    0.1739274225687269,
    0.3260725774312731,
    0.3260725774312731,
    0.1739274225687269
  };
  constexpr double MatrixTolerance = 1e-13;

  constexpr auto Interpolation = exahype2::dg::getInterpolationMatrix1D<MatrixNodesPerAxis>(MatrixNodes);
  constexpr auto Restriction   = exahype2::dg::getRestrictionMatrix1D<MatrixNodesPerAxis>(MatrixNodes);
  constexpr auto WeightedRestriction =
    exahype2::dg::getWeightedRestrictionMatrix1D<MatrixNodesPerAxis>(MatrixNodes, MatrixWeights);

  static_assert(
    Interpolation.size() == static_cast<std::size_t>(SubdivisionFactor) * MatrixNodesPerAxis * MatrixNodesPerAxis
  );

  constexpr int getIndex(const int child, const int row, const int column) {
    return (child * MatrixNodesPerAxis + row) * MatrixNodesPerAxis + column;
  }

  constexpr int getMirrorIndex(const int child, const int row, const int column) {
    return getIndex(SubdivisionFactor - 1 - child, MatrixNodesPerAxis - 1 - row, MatrixNodesPerAxis - 1 - column);
  }

  double getCubicValue(const double x) { return 1.0 + 2.0 * x - 3.0 * x * x + 4.0 * x * x * x; }

  using Setup = exahype2::dg::tests::TransferSetup;

  constexpr int ProjectionInputDofs  = 3;
  constexpr int ProjectionOutputDofs = 4;
  constexpr int ProjectionValues     = 2;

  using ProjectionMatrix = std::array<double, static_cast<std::size_t>(ProjectionInputDofs* ProjectionOutputDofs)>;

  /**
   * Returns a projection matrix that is its own mirror image and whose entries are not dyadic.
   */
  ProjectionMatrix getMirrorSymmetricProjection() {
    const auto getTerm = [](const int inputDof, const int outputDof) {
      return 0.13 * (inputDof + 1) * (outputDof + 2) + 0.07 / (inputDof + outputDof + 3);
    };
    ProjectionMatrix matrix{};
    for (int inputDof = 0; inputDof < ProjectionInputDofs; inputDof++) {
      for (int outputDof = 0; outputDof < ProjectionOutputDofs; outputDof++) {
        matrix[static_cast<std::size_t>(inputDof * ProjectionOutputDofs + outputDof)] = getTerm(inputDof, outputDof)
          + getTerm(ProjectionInputDofs - 1 - inputDof, ProjectionOutputDofs - 1 - outputDof);
      }
    }
    return matrix;
  }

  /**
   * Returns the position of value in a field of ProjectionValues values per degree of freedom and
   * dofsPerAxis degrees of freedom along each of Dimensions axes after a swap of the axes first and
   * second. A negative second reflects the axis first.
   */
  template <int Dimensions>
  int getMovedValue(const int value, const int dofsPerAxis, const int first, const int second) {
    const int entry = value % ProjectionValues;
    int       rest  = value / ProjectionValues;
    int       index[Dimensions];
    for (int axis = 0; axis < Dimensions; axis++) {
      index[axis] = rest % dofsPerAxis;
      rest /= dofsPerAxis;
    }
    if (second < 0) {
      index[first] = dofsPerAxis - 1 - index[first];
    } else {
      std::swap(index[first], index[second]);
    }
    int moved = 0;
    for (int axis = Dimensions - 1; axis >= 0; axis--) {
      moved = moved * dofsPerAxis + index[axis];
    }
    return moved * ProjectionValues + entry;
  }

  /**
   * Returns a field of ProjectionInputDofs degrees of freedom per axis with values that are not
   * dyadic.
   */
  template <int Dimensions>
  std::vector<double> getProjectionInput() {
    std::vector<double> input(
      static_cast<std::size_t>(peano4::power(ProjectionInputDofs, Dimensions) * ProjectionValues)
    );
    for (std::size_t value = 0; value < input.size(); value++) {
      input[value] = 1.0 + 0.37 * static_cast<double>(value % 11) - 0.05 * static_cast<double>(value);
    }
    return input;
  }

  /**
   * Returns the projection of input with getMirrorSymmetricProjection() on one thread.
   */
  template <int Dimensions, exahype2::dg::ProjectionRule Rule>
  std::vector<double> project(const std::vector<double>& input) {
    const ProjectionMatrix matrix = getMirrorSymmetricProjection();
    std::vector<double>    output(
      static_cast<std::size_t>(peano4::power(ProjectionOutputDofs, Dimensions) * ProjectionValues)
    );
    exahype2::dg::projectTensorProduct<Dimensions, ProjectionInputDofs, ProjectionOutputDofs, ProjectionValues, Rule>(
      input.data(),
      output.data(),
      matrix.data()
    );
    return output;
  }

  /**
   * Fills every node of fineCell from coarseCell with exahype2::dg::interpolateNode().
   */
  void interpolateEveryNode(
    const double* const coarseCell,
    double* const       fineCell,
    const double* const prolongationMatrix1D,
    const int           childPosition
  ) {
    double referencesAndBounds[exahype2::dg::ReferencesAndBoundsEntries<Setup::ValuesPerNode>];
    for (int value = 0; value < Setup::ValuesPerNode; value++) {
      exahype2::dg::computeReferenceAndBoundOfParent<Setup::NodesPerAxis, Setup::ValuesPerNode>(
        coarseCell,
        prolongationMatrix1D,
        value,
        referencesAndBounds
      );
    }
    for (int fineNode = 0; fineNode < Setup::CellNodes; fineNode++) {
      exahype2::dg::interpolateNode<Setup::NodesPerAxis, Setup::ValuesPerNode>(
        coarseCell,
        fineCell,
        prolongationMatrix1D,
        referencesAndBounds,
        childPosition,
        fineNode
      );
    }
  }

  /**
   * Fills every node of coarseCell from fineCells with exahype2::dg::restrictNode().
   */
  void restrictEveryNode(
    const double* const fineCells[peano4::ChildrenPerCell],
    double* const       coarseCell,
    const double* const restrictionMatrix1D
  ) {
    double referencesAndBounds[exahype2::dg::ReferencesAndBoundsEntries<Setup::ValuesPerNode>];
    for (int value = 0; value < Setup::ValuesPerNode; value++) {
      exahype2::dg::computeReferenceAndBoundOfChildren<Setup::NodesPerAxis, Setup::ValuesPerNode>(
        fineCells,
        restrictionMatrix1D,
        value,
        referencesAndBounds
      );
    }
    for (int coarseNode = 0; coarseNode < Setup::CellNodes; coarseNode++) {
      exahype2::dg::restrictNode<Setup::NodesPerAxis, Setup::ValuesPerNode>(
        fineCells,
        coarseCell,
        restrictionMatrix1D,
        referencesAndBounds,
        coarseNode
      );
    }
  }
} // namespace

/**
 * Checks the interpolant of the affine field at its first node and at the cell centre.
 */
template <int Order>
void
exahype2::dg::tests::BasisTest::validateAffineInterpolant(const double* const nodes) {
  constexpr int NodesPerCell = exahype2::dg::getNodesPerCell(Order + 1);
  const auto    cellCentre   = getRampOverAxes(0.4, 0.2);
  const auto    cellSize     = tarch::la::Vector<DIMENSIONS, double>(0.2);

  double values[NodesPerCell];
  fillAffineNodalValues(cellCentre, cellSize, Order, nodes, values);

  auto firstNode = cellCentre - 0.5 * cellSize;
  for (int axis = 0; axis < DIMENSIONS; axis++) {
    firstNode(axis) += nodes[0] * cellSize(axis);
  }
  const double valueAtFirstNode =
    exahype2::dg::evaluatePolynomial(cellCentre, cellSize, Order, nodes, 1, values, firstNode, 0);
  validateNumericalEqualsWithParams1(valueAtFirstNode, getAffineField(firstNode), Order);

  const double valueAtCentre =
    exahype2::dg::evaluatePolynomial(cellCentre, cellSize, Order, nodes, 1, values, cellCentre, 0);
  validateNumericalEqualsWithParams1(valueAtCentre, getAffineField(cellCentre), Order);
}

exahype2::dg::tests::BasisTest::BasisTest() :
  tarch::tests::TestCase("exahype2::dg::tests::BasisTest") {}

void
exahype2::dg::tests::BasisTest::run() {
  testMethod(testNodeCountsFollowTheNumberOfNodesPerAxis);
  testMethod(testEveryNodeIsPaddedToTheVectorWidth);
  testMethod(testTheNodeCountsFollowTheDimensionCount);
  testMethod(testTheBasisTablesPointNowhereUntilTheyAreSet);
  testMethod(testGetQuadraturePointMapsTheReferenceNode);
  testMethod(testVisitsEveryNodeWithItsQuadratureWeight);
  testMethod(testReadsFloatValuesInPlace);
  testMethod(testEvaluatePolynomialOrder1);
  testMethod(testEvaluatePolynomialOrder2);
  testMethod(testTheReferencesAndBoundsFollowTheValueRanges);
  testMethod(testTheLargestProductOfEntriesIsThePowerOfTheLargestMagnitude);
  testMethod(testProductMatchesTheDefinition);
  testMethod(testResultIsAccumulatedOrOverwritten);
  testMethod(testScalingMultipliesEveryProduct);
  testMethod(testStridesAddressTheIntendedSlices);
  testMethod(testANarrowerResultIsRoundedOnce);
  testMethod(testThePairWeightsRecombineToTheOperatorRows);
  testMethod(testTheDerivativeOfAConstantLineIsExactlyZero);
  testMethod(testTheDerivativeActsOnTheDifferencesFromTheCentreNode);
  testMethod(testADerivativeInPlaceEqualsADerivativeIntoAnotherBuffer);
  testMethod(testTheDerivativeOfAReflectedLineIsTheReflectedDerivative);
  testMethod(testGradientOfALinearPolynomialIsItsSlope);
  testMethod(testEachBlockStoresTheDerivativeAlongItsOwnAxis);
  testMethod(testNarrowStorageIsDifferentiatedInTheComputePrecision);
  testMethod(testTheGradientOfAConstantStateIsExactlyZero);
  testMethod(testALinearFluxDifferentiatesToItsPhysicalSlope);
  testMethod(testAConstantFluxDifferentiatesToExactZeros);
  testMethod(testTwoCallsOverAlternateLinesDifferentiateLikeOneCall);
  testMethod(testAPaddedNodeStrideProjectsOnlyTheEntries);
  testMethod(testTheLowerAndUpperFaceUseTheirOwnCoefficients);
  testMethod(testTheFacesOfAReflectedFieldAreTheSwappedFaces);
  testMethod(testTheFaceNodesRunWithTheLowestTangentialAxisFastest);
  testMethod(testAFieldOfFloatAccumulatesTheFacesInDouble);
  testMethod(testThreadsThatDivideTheFaceItemsWriteThePairedFaces);
  testMethod(testTheLinesOfAnAxisRunThroughTheFaceNodes);
  testMethod(testOneDimensionalIdentityLeavesDataUnchanged);
  testMethod(testTwoDimensionalIdentityLeavesDataUnchanged);
  testMethod(testThreeDimensionalIdentityLeavesDataUnchanged);
  testMethod(testProjectionPreservesAConstantField);
  testMethod(testTheProjectionOfASwappedFieldIsTheSwappedProjection);
  testMethod(testTheProjectionOfAReflectedFieldIsTheReflectedProjection);
  testMethod(testBothProjectionRulesProjectTheSameField);
  testMethod(testEverySplitOfTheStagesAmongThreadsProjectsLikeOneThread);
  testMethod(testLagrangeBasisFunctionIsCardinal);
  testMethod(testTheLagrangeBasisFunctionsReproduceALinearFunction);
  testMethod(testTheMirrorBasisFunctionAtTheNegatedOffsetReturnsTheSameBits);
  testMethod(testInterpolationEvaluatesTheCoarsePolynomialAtTheChildNodes);
  testMethod(testRestrictionIsTheInterpolationTransposedOverTheChildWidth);
  testMethod(testWeightedRestrictionPreservesConstants);
  testMethod(testMatricesAreMirrorSymmetricToTheLastBit);
  testMethod(testAverageMirrorPairsKeepsSymmetricEntries);
  testMethod(testTransposePerChildSwapsTheNodeIndices);
  testMethod(testOrderZeroMatrices);
  testMethod(testFaceInterpolationReproducesALinearProfile);
  testMethod(testCellInterpolationReproducesALinearState);
  testMethod(testAConstantFaceAndCellInterpolateBitwise);
  testMethod(testFaceRestrictionProjectsALinearProfile);
  testMethod(testCellRestrictionProjectsALinearState);
  testMethod(testConstantFacesAndCellsRestrictBitwise);
  testMethod(testANegatedFieldTransfersToTheNegatedFieldBitwise);
}

/**
 * A cell has one node per combination of axis nodes and a face has one per combination of
 * tangential nodes.
 */
void
exahype2::dg::tests::BasisTest::testNodeCountsFollowTheNumberOfNodesPerAxis() {
  int nodesInCell = 0;
  peano4::dForEach(NodesPerAxis, [&]([[maybe_unused]] const tarch::la::Vector<DIMENSIONS, int>& node) {
    nodesInCell++;
  });

  validateEquals(exahype2::dg::getNodesPerCell(NodesPerAxis), nodesInCell);
  validateEquals(exahype2::dg::getNodesPerFace(NodesPerAxis), nodesInCell / NodesPerAxis);
}

/**
 * A node of the state and a node of an unknown field grow to the next multiple of the vector width.
 */
void
exahype2::dg::tests::BasisTest::testEveryNodeIsPaddedToTheVectorWidth() {
  validateEquals(exahype2::dg::getPaddedCountOf(0), 0);
  validateEquals(exahype2::dg::getPaddedCountOf(1), PEANO_WITH_SIMDLEN);
  validateEquals(exahype2::dg::getPaddedCountOf(PEANO_WITH_SIMDLEN), PEANO_WITH_SIMDLEN);
  validateEquals(exahype2::dg::getPaddedCountOf(PEANO_WITH_SIMDLEN + 1), 2 * PEANO_WITH_SIMDLEN);

  validateEquals(PaddedCell::NodesPerAxis, NodesPerAxis);
  validateEquals(PaddedCell::NumberOfUnknowns, NumberOfUnknowns);
  validateEquals(PaddedCell::NumberOfData, NumberOfData);

  validateEquals(PaddedCell::PaddedNumberOfUnknowns % PEANO_WITH_SIMDLEN, 0);
  validate(PaddedCell::PaddedNumberOfUnknowns >= NumberOfUnknowns);
  validate(PaddedCell::PaddedNumberOfUnknowns - NumberOfUnknowns < PEANO_WITH_SIMDLEN);

  validateEquals(PaddedCell::PaddedNumberOfData % PEANO_WITH_SIMDLEN, 0);
  validate(PaddedCell::PaddedNumberOfData >= NumberOfData);
  validate(PaddedCell::PaddedNumberOfData - NumberOfData < PEANO_WITH_SIMDLEN);

  using ExactWidth = exahype2::dg::PaddedLayout<ExactWidthLayout>;
  validateEquals(ExactWidth::NodesPerAxis, NodesPerAxis);
  validateEquals(ExactWidth::PaddedNumberOfUnknowns, PEANO_WITH_SIMDLEN);
  validateEquals(ExactWidth::PaddedNumberOfData, PEANO_WITH_SIMDLEN);

  using OneAboveWidth = exahype2::dg::PaddedLayout<OneAboveWidthLayout>;
  validateEquals(OneAboveWidth::NodesPerAxis, NodesPerAxis);
  validateEquals(OneAboveWidth::PaddedNumberOfUnknowns, 2 * PEANO_WITH_SIMDLEN);
  validateEquals(OneAboveWidth::PaddedNumberOfData, 2 * PEANO_WITH_SIMDLEN);
}

/**
 * The node counts of PaddedLayout follow the dimension count and a space-time block has one cell
 * per time node.
 */
void
exahype2::dg::tests::BasisTest::testTheNodeCountsFollowTheDimensionCount() {
  constexpr int ExpectedNodesPerAxisIn3D = (DIMENSIONS > 2) ? NodesPerAxis : 1;
  validateEquals(PaddedCell::NodesPerAxisIn3D, ExpectedNodesPerAxisIn3D);
  validateEquals(PaddedCell::NodesPerFace, FaceNodes);
  validateEquals(PaddedCell::NodesPerCell, CellNodes);
  validateEquals(PaddedCell::SpaceTimeNodes, CellNodes * NodesPerAxis);
  validateEquals(PaddedCell::NodesPerCell, PaddedCell::NodesPerFace * NodesPerAxis);
}

/**
 * A default-constructed table bundle stores null pointers and every field keeps the table that the test assigns.
 */
void
exahype2::dg::tests::BasisTest::testTheBasisTablesPointNowhereUntilTheyAreSet() {
  constexpr int NumberOfTables             = 11;
  const double  stubTables[NumberOfTables] = {};

  const exahype2::dg::BasisTables<double> emptyTables;
  validateEquals(emptyTables.quadraturePoints, static_cast<const double*>(nullptr));
  validateEquals(emptyTables.quadratureWeights, static_cast<const double*>(nullptr));
  validateEquals(emptyTables.inverseQuadratureWeights, static_cast<const double*>(nullptr));
  validateEquals(emptyTables.cellQuadratureWeights, static_cast<const double*>(nullptr));
  validateEquals(emptyTables.inverseCellQuadratureWeights, static_cast<const double*>(nullptr));
  validateEquals(emptyTables.stiffnessOperator, static_cast<const double*>(nullptr));
  validateEquals(emptyTables.stiffnessOperatorTransposed, static_cast<const double*>(nullptr));
  validateEquals(emptyTables.derivativeOperator, static_cast<const double*>(nullptr));
  validateEquals(emptyTables.inversePicardOperator, static_cast<const double*>(nullptr));
  validateEquals(emptyTables.leftFaceCoefficients, static_cast<const double*>(nullptr));
  validateEquals(emptyTables.rightFaceCoefficients, static_cast<const double*>(nullptr));

  exahype2::dg::BasisTables<double> tables;
  tables.quadraturePoints             = stubTables + 0;
  tables.quadratureWeights            = stubTables + 1;
  tables.inverseQuadratureWeights     = stubTables + 2;
  tables.cellQuadratureWeights        = stubTables + 3;
  tables.inverseCellQuadratureWeights = stubTables + 4;
  tables.stiffnessOperator            = stubTables + 5;
  tables.stiffnessOperatorTransposed  = stubTables + 6;
  tables.derivativeOperator           = stubTables + 7;
  tables.inversePicardOperator        = stubTables + 8;
  tables.leftFaceCoefficients         = stubTables + 9;
  tables.rightFaceCoefficients        = stubTables + 10;

  validateEquals(tables.quadraturePoints, stubTables + 0);
  validateEquals(tables.quadratureWeights, stubTables + 1);
  validateEquals(tables.inverseQuadratureWeights, stubTables + 2);
  validateEquals(tables.cellQuadratureWeights, stubTables + 3);
  validateEquals(tables.inverseCellQuadratureWeights, stubTables + 4);
  validateEquals(tables.stiffnessOperator, stubTables + 5);
  validateEquals(tables.stiffnessOperatorTransposed, stubTables + 6);
  validateEquals(tables.derivativeOperator, stubTables + 7);
  validateEquals(tables.inversePicardOperator, stubTables + 8);
  validateEquals(tables.leftFaceCoefficients, stubTables + 9);
  validateEquals(tables.rightFaceCoefficients, stubTables + 10);

  const exahype2::dg::BasisTables<float> narrowTables;
  validateEquals(narrowTables.quadraturePoints, static_cast<const float*>(nullptr));
}

/**
 * A reference node maps onto the cell offset plus the node's fraction of the cell size.
 */
void
exahype2::dg::tests::BasisTest::testGetQuadraturePointMapsTheReferenceNode() {
  const auto                         cellCentre = getRampOverAxes(0.4, 0.2);
  const auto                         cellSize   = getRampOverAxes(0.2, 0.2);
  tarch::la::Vector<DIMENSIONS, int> index;
  for (int axis = 0; axis < DIMENSIONS; axis++) {
    index(axis) = (2 * axis) % 3;
  }

  const auto position = exahype2::dg::getQuadraturePoint(cellCentre, cellSize, index, QuadratureNodes1DP2);
  for (int axis = 0; axis < DIMENSIONS; axis++) {
    const double expected = cellCentre(axis) + (QuadratureNodes1DP2[index(axis)] - 0.5) * cellSize(axis);
    validateNumericalEqualsWithParams1(position(axis), expected, axis);
  }

  // getQuadraturePoint() widens a table in another precision to double before it scales the cell.
  const float narrowNodes[]  = {0.25F, 0.5F, 0.75F};
  const auto  narrowPosition = exahype2::dg::getQuadraturePoint(cellCentre, cellSize, index, narrowNodes);
  for (int axis = 0; axis < DIMENSIONS; axis++) {
    const double expected = cellCentre(axis) + (static_cast<double>(narrowNodes[index(axis)]) - 0.5) * cellSize(axis);
    validateNumericalEqualsWithParams1(narrowPosition(axis), expected, axis);
  }
}

/**
 * forEachNode() visits every node in order with its position, the cell size and its quadrature
 * weight.
 */
void
exahype2::dg::tests::BasisTest::testVisitsEveryNodeWithItsQuadratureWeight() {
  const tarch::la::Vector<DIMENSIONS, double> cellCentre(1.0);
  const tarch::la::Vector<DIMENSIONS, double> cellSize(WalkCellWidth);
  std::array<double, WalkEntries>             values{};
  int                                         visits = 0;

  exahype2::dg::forEachNode<WalkNodesPerAxis, WalkValues>(
    cellCentre,
    cellSize,
    WalkPoints,
    WalkWeights,
    values.data(),
    [&](double* const value, const auto& x, const auto& h, const double weight, const int index) {
      validateEquals(index, visits);
      validateEquals(value - values.data(), static_cast<std::ptrdiff_t>(index) * WalkValues);
      int remainingIndex = index;
      for (int axis = 0; axis < DIMENSIONS; axis++) {
        validateNumericalEqualsWithParams2(x(axis), 0.5 + (remainingIndex % WalkNodesPerAxis) * 1.0, index, axis);
        validateNumericalEqualsWithParams2(h(axis), WalkCellWidth, index, axis);
        remainingIndex /= WalkNodesPerAxis;
      }
      const double cellVolume = std::pow(WalkCellWidth, DIMENSIONS);
      validateNumericalEquals(weight, cellVolume / WalkNodes);
      visits++;
    }
  );
  validateEquals(visits, WalkNodes);
}

/**
 * A single-precision buffer reaches the functor as a pointer into that buffer.
 */
void
exahype2::dg::tests::BasisTest::testReadsFloatValuesInPlace() {
  const tarch::la::Vector<DIMENSIONS, double> cellCentre(1.0);
  const tarch::la::Vector<DIMENSIONS, double> cellSize(WalkCellWidth);
  std::array<float, WalkEntries>              values{};
  for (int entry = 0; entry < WalkNodes * WalkValues; entry++) {
    values[entry] = static_cast<float>(entry);
  }
  const float* const constValues = values.data();
  double             sum         = 0.0;

  exahype2::dg::forEachNode<WalkNodesPerAxis, WalkValues>(
    cellCentre,
    cellSize,
    WalkPoints,
    WalkWeights,
    constValues,
    [&](
      const float* const            value,
      [[maybe_unused]] const auto&  x,
      [[maybe_unused]] const auto&  h,
      [[maybe_unused]] const double weight,
      const int                     index
    ) {
      validateNumericalEquals(static_cast<double>(value[0]), static_cast<double>(index * WalkValues));
      sum += value[WalkValues - 1];
    }
  );
  double expected = 0.0;
  for (int node = 0; node < WalkNodes; node++) {
    expected += node * WalkValues + WalkValues - 1;
  }
  validateNumericalEquals(sum, expected);
}

/**
 * A polynomial of order one returns the affine field it interpolates at every point of the cell.
 */
void
exahype2::dg::tests::BasisTest::testEvaluatePolynomialOrder1() {
  validateAffineInterpolant<1>(QuadratureNodes1DP1);
}

/**
 * A polynomial of order two returns the affine field it interpolates at every point of the cell.
 */
void
exahype2::dg::tests::BasisTest::testEvaluatePolynomialOrder2() {
  validateAffineInterpolant<2>(QuadratureNodes1DP2);
}

/**
 * Every reference is the midpoint of the value range of its entry, every bound the larger of its
 * magnitude and the weight times the width of the range, only an entry with a range wider than zero
 * varies, and negated nodes negate the references.
 */
void
exahype2::dg::tests::BasisTest::testTheReferencesAndBoundsFollowTheValueRanges() {
  constexpr int    Values                      = 2;
  constexpr int    Nodes                       = 3;
  constexpr double LargestWeight               = 0.25;
  const float      first[Nodes * Values]       = {4.0F, -1.0F, 2.5F, 7.0F, 3.0F, 0.5F};
  const float      second[Nodes * Values]      = {0.1F, 1.0F, 5.0F, 2.0F, 3.0F, 0.3F};
  const float      negated[Nodes * Values]     = {-3.0F, -0.3F, -5.0F, -2.0F, -0.1F, -1.0F};
  const float      negatedRest[Nodes * Values] = {-3.0F, -0.5F, -2.5F, -7.0F, -4.0F, 1.0F};
  const float      oneEntry[Nodes * Values]    = {2.0F, -1.0F, 2.0F, 6.0F, 2.0F, 0.5F};
  const float*     arrays[2]                   = {first, second};
  const float*     negatedArrays[2]            = {negated, negatedRest};
  double           reference[Values]           = {};
  double           bound[Values]               = {};
  bool             varies[Values]              = {};
  double           negatedReference[Values]    = {};
  double           negatedBound[Values]        = {};
  bool             negatedVaries[Values]       = {};
  exahype2::dg::computeReferencesAndBoundsOfTransferTerms<Values>(
    reference,
    bound,
    varies,
    arrays,
    2,
    Nodes,
    LargestWeight
  );
  exahype2::dg::computeReferencesAndBoundsOfTransferTerms<Values>(
    negatedReference,
    negatedBound,
    negatedVaries,
    negatedArrays,
    2,
    Nodes,
    LargestWeight
  );
  validateEquals(reference[0], 0.5 * (static_cast<double>(0.1F) + 5.0));
  validateEquals(reference[1], 0.5 * (-1.0 + 7.0));
  validateEquals(bound[0], 0.5 * (static_cast<double>(0.1F) + 5.0));
  validateEquals(bound[1], 3.0);
  validate(varies[0] and varies[1]);
  validateEquals(negatedReference[0], -reference[0]);
  validateEquals(negatedReference[1], -reference[1]);
  validateEquals(negatedBound[0], bound[0]);
  validateEquals(negatedBound[1], bound[1]);
  validate(negatedVaries[0] and negatedVaries[1]);

  const float* constantArrays[1] = {second};
  exahype2::dg::computeReferencesAndBoundsOfTransferTerms<Values>(reference, bound, varies, constantArrays, 1, 1, 4.0);
  validateEquals(reference[0], static_cast<double>(0.1F));
  validateEquals(bound[0], static_cast<double>(0.1F));
  validate(not varies[0] and not varies[1]);

  const float* oneEntryArrays[1] = {oneEntry};
  exahype2::dg::computeReferencesAndBoundsOfTransferTerms<Values>(
    reference,
    bound,
    varies,
    oneEntryArrays,
    1,
    Nodes,
    4.0
  );
  validateEquals(reference[0], 2.0);
  validate(not varies[0] and varies[1]);
}

/**
 * The largest product of entries is the power of the largest magnitude among the entries.
 */
void
exahype2::dg::tests::BasisTest::testTheLargestProductOfEntriesIsThePowerOfTheLargestMagnitude() {
  const double matrix[4] = {0.5, -1.5, 0.25, 1.0};
  validateEquals(exahype2::dg::getLargestProductOfEntries<0>(matrix, 4), 1.0);
  validateEquals(exahype2::dg::getLargestProductOfEntries<1>(matrix, 4), 1.5);
  validateEquals(exahype2::dg::getLargestProductOfEntries<3>(matrix, 4), 1.5 * 1.5 * 1.5);
}

void
exahype2::dg::tests::BasisTest::testProductMatchesTheDefinition() {
  double values[NumberOfSourceNodes * NumberOfEntries];
  double operatorMatrix[NumberOfTargetNodes * NumberOfSourceNodes];
  fillOperands(values, operatorMatrix);

  double result[NumberOfTargetNodes * NumberOfEntries] = {};

  exahype2::dg::applyOperatorAlongAxis<
    NumberOfEntries,
    NumberOfTargetNodes,
    NumberOfSourceNodes,
    NumberOfEntries,
    NumberOfSourceNodes,
    NumberOfEntries,
    true,
    double,
    double
  >(result, values, operatorMatrix, 1.0);

  for (int targetNode = 0; targetNode < NumberOfTargetNodes; targetNode++) {
    for (int entry = 0; entry < NumberOfEntries; entry++) {
      double expected = 0.0;
      for (int sourceNode = 0; sourceNode < NumberOfSourceNodes; sourceNode++) {
        expected +=
          values[sourceNode * NumberOfEntries + entry] * operatorMatrix[targetNode * NumberOfSourceNodes + sourceNode];
      }
      validateNumericalEquals(result[targetNode * NumberOfEntries + entry], expected);
    }
  }
}

void
exahype2::dg::tests::BasisTest::testResultIsAccumulatedOrOverwritten() {
  double values[NumberOfSourceNodes * NumberOfEntries];
  double operatorMatrix[NumberOfTargetNodes * NumberOfSourceNodes];
  fillOperands(values, operatorMatrix);

  double accumulated[NumberOfTargetNodes * NumberOfEntries];
  double overwritten[NumberOfTargetNodes * NumberOfEntries];
  for (int entry = 0; entry < NumberOfTargetNodes * NumberOfEntries; entry++) {
    accumulated[entry] = 5.0 + entry;
    overwritten[entry] = 5.0 + entry;
  }

  exahype2::dg::applyOperatorAlongAxis<
    NumberOfEntries,
    NumberOfTargetNodes,
    NumberOfSourceNodes,
    NumberOfEntries,
    NumberOfSourceNodes,
    NumberOfEntries,
    false,
    double,
    double
  >(accumulated, values, operatorMatrix, 1.0);
  exahype2::dg::applyOperatorAlongAxis<
    NumberOfEntries,
    NumberOfTargetNodes,
    NumberOfSourceNodes,
    NumberOfEntries,
    NumberOfSourceNodes,
    NumberOfEntries,
    true,
    double,
    double
  >(overwritten, values, operatorMatrix, 1.0);

  for (int entry = 0; entry < NumberOfTargetNodes * NumberOfEntries; entry++) {
    validateNumericalEquals(accumulated[entry], overwritten[entry] + 5.0 + entry);
  }
}

void
exahype2::dg::tests::BasisTest::testScalingMultipliesEveryProduct() {
  double values[NumberOfSourceNodes * NumberOfEntries];
  double operatorMatrix[NumberOfTargetNodes * NumberOfSourceNodes];
  fillOperands(values, operatorMatrix);

  double unscaled[NumberOfTargetNodes * NumberOfEntries] = {};
  double scaled[NumberOfTargetNodes * NumberOfEntries]   = {};

  exahype2::dg::applyOperatorAlongAxis<
    NumberOfEntries,
    NumberOfTargetNodes,
    NumberOfSourceNodes,
    NumberOfEntries,
    NumberOfSourceNodes,
    NumberOfEntries,
    true,
    double,
    double
  >(unscaled, values, operatorMatrix, 1.0);
  exahype2::dg::applyOperatorAlongAxis<
    NumberOfEntries,
    NumberOfTargetNodes,
    NumberOfSourceNodes,
    NumberOfEntries,
    NumberOfSourceNodes,
    NumberOfEntries,
    true,
    double,
    double
  >(scaled, values, operatorMatrix, -3.0);

  for (int entry = 0; entry < NumberOfTargetNodes * NumberOfEntries; entry++) {
    validateNumericalEquals(scaled[entry], -3.0 * unscaled[entry]);
  }
}

void
exahype2::dg::tests::BasisTest::testStridesAddressTheIntendedSlices() {
  constexpr int ValueStride    = NumberOfEntries + 2;
  constexpr int OperatorStride = NumberOfSourceNodes + 1;
  constexpr int ResultStride   = NumberOfEntries + 3;

  double values[NumberOfSourceNodes * ValueStride]            = {};
  double operatorMatrix[NumberOfTargetNodes * OperatorStride] = {};
  for (int sourceNode = 0; sourceNode < NumberOfSourceNodes; sourceNode++) {
    for (int entry = 0; entry < NumberOfEntries; entry++) {
      values[sourceNode * ValueStride + entry] = 1.0 + 0.25 * (sourceNode * NumberOfEntries + entry);
    }
  }
  for (int targetNode = 0; targetNode < NumberOfTargetNodes; targetNode++) {
    for (int sourceNode = 0; sourceNode < NumberOfSourceNodes; sourceNode++) {
      operatorMatrix[targetNode * OperatorStride + sourceNode] =
        2.0 - 0.125 * (targetNode * NumberOfSourceNodes + sourceNode);
    }
  }

  double result[NumberOfTargetNodes * ResultStride];
  for (int entry = 0; entry < NumberOfTargetNodes * ResultStride; entry++) {
    result[entry] = 7.0;
  }

  exahype2::dg::applyOperatorAlongAxis<
    NumberOfEntries,
    NumberOfTargetNodes,
    NumberOfSourceNodes,
    ValueStride,
    OperatorStride,
    ResultStride,
    true,
    double,
    double
  >(result, values, operatorMatrix, 1.0);

  for (int targetNode = 0; targetNode < NumberOfTargetNodes; targetNode++) {
    for (int entry = 0; entry < NumberOfEntries; entry++) {
      double expected = 0.0;
      for (int sourceNode = 0; sourceNode < NumberOfSourceNodes; sourceNode++) {
        expected += values[sourceNode * ValueStride + entry] * operatorMatrix[targetNode * OperatorStride + sourceNode];
      }
      validateNumericalEquals(result[targetNode * ResultStride + entry], expected);
    }
    // The padding between two result nodes stays unchanged.
    for (int entry = NumberOfEntries; entry < ResultStride; entry++) {
      validateNumericalEquals(result[targetNode * ResultStride + entry], 7.0);
    }
  }
}

void
exahype2::dg::tests::BasisTest::testANarrowerResultIsRoundedOnce() {
  // A product that a float cannot represent shows whether the accumulation runs in the wider type
  // before the store.
  constexpr double Operand = 1.0 + 1.0 / 1048576.0;

  const double values[1]         = {Operand};
  const double operatorMatrix[1] = {Operand};
  float        result[1]         = {0.0F};

  exahype2::dg::applyOperatorAlongAxis<1, 1, 1, 1, 1, 1, false, float, double>(result, values, operatorMatrix, 1.0);

  const auto expected = static_cast<double>(static_cast<float>(Operand * Operand));
  validateNumericalEquals(static_cast<double>(result[0]), expected);
}

/**
 * The half sum and the half difference of the weights of a mirror node pair recombine to the
 * weights of both nodes.
 */
void
exahype2::dg::tests::BasisTest::testThePairWeightsRecombineToTheOperatorRows() {
  const DerivativeOperator derivativeOperator;
  const auto               derivativeWeights =
    exahype2::dg::getDerivativeWeightsOfNodePairs<NodesPerAxis>(derivativeOperator.entries);

  for (int row = 0; row < NodesPerAxis; row++) {
    for (int pair = 0; pair < NodesPerAxis / 2; pair++) {
      validateNumericalEqualsWithParams2(
        derivativeWeights.sumWeights[row][pair] + derivativeWeights.differenceWeights[row][pair],
        derivativeOperator.entries[row * NodesPerAxis + pair],
        row,
        pair
      );
      validateNumericalEqualsWithParams2(
        derivativeWeights.sumWeights[row][pair] - derivativeWeights.differenceWeights[row][pair],
        derivativeOperator.entries[row * NodesPerAxis + NodesPerAxis - 1 - pair],
        row,
        pair
      );
    }
  }
}

void
exahype2::dg::tests::BasisTest::testTheDerivativeOfAConstantLineIsExactlyZero() {
  const DerivativeOperator derivativeOperator;

  // A rounding error shows for constants without an exact binary representation.
  double values[NodesPerAxis * NumberOfEntries];
  for (int node = 0; node < NodesPerAxis; node++) {
    for (int entry = 0; entry < NumberOfEntries; entry++) {
      values[node * NumberOfEntries + entry] = 1.0 / 3.0 + 0.1 * entry;
    }
  }

  double result[NodesPerAxis * NumberOfEntries];
  for (int entry = 0; entry < NodesPerAxis * NumberOfEntries; entry++) {
    result[entry] = 7.0;
  }

  exahype2::dg::differentiateAlongAxis<NumberOfEntries, NodesPerAxis, true, double, double>(
    result,
    NumberOfEntries,
    values,
    NumberOfEntries,
    exahype2::dg::getDerivativeWeightsOfNodePairs<NodesPerAxis>(derivativeOperator.entries),
    3.7,
    1.0 / 0.3
  );

  for (int entry = 0; entry < NodesPerAxis * NumberOfEntries; entry++) {
    validateEqualsWithParams1(result[entry], 0.0, entry);
  }
}

void
exahype2::dg::tests::BasisTest::testTheDerivativeActsOnTheDifferencesFromTheCentreNode() {
  const DerivativeOperator derivativeOperator;
  constexpr int            CentreNode = NodesPerAxis / 2;

  // The line falls along the axis.
  double values[NodesPerAxis * NumberOfEntries];
  for (int node = 0; node < NodesPerAxis; node++) {
    for (int entry = 0; entry < NumberOfEntries; entry++) {
      values[node * NumberOfEntries + entry] = 2.0 + entry - (1.5 + entry) * QuadratureNodes[node];
    }
  }
  double differences[NodesPerAxis * NumberOfEntries];
  for (int node = 0; node < NodesPerAxis; node++) {
    for (int entry = 0; entry < NumberOfEntries; entry++) {
      differences[node * NumberOfEntries + entry] =
        values[node * NumberOfEntries + entry] - values[CentreNode * NumberOfEntries + entry];
    }
  }

  double accumulated[NodesPerAxis * NumberOfEntries];
  double expected[NodesPerAxis * NumberOfEntries];
  for (int entry = 0; entry < NodesPerAxis * NumberOfEntries; entry++) {
    accumulated[entry] = 5.0 + entry;
    expected[entry]    = 5.0 + entry;
  }

  exahype2::dg::differentiateAlongAxis<NumberOfEntries, NodesPerAxis, false, double, double>(
    accumulated,
    NumberOfEntries,
    values,
    NumberOfEntries,
    exahype2::dg::getDerivativeWeightsOfNodePairs<NodesPerAxis>(derivativeOperator.entries),
    0.25
  );
  exahype2::dg::applyOperatorAlongAxis<
    NumberOfEntries,
    NodesPerAxis,
    NodesPerAxis,
    NumberOfEntries,
    NodesPerAxis,
    NumberOfEntries,
    false,
    double,
    double
  >(expected, differences, derivativeOperator.entries, 0.25);

  for (int node = 0; node < NodesPerAxis; node++) {
    for (int entry = 0; entry < NumberOfEntries; entry++) {
      validateEqualsWithParams2(
        accumulated[node * NumberOfEntries + entry],
        expected[node * NumberOfEntries + entry],
        node,
        entry
      );
      validateNumericalEqualsWithParams2(
        accumulated[node * NumberOfEntries + entry],
        5.0 + node * NumberOfEntries + entry - 0.25 * (1.5 + entry),
        node,
        entry
      );
    }
  }
}

/**
 * A derivative that overwrites or accumulates into its own line returns the bits of the same
 * derivative into another buffer.
 */
void
exahype2::dg::tests::BasisTest::testADerivativeInPlaceEqualsADerivativeIntoAnotherBuffer() {
  const DerivativeOperator derivativeOperator;
  const auto               derivativeWeights =
    exahype2::dg::getDerivativeWeightsOfNodePairs<NodesPerAxis>(derivativeOperator.entries);

  double values[NodesPerAxis * NumberOfEntries];
  for (int node = 0; node < NodesPerAxis; node++) {
    for (int entry = 0; entry < NumberOfEntries; entry++) {
      values[node * NumberOfEntries + entry] = 1.0 / 7.0 * (1 + entry) + 0.3 * node * node - 0.1 * entry * node;
    }
  }
  double overwritten[NodesPerAxis * NumberOfEntries] = {};
  double accumulated[NodesPerAxis * NumberOfEntries];
  double overwrittenInPlace[NodesPerAxis * NumberOfEntries];
  double accumulatedInPlace[NodesPerAxis * NumberOfEntries];
  std::copy(std::begin(values), std::end(values), std::begin(accumulated));
  std::copy(std::begin(values), std::end(values), std::begin(overwrittenInPlace));
  std::copy(std::begin(values), std::end(values), std::begin(accumulatedInPlace));

  exahype2::dg::differentiateAlongAxis<NumberOfEntries, NodesPerAxis, true, double, double>(
    overwritten,
    NumberOfEntries,
    values,
    NumberOfEntries,
    derivativeWeights,
    2.5
  );
  exahype2::dg::differentiateAlongAxis<NumberOfEntries, NodesPerAxis, false, double, double>(
    accumulated,
    NumberOfEntries,
    values,
    NumberOfEntries,
    derivativeWeights,
    2.5
  );
  exahype2::dg::differentiateAlongAxis<NumberOfEntries, NodesPerAxis, true, double, double>(
    overwrittenInPlace,
    NumberOfEntries,
    overwrittenInPlace,
    NumberOfEntries,
    derivativeWeights,
    2.5
  );
  exahype2::dg::differentiateAlongAxis<NumberOfEntries, NodesPerAxis, false, double, double>(
    accumulatedInPlace,
    NumberOfEntries,
    accumulatedInPlace,
    NumberOfEntries,
    derivativeWeights,
    2.5
  );

  for (int entry = 0; entry < NodesPerAxis * NumberOfEntries; entry++) {
    validateEqualsWithParams1(overwrittenInPlace[entry], overwritten[entry], entry);
    validateEqualsWithParams1(accumulatedInPlace[entry], accumulated[entry], entry);
  }
}

/**
 * A line reflected along its axis, with or without the sign of its values swapped, returns the
 * reflected derivative with the opposite sign change bitwise, for two to five nodes.
 */
void
exahype2::dg::tests::BasisTest::testTheDerivativeOfAReflectedLineIsTheReflectedDerivative() {
  // The reflection of the rows and the columns negates every operator.
  constexpr double TwoNodeOperator[4]   = {-1.1, 1.1, -1.1, 1.1};
  constexpr double ThreeNodeOperator[9] = {-1.7, 2.3, -0.6, -0.45, 0.0, 0.45, 0.6, -2.3, 1.7};
  constexpr double FourNodeOperator[16] =
    {-2.7, 3.9, -1.7, 0.5, -0.8, -0.3, 1.4, -0.3, 0.3, -1.4, 0.3, 0.8, -0.5, 1.7, -3.9, 2.7};
  constexpr double FiveNodeOperator[25] = {-5.1, 6.9,  -2.9, 1.3, -0.2, -1.2, -0.6, 2.4, -0.8, 0.2, 0.4,  -1.3, 0.0,
                                           1.3,  -0.4, -0.2, 0.8, -2.4, 0.6,  1.2,  0.2, -1.3, 2.9, -6.9, 5.1};
  validateReflectedDerivative<2>(TwoNodeOperator);
  validateReflectedDerivative<3>(ThreeNodeOperator);
  validateReflectedDerivative<4>(FourNodeOperator);
  validateReflectedDerivative<5>(FiveNodeOperator);
}

template <int Nodes>
void
exahype2::dg::tests::BasisTest::validateReflectedDerivative(const double* const operatorMatrix) {
  constexpr int Entries = PEANO_WITH_SIMDLEN;
  for (const double sign : {1.0, -1.0}) {
    double values[Nodes * Entries];
    double reflected[Nodes * Entries];
    for (int node = 0; node < Nodes; node++) {
      for (int entry = 0; entry < Entries; entry++) {
        values[node * Entries + entry]                  = 0.1 * (1 + 3 * node) - 0.7 * entry + 0.013 * node * node;
        reflected[(Nodes - 1 - node) * Entries + entry] = sign * values[node * Entries + entry];
      }
    }

    const auto derivativeWeights = exahype2::dg::getDerivativeWeightsOfNodePairs<Nodes>(operatorMatrix);
    double     derivative[Nodes * Entries];
    double     reflectedDerivative[Nodes * Entries];
    exahype2::dg::differentiateAlongAxis<Entries, Nodes, true, double, double>(
      derivative,
      Entries,
      values,
      Entries,
      derivativeWeights,
      0.5
    );
    exahype2::dg::differentiateAlongAxis<Entries, Nodes, true, double, double>(
      reflectedDerivative,
      Entries,
      reflected,
      Entries,
      derivativeWeights,
      0.5
    );

    for (int node = 0; node < Nodes; node++) {
      for (int entry = 0; entry < Entries; entry++) {
        validateEqualsWithParams3(
          reflectedDerivative[(Nodes - 1 - node) * Entries + entry],
          -sign * derivative[node * Entries + entry],
          Nodes,
          node,
          entry
        );
      }
    }
  }
}

void
exahype2::dg::tests::BasisTest::testGradientOfALinearPolynomialIsItsSlope() {
  const DerivativeOperator derivativeOperator;
  const auto               cellSize = getCellSize();

  // Every value grows with its own slope along every axis.
  double cellState[CellNodes * NumberOfData];
  for (int cellNode = 0; cellNode < CellNodes; cellNode++) {
    const auto nodeIndex = peano4::dDelinearised(cellNode, NodesPerAxis);
    for (int value = 0; value < NumberOfData; value++) {
      double entry = 1.0 + value;
      for (int axis = 0; axis < DIMENSIONS; axis++) {
        entry += (2.0 + value + axis) * QuadratureNodes[nodeIndex(axis)];
      }
      cellState[cellNode * NumberOfData + value] = entry;
    }
  }

  double gradients[DIMENSIONS * EntriesPerAxis];
  exahype2::dg::computeNodalGradients<NodesPerAxis, NumberOfData, double, double>(
    gradients,
    cellState,
    derivativeOperator.entries,
    cellSize
  );

  // The reference slope over the cell extent is the physical slope.
  for (int axis = 0; axis < DIMENSIONS; axis++) {
    for (int cellNode = 0; cellNode < CellNodes; cellNode++) {
      for (int value = 0; value < NumberOfData; value++) {
        const double expected = (2.0 + value + axis) / cellSize(axis);
        validateNumericalEqualsWithParams3(
          gradients[axis * EntriesPerAxis + cellNode * NumberOfData + value],
          expected,
          axis,
          cellNode,
          value
        );
      }
    }
  }
}

void
exahype2::dg::tests::BasisTest::testEachBlockStoresTheDerivativeAlongItsOwnAxis() {
  const DerivativeOperator derivativeOperator;
  const auto               cellSize = getCellSize();

  for (int varyingAxis = 0; varyingAxis < DIMENSIONS; varyingAxis++) {
    double cellState[CellNodes * NumberOfData];
    for (int cellNode = 0; cellNode < CellNodes; cellNode++) {
      const double reference = QuadratureNodes[peano4::dDelinearised(cellNode, NodesPerAxis)(varyingAxis)];
      for (int value = 0; value < NumberOfData; value++) {
        cellState[cellNode * NumberOfData + value] = 3.0 * reference * reference;
      }
    }

    double gradients[DIMENSIONS * EntriesPerAxis];
    exahype2::dg::computeNodalGradients<NodesPerAxis, NumberOfData, double, double>(
      gradients,
      cellState,
      derivativeOperator.entries,
      cellSize
    );

    for (int axis = 0; axis < DIMENSIONS; axis++) {
      for (int cellNode = 0; cellNode < CellNodes; cellNode++) {
        const double reference = QuadratureNodes[peano4::dDelinearised(cellNode, NodesPerAxis)(varyingAxis)];
        const double expected  = axis == varyingAxis ? 6.0 * reference / cellSize(axis) : 0.0;
        for (int value = 0; value < NumberOfData; value++) {
          validateNumericalEqualsWithParams3(
            gradients[axis * EntriesPerAxis + cellNode * NumberOfData + value],
            expected,
            varyingAxis,
            axis,
            cellNode
          );
        }
      }
    }
  }
}

void
exahype2::dg::tests::BasisTest::testNarrowStorageIsDifferentiatedInTheComputePrecision() {
  const DerivativeOperator derivativeOperator;
  const auto               cellSize = getCellSize();

  float cellState[CellNodes * NumberOfData];
  for (int cellNode = 0; cellNode < CellNodes; cellNode++) {
    const double reference = QuadratureNodes[peano4::dDelinearised(cellNode, NodesPerAxis)(0)];
    for (int value = 0; value < NumberOfData; value++) {
      cellState[cellNode * NumberOfData + value] = static_cast<float>(0.5 + 2.0 * reference);
    }
  }

  double gradients[DIMENSIONS * EntriesPerAxis];
  exahype2::dg::computeNodalGradients<NodesPerAxis, NumberOfData, float, double>(
    gradients,
    cellState,
    derivativeOperator.entries,
    cellSize
  );

  // The stored values round to single precision and the slope matches to that accuracy only. The
  // state varies along the first axis alone and the blocks of the other axes stay zero.
  constexpr double Tolerance = 1.0e-5;
  for (int axis = 0; axis < DIMENSIONS; axis++) {
    const double expected = (axis == 0) ? 2.0 / cellSize(0) : 0.0;
    for (int cellNode = 0; cellNode < CellNodes; cellNode++) {
      for (int value = 0; value < NumberOfData; value++) {
        const double slope = gradients[axis * EntriesPerAxis + cellNode * NumberOfData + value];
        validateWithParams3(std::abs(slope - expected) < Tolerance, slope, axis, cellNode);
      }
    }
  }
}

void
exahype2::dg::tests::BasisTest::testTheGradientOfAConstantStateIsExactlyZero() {
  const DerivativeOperator derivativeOperator;
  const auto               cellSize = getCellSize();

  // A rounding error shows for constants without an exact binary representation.
  double cellState[CellNodes * NumberOfData];
  for (int cellNode = 0; cellNode < CellNodes; cellNode++) {
    for (int value = 0; value < NumberOfData; value++) {
      cellState[cellNode * NumberOfData + value] = 1.0 / 3.0 + 0.1 * value;
    }
  }

  double gradients[DIMENSIONS * EntriesPerAxis];
  exahype2::dg::computeNodalGradients<NodesPerAxis, NumberOfData, double, double>(
    gradients,
    cellState,
    derivativeOperator.entries,
    cellSize
  );

  for (int entry = 0; entry < DIMENSIONS * EntriesPerAxis; entry++) {
    validateEqualsWithParams1(gradients[entry], 0.0, entry);
  }
}

/**
 * A flux that is linear along its own axis differentiates to its constant physical slope.
 */
void
exahype2::dg::tests::BasisTest::testALinearFluxDifferentiatesToItsPhysicalSlope() {
  const DerivativeOperator derivativeOperator;
  const auto               cellSize = getCellSize();

  tarch::la::Vector<DIMENSIONS, double> inverseCellSize;
  for (int axis = 0; axis < DIMENSIONS; axis++) {
    inverseCellSize(axis) = 1.0 / cellSize(axis);
  }

  double flux[DIMENSIONS * FluxAxisStride] = {};
  for (int axis = 0; axis < DIMENSIONS; axis++) {
    for (int cellNode = 0; cellNode < CellNodes; cellNode++) {
      const double reference = QuadratureNodes[peano4::dDelinearised(cellNode, NodesPerAxis)(axis)];
      for (int unknown = 0; unknown < NumberOfUnknowns; unknown++) {
        flux[axis * FluxAxisStride + cellNode * PaddedCell::PaddedNumberOfUnknowns + unknown] =
          1.0 + unknown + 3.0 * (1.0 + axis) * reference;
      }
    }
  }

  exahype2::dg::differentiateFluxInPlace<Order2Layout>(flux, derivativeOperator.entries, inverseCellSize);

  for (int axis = 0; axis < DIMENSIONS; axis++) {
    for (int cellNode = 0; cellNode < CellNodes; cellNode++) {
      const int base = axis * FluxAxisStride + cellNode * PaddedCell::PaddedNumberOfUnknowns;
      for (int unknown = 0; unknown < NumberOfUnknowns; unknown++) {
        validateNumericalEqualsWithParams3(
          flux[base + unknown],
          3.0 * (1.0 + axis) * inverseCellSize(axis),
          axis,
          cellNode,
          unknown
        );
      }
      for (int unknown = NumberOfUnknowns; unknown < PaddedCell::PaddedNumberOfUnknowns; unknown++) {
        validateNumericalEqualsWithParams3(flux[base + unknown], 0.0, axis, cellNode, unknown);
      }
    }
  }
}

/**
 * A flux that is constant along every axis differentiates to zeros bitwise.
 */
void
exahype2::dg::tests::BasisTest::testAConstantFluxDifferentiatesToExactZeros() {
  const DerivativeOperator derivativeOperator;
  const auto               cellSize = getCellSize();

  tarch::la::Vector<DIMENSIONS, double> inverseCellSize;
  for (int axis = 0; axis < DIMENSIONS; axis++) {
    inverseCellSize(axis) = 1.0 / cellSize(axis);
  }

  // A rounding error shows for constants without an exact binary representation.
  double flux[DIMENSIONS * FluxAxisStride] = {};
  for (int axis = 0; axis < DIMENSIONS; axis++) {
    for (int cellNode = 0; cellNode < CellNodes; cellNode++) {
      for (int unknown = 0; unknown < NumberOfUnknowns; unknown++) {
        flux[axis * FluxAxisStride + cellNode * PaddedCell::PaddedNumberOfUnknowns + unknown] =
          1.0 / 3.0 + 0.1 * unknown;
      }
    }
  }

  exahype2::dg::differentiateFluxInPlace<Order2Layout>(flux, derivativeOperator.entries, inverseCellSize);

  for (int entry = 0; entry < DIMENSIONS * FluxAxisStride; entry++) {
    validateEqualsWithParams1(flux[entry], 0.0, entry);
  }
}

/**
 * Two calls with the first lines zero and one and the line stride two differentiate the flux
 * bitwise like one call over all lines.
 */
void
exahype2::dg::tests::BasisTest::testTwoCallsOverAlternateLinesDifferentiateLikeOneCall() {
  const DerivativeOperator derivativeOperator;
  const auto               cellSize = getCellSize();

  tarch::la::Vector<DIMENSIONS, double> inverseCellSize;
  for (int axis = 0; axis < DIMENSIONS; axis++) {
    inverseCellSize(axis) = 1.0 / cellSize(axis);
  }

  double wholeFlux[DIMENSIONS * FluxAxisStride] = {};
  for (int axis = 0; axis < DIMENSIONS; axis++) {
    for (int cellNode = 0; cellNode < CellNodes; cellNode++) {
      for (int unknown = 0; unknown < NumberOfUnknowns; unknown++) {
        wholeFlux[axis * FluxAxisStride + cellNode * PaddedCell::PaddedNumberOfUnknowns + unknown] =
          1.0 / 7.0 * (1 + axis) + 0.3 * cellNode * cellNode - 0.1 * unknown;
      }
    }
  }
  double alternateFlux[DIMENSIONS * FluxAxisStride];
  std::copy(std::begin(wholeFlux), std::end(wholeFlux), std::begin(alternateFlux));

  exahype2::dg::differentiateFluxInPlace<Order2Layout>(wholeFlux, derivativeOperator.entries, inverseCellSize);
  for (int firstItem = 0; firstItem < 2; firstItem++) {
    exahype2::dg::differentiateFluxInPlace<Order2Layout>(
      alternateFlux,
      derivativeOperator.entries,
      inverseCellSize,
      firstItem,
      2
    );
  }

  for (int entry = 0; entry < DIMENSIONS * FluxAxisStride; entry++) {
    validateEqualsWithParams1(alternateFlux[entry], wholeFlux[entry], entry);
  }
}

void
exahype2::dg::tests::BasisTest::testAPaddedNodeStrideProjectsOnlyTheEntries() {
  // The padding lanes store a value that would dominate every face if the projection read them.
  double field[CellNodes * PaddedNodeStride];
  for (int cellNode = 0; cellNode < CellNodes; cellNode++) {
    for (int lane = 0; lane < PaddedNodeStride; lane++) {
      field[cellNode * PaddedNodeStride + lane] = (lane < NumberOfData) ? (1.0 + cellNode + 0.25 * lane) : 1000.0;
    }
  }

  double coefficients[NodesPerAxis];
  for (int index = 0; index < NodesPerAxis; index++) {
    coefficients[index] = 1.0 / NodesPerAxis;
  }

  for (int direction = 0; direction < DIMENSIONS; direction++) {
    double lowerFace[FaceNodes * NumberOfData];
    double upperFace[FaceNodes * NumberOfData];
    for (int entry = 0; entry < FaceNodes * NumberOfData; entry++) {
      lowerFace[entry] = 9.0;
      upperFace[entry] = 9.0;
    }

    exahype2::dg::extrapolateNodalFieldToFacePair<NodesPerAxis, NumberOfData, PaddedNodeStride, double, double, double>(
      lowerFace,
      upperFace,
      field,
      coefficients,
      coefficients,
      direction
    );

    for (int faceNode = 0; faceNode < FaceNodes; faceNode++) {
      for (int entry = 0; entry < NumberOfData; entry++) {
        double expected = 0.0;
        for (int normalIndex = 0; normalIndex < NodesPerAxis; normalIndex++) {
          const int cellNode = getCellNodeInAxisOrder(direction, faceNode, normalIndex);
          expected += field[cellNode * PaddedNodeStride + entry] / NodesPerAxis;
        }
        validateNumericalEqualsWithParams3(
          lowerFace[faceNode * NumberOfData + entry],
          expected,
          direction,
          faceNode,
          entry
        );
        validateNumericalEqualsWithParams3(
          upperFace[faceNode * NumberOfData + entry],
          expected,
          direction,
          faceNode,
          entry
        );
      }
    }
  }
}

void
exahype2::dg::tests::BasisTest::testTheLowerAndUpperFaceUseTheirOwnCoefficients() {
  double field[CellNodes * NumberOfData];
  for (int entry = 0; entry < CellNodes * NumberOfData; entry++) {
    field[entry] = 1.0 + 0.5 * entry;
  }

  const EndCoefficients coefficients;
  const double* const   leftCoefficients  = coefficients.left;
  const double* const   rightCoefficients = coefficients.right;

  for (int direction = 0; direction < DIMENSIONS; direction++) {
    double lowerFace[FaceNodes * NumberOfData] = {};
    double upperFace[FaceNodes * NumberOfData] = {};

    exahype2::dg::extrapolateNodalFieldToFacePair<NodesPerAxis, NumberOfData, NumberOfData, double, double, double>(
      lowerFace,
      upperFace,
      field,
      leftCoefficients,
      rightCoefficients,
      direction
    );

    for (int faceNode = 0; faceNode < FaceNodes; faceNode++) {
      for (int entry = 0; entry < NumberOfData; entry++) {
        double expectedLower = 0.0;
        double expectedUpper = 0.0;
        for (int normalIndex = 0; normalIndex < NodesPerAxis; normalIndex++) {
          const int    cellNode = getCellNodeInAxisOrder(direction, faceNode, normalIndex);
          const double value    = field[cellNode * NumberOfData + entry];
          expectedLower += value * leftCoefficients[normalIndex];
          expectedUpper += value * rightCoefficients[normalIndex];
        }
        validateNumericalEqualsWithParams3(
          lowerFace[faceNode * NumberOfData + entry],
          expectedLower,
          direction,
          faceNode,
          entry
        );
        validateNumericalEqualsWithParams3(
          upperFace[faceNode * NumberOfData + entry],
          expectedUpper,
          direction,
          faceNode,
          entry
        );
      }
    }
  }
}

/**
 * A field reflected along the first axis, with or without the sign of its values swapped, returns
 * the two faces swapped with that sign change bitwise, for two to five nodes.
 */
void
exahype2::dg::tests::BasisTest::testTheFacesOfAReflectedFieldAreTheSwappedFaces() {
  // The right coefficients are the reflected left coefficients, and both sum to one.
  constexpr double TwoNodeLeft[2]    = {0.7, 0.3};
  constexpr double TwoNodeRight[2]   = {0.3, 0.7};
  constexpr double ThreeNodeLeft[3]  = {0.77, 0.46, -0.23};
  constexpr double ThreeNodeRight[3] = {-0.23, 0.46, 0.77};
  constexpr double FourNodeLeft[4]   = {1.1, -0.3, 0.27, -0.07};
  constexpr double FourNodeRight[4]  = {-0.07, 0.27, -0.3, 1.1};
  constexpr double FiveNodeLeft[5]   = {1.3, -0.6, 0.45, -0.2, 0.05};
  constexpr double FiveNodeRight[5]  = {0.05, -0.2, 0.45, -0.6, 1.3};
  validateReflectedFaces<2>(TwoNodeLeft, TwoNodeRight);
  validateReflectedFaces<3>(ThreeNodeLeft, ThreeNodeRight);
  validateReflectedFaces<4>(FourNodeLeft, FourNodeRight);
  validateReflectedFaces<5>(FiveNodeLeft, FiveNodeRight);
}

template <int Nodes>
void
exahype2::dg::tests::BasisTest::validateReflectedFaces(
  const double* const leftCoefficients,
  const double* const rightCoefficients
) {
  constexpr int Entries       = 2;
  constexpr int NodesOfCell   = exahype2::dg::getNodesPerCell(Nodes);
  constexpr int NodesOfFace   = exahype2::dg::getNodesPerFace(Nodes);
  constexpr int ReflectedAxis = 0;
  for (const double sign : {1.0, -1.0}) {
    double field[NodesOfCell * Entries];
    double reflected[NodesOfCell * Entries];
    for (int cellNode = 0; cellNode < NodesOfCell; cellNode++) {
      auto mirroredNode           = peano4::dDelinearised(cellNode, Nodes);
      mirroredNode(ReflectedAxis) = Nodes - 1 - mirroredNode(ReflectedAxis);
      const int mirroredCellNode  = peano4::dLinearised(mirroredNode, Nodes);
      for (int entry = 0; entry < Entries; entry++) {
        field[cellNode * Entries + entry] = 0.1 * (1 + cellNode) - 0.7 * entry + 0.013 * cellNode * cellNode;
        reflected[mirroredCellNode * Entries + entry] = sign * field[cellNode * Entries + entry];
      }
    }

    double lowerFace[NodesOfFace * Entries];
    double upperFace[NodesOfFace * Entries];
    double lowerFaceOfReflected[NodesOfFace * Entries];
    double upperFaceOfReflected[NodesOfFace * Entries];
    exahype2::dg::extrapolateNodalFieldToFacePair<Nodes, Entries, Entries, double, double, double>(
      lowerFace,
      upperFace,
      field,
      leftCoefficients,
      rightCoefficients,
      ReflectedAxis
    );
    exahype2::dg::extrapolateNodalFieldToFacePair<Nodes, Entries, Entries, double, double, double>(
      lowerFaceOfReflected,
      upperFaceOfReflected,
      reflected,
      leftCoefficients,
      rightCoefficients,
      ReflectedAxis
    );

    for (int faceNode = 0; faceNode < NodesOfFace; faceNode++) {
      for (int entry = 0; entry < Entries; entry++) {
        validateEqualsWithParams3(
          lowerFaceOfReflected[faceNode * Entries + entry],
          sign * upperFace[faceNode * Entries + entry],
          Nodes,
          faceNode,
          entry
        );
        validateEqualsWithParams3(
          upperFaceOfReflected[faceNode * Entries + entry],
          sign * lowerFace[faceNode * Entries + entry],
          Nodes,
          faceNode,
          entry
        );
      }
    }
  }
}

void
exahype2::dg::tests::BasisTest::testTheFaceNodesRunWithTheLowestTangentialAxisFastest() {
  // Every cell node has its own value. A coefficient vector with a single one copies the value at
  // that line position to the face node.
  double field[CellNodes * NumberOfData];
  for (int cellNode = 0; cellNode < CellNodes; cellNode++) {
    for (int entry = 0; entry < NumberOfData; entry++) {
      field[cellNode * NumberOfData + entry] = 1.0 + cellNode + 0.125 * entry;
    }
  }

  for (int direction = 0; direction < DIMENSIONS; direction++) {
    for (int markedIndex = 0; markedIndex < NodesPerAxis; markedIndex++) {
      double leftCoefficients[NodesPerAxis]             = {};
      double rightCoefficients[NodesPerAxis]            = {};
      leftCoefficients[markedIndex]                     = 1.0;
      rightCoefficients[NodesPerAxis - 1 - markedIndex] = 1.0;

      double lowerFace[FaceNodes * NumberOfData] = {};
      double upperFace[FaceNodes * NumberOfData] = {};

      exahype2::dg::extrapolateNodalFieldToFacePair<NodesPerAxis, NumberOfData, NumberOfData, double, double, double>(
        lowerFace,
        upperFace,
        field,
        leftCoefficients,
        rightCoefficients,
        direction
      );

      for (int faceNode = 0; faceNode < FaceNodes; faceNode++) {
        const int lowerCellNode = getCellNodeInAxisOrder(direction, faceNode, markedIndex);
        const int upperCellNode = getCellNodeInAxisOrder(direction, faceNode, NodesPerAxis - 1 - markedIndex);
        for (int entry = 0; entry < NumberOfData; entry++) {
          validateNumericalEqualsWithParams3(
            lowerFace[faceNode * NumberOfData + entry],
            field[lowerCellNode * NumberOfData + entry],
            direction,
            markedIndex,
            faceNode
          );
          validateNumericalEqualsWithParams3(
            upperFace[faceNode * NumberOfData + entry],
            field[upperCellNode * NumberOfData + entry],
            direction,
            markedIndex,
            faceNode
          );
        }
      }
    }
  }
}

void
exahype2::dg::tests::BasisTest::testAFieldOfFloatAccumulatesTheFacesInDouble() {
  // Half the small value is half the distance between half the large value and its float
  // neighbour. A sum in float drops it and a sum in double keeps it.
  constexpr double LargeValue = 1024.0;
  constexpr double SmallValue = 0.00006103515625;

  // Both faces weigh the large node and one small node by one half each.
  const double leftCoefficients[NodesPerAxis]  = {0.5, 0.5, 0.0};
  const double rightCoefficients[NodesPerAxis] = {0.5, 0.0, 0.5};
  const double expected                        = 0.5 * LargeValue + 0.5 * SmallValue;

  for (int direction = 0; direction < DIMENSIONS; direction++) {
    float field[CellNodes * NumberOfData];
    for (int faceNode = 0; faceNode < FaceNodes; faceNode++) {
      for (int normalIndex = 0; normalIndex < NodesPerAxis; normalIndex++) {
        const int  cellNode = getCellNodeInAxisOrder(direction, faceNode, normalIndex);
        const auto value    = static_cast<float>((normalIndex == 0) ? LargeValue : SmallValue);
        for (int entry = 0; entry < NumberOfData; entry++) {
          field[cellNode * NumberOfData + entry] = value;
        }
      }
    }

    double lowerFace[FaceNodes * NumberOfData] = {};
    double upperFace[FaceNodes * NumberOfData] = {};

    exahype2::dg::extrapolateNodalFieldToFacePair<NodesPerAxis, NumberOfData, NumberOfData, float, double, double>(
      lowerFace,
      upperFace,
      field,
      leftCoefficients,
      rightCoefficients,
      direction
    );

    for (int entry = 0; entry < FaceNodes * NumberOfData; entry++) {
      validateEqualsWithParams2(lowerFace[entry], expected, direction, entry);
      validateEqualsWithParams2(upperFace[entry], expected, direction, entry);
    }
  }
}

/**
 * Threads that divide the items of extrapolateNodalFieldToFaces() write the faces of
 * extrapolateNodalFieldToFacePair() bitwise.
 */
void
exahype2::dg::tests::BasisTest::testThreadsThatDivideTheFaceItemsWriteThePairedFaces() {
  constexpr int Threads    = 3;
  constexpr int FieldSize  = CellNodes * NumberOfData;
  constexpr int FaceValues = FaceNodes * NumberOfData;
  double        field[DIMENSIONS * FieldSize];
  for (int entry = 0; entry < DIMENSIONS * FieldSize; entry++) {
    field[entry] = 1.0 + 0.5 * entry + 0.01 * entry * entry;
  }
  const EndCoefficients coefficients;

  double pairedFaces[peano4::FacesPerCell][FaceValues] = {};
  for (int direction = 0; direction < DIMENSIONS; direction++) {
    exahype2::dg::extrapolateNodalFieldToFacePair<NodesPerAxis, NumberOfData, NumberOfData, double, double, double>(
      pairedFaces[direction],
      pairedFaces[direction + DIMENSIONS],
      field + direction * FieldSize,
      coefficients.left,
      coefficients.right,
      direction
    );
  }

  double  faceValues[peano4::FacesPerCell][FaceValues] = {};
  double* faces[peano4::FacesPerCell];
  for (int face = 0; face < peano4::FacesPerCell; face++) {
    faces[face] = faceValues[face];
  }
  for (int thread = 0; thread < Threads; thread++) {
    exahype2::dg::extrapolateNodalFieldToFaces<
      NodesPerAxis,
      NumberOfData,
      NumberOfData,
      FieldSize,
      double,
      double,
      double
    >(faces, field, coefficients.left, coefficients.right, thread, Threads);
  }

  for (int face = 0; face < peano4::FacesPerCell; face++) {
    for (int entry = 0; entry < FaceValues; entry++) {
      validateEqualsWithParams2(faceValues[face][entry], pairedFaces[face][entry], face, entry);
    }
  }
}

/**
 * Line i of an axis runs through face node i of the faces normal to the axis.
 */
void
exahype2::dg::tests::BasisTest::testTheLinesOfAnAxisRunThroughTheFaceNodes() {
  for (int axis = 0; axis < DIMENSIONS; axis++) {
    for (int line = 0; line < FaceNodes; line++) {
      for (int normalIndex = 0; normalIndex < NodesPerAxis; normalIndex++) {
        validateEqualsWithParams3(
          exahype2::dg::getFirstNodeOfLine(NodesPerAxis, axis, line)
            + normalIndex * exahype2::dg::getNodeStrideOfAxis(NodesPerAxis, axis),
          getCellNodeInAxisOrder(axis, line, normalIndex),
          axis,
          line,
          normalIndex
        );
      }
    }
  }
}

void
exahype2::dg::tests::BasisTest::testOneDimensionalIdentityLeavesDataUnchanged() {
  constexpr int                           Dofs          = 2;
  constexpr int                           ValuesPerDof  = 2;
  constexpr auto                          MatrixEntries = static_cast<std::size_t>(Dofs) * Dofs;
  constexpr auto                          FieldEntries  = static_cast<std::size_t>(Dofs) * ValuesPerDof;
  const std::array<double, MatrixEntries> identity{1.0, 0.0, 0.0, 1.0};
  const std::array<double, FieldEntries>  input{1.0, 2.0, 3.0, 4.0};
  std::array<double, FieldEntries>        output{};

  exahype2::dg::projectTensorProduct<1, Dofs, Dofs, ValuesPerDof, exahype2::dg::ProjectionRule::PairedProducts>(
    input.data(),
    output.data(),
    identity.data()
  );

  for (int index = 0; index < Dofs * ValuesPerDof; index++) {
    validateNumericalEquals(output[index], input[index]);
  }
}

void
exahype2::dg::tests::BasisTest::testTwoDimensionalIdentityLeavesDataUnchanged() {
  constexpr int                           Dofs          = 2;
  constexpr auto                          MatrixEntries = static_cast<std::size_t>(Dofs) * Dofs;
  const std::array<double, MatrixEntries> identity{1.0, 0.0, 0.0, 1.0};
  const std::array<double, MatrixEntries> input{1.0, 2.0, 3.0, 4.0};
  std::array<double, MatrixEntries>       output{};

  exahype2::dg::projectTensorProduct<2, Dofs, Dofs, 1, exahype2::dg::ProjectionRule::PairedProducts>(
    input.data(),
    output.data(),
    identity.data()
  );

  for (int index = 0; index < Dofs * Dofs; index++) {
    validateNumericalEquals(output[index], input[index]);
  }
}

void
exahype2::dg::tests::BasisTest::testThreeDimensionalIdentityLeavesDataUnchanged() {
  constexpr int                           Dofs          = 2;
  constexpr auto                          MatrixEntries = static_cast<std::size_t>(Dofs) * Dofs;
  constexpr auto                          CellEntries   = static_cast<std::size_t>(Dofs) * Dofs * Dofs;
  const std::array<double, MatrixEntries> identity{1.0, 0.0, 0.0, 1.0};
  const std::array<double, CellEntries>   input{1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0};
  std::array<double, CellEntries>         output{};

  exahype2::dg::projectTensorProduct<3, Dofs, Dofs, 1, exahype2::dg::ProjectionRule::PairedProducts>(
    input.data(),
    output.data(),
    identity.data()
  );

  for (int index = 0; index < Dofs * Dofs * Dofs; index++) {
    validateNumericalEquals(output[index], input[index]);
  }
}

void
exahype2::dg::tests::BasisTest::testProjectionPreservesAConstantField() {
  constexpr int    Dofs          = 2;
  constexpr double Constant      = 7.0;
  constexpr auto   MatrixEntries = static_cast<std::size_t>(Dofs) * Dofs;
  // Each column sums to one. The projection then reproduces a constant field exactly.
  const std::array<double, MatrixEntries> columnStochasticMatrix{0.25, 0.75, 0.75, 0.25};
  const std::array<double, MatrixEntries> input{Constant, Constant, Constant, Constant};
  std::array<double, MatrixEntries>       pairedOutput{};
  std::array<double, MatrixEntries>       butterflyOutput{};

  exahype2::dg::projectTensorProduct<2, Dofs, Dofs, 1, exahype2::dg::ProjectionRule::PairedProducts>(
    input.data(),
    pairedOutput.data(),
    columnStochasticMatrix.data()
  );
  exahype2::dg::projectTensorProduct<2, Dofs, Dofs, 1, exahype2::dg::ProjectionRule::Butterfly>(
    input.data(),
    butterflyOutput.data(),
    columnStochasticMatrix.data()
  );

  for (int index = 0; index < Dofs * Dofs; index++) {
    validateNumericalEqualsWithParams1(pairedOutput[index], Constant, index);
    validateNumericalEqualsWithParams1(butterflyOutput[index], Constant, index);
  }
}

/**
 * The projection of a field with two swapped axes is the projection with these axes swapped,
 * bitwise, in 2D and 3D.
 */
void
exahype2::dg::tests::BasisTest::testTheProjectionOfASwappedFieldIsTheSwappedProjection() {
  const auto validateSwaps = [this]<int Dimensions, exahype2::dg::ProjectionRule Rule>() {
    const std::vector<double> input  = getProjectionInput<Dimensions>();
    const std::vector<double> output = project<Dimensions, Rule>(input);
    for (int first = 0; first < Dimensions; first++) {
      for (int second = first + 1; second < Dimensions; second++) {
        std::vector<double> swappedInput(input.size());
        for (int value = 0; value < static_cast<int>(input.size()); value++) {
          swappedInput[getMovedValue<Dimensions>(value, ProjectionInputDofs, first, second)] = input[value];
        }
        const std::vector<double> swappedOutput = project<Dimensions, Rule>(swappedInput);
        for (int value = 0; value < static_cast<int>(output.size()); value++) {
          validateEqualsWithParams4(
            swappedOutput[getMovedValue<Dimensions>(value, ProjectionOutputDofs, first, second)],
            output[value],
            Dimensions,
            first,
            second,
            value
          );
        }
      }
    }
  };
  validateSwaps.template operator()<2, exahype2::dg::ProjectionRule::PairedProducts>();
  validateSwaps.template operator()<3, exahype2::dg::ProjectionRule::PairedProducts>();
  validateSwaps.template operator()<2, exahype2::dg::ProjectionRule::Butterfly>();
  validateSwaps.template operator()<3, exahype2::dg::ProjectionRule::Butterfly>();
}

/**
 * The projection of a field reflected along one axis, with its second value negated, is the
 * reflected and negated projection, bitwise, in 2D and 3D.
 */
void
exahype2::dg::tests::BasisTest::testTheProjectionOfAReflectedFieldIsTheReflectedProjection() {
  const auto validateReflections = [this]<int Dimensions, exahype2::dg::ProjectionRule Rule>() {
    const std::vector<double> input  = getProjectionInput<Dimensions>();
    const std::vector<double> output = project<Dimensions, Rule>(input);
    for (int axis = 0; axis < Dimensions; axis++) {
      std::vector<double> reflectedInput(input.size());
      for (int value = 0; value < static_cast<int>(input.size()); value++) {
        const double sign = value % ProjectionValues == 1 ? -1.0 : 1.0;
        reflectedInput[getMovedValue<Dimensions>(value, ProjectionInputDofs, axis, -1)] = sign * input[value];
      }
      const std::vector<double> reflectedOutput = project<Dimensions, Rule>(reflectedInput);
      for (int value = 0; value < static_cast<int>(output.size()); value++) {
        const double sign = value % ProjectionValues == 1 ? -1.0 : 1.0;
        validateEqualsWithParams3(
          reflectedOutput[getMovedValue<Dimensions>(value, ProjectionOutputDofs, axis, -1)],
          sign * output[value],
          Dimensions,
          axis,
          value
        );
      }
    }
  };
  validateReflections.template operator()<2, exahype2::dg::ProjectionRule::PairedProducts>();
  validateReflections.template operator()<3, exahype2::dg::ProjectionRule::PairedProducts>();
  validateReflections.template operator()<2, exahype2::dg::ProjectionRule::Butterfly>();
  validateReflections.template operator()<3, exahype2::dg::ProjectionRule::Butterfly>();
}

/**
 * Both projection rules return the same projection up to rounding in 2D and 3D.
 */
void
exahype2::dg::tests::BasisTest::testBothProjectionRulesProjectTheSameField() {
  const auto validateRules = [this]<int Dimensions>() {
    const std::vector<double> input    = getProjectionInput<Dimensions>();
    const std::vector<double> paired   = project<Dimensions, exahype2::dg::ProjectionRule::PairedProducts>(input);
    const std::vector<double> mirrored = project<Dimensions, exahype2::dg::ProjectionRule::Butterfly>(input);
    validateNumericalBuffersEquals(mirrored, paired);
  };
  validateRules.template operator()<2>();
  validateRules.template operator()<3>();
}

/**
 * Every split of the lines or of the values of each stage among threads projects the bits of one
 * thread in 2D and 3D.
 */
void
exahype2::dg::tests::BasisTest::testEverySplitOfTheStagesAmongThreadsProjectsLikeOneThread() {
  using Rule                            = exahype2::dg::ProjectionRule;
  using Item                            = exahype2::dg::ProjectionItem;
  const ProjectionMatrix matrix         = getMirrorSymmetricProjection();
  const auto             validateSplits = [this, &matrix]<int Dimensions, Rule StageRule, Item StageItem>() {
    constexpr int ScratchValues = exahype2::dg::getScratchValuesOfTensorProduct<
      Dimensions,
      ProjectionInputDofs,
      ProjectionOutputDofs,
      ProjectionValues
    >();
    const std::vector<double> input  = getProjectionInput<Dimensions>();
    const std::vector<double> output = project<Dimensions, StageRule>(input);
    const auto                weights =
      exahype2::dg::getProjectionWeightsOfDofPairs<ProjectionInputDofs, ProjectionOutputDofs>(matrix.data());
    for (const int threads : {1, 2, 3, 7, 32}) {
      std::vector<double> scratch(ScratchValues);
      std::vector<double> splitOutput(output.size());
      for (int stage = 0; stage < exahype2::dg::getStagesOfTensorProduct<Dimensions>(); stage++) {
        for (int thread = threads - 1; thread >= 0; thread--) {
          exahype2::dg::projectTensorProductStage<
            Dimensions,
            ProjectionInputDofs,
            ProjectionOutputDofs,
            ProjectionValues,
            StageRule,
            StageItem
          >(stage, input.data(), splitOutput.data(), matrix.data(), weights, scratch.data(), thread, threads);
        }
      }
      for (int value = 0; value < static_cast<int>(output.size()); value++) {
        validateEqualsWithParams3(splitOutput[value], output[value], Dimensions, threads, value);
      }
    }
  };
  validateSplits.template operator()<2, Rule::PairedProducts, Item::Line>();
  validateSplits.template operator()<3, Rule::PairedProducts, Item::Line>();
  validateSplits.template operator()<2, Rule::Butterfly, Item::Line>();
  validateSplits.template operator()<3, Rule::Butterfly, Item::Line>();
  validateSplits.template operator()<2, Rule::PairedProducts, Item::ValueOfLine>();
  validateSplits.template operator()<3, Rule::PairedProducts, Item::ValueOfLine>();
  validateSplits.template operator()<2, Rule::Butterfly, Item::ValueOfLine>();
  validateSplits.template operator()<3, Rule::Butterfly, Item::ValueOfLine>();
}

void
exahype2::dg::tests::BasisTest::testLagrangeBasisFunctionIsCardinal() {
  for (int basisFunction = 0; basisFunction < MatrixNodesPerAxis; basisFunction++) {
    for (int node = 0; node < MatrixNodesPerAxis; node++) {
      const double value = exahype2::dg::getLagrangeBasisFunction1D<MatrixNodesPerAxis>(
        MatrixNodes,
        basisFunction,
        MatrixNodes[node] - 0.5
      );
      const double expected = node == basisFunction ? 1.0 : 0.0;
      validateNumericalEqualsWithEpsWithParams2(value, expected, MatrixTolerance, basisFunction, node);
    }
  }
  double partitionOfUnity = 0.0;
  for (int basisFunction = 0; basisFunction < MatrixNodesPerAxis; basisFunction++) {
    partitionOfUnity += exahype2::dg::getLagrangeBasisFunction1D<MatrixNodesPerAxis>(MatrixNodes, basisFunction, -0.2);
  }
  validateNumericalEqualsWithEps(partitionOfUnity, 1.0, MatrixTolerance);
}

/**
 * The basis functions reproduce a linear function of the offset from the centre of the interval.
 */
void
exahype2::dg::tests::BasisTest::testTheLagrangeBasisFunctionsReproduceALinearFunction() {
  for (int sample = 0; sample <= 8; sample++) {
    const double offsetFromCentre = 0.125 * sample - 0.5;
    double       linear           = 0.0;
    for (int basisFunction = 0; basisFunction < MatrixNodesPerAxis; basisFunction++) {
      linear +=
        exahype2::dg::getLagrangeBasisFunction1D<MatrixNodesPerAxis>(MatrixNodes, basisFunction, offsetFromCentre)
        * (MatrixNodes[basisFunction] - 0.5);
    }
    validateNumericalEqualsWithEpsWithParams1(linear, offsetFromCentre, MatrixTolerance, sample);
  }
}

/**
 * On nodes that mirror exactly at one half, the mirror basis function at the negated offset returns
 * the same bits as the basis function at the offset, for two to five nodes.
 */
void
exahype2::dg::tests::BasisTest::testTheMirrorBasisFunctionAtTheNegatedOffsetReturnsTheSameBits() {
  validateMirroredBasisFunctions<2>();
  validateMirroredBasisFunctions<3>();
  validateMirroredBasisFunctions<4>();
  validateMirroredBasisFunctions<5>();
}

template <int Nodes>
void
exahype2::dg::tests::BasisTest::validateMirroredBasisFunctions() {
  // One minus an upper node is exact. The lower nodes then mirror the upper nodes exactly.
  double nodes[Nodes];
  for (int node = Nodes / 2; node < Nodes; node++) {
    nodes[node] = 0.5 + 0.07 * (node - Nodes / 2) + 0.031 * (node + 1);
  }
  if constexpr (Nodes % 2 == 1) {
    nodes[Nodes / 2] = 0.5;
  }
  for (int node = 0; node < Nodes / 2; node++) {
    nodes[node] = 1.0 - nodes[Nodes - 1 - node];
  }

  for (const double offsetFromCentre : {0.0, 0.1, 0.123, 0.29, 0.37, 0.5}) {
    for (int basisFunction = 0; basisFunction < Nodes; basisFunction++) {
      validateEqualsWithParams3(
        exahype2::dg::getLagrangeBasisFunction1D<Nodes>(nodes, basisFunction, offsetFromCentre),
        exahype2::dg::getLagrangeBasisFunction1D<Nodes>(nodes, Nodes - 1 - basisFunction, -offsetFromCentre),
        Nodes,
        basisFunction,
        offsetFromCentre
      );
    }
  }
}

void
exahype2::dg::tests::BasisTest::testInterpolationEvaluatesTheCoarsePolynomialAtTheChildNodes() {
  for (int child = 0; child < SubdivisionFactor; child++) {
    for (int fine = 0; fine < MatrixNodesPerAxis; fine++) {
      double interpolated = 0.0;
      for (int coarse = 0; coarse < MatrixNodesPerAxis; coarse++) {
        interpolated += Interpolation[getIndex(child, fine, coarse)] * getCubicValue(MatrixNodes[coarse]);
      }
      const double fineNode = (MatrixNodes[fine] + child) / SubdivisionFactor;
      validateNumericalEqualsWithEpsWithParams2(interpolated, getCubicValue(fineNode), MatrixTolerance, child, fine);
    }
  }
}

void
exahype2::dg::tests::BasisTest::testRestrictionIsTheInterpolationTransposedOverTheChildWidth() {
  for (int child = 0; child < SubdivisionFactor; child++) {
    for (int coarse = 0; coarse < MatrixNodesPerAxis; coarse++) {
      for (int fine = 0; fine < MatrixNodesPerAxis; fine++) {
        validateNumericalEqualsWithEpsWithParams3(
          Restriction[getIndex(child, coarse, fine)],
          Interpolation[getIndex(child, fine, coarse)] / SubdivisionFactor,
          MatrixTolerance,
          child,
          coarse,
          fine
        );
      }
    }
  }
}

/**
 * The weighted restriction of children that are all one gives one at every coarse node.
 */
void
exahype2::dg::tests::BasisTest::testWeightedRestrictionPreservesConstants() {
  for (int coarse = 0; coarse < MatrixNodesPerAxis; coarse++) {
    double restricted = 0.0;
    for (int child = 0; child < SubdivisionFactor; child++) {
      for (int fine = 0; fine < MatrixNodesPerAxis; fine++) {
        restricted += WeightedRestriction[getIndex(child, coarse, fine)];
      }
    }
    validateNumericalEqualsWithEpsWithParams1(restricted, 1.0, MatrixTolerance, coarse);
  }
}

void
exahype2::dg::tests::BasisTest::testMatricesAreMirrorSymmetricToTheLastBit() {
  for (int child = 0; child < SubdivisionFactor; child++) {
    for (int row = 0; row < MatrixNodesPerAxis; row++) {
      for (int column = 0; column < MatrixNodesPerAxis; column++) {
        validateEqualsWithParams1(
          Interpolation[getIndex(child, row, column)],
          Interpolation[getMirrorIndex(child, row, column)],
          child
        );
        validateEqualsWithParams1(
          Restriction[getIndex(child, row, column)],
          Restriction[getMirrorIndex(child, row, column)],
          child
        );
        validateEqualsWithParams1(
          WeightedRestriction[getIndex(child, row, column)],
          WeightedRestriction[getMirrorIndex(child, row, column)],
          child
        );
      }
    }
  }
}

void
exahype2::dg::tests::BasisTest::testAverageMirrorPairsKeepsSymmetricEntries() {
  auto matrices = Interpolation;
  exahype2::dg::averageMirrorPairs<MatrixNodesPerAxis>(matrices);
  validate(matrices == Interpolation);

  exahype2::dg::AMRMatrices1D<MatrixNodesPerAxis> perturbed{};
  perturbed[getIndex(0, 0, 0)]                                                               = 1.0;
  perturbed[getIndex(SubdivisionFactor - 1, MatrixNodesPerAxis - 1, MatrixNodesPerAxis - 1)] = 3.0;
  exahype2::dg::averageMirrorPairs<MatrixNodesPerAxis>(perturbed);
  validateEquals(perturbed[getIndex(0, 0, 0)], 2.0);
  validateEquals(perturbed[getIndex(SubdivisionFactor - 1, MatrixNodesPerAxis - 1, MatrixNodesPerAxis - 1)], 2.0);
}

void
exahype2::dg::tests::BasisTest::testTransposePerChildSwapsTheNodeIndices() {
  const auto transposed = exahype2::dg::transposePerChild<MatrixNodesPerAxis>(WeightedRestriction);
  for (int child = 0; child < SubdivisionFactor; child++) {
    for (int row = 0; row < MatrixNodesPerAxis; row++) {
      for (int column = 0; column < MatrixNodesPerAxis; column++) {
        validateEqualsWithParams3(
          transposed[getIndex(child, column, row)],
          WeightedRestriction[getIndex(child, row, column)],
          child,
          row,
          column
        );
      }
    }
  }
  validate(exahype2::dg::transposePerChild<MatrixNodesPerAxis>(transposed) == WeightedRestriction);
}

void
exahype2::dg::tests::BasisTest::testOrderZeroMatrices() {
  constexpr double Node{0.5};
  constexpr double Weight{1.0};
  const auto       interpolation = exahype2::dg::getInterpolationMatrix1D<1>(&Node);
  const auto       restriction   = exahype2::dg::getRestrictionMatrix1D<1>(&Node);
  const auto       weighted      = exahype2::dg::getWeightedRestrictionMatrix1D<1>(&Node, &Weight);
  for (int child = 0; child < SubdivisionFactor; child++) {
    validateEquals(interpolation[child], 1.0);
    validateEquals(restriction[child], 1.0 / SubdivisionFactor);
    validateEquals(weighted[child], 1.0 / SubdivisionFactor);
  }
}

/**
 * Every subface takes the coarse face values at its own node positions.
 */
void
exahype2::dg::tests::BasisTest::testFaceInterpolationReproducesALinearProfile() {
  const TransferOperators operators;
  double                  coarseFace[Setup::FaceNodes * Setup::ValuesPerNode];
  fillLinearTransferProfile<Setup::FaceNodes>(coarseFace, DIMENSIONS - 1, -1);

  for (int subface = 0; subface < peano4::SubfacesPerFace; subface++) {
    double fineFace[Setup::FaceNodes * Setup::ValuesPerNode];
    exahype2::dg::interpolateFace<Setup::NodesPerAxis, Setup::ValuesPerNode>(
      coarseFace,
      fineFace,
      operators.prolongation.data(),
      subface
    );
    for (int node = 0; node < Setup::FaceNodes; node++) {
      double coordinates[DIMENSIONS];
      setTransferCoordinatesOf(coordinates, DIMENSIONS - 1, node, subface);
      for (int value = 0; value < Setup::ValuesPerNode; value++) {
        validateNumericalEqualsWithParams3(
          fineFace[node * Setup::ValuesPerNode + value],
          getLinearTransferProfile(coordinates, DIMENSIONS - 1, value),
          subface,
          node,
          value
        );
      }
    }
  }
}

/**
 * Every child takes the coarse state at its own node positions.
 */
void
exahype2::dg::tests::BasisTest::testCellInterpolationReproducesALinearState() {
  const TransferOperators operators;
  double                  coarseCell[Setup::CellNodes * Setup::ValuesPerNode];
  fillLinearTransferProfile<Setup::CellNodes>(coarseCell, DIMENSIONS, -1);

  for (int child = 0; child < Setup::ChildrenPerCell; child++) {
    double fineCell[Setup::CellNodes * Setup::ValuesPerNode] = {};
    interpolateEveryNode(coarseCell, fineCell, operators.prolongation.data(), child);
    for (int node = 0; node < Setup::CellNodes; node++) {
      double coordinates[DIMENSIONS];
      setTransferCoordinatesOf(coordinates, DIMENSIONS, node, child);
      for (int value = 0; value < Setup::ValuesPerNode; value++) {
        validateNumericalEqualsWithParams3(
          fineCell[node * Setup::ValuesPerNode + value],
          getLinearTransferProfile(coordinates, DIMENSIONS, value),
          child,
          node,
          value
        );
      }
    }
  }
}

/**
 * A constant face and a constant cell return their bits on every subface and child.
 */
void
exahype2::dg::tests::BasisTest::testAConstantFaceAndCellInterpolateBitwise() {
  const TransferOperators operators;
  double                  coarseFace[Setup::FaceNodes * Setup::ValuesPerNode];
  double                  coarseCell[Setup::CellNodes * Setup::ValuesPerNode];
  fillTransferConstant<Setup::FaceNodes>(coarseFace);
  fillTransferConstant<Setup::CellNodes>(coarseCell);

  for (int subface = 0; subface < peano4::SubfacesPerFace; subface++) {
    double fineFace[Setup::FaceNodes * Setup::ValuesPerNode];
    exahype2::dg::interpolateFace<Setup::NodesPerAxis, Setup::ValuesPerNode>(
      coarseFace,
      fineFace,
      operators.prolongation.data(),
      subface
    );
    for (int node = 0; node < Setup::FaceNodes; node++) {
      for (int value = 0; value < Setup::ValuesPerNode; value++) {
        validateEqualsWithParams3(
          fineFace[node * Setup::ValuesPerNode + value],
          getTransferConstantOf(value),
          subface,
          node,
          value
        );
      }
    }
  }
  for (int child = 0; child < Setup::ChildrenPerCell; child++) {
    double fineCell[Setup::CellNodes * Setup::ValuesPerNode] = {};
    interpolateEveryNode(coarseCell, fineCell, operators.prolongation.data(), child);
    for (int node = 0; node < Setup::CellNodes; node++) {
      for (int value = 0; value < Setup::ValuesPerNode; value++) {
        validateEqualsWithParams3(
          fineCell[node * Setup::ValuesPerNode + value],
          getTransferConstantOf(value),
          child,
          node,
          value
        );
      }
    }
  }
}

/**
 * The coarse face takes the values the subfaces represent at its own node positions.
 */
void
exahype2::dg::tests::BasisTest::testFaceRestrictionProjectsALinearProfile() {
  const TransferOperators operators;
  double                  fineFaces[peano4::SubfacesPerFace][Setup::FaceNodes * Setup::ValuesPerNode];
  const double*           fineFacePointers[peano4::SubfacesPerFace];
  for (int subface = 0; subface < peano4::SubfacesPerFace; subface++) {
    fillLinearTransferProfile<Setup::FaceNodes>(fineFaces[subface], DIMENSIONS - 1, subface);
    fineFacePointers[subface] = fineFaces[subface];
  }

  double coarseFace[Setup::FaceNodes * Setup::ValuesPerNode];
  exahype2::dg::restrictFace<Setup::NodesPerAxis, Setup::ValuesPerNode>(
    fineFacePointers,
    coarseFace,
    operators.restriction.data()
  );

  for (int node = 0; node < Setup::FaceNodes; node++) {
    double coordinates[DIMENSIONS];
    setTransferCoordinatesOf(coordinates, DIMENSIONS - 1, node, -1);
    for (int value = 0; value < Setup::ValuesPerNode; value++) {
      validateNumericalEqualsWithParams2(
        coarseFace[node * Setup::ValuesPerNode + value],
        getLinearTransferProfile(coordinates, DIMENSIONS - 1, value),
        node,
        value
      );
    }
  }
}

/**
 * The coarse cell takes the state the children represent at its own node positions.
 */
void
exahype2::dg::tests::BasisTest::testCellRestrictionProjectsALinearState() {
  const TransferOperators operators;
  double                  fineCells[Setup::ChildrenPerCell][Setup::CellNodes * Setup::ValuesPerNode];
  const double*           fineCellPointers[Setup::ChildrenPerCell];
  for (int child = 0; child < Setup::ChildrenPerCell; child++) {
    fillLinearTransferProfile<Setup::CellNodes>(fineCells[child], DIMENSIONS, child);
    fineCellPointers[child] = fineCells[child];
  }

  double coarseCell[Setup::CellNodes * Setup::ValuesPerNode];
  restrictEveryNode(fineCellPointers, coarseCell, operators.restriction.data());

  for (int node = 0; node < Setup::CellNodes; node++) {
    double coordinates[DIMENSIONS];
    setTransferCoordinatesOf(coordinates, DIMENSIONS, node, -1);
    for (int value = 0; value < Setup::ValuesPerNode; value++) {
      validateNumericalEqualsWithParams2(
        coarseCell[node * Setup::ValuesPerNode + value],
        getLinearTransferProfile(coordinates, DIMENSIONS, value),
        node,
        value
      );
    }
  }
}

/**
 * The restriction and the interpolation of a negated field return the negated field bitwise.
 */
void
exahype2::dg::tests::BasisTest::testANegatedFieldTransfersToTheNegatedFieldBitwise() {
  constexpr int           FaceValues = Setup::FaceNodes * Setup::ValuesPerNode;
  constexpr int           CellValues = Setup::CellNodes * Setup::ValuesPerNode;
  const TransferOperators operators;

  double        fineFaces[peano4::SubfacesPerFace][FaceValues];
  double        negatedFineFaces[peano4::SubfacesPerFace][FaceValues];
  double        fineCells[Setup::ChildrenPerCell][CellValues];
  double        negatedFineCells[Setup::ChildrenPerCell][CellValues];
  const double* fineFacePointers[peano4::SubfacesPerFace];
  const double* negatedFineFacePointers[peano4::SubfacesPerFace];
  const double* fineCellPointers[Setup::ChildrenPerCell];
  const double* negatedFineCellPointers[Setup::ChildrenPerCell];
  for (int subface = 0; subface < peano4::SubfacesPerFace; subface++) {
    for (int entry = 0; entry < FaceValues; entry++) {
      fineFaces[subface][entry]        = 0.3 + 0.37 * ((subface * FaceValues + entry) % 7) - 0.11 * subface;
      negatedFineFaces[subface][entry] = -fineFaces[subface][entry];
    }
    fineFacePointers[subface]        = fineFaces[subface];
    negatedFineFacePointers[subface] = negatedFineFaces[subface];
  }
  for (int child = 0; child < Setup::ChildrenPerCell; child++) {
    for (int entry = 0; entry < CellValues; entry++) {
      fineCells[child][entry]        = 0.3 + 0.37 * ((child * CellValues + entry) % 11) - 0.07 * child;
      negatedFineCells[child][entry] = -fineCells[child][entry];
    }
    fineCellPointers[child]        = fineCells[child];
    negatedFineCellPointers[child] = negatedFineCells[child];
  }

  double coarseFace[FaceValues];
  double negatedCoarseFace[FaceValues];
  double coarseCell[CellValues];
  double negatedCoarseCell[CellValues];
  exahype2::dg::restrictFace<Setup::NodesPerAxis, Setup::ValuesPerNode>(
    fineFacePointers,
    coarseFace,
    operators.restriction.data()
  );
  exahype2::dg::restrictFace<Setup::NodesPerAxis, Setup::ValuesPerNode>(
    negatedFineFacePointers,
    negatedCoarseFace,
    operators.restriction.data()
  );
  restrictEveryNode(fineCellPointers, coarseCell, operators.restriction.data());
  restrictEveryNode(negatedFineCellPointers, negatedCoarseCell, operators.restriction.data());
  for (int entry = 0; entry < FaceValues; entry++) {
    validateEqualsWithParams1(negatedCoarseFace[entry], -coarseFace[entry], entry);
  }
  for (int entry = 0; entry < CellValues; entry++) {
    validateEqualsWithParams1(negatedCoarseCell[entry], -coarseCell[entry], entry);
  }

  double fineFace[FaceValues];
  double negatedFineFace[FaceValues];
  double fineCell[CellValues];
  double negatedFineCell[CellValues];
  exahype2::dg::interpolateFace<Setup::NodesPerAxis, Setup::ValuesPerNode>(
    coarseFace,
    fineFace,
    operators.prolongation.data(),
    1
  );
  exahype2::dg::interpolateFace<Setup::NodesPerAxis, Setup::ValuesPerNode>(
    negatedCoarseFace,
    negatedFineFace,
    operators.prolongation.data(),
    1
  );
  interpolateEveryNode(coarseCell, fineCell, operators.prolongation.data(), 1);
  interpolateEveryNode(negatedCoarseCell, negatedFineCell, operators.prolongation.data(), 1);
  for (int entry = 0; entry < FaceValues; entry++) {
    validateEqualsWithParams1(negatedFineFace[entry], -fineFace[entry], entry);
  }
  for (int entry = 0; entry < CellValues; entry++) {
    validateEqualsWithParams1(negatedFineCell[entry], -fineCell[entry], entry);
  }
}

/**
 * Constant fine faces and constant children return their bits on the coarse face and cell.
 */
void
exahype2::dg::tests::BasisTest::testConstantFacesAndCellsRestrictBitwise() {
  const TransferOperators operators;
  double                  fineFace[Setup::FaceNodes * Setup::ValuesPerNode];
  double                  fineCell[Setup::CellNodes * Setup::ValuesPerNode];
  fillTransferConstant<Setup::FaceNodes>(fineFace);
  fillTransferConstant<Setup::CellNodes>(fineCell);
  const double* fineFacePointers[peano4::SubfacesPerFace];
  const double* fineCellPointers[Setup::ChildrenPerCell];
  for (int subface = 0; subface < peano4::SubfacesPerFace; subface++) {
    fineFacePointers[subface] = fineFace;
  }
  for (int child = 0; child < Setup::ChildrenPerCell; child++) {
    fineCellPointers[child] = fineCell;
  }

  double coarseFace[Setup::FaceNodes * Setup::ValuesPerNode];
  double coarseCell[Setup::CellNodes * Setup::ValuesPerNode];
  exahype2::dg::restrictFace<Setup::NodesPerAxis, Setup::ValuesPerNode>(
    fineFacePointers,
    coarseFace,
    operators.restriction.data()
  );
  restrictEveryNode(fineCellPointers, coarseCell, operators.restriction.data());
  for (int node = 0; node < Setup::FaceNodes; node++) {
    for (int value = 0; value < Setup::ValuesPerNode; value++) {
      validateEqualsWithParams2(
        coarseFace[node * Setup::ValuesPerNode + value],
        getTransferConstantOf(value),
        node,
        value
      );
    }
  }
  for (int node = 0; node < Setup::CellNodes; node++) {
    for (int value = 0; value < Setup::ValuesPerNode; value++) {
      validateEqualsWithParams2(
        coarseCell[node * Setup::ValuesPerNode + value],
        getTransferConstantOf(value),
        node,
        value
      );
    }
  }
}
