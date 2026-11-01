// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "exahype2/dg/Basis.h"

#include "peano4/CellConnectivity.h"
#include "peano4/Constants.h"
#include "peano4/Loop.h"
#include "tarch/accelerator/Buffer.h"
#include "tarch/accelerator/MemoryLocation.h"
#include "tarch/accelerator/ParallelFor.h"
#include "tarch/accelerator/Range.h"
#include "tarch/la/Vector.h"
#include "tarch/testing/UnitTest.h"
#include "unittests/exahype2/dg/TransferTestData.h"

namespace {
  constexpr int NodesPerAxis               = 3;
  constexpr int NumberOfUnknowns           = 2;
  constexpr int NumberOfAuxiliaryVariables = 1;
  constexpr int NumberOfData               = NumberOfUnknowns + NumberOfAuxiliaryVariables;
  constexpr int FaceNodes                  = exahype2::dg::getNodesPerFace(NodesPerAxis);
  constexpr int CellNodes                  = exahype2::dg::getNodesPerCell(NodesPerAxis);
  constexpr int EntriesPerAxis             = CellNodes * NumberOfData;
  constexpr int PaddedNodeStride           = NumberOfData + 2;

  struct Order2Layout final {
    static constexpr int DGOrder                    = NodesPerAxis - 1;
    static constexpr int NumberOfUnknowns           = ::NumberOfUnknowns;
    static constexpr int NumberOfAuxiliaryVariables = ::NumberOfAuxiliaryVariables;
  };

  /**
   * A solver layout whose unknowns fill the vector width exactly.
   */
  struct ExactWidthLayout final {
    static constexpr int DGOrder                    = NodesPerAxis - 1;
    static constexpr int NumberOfUnknowns           = PEANO_WITH_SIMDLEN;
    static constexpr int NumberOfAuxiliaryVariables = 0;
  };

  /**
   * A solver layout whose unknowns exceed the vector width by one.
   */
  struct OneAboveWidthLayout final {
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

  // The entries are distinct and non-symmetric. An index mix-up does not cancel out.
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

  using Setup = exahype2::dg::unittests::TransferSetup;

  using ProjectionRule = exahype2::dg::ProjectionRule;
  using ProjectionItem = exahype2::dg::ProjectionItem;

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
  template <int Dimensions, ProjectionRule Rule>
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
   * Counts the values of the projection of every input with two swapped axes that differ from the
   * projection of the input with these axes swapped.
   */
  template <int Dimensions, ProjectionRule Rule>
  int countSwappedProjectionMismatches() {
    const std::vector<double> input      = getProjectionInput<Dimensions>();
    const std::vector<double> output     = project<Dimensions, Rule>(input);
    int                       mismatches = 0;
    for (int first = 0; first < Dimensions; first++) {
      for (int second = first + 1; second < Dimensions; second++) {
        std::vector<double> swappedInput(input.size());
        for (int value = 0; value < static_cast<int>(input.size()); value++) {
          swappedInput[getMovedValue<Dimensions>(value, ProjectionInputDofs, first, second)] = input[value];
        }
        const std::vector<double> swappedOutput = project<Dimensions, Rule>(swappedInput);
        for (int value = 0; value < static_cast<int>(output.size()); value++) {
          if (swappedOutput[getMovedValue<Dimensions>(value, ProjectionOutputDofs, first, second)] != output[value]) {
            mismatches++;
          }
        }
      }
    }
    return mismatches;
  }

  /**
   * Counts the values of the projection of every input reflected along one axis with its second
   * value negated that differ from the reflected and negated projection of the input.
   */
  template <int Dimensions, ProjectionRule Rule>
  int countReflectedProjectionMismatches() {
    const std::vector<double> input      = getProjectionInput<Dimensions>();
    const std::vector<double> output     = project<Dimensions, Rule>(input);
    int                       mismatches = 0;
    for (int axis = 0; axis < Dimensions; axis++) {
      std::vector<double> reflectedInput(input.size());
      for (int value = 0; value < static_cast<int>(input.size()); value++) {
        const double sign = value % ProjectionValues == 1 ? -1.0 : 1.0;
        reflectedInput[getMovedValue<Dimensions>(value, ProjectionInputDofs, axis, -1)] = sign * input[value];
      }
      const std::vector<double> reflectedOutput = project<Dimensions, Rule>(reflectedInput);
      for (int value = 0; value < static_cast<int>(output.size()); value++) {
        const double sign = value % ProjectionValues == 1 ? -1.0 : 1.0;
        if (reflectedOutput[getMovedValue<Dimensions>(value, ProjectionOutputDofs, axis, -1)] != sign * output[value]) {
          mismatches++;
        }
      }
    }
    return mismatches;
  }

  /**
   * Counts the values that 1, 2, 3, 7 and 32 threads write differently from one thread when they
   * split every stage of the projection among themselves by StageItem.
   */
  template <int Dimensions, ProjectionRule StageRule, ProjectionItem StageItem>
  int countSplitProjectionMismatches() {
    constexpr int ScratchValues = exahype2::dg::getScratchValuesOfTensorProduct<
      Dimensions,
      ProjectionInputDofs,
      ProjectionOutputDofs,
      ProjectionValues
    >();
    const ProjectionMatrix    matrix = getMirrorSymmetricProjection();
    const std::vector<double> input  = getProjectionInput<Dimensions>();
    const std::vector<double> output = project<Dimensions, StageRule>(input);
    const auto                weights =
      exahype2::dg::getProjectionWeightsOfDofPairs<ProjectionInputDofs, ProjectionOutputDofs>(matrix.data());
    int mismatches = 0;
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
      for (std::size_t value = 0; value < output.size(); value++) {
        if (splitOutput[value] != output[value]) {
          mismatches++;
        }
      }
    }
    return mismatches;
  }

  /**
   * Counts the entries of the derivative of a line reflected along its axis, with and without its
   * sign swapped, that differ from the reflected derivative with the opposite sign change.
   */
  template <int Nodes>
  int countReflectedDerivativeMismatches(const double* const operatorMatrix) {
    constexpr int Entries    = PEANO_WITH_SIMDLEN;
    int           mismatches = 0;
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
          if (reflectedDerivative[(Nodes - 1 - node) * Entries + entry] != -sign * derivative[node * Entries + entry]) {
            mismatches++;
          }
        }
      }
    }
    return mismatches;
  }

  /**
   * Counts the face entries of a field reflected along the first axis, with and without its sign
   * swapped, that differ from the swapped faces of the field with that sign change.
   */
  template <int Nodes>
  int countReflectedFaceMismatches(const double* const leftCoefficients, const double* const rightCoefficients) {
    constexpr int Entries       = 2;
    constexpr int NodesOfCell   = exahype2::dg::getNodesPerCell(Nodes);
    constexpr int NodesOfFace   = exahype2::dg::getNodesPerFace(Nodes);
    constexpr int ReflectedAxis = 0;
    int           mismatches    = 0;
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
      for (int entry = 0; entry < NodesOfFace * Entries; entry++) {
        if (lowerFaceOfReflected[entry] != sign * upperFace[entry]) {
          mismatches++;
        }
        if (upperFaceOfReflected[entry] != sign * lowerFace[entry]) {
          mismatches++;
        }
      }
    }
    return mismatches;
  }

  /**
   * Counts the basis functions on nodes that mirror exactly at one half whose mirror function at the
   * negated offset returns other bits than the function at six offsets from the centre.
   */
  template <int Nodes>
  int countMirroredBasisFunctionMismatches() {
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

    int mismatches = 0;
    for (const double offsetFromCentre : {0.0, 0.1, 0.123, 0.29, 0.37, 0.5}) {
      for (int basisFunction = 0; basisFunction < Nodes; basisFunction++) {
        if (
          exahype2::dg::getLagrangeBasisFunction1D<Nodes>(nodes, basisFunction, offsetFromCentre)
          != exahype2::dg::getLagrangeBasisFunction1D<Nodes>(nodes, Nodes - 1 - basisFunction, -offsetFromCentre)
        ) {
          mismatches++;
        }
      }
    }
    return mismatches;
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

  /**
   * Writes the derivative of the line with the dispatch index.
   */
  template <int Nodes>
  struct DifferentiateLine final {
    const double* values{nullptr};
    const double* derivativeOperator{nullptr};
    double*       derivatives{nullptr};

    INLINE_METHOD GPU_CALLABLE_METHOD void operator()(const int line) const {
      constexpr int Entries = PEANO_WITH_SIMDLEN;
      exahype2::dg::differentiateAlongAxis<Entries, Nodes, true, double, double>(
        derivatives + line * Nodes * Entries,
        Entries,
        values + line * Nodes * Entries,
        Entries,
        exahype2::dg::getDerivativeWeightsOfNodePairs<Nodes>(derivativeOperator),
        1.5,
        0.25
      );
    }
  };

  /**
   * Returns the derivatives of the lines from a dispatch and from the host. The first vector stores
   * the derivatives of the dispatch.
   */
  template <int Nodes>
  std::pair<std::vector<double>, std::vector<double>> getDerivativesOfDispatchAndHost() {
    constexpr int       Lines   = 64;
    constexpr int       Entries = PEANO_WITH_SIMDLEN;
    std::vector<double> values(Lines * Nodes * Entries);
    for (std::size_t value = 0; value < values.size(); value++) {
      values[value] = 0.001 * static_cast<double>(value % 97) - 0.04;
    }
    std::vector<double> derivativeOperator(Nodes * Nodes);
    for (std::size_t entry = 0; entry < derivativeOperator.size(); entry++) {
      derivativeOperator[entry] = 0.01 * static_cast<double>(entry % 13) - 0.06;
    }

    tarch::accelerator::Buffer<double> deviceValues(tarch::MemoryLocation::Device);
    tarch::accelerator::Buffer<double> deviceOperator(tarch::MemoryLocation::Device);
    tarch::accelerator::Buffer<double> deviceDerivatives(tarch::MemoryLocation::Device);
    deviceValues.setValues(values);
    deviceOperator.setValues(derivativeOperator);
    deviceDerivatives.setValues(std::vector<double>(values.size(), 0.0));
    tarch::accelerator::parallelFor(
      tarch::accelerator::Range(Lines),
      DifferentiateLine<Nodes>{deviceValues.getData(), deviceOperator.getData(), deviceDerivatives.getData()}
    );

    std::vector<double>            hostDerivatives(values.size(), 0.0);
    const DifferentiateLine<Nodes> differentiateOnHost{
      values.data(),
      derivativeOperator.data(),
      hostDerivatives.data()
    };
    for (int line = 0; line < Lines; line++) {
      differentiateOnHost(line);
    }
    return {deviceDerivatives.getValues(), hostDerivatives};
  }
} // namespace

UNIT_TEST("A cell stores one node per combination of the axis nodes and a face one per tangential combination") {
  GIVEN("The number of node combinations that a loop over the nodes along every axis visits") {
    int nodesInCell = 0;
    peano4::dForEach(NodesPerAxis, [&]([[maybe_unused]] const tarch::la::Vector<DIMENSIONS, int>& node) {
      nodesInCell++;
    });

    WHEN("The test counts the nodes of a cell and of a face") {
      const int nodesPerCell = exahype2::dg::getNodesPerCell(NodesPerAxis);
      const int nodesPerFace = exahype2::dg::getNodesPerFace(NodesPerAxis);

      THEN("A cell stores every combination and a face the combinations over the nodes per axis") {
        VALIDATE_EQUALS(nodesPerCell, nodesInCell);
        VALIDATE_EQUALS(nodesPerFace, nodesInCell / NodesPerAxis);
      }
    }
  }
}

UNIT_TEST("The padded layout pads the state and the unknowns of a node to the next multiple of the vector width") {
  GIVEN("The padded layout of order two and those of unknowns that fill the vector width or exceed it by one") {
    using ExactWidth    = exahype2::dg::PaddedLayout<ExactWidthLayout>;
    using OneAboveWidth = exahype2::dg::PaddedLayout<OneAboveWidthLayout>;

    THEN("The layout of order two pads its unknowns and its data to the next multiple of the vector width") {
      VALIDATE_EQUALS(PaddedCell::NodesPerAxis, NodesPerAxis);
      VALIDATE_EQUALS(PaddedCell::NumberOfUnknowns, NumberOfUnknowns);
      VALIDATE_EQUALS(PaddedCell::NumberOfData, NumberOfData);

      VALIDATE_EQUALS(PaddedCell::PaddedNumberOfUnknowns % PEANO_WITH_SIMDLEN, 0);
      VALIDATE(PaddedCell::PaddedNumberOfUnknowns >= NumberOfUnknowns);
      VALIDATE(PaddedCell::PaddedNumberOfUnknowns - NumberOfUnknowns < PEANO_WITH_SIMDLEN);

      VALIDATE_EQUALS(PaddedCell::PaddedNumberOfData % PEANO_WITH_SIMDLEN, 0);
      VALIDATE(PaddedCell::PaddedNumberOfData >= NumberOfData);
      VALIDATE(PaddedCell::PaddedNumberOfData - NumberOfData < PEANO_WITH_SIMDLEN);
    }

    THEN("Unknowns that fill the vector width pad to one width and unknowns one above it pad to two widths") {
      VALIDATE_EQUALS(ExactWidth::NodesPerAxis, NodesPerAxis);
      VALIDATE_EQUALS(ExactWidth::PaddedNumberOfUnknowns, PEANO_WITH_SIMDLEN);
      VALIDATE_EQUALS(ExactWidth::PaddedNumberOfData, PEANO_WITH_SIMDLEN);

      VALIDATE_EQUALS(OneAboveWidth::NodesPerAxis, NodesPerAxis);
      VALIDATE_EQUALS(OneAboveWidth::PaddedNumberOfUnknowns, 2 * PEANO_WITH_SIMDLEN);
      VALIDATE_EQUALS(OneAboveWidth::PaddedNumberOfData, 2 * PEANO_WITH_SIMDLEN);
    }

    THEN("A count pads to zero, to one vector width or to two widths if it exceeds one width") {
      VALIDATE_EQUALS(exahype2::dg::getPaddedCountOf(0), 0);
      VALIDATE_EQUALS(exahype2::dg::getPaddedCountOf(1), PEANO_WITH_SIMDLEN);
      VALIDATE_EQUALS(exahype2::dg::getPaddedCountOf(PEANO_WITH_SIMDLEN), PEANO_WITH_SIMDLEN);
      VALIDATE_EQUALS(exahype2::dg::getPaddedCountOf(PEANO_WITH_SIMDLEN + 1), 2 * PEANO_WITH_SIMDLEN);
    }
  }
}

UNIT_TEST("A cell stores one node per axis combination and a space-time block stores one cell per time node") {
  GIVEN("The number of nodes along a third axis, one in 2D") {
    constexpr int ExpectedNodesPerAxisIn3D = (DIMENSIONS > 2) ? NodesPerAxis : 1;

    THEN("The layout of order two counts the nodes of a face, of a cell and of a space-time block") {
      VALIDATE_EQUALS(PaddedCell::NodesPerAxisIn3D, ExpectedNodesPerAxisIn3D);
      VALIDATE_EQUALS(PaddedCell::NodesPerFace, FaceNodes);
      VALIDATE_EQUALS(PaddedCell::NodesPerCell, CellNodes);
      VALIDATE_EQUALS(PaddedCell::SpaceTimeNodes, CellNodes * NodesPerAxis);
      VALIDATE_EQUALS(PaddedCell::NodesPerCell, PaddedCell::NodesPerFace * NodesPerAxis);
    }
  }
}

UNIT_TEST("A default-constructed table bundle stores null pointers and every field keeps the table that it receives") {
  GIVEN("A default-constructed bundle of double tables and one of float tables") {
    const exahype2::dg::BasisTables<double> emptyTables;
    const exahype2::dg::BasisTables<float>  narrowTables;

    THEN("Every table of the double bundle and the quadrature points of the float bundle are null pointers") {
      VALIDATE_EQUALS(emptyTables.quadraturePoints, static_cast<const double*>(nullptr));
      VALIDATE_EQUALS(emptyTables.quadratureWeights, static_cast<const double*>(nullptr));
      VALIDATE_EQUALS(emptyTables.inverseQuadratureWeights, static_cast<const double*>(nullptr));
      VALIDATE_EQUALS(emptyTables.cellQuadratureWeights, static_cast<const double*>(nullptr));
      VALIDATE_EQUALS(emptyTables.inverseCellQuadratureWeights, static_cast<const double*>(nullptr));
      VALIDATE_EQUALS(emptyTables.stiffnessOperator, static_cast<const double*>(nullptr));
      VALIDATE_EQUALS(emptyTables.stiffnessOperatorTransposed, static_cast<const double*>(nullptr));
      VALIDATE_EQUALS(emptyTables.derivativeOperator, static_cast<const double*>(nullptr));
      VALIDATE_EQUALS(emptyTables.inversePicardOperator, static_cast<const double*>(nullptr));
      VALIDATE_EQUALS(emptyTables.leftFaceCoefficients, static_cast<const double*>(nullptr));
      VALIDATE_EQUALS(emptyTables.rightFaceCoefficients, static_cast<const double*>(nullptr));
      VALIDATE_EQUALS(narrowTables.quadraturePoints, static_cast<const float*>(nullptr));
    }
  }

  GIVEN("Stub storage of eleven tables and a bundle of double tables") {
    constexpr int                     NumberOfTables             = 11;
    const double                      stubTables[NumberOfTables] = {};
    exahype2::dg::BasisTables<double> tables;

    WHEN("Every field of the bundle takes its own stub table") {
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

      THEN("Every field keeps its stub table") {
        VALIDATE_EQUALS(tables.quadraturePoints, stubTables + 0);
        VALIDATE_EQUALS(tables.quadratureWeights, stubTables + 1);
        VALIDATE_EQUALS(tables.inverseQuadratureWeights, stubTables + 2);
        VALIDATE_EQUALS(tables.cellQuadratureWeights, stubTables + 3);
        VALIDATE_EQUALS(tables.inverseCellQuadratureWeights, stubTables + 4);
        VALIDATE_EQUALS(tables.stiffnessOperator, stubTables + 5);
        VALIDATE_EQUALS(tables.stiffnessOperatorTransposed, stubTables + 6);
        VALIDATE_EQUALS(tables.derivativeOperator, stubTables + 7);
        VALIDATE_EQUALS(tables.inversePicardOperator, stubTables + 8);
        VALIDATE_EQUALS(tables.leftFaceCoefficients, stubTables + 9);
        VALIDATE_EQUALS(tables.rightFaceCoefficients, stubTables + 10);
      }
    }
  }
}

UNIT_TEST("A reference node maps onto the cell offset plus the node's fraction of the cell size") {
  GIVEN("A cell, a node index that differs per axis and node tables in double and in float precision") {
    const auto                         cellCentre    = getRampOverAxes(0.4, 0.2);
    const auto                         cellSize      = getRampOverAxes(0.2, 0.2);
    const float                        narrowNodes[] = {0.25F, 0.5F, 0.75F};
    tarch::la::Vector<DIMENSIONS, int> index;
    for (int axis = 0; axis < DIMENSIONS; axis++) {
      index(axis) = (2 * axis) % 3;
    }

    WHEN("The test reads the quadrature point of the node index from both tables") {
      const auto position       = exahype2::dg::getQuadraturePoint(cellCentre, cellSize, index, QuadratureNodes1DP2);
      const auto narrowPosition = exahype2::dg::getQuadraturePoint(cellCentre, cellSize, index, narrowNodes);

      THEN("Every coordinate is the offset plus the node times the cell size, with the float node widened to double") {
        for (int axis = 0; axis < DIMENSIONS; axis++) {
          const double expected = cellCentre(axis) + (QuadratureNodes1DP2[index(axis)] - 0.5) * cellSize(axis);
          VALIDATE_NUMERICAL_EQUALS(position(axis), expected, axis);
        }
        for (int axis = 0; axis < DIMENSIONS; axis++) {
          const double expected =
            cellCentre(axis) + (static_cast<double>(narrowNodes[index(axis)]) - 0.5) * cellSize(axis);
          VALIDATE_NUMERICAL_EQUALS(narrowPosition(axis), expected, axis);
        }
      }
    }
  }
}

UNIT_TEST("Every node receives the weight of the cell volume divided by the node count") {
  GIVEN("A cell of width two around the point one and a buffer of three values at each of its nodes") {
    const tarch::la::Vector<DIMENSIONS, double> cellCentre(1.0);
    const tarch::la::Vector<DIMENSIONS, double> cellSize(WalkCellWidth);
    std::array<double, WalkEntries>             values{};

    WHEN("The loop over the nodes records the arguments of every visit") {
      std::vector<int>                                   indexOfVisit;
      std::vector<std::ptrdiff_t>                        offsetOfVisit;
      std::vector<tarch::la::Vector<DIMENSIONS, double>> positionOfVisit;
      std::vector<tarch::la::Vector<DIMENSIONS, double>> cellSizeOfVisit;
      std::vector<double>                                weightOfVisit;
      exahype2::dg::forEachNode<WalkNodesPerAxis, WalkValues>(
        cellCentre,
        cellSize,
        WalkPoints,
        WalkWeights,
        values.data(),
        [&](double* const value, const auto& x, const auto& h, const double weight, const int index) {
          indexOfVisit.push_back(index);
          offsetOfVisit.push_back(value - values.data());
          positionOfVisit.push_back(x);
          cellSizeOfVisit.push_back(h);
          weightOfVisit.push_back(weight);
        }
      );
      const auto visits = static_cast<int>(indexOfVisit.size());

      THEN("The loop visits every node once in index order with its values, position, cell size and weight") {
        VALIDATE_EQUALS(visits, WalkNodes);
        const double cellVolume = std::pow(WalkCellWidth, DIMENSIONS);
        for (int visit = 0; visit < visits; visit++) {
          const int index = indexOfVisit[visit];
          VALIDATE_EQUALS(index, visit);
          VALIDATE_EQUALS(offsetOfVisit[visit], static_cast<std::ptrdiff_t>(index) * WalkValues);
          int remainingIndex = index;
          for (int axis = 0; axis < DIMENSIONS; axis++) {
            VALIDATE_NUMERICAL_EQUALS(
              positionOfVisit[visit](axis),
              0.5 + (remainingIndex % WalkNodesPerAxis) * 1.0,
              index,
              axis
            );
            VALIDATE_NUMERICAL_EQUALS(cellSizeOfVisit[visit](axis), WalkCellWidth, index, axis);
            remainingIndex /= WalkNodesPerAxis;
          }
          VALIDATE_NUMERICAL_EQUALS(weightOfVisit[visit], cellVolume / WalkNodes);
        }
      }
    }
  }
}

UNIT_TEST("The loop over the nodes passes a single-precision buffer to the functor as a pointer into that buffer") {
  GIVEN("A cell of width two around the point one and a float buffer of three values per node counting from zero") {
    const tarch::la::Vector<DIMENSIONS, double> cellCentre(1.0);
    const tarch::la::Vector<DIMENSIONS, double> cellSize(WalkCellWidth);
    std::array<float, WalkEntries>              values{};
    for (int entry = 0; entry < WalkNodes * WalkValues; entry++) {
      values[entry] = static_cast<float>(entry);
    }
    const float* const constValues = values.data();

    WHEN("The loop over the read-only buffer records the first value of every node and sums the last ones") {
      std::vector<int>    indexOfVisit;
      std::vector<double> firstValueOfVisit;
      double              sum = 0.0;
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
          indexOfVisit.push_back(index);
          firstValueOfVisit.push_back(static_cast<double>(value[0]));
          sum += value[WalkValues - 1];
        }
      );

      THEN("Every visit reads the first value of its own node and the last values sum to their total") {
        for (std::size_t visit = 0; visit < indexOfVisit.size(); visit++) {
          VALIDATE_NUMERICAL_EQUALS(firstValueOfVisit[visit], static_cast<double>(indexOfVisit[visit] * WalkValues));
        }
        double expected = 0.0;
        for (int node = 0; node < WalkNodes; node++) {
          expected += node * WalkValues + WalkValues - 1;
        }
        VALIDATE_NUMERICAL_EQUALS(sum, expected);
      }
    }
  }
}

UNIT_TEST("A polynomial of order one returns the affine field it interpolates at every point of the cell") {
  GIVEN("A cell, the values of an affine field at its nodes of order one and the position of its first node") {
    constexpr int Order        = 1;
    constexpr int NodesPerCell = exahype2::dg::getNodesPerCell(Order + 1);
    const auto    cellCentre   = getRampOverAxes(0.4, 0.2);
    const auto    cellSize     = tarch::la::Vector<DIMENSIONS, double>(0.2);

    double values[NodesPerCell];
    fillAffineNodalValues(cellCentre, cellSize, Order, QuadratureNodes1DP1, values);

    auto firstNode = cellCentre - 0.5 * cellSize;
    for (int axis = 0; axis < DIMENSIONS; axis++) {
      firstNode(axis) += QuadratureNodes1DP1[0] * cellSize(axis);
    }

    WHEN("The test evaluates the polynomial at the first node and at the cell centre") {
      const double valueAtFirstNode =
        exahype2::dg::evaluatePolynomial(cellCentre, cellSize, Order, QuadratureNodes1DP1, 1, values, firstNode, 0);
      const double valueAtCentre =
        exahype2::dg::evaluatePolynomial(cellCentre, cellSize, Order, QuadratureNodes1DP1, 1, values, cellCentre, 0);

      THEN("Both values equal the affine field at their position") {
        VALIDATE_NUMERICAL_EQUALS(valueAtFirstNode, getAffineField(firstNode), Order);
        VALIDATE_NUMERICAL_EQUALS(valueAtCentre, getAffineField(cellCentre), Order);
      }
    }
  }
}

UNIT_TEST("A polynomial of order two returns the affine field it interpolates at every point of the cell") {
  GIVEN("A cell, the values of an affine field at its nodes of order two and the position of its first node") {
    constexpr int Order        = 2;
    constexpr int NodesPerCell = exahype2::dg::getNodesPerCell(Order + 1);
    const auto    cellCentre   = getRampOverAxes(0.4, 0.2);
    const auto    cellSize     = tarch::la::Vector<DIMENSIONS, double>(0.2);

    double values[NodesPerCell];
    fillAffineNodalValues(cellCentre, cellSize, Order, QuadratureNodes1DP2, values);

    auto firstNode = cellCentre - 0.5 * cellSize;
    for (int axis = 0; axis < DIMENSIONS; axis++) {
      firstNode(axis) += QuadratureNodes1DP2[0] * cellSize(axis);
    }

    WHEN("The test evaluates the polynomial at the first node and at the cell centre") {
      const double valueAtFirstNode =
        exahype2::dg::evaluatePolynomial(cellCentre, cellSize, Order, QuadratureNodes1DP2, 1, values, firstNode, 0);
      const double valueAtCentre =
        exahype2::dg::evaluatePolynomial(cellCentre, cellSize, Order, QuadratureNodes1DP2, 1, values, cellCentre, 0);

      THEN("Both values equal the affine field at their position") {
        VALIDATE_NUMERICAL_EQUALS(valueAtFirstNode, getAffineField(firstNode), Order);
        VALIDATE_NUMERICAL_EQUALS(valueAtCentre, getAffineField(cellCentre), Order);
      }
    }
  }
}

UNIT_TEST("The references and bounds of the transfer terms follow the value ranges of their entries") {
  GIVEN("Two arrays of three float nodes with two values, their negations and arrays with a constant entry") {
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

    WHEN("The references and bounds of both arrays and of their negations take the largest weight 0.25") {
      double negatedReference[Values] = {};
      double negatedBound[Values]     = {};
      bool   negatedVaries[Values]    = {};
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

      THEN("Every reference is the midpoint of the value range of its entry and both entries vary") {
        VALIDATE_EQUALS(reference[0], 0.5 * (static_cast<double>(0.1F) + 5.0));
        VALIDATE_EQUALS(reference[1], 0.5 * (-1.0 + 7.0));
        VALIDATE(varies[0] and varies[1]);
      }

      THEN("Every bound is the larger of the magnitude of its reference and the weighted width of its range") {
        VALIDATE_EQUALS(bound[0], 0.5 * (static_cast<double>(0.1F) + 5.0));
        VALIDATE_EQUALS(bound[1], 3.0);
      }

      THEN("The negated nodes negate the references and keep the bounds") {
        VALIDATE_EQUALS(negatedReference[0], -reference[0]);
        VALIDATE_EQUALS(negatedReference[1], -reference[1]);
        VALIDATE_EQUALS(negatedBound[0], bound[0]);
        VALIDATE_EQUALS(negatedBound[1], bound[1]);
        VALIDATE(negatedVaries[0] and negatedVaries[1]);
      }
    }

    WHEN("The references and bounds of the first node of the second array take the weight four") {
      const float* constantArrays[1] = {second};
      exahype2::dg::computeReferencesAndBoundsOfTransferTerms<Values>(
        reference,
        bound,
        varies,
        constantArrays,
        1,
        1,
        4.0
      );

      THEN("The first reference and bound are the first value of the node and every entry is constant") {
        VALIDATE_EQUALS(reference[0], static_cast<double>(0.1F));
        VALIDATE_EQUALS(bound[0], static_cast<double>(0.1F));
        VALIDATE(not varies[0] and not varies[1]);
      }
    }

    WHEN("The references and bounds of three nodes with a constant first entry take the weight four") {
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

      THEN("The first reference is the constant and only the second entry varies") {
        VALIDATE_EQUALS(reference[0], 2.0);
        VALIDATE(not varies[0] and varies[1]);
      }
    }
  }
}

UNIT_TEST("The largest product of entries is the power of the largest magnitude among the entries") {
  GIVEN("A matrix of the entries 0.5, -1.5, 0.25 and 1") {
    const double matrix[4] = {0.5, -1.5, 0.25, 1.0};

    THEN("The largest products of zero, one and three entries are one, 1.5 and the cube of 1.5") {
      VALIDATE_EQUALS(exahype2::dg::getLargestProductOfEntries<0>(matrix, 4), 1.0);
      VALIDATE_EQUALS(exahype2::dg::getLargestProductOfEntries<1>(matrix, 4), 1.5);
      VALIDATE_EQUALS(exahype2::dg::getLargestProductOfEntries<3>(matrix, 4), 1.5 * 1.5 * 1.5);
    }
  }
}

UNIT_TEST("The operator along an axis sums the products of the source values and the operator entries") {
  GIVEN("Distinct values of three entries at two source nodes and distinct operator entries for four target nodes") {
    double values[NumberOfSourceNodes * NumberOfEntries];
    double operatorMatrix[NumberOfTargetNodes * NumberOfSourceNodes];
    fillOperands(values, operatorMatrix);

    WHEN("The operator overwrites a zero result with its product") {
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

      THEN("Every target entry is the sum over the source nodes of the value times the operator entry") {
        for (int targetNode = 0; targetNode < NumberOfTargetNodes; targetNode++) {
          for (int entry = 0; entry < NumberOfEntries; entry++) {
            double expected = 0.0;
            for (int sourceNode = 0; sourceNode < NumberOfSourceNodes; sourceNode++) {
              expected += values[sourceNode * NumberOfEntries + entry]
                * operatorMatrix[targetNode * NumberOfSourceNodes + sourceNode];
            }
            VALIDATE_NUMERICAL_EQUALS(result[targetNode * NumberOfEntries + entry], expected);
          }
        }
      }
    }
  }
}

UNIT_TEST("The operator along an axis adds its product to the result or overwrites the result") {
  GIVEN("Distinct values and operator entries and two results of five plus the entry index") {
    double values[NumberOfSourceNodes * NumberOfEntries];
    double operatorMatrix[NumberOfTargetNodes * NumberOfSourceNodes];
    fillOperands(values, operatorMatrix);

    double accumulated[NumberOfTargetNodes * NumberOfEntries];
    double overwritten[NumberOfTargetNodes * NumberOfEntries];
    for (int entry = 0; entry < NumberOfTargetNodes * NumberOfEntries; entry++) {
      accumulated[entry] = 5.0 + entry;
      overwritten[entry] = 5.0 + entry;
    }

    WHEN("The operator adds its product to the first result and overwrites the second result") {
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

      THEN("The first result exceeds the second one by its former value") {
        for (int entry = 0; entry < NumberOfTargetNodes * NumberOfEntries; entry++) {
          VALIDATE_NUMERICAL_EQUALS(accumulated[entry], overwritten[entry] + 5.0 + entry);
        }
      }
    }
  }
}

UNIT_TEST("The scaling of the operator along an axis multiplies every product") {
  GIVEN("Distinct values and operator entries") {
    double values[NumberOfSourceNodes * NumberOfEntries];
    double operatorMatrix[NumberOfTargetNodes * NumberOfSourceNodes];
    fillOperands(values, operatorMatrix);

    WHEN("The operator overwrites two zero results with its product under the scalings one and minus three") {
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

      THEN("Every scaled entry is minus three times its unscaled entry") {
        for (int entry = 0; entry < NumberOfTargetNodes * NumberOfEntries; entry++) {
          VALIDATE_NUMERICAL_EQUALS(scaled[entry], -3.0 * unscaled[entry]);
        }
      }
    }
  }
}

UNIT_TEST("The strides of the operator along an axis address the entries of every node and skip the padding") {
  GIVEN("Values, operator entries and a result with padding behind every node, the result filled with seven") {
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

    WHEN("The operator overwrites the result with its product") {
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

      THEN("Every target entry is the sum of its products and the padding between two result nodes keeps seven") {
        for (int targetNode = 0; targetNode < NumberOfTargetNodes; targetNode++) {
          for (int entry = 0; entry < NumberOfEntries; entry++) {
            double expected = 0.0;
            for (int sourceNode = 0; sourceNode < NumberOfSourceNodes; sourceNode++) {
              expected +=
                values[sourceNode * ValueStride + entry] * operatorMatrix[targetNode * OperatorStride + sourceNode];
            }
            VALIDATE_NUMERICAL_EQUALS(result[targetNode * ResultStride + entry], expected);
          }
          for (int entry = NumberOfEntries; entry < ResultStride; entry++) {
            VALIDATE_NUMERICAL_EQUALS(result[targetNode * ResultStride + entry], 7.0);
          }
        }
      }
    }
  }
}

UNIT_TEST("The operator along an axis rounds the product of doubles once into a float result") {
  GIVEN("A double value and operator entry whose product a float cannot represent and a float result of zero") {
    constexpr double Operand = 1.0 + 1.0 / 1048576.0;

    const double values[1]         = {Operand};
    const double operatorMatrix[1] = {Operand};
    float        result[1]         = {0.0F};

    WHEN("The operator adds its product to the float result in double precision") {
      exahype2::dg::applyOperatorAlongAxis<1, 1, 1, 1, 1, 1, false, float, double>(result, values, operatorMatrix, 1.0);

      THEN("The result is the double product rounded to float") {
        const auto expected = static_cast<double>(static_cast<float>(Operand * Operand));
        VALIDATE_NUMERICAL_EQUALS(static_cast<double>(result[0]), expected);
      }
    }
  }
}

UNIT_TEST("The half sum and the half difference of the weights of a mirror node pair recombine to both weights") {
  GIVEN("The derivative operator and its weights of the mirror node pairs") {
    const DerivativeOperator derivativeOperator;
    const auto               derivativeWeights =
      exahype2::dg::getDerivativeWeightsOfNodePairs<NodesPerAxis>(derivativeOperator.entries);

    THEN("The sum weight plus and minus the difference weight returns the operator entries of both nodes") {
      for (int row = 0; row < NodesPerAxis; row++) {
        for (int pair = 0; pair < NodesPerAxis / 2; pair++) {
          VALIDATE_NUMERICAL_EQUALS(
            derivativeWeights.sumWeights[row][pair] + derivativeWeights.differenceWeights[row][pair],
            derivativeOperator.entries[row * NodesPerAxis + pair],
            row,
            pair
          );
          VALIDATE_NUMERICAL_EQUALS(
            derivativeWeights.sumWeights[row][pair] - derivativeWeights.differenceWeights[row][pair],
            derivativeOperator.entries[row * NodesPerAxis + NodesPerAxis - 1 - pair],
            row,
            pair
          );
        }
      }
    }
  }
}

UNIT_TEST("The derivative of a constant line is zero bitwise") {
  GIVEN("The derivative operator, a line of constants without an exact binary representation and a result of sevens") {
    const DerivativeOperator derivativeOperator;

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

    WHEN("The derivative of the line under the scalings 3.7 and one over 0.3 overwrites the result") {
      exahype2::dg::differentiateAlongAxis<NumberOfEntries, NodesPerAxis, true, double, double>(
        result,
        NumberOfEntries,
        values,
        NumberOfEntries,
        exahype2::dg::getDerivativeWeightsOfNodePairs<NodesPerAxis>(derivativeOperator.entries),
        3.7,
        1.0 / 0.3
      );

      THEN("Every entry of the result is zero") {
        for (int entry = 0; entry < NodesPerAxis * NumberOfEntries; entry++) {
          VALIDATE_EQUALS(result[entry], 0.0, entry);
        }
      }
    }
  }
}

UNIT_TEST("The derivative along an axis acts on the differences from the centre node") {
  GIVEN("A falling line, its differences from its centre node and two results counting up from five") {
    const DerivativeOperator derivativeOperator;
    constexpr int            CentreNode = NodesPerAxis / 2;

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

    WHEN("The derivative of the line and the operator on the differences add to the results with the scaling 0.25") {
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

      THEN("Both results agree bitwise and change by the slope of the line times the scaling") {
        for (int node = 0; node < NodesPerAxis; node++) {
          for (int entry = 0; entry < NumberOfEntries; entry++) {
            VALIDATE_EQUALS(
              accumulated[node * NumberOfEntries + entry],
              expected[node * NumberOfEntries + entry],
              node,
              entry
            );
            VALIDATE_NUMERICAL_EQUALS(
              accumulated[node * NumberOfEntries + entry],
              5.0 + node * NumberOfEntries + entry - 0.25 * (1.5 + entry),
              node,
              entry
            );
          }
        }
      }
    }
  }
}

UNIT_TEST("A derivative that overwrites or accumulates into its own line returns the bits of one into another buffer") {
  GIVEN("The weights of the derivative operator and a line whose values are not dyadic") {
    const DerivativeOperator derivativeOperator;
    const auto               derivativeWeights =
      exahype2::dg::getDerivativeWeightsOfNodePairs<NodesPerAxis>(derivativeOperator.entries);
    double values[NodesPerAxis * NumberOfEntries];
    for (int node = 0; node < NodesPerAxis; node++) {
      for (int entry = 0; entry < NumberOfEntries; entry++) {
        values[node * NumberOfEntries + entry] = 1.0 / 7.0 * (1 + entry) + 0.3 * node * node - 0.1 * entry * node;
      }
    }

    WHEN("The derivative overwrites and accumulates into another buffer and into the line itself") {
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

      THEN("The derivatives in place have the bits of the derivatives into another buffer") {
        for (int entry = 0; entry < NodesPerAxis * NumberOfEntries; entry++) {
          VALIDATE_EQUALS(overwrittenInPlace[entry], overwritten[entry], entry);
          VALIDATE_EQUALS(accumulatedInPlace[entry], accumulated[entry], entry);
        }
      }
    }
  }
}

UNIT_TEST("The derivative of a reflected line is the reflected derivative with the opposite sign change bitwise") {
  GIVEN("Derivative operators of two to five nodes that the reflection of their rows and columns negates") {
    static constexpr double TwoNodeOperator[4]   = {-1.1, 1.1, -1.1, 1.1};
    static constexpr double ThreeNodeOperator[9] = {-1.7, 2.3, -0.6, -0.45, 0.0, 0.45, 0.6, -2.3, 1.7};
    static constexpr double FourNodeOperator[16] =
      {-2.7, 3.9, -1.7, 0.5, -0.8, -0.3, 1.4, -0.3, 0.3, -1.4, 0.3, 0.8, -0.5, 1.7, -3.9, 2.7};
    static constexpr double FiveNodeOperator[25] = {-5.1, 6.9, -2.9, 1.3,  -0.2, -1.2, -0.6, 2.4, -0.8,
                                                    0.2,  0.4, -1.3, 0.0,  1.3,  -0.4, -0.2, 0.8, -2.4,
                                                    0.6,  1.2, 0.2,  -1.3, 2.9,  -6.9, 5.1};

    WHEN("Every operator differentiates a line and the line reflected with and without its sign swapped") {
      const int twoNodeMismatches   = countReflectedDerivativeMismatches<2>(TwoNodeOperator);
      const int threeNodeMismatches = countReflectedDerivativeMismatches<3>(ThreeNodeOperator);
      const int fourNodeMismatches  = countReflectedDerivativeMismatches<4>(FourNodeOperator);
      const int fiveNodeMismatches  = countReflectedDerivativeMismatches<5>(FiveNodeOperator);

      THEN("Every entry of every reflected derivative is the negated or kept entry of the derivative bitwise") {
        VALIDATE_EQUALS(twoNodeMismatches, 0);
        VALIDATE_EQUALS(threeNodeMismatches, 0);
        VALIDATE_EQUALS(fourNodeMismatches, 0);
        VALIDATE_EQUALS(fiveNodeMismatches, 0);
      }
    }
  }
}

UNIT_TEST("A dispatch differentiates lines of eight and sixteen nodes like the host") {
  GIVEN("Sixty-four lines of eight nodes and sixty-four lines of sixteen nodes") {
    WHEN("A dispatch and the host differentiate every line") {
      const auto [eightNodeDispatch, eightNodeHost]     = getDerivativesOfDispatchAndHost<8>();
      const auto [sixteenNodeDispatch, sixteenNodeHost] = getDerivativesOfDispatchAndHost<16>();

      THEN("The derivatives of the dispatch equal the derivatives of the host") {
        VALIDATE_NUMERICAL_BUFFERS_EQUALS(eightNodeDispatch, eightNodeHost);
        VALIDATE_NUMERICAL_BUFFERS_EQUALS(sixteenNodeDispatch, sixteenNodeHost);
      }
    }
  }
}

UNIT_TEST("The nodal gradients of a linear state are its physical slopes") {
  GIVEN("A cell whose extent differs per axis and a state whose values grow with their own slope along every axis") {
    const DerivativeOperator derivativeOperator;
    const auto               cellSize = getCellSize();

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

    WHEN("The test computes the nodal gradients") {
      double gradients[DIMENSIONS * EntriesPerAxis];
      exahype2::dg::computeNodalGradients<NodesPerAxis, NumberOfData, double, double>(
        gradients,
        cellState,
        derivativeOperator.entries,
        cellSize
      );

      THEN("Every gradient is the slope over the reference interval divided by the cell extent along its axis") {
        for (int axis = 0; axis < DIMENSIONS; axis++) {
          for (int cellNode = 0; cellNode < CellNodes; cellNode++) {
            for (int value = 0; value < NumberOfData; value++) {
              const double expected = (2.0 + value + axis) / cellSize(axis);
              VALIDATE_NUMERICAL_EQUALS(
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
    }
  }
}

UNIT_TEST("Each block of the nodal gradients stores the derivative along its own axis") {
  GIVEN("A cell whose extent differs per axis and, per axis, a state that is quadratic along that axis alone") {
    const DerivativeOperator derivativeOperator;
    const auto               cellSize = getCellSize();

    double cellStates[DIMENSIONS][CellNodes * NumberOfData];
    for (int varyingAxis = 0; varyingAxis < DIMENSIONS; varyingAxis++) {
      for (int cellNode = 0; cellNode < CellNodes; cellNode++) {
        const double reference = QuadratureNodes[peano4::dDelinearised(cellNode, NodesPerAxis)(varyingAxis)];
        for (int value = 0; value < NumberOfData; value++) {
          cellStates[varyingAxis][cellNode * NumberOfData + value] = 3.0 * reference * reference;
        }
      }
    }

    WHEN("The test computes the nodal gradients of every state") {
      double gradients[DIMENSIONS][DIMENSIONS * EntriesPerAxis];
      for (int varyingAxis = 0; varyingAxis < DIMENSIONS; varyingAxis++) {
        exahype2::dg::computeNodalGradients<NodesPerAxis, NumberOfData, double, double>(
          gradients[varyingAxis],
          cellStates[varyingAxis],
          derivativeOperator.entries,
          cellSize
        );
      }

      THEN("The block of the varying axis contains the derivative over the cell extent and every other block zero") {
        for (int varyingAxis = 0; varyingAxis < DIMENSIONS; varyingAxis++) {
          for (int axis = 0; axis < DIMENSIONS; axis++) {
            for (int cellNode = 0; cellNode < CellNodes; cellNode++) {
              const double reference = QuadratureNodes[peano4::dDelinearised(cellNode, NodesPerAxis)(varyingAxis)];
              const double expected  = axis == varyingAxis ? 6.0 * reference / cellSize(axis) : 0.0;
              for (int value = 0; value < NumberOfData; value++) {
                VALIDATE_NUMERICAL_EQUALS(
                  gradients[varyingAxis][axis * EntriesPerAxis + cellNode * NumberOfData + value],
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
    }
  }
}

UNIT_TEST("The gradient of a state in float storage evaluates in double precision") {
  GIVEN("A cell whose extent differs per axis and a float state that grows with the slope two along the first axis") {
    const DerivativeOperator derivativeOperator;
    const auto               cellSize = getCellSize();

    float cellState[CellNodes * NumberOfData];
    for (int cellNode = 0; cellNode < CellNodes; cellNode++) {
      const double reference = QuadratureNodes[peano4::dDelinearised(cellNode, NodesPerAxis)(0)];
      for (int value = 0; value < NumberOfData; value++) {
        cellState[cellNode * NumberOfData + value] = static_cast<float>(0.5 + 2.0 * reference);
      }
    }

    WHEN("The test computes the nodal gradients of the float state in double precision") {
      double gradients[DIMENSIONS * EntriesPerAxis];
      exahype2::dg::computeNodalGradients<NodesPerAxis, NumberOfData, float, double>(
        gradients,
        cellState,
        derivativeOperator.entries,
        cellSize
      );

      THEN("The first block contains the double-precision derivative of the float state and every other block zero") {
        for (int cellNode = 0; cellNode < CellNodes; cellNode++) {
          tarch::la::Vector<DIMENSIONS, int> lineNode   = peano4::dDelinearised(cellNode, NodesPerAxis);
          const int                          nodeOnAxis = lineNode(0);
          for (int value = 0; value < NumberOfData; value++) {
            double derivative = 0.0;
            for (int node = 0; node < NodesPerAxis; node++) {
              lineNode(0)        = node;
              const double state = cellState[peano4::dLinearised(lineNode, NodesPerAxis) * NumberOfData + value];
              derivative += derivativeOperator.entries[nodeOnAxis * NodesPerAxis + node] * state / cellSize(0);
            }
            for (int axis = 0; axis < DIMENSIONS; axis++) {
              const double gradient = gradients[axis * EntriesPerAxis + cellNode * NumberOfData + value];
              const double expected = axis == 0 ? derivative : 0.0;
              VALIDATE_NUMERICAL_EQUALS(gradient, expected, axis, cellNode);
            }
          }
        }
      }
    }
  }
}

UNIT_TEST("The nodal gradients of a constant state are zero bitwise") {
  GIVEN("A cell whose extent differs per axis and a constant state without an exact binary representation") {
    const DerivativeOperator derivativeOperator;
    const auto               cellSize = getCellSize();

    double cellState[CellNodes * NumberOfData];
    for (int cellNode = 0; cellNode < CellNodes; cellNode++) {
      for (int value = 0; value < NumberOfData; value++) {
        cellState[cellNode * NumberOfData + value] = 1.0 / 3.0 + 0.1 * value;
      }
    }

    WHEN("The test computes the nodal gradients") {
      double gradients[DIMENSIONS * EntriesPerAxis];
      exahype2::dg::computeNodalGradients<NodesPerAxis, NumberOfData, double, double>(
        gradients,
        cellState,
        derivativeOperator.entries,
        cellSize
      );

      THEN("Every gradient is zero") {
        for (int entry = 0; entry < DIMENSIONS * EntriesPerAxis; entry++) {
          VALIDATE_EQUALS(gradients[entry], 0.0, entry);
        }
      }
    }
  }
}

UNIT_TEST("A flux that is linear along its own axis differentiates to its constant physical slope") {
  GIVEN("The inverse extents of a cell and a padded flux whose unknowns grow along the axis of their block") {
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

    WHEN("The test differentiates the flux in place") {
      exahype2::dg::differentiateFluxInPlace<Order2Layout>(flux, derivativeOperator.entries, inverseCellSize);

      THEN("Every unknown equals its slope times the inverse extent and every padding entry equals zero") {
        for (int axis = 0; axis < DIMENSIONS; axis++) {
          for (int cellNode = 0; cellNode < CellNodes; cellNode++) {
            const int base = axis * FluxAxisStride + cellNode * PaddedCell::PaddedNumberOfUnknowns;
            for (int unknown = 0; unknown < NumberOfUnknowns; unknown++) {
              VALIDATE_NUMERICAL_EQUALS(
                flux[base + unknown],
                3.0 * (1.0 + axis) * inverseCellSize(axis),
                axis,
                cellNode,
                unknown
              );
            }
            for (int unknown = NumberOfUnknowns; unknown < PaddedCell::PaddedNumberOfUnknowns; unknown++) {
              VALIDATE_NUMERICAL_EQUALS(flux[base + unknown], 0.0, axis, cellNode, unknown);
            }
          }
        }
      }
    }
  }
}

UNIT_TEST("A flux that is constant along every axis differentiates to zeros bitwise") {
  GIVEN("The inverse extents of a cell and a padded flux of constants without an exact binary representation") {
    const DerivativeOperator derivativeOperator;
    const auto               cellSize = getCellSize();

    tarch::la::Vector<DIMENSIONS, double> inverseCellSize;
    for (int axis = 0; axis < DIMENSIONS; axis++) {
      inverseCellSize(axis) = 1.0 / cellSize(axis);
    }

    double flux[DIMENSIONS * FluxAxisStride] = {};
    for (int axis = 0; axis < DIMENSIONS; axis++) {
      for (int cellNode = 0; cellNode < CellNodes; cellNode++) {
        for (int unknown = 0; unknown < NumberOfUnknowns; unknown++) {
          flux[axis * FluxAxisStride + cellNode * PaddedCell::PaddedNumberOfUnknowns + unknown] =
            1.0 / 3.0 + 0.1 * unknown;
        }
      }
    }

    WHEN("The test differentiates the flux in place") {
      exahype2::dg::differentiateFluxInPlace<Order2Layout>(flux, derivativeOperator.entries, inverseCellSize);

      THEN("Every entry of the flux is zero") {
        for (int entry = 0; entry < DIMENSIONS * FluxAxisStride; entry++) {
          VALIDATE_EQUALS(flux[entry], 0.0, entry);
        }
      }
    }
  }
}

UNIT_TEST("Two calls over alternate lines differentiate the flux bitwise like one call over all lines") {
  GIVEN("The inverse extents of a cell and two copies of a padded flux that varies per axis, node and unknown") {
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

    WHEN("One call differentiates the first copy and two calls over alternate lines differentiate the second") {
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

      THEN("Both copies store the same bits") {
        for (int entry = 0; entry < DIMENSIONS * FluxAxisStride; entry++) {
          VALIDATE_EQUALS(alternateFlux[entry], wholeFlux[entry], entry);
        }
      }
    }
  }
}

UNIT_TEST("The projection with a padded node stride reads the entries of every node and skips the lanes behind them") {
  GIVEN("A field with 1000 in the lanes behind the entries of every node, averaging coefficients and faces of nines") {
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

    double lowerFaces[DIMENSIONS][FaceNodes * NumberOfData];
    double upperFaces[DIMENSIONS][FaceNodes * NumberOfData];
    for (int direction = 0; direction < DIMENSIONS; direction++) {
      for (int entry = 0; entry < FaceNodes * NumberOfData; entry++) {
        lowerFaces[direction][entry] = 9.0;
        upperFaces[direction][entry] = 9.0;
      }
    }

    WHEN("The test extrapolates the field to the face pair of every direction") {
      for (int direction = 0; direction < DIMENSIONS; direction++) {
        exahype2::dg::extrapolateNodalFieldToFacePair<
          NodesPerAxis,
          NumberOfData,
          PaddedNodeStride,
          double,
          double,
          double
        >(lowerFaces[direction], upperFaces[direction], field, coefficients, coefficients, direction);
      }

      THEN("Both faces contain the average of the entries along the line of every face node") {
        for (int direction = 0; direction < DIMENSIONS; direction++) {
          for (int faceNode = 0; faceNode < FaceNodes; faceNode++) {
            for (int entry = 0; entry < NumberOfData; entry++) {
              double expected = 0.0;
              for (int normalIndex = 0; normalIndex < NodesPerAxis; normalIndex++) {
                const int cellNode = getCellNodeInAxisOrder(direction, faceNode, normalIndex);
                expected += field[cellNode * PaddedNodeStride + entry] / NodesPerAxis;
              }
              VALIDATE_NUMERICAL_EQUALS(
                lowerFaces[direction][faceNode * NumberOfData + entry],
                expected,
                direction,
                faceNode,
                entry
              );
              VALIDATE_NUMERICAL_EQUALS(
                upperFaces[direction][faceNode * NumberOfData + entry],
                expected,
                direction,
                faceNode,
                entry
              );
            }
          }
        }
      }
    }
  }
}

UNIT_TEST("The lower and the upper face use their own coefficients") {
  GIVEN("A field that grows with the entry index and the values of the basis functions at both ends of the interval") {
    double field[CellNodes * NumberOfData];
    for (int entry = 0; entry < CellNodes * NumberOfData; entry++) {
      field[entry] = 1.0 + 0.5 * entry;
    }

    const EndCoefficients coefficients;
    const double* const   leftCoefficients  = coefficients.left;
    const double* const   rightCoefficients = coefficients.right;

    WHEN("The test extrapolates the field to the face pair of every direction") {
      double lowerFaces[DIMENSIONS][FaceNodes * NumberOfData] = {};
      double upperFaces[DIMENSIONS][FaceNodes * NumberOfData] = {};
      for (int direction = 0; direction < DIMENSIONS; direction++) {
        exahype2::dg::extrapolateNodalFieldToFacePair<NodesPerAxis, NumberOfData, NumberOfData, double, double, double>(
          lowerFaces[direction],
          upperFaces[direction],
          field,
          leftCoefficients,
          rightCoefficients,
          direction
        );
      }

      THEN("The lower face stores the sum with the left coefficients and the upper face the sum with the right ones") {
        for (int direction = 0; direction < DIMENSIONS; direction++) {
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
              VALIDATE_NUMERICAL_EQUALS(
                lowerFaces[direction][faceNode * NumberOfData + entry],
                expectedLower,
                direction,
                faceNode,
                entry
              );
              VALIDATE_NUMERICAL_EQUALS(
                upperFaces[direction][faceNode * NumberOfData + entry],
                expectedUpper,
                direction,
                faceNode,
                entry
              );
            }
          }
        }
      }
    }
  }
}

UNIT_TEST("The faces of a reflected field are the swapped faces of the field with the same sign change bitwise") {
  GIVEN("Coefficients of two to five nodes whose right ones are the reflected left ones and sum to one") {
    static constexpr double TwoNodeLeft[2]    = {0.7, 0.3};
    static constexpr double TwoNodeRight[2]   = {0.3, 0.7};
    static constexpr double ThreeNodeLeft[3]  = {0.77, 0.46, -0.23};
    static constexpr double ThreeNodeRight[3] = {-0.23, 0.46, 0.77};
    static constexpr double FourNodeLeft[4]   = {1.1, -0.3, 0.27, -0.07};
    static constexpr double FourNodeRight[4]  = {-0.07, 0.27, -0.3, 1.1};
    static constexpr double FiveNodeLeft[5]   = {1.3, -0.6, 0.45, -0.2, 0.05};
    static constexpr double FiveNodeRight[5]  = {0.05, -0.2, 0.45, -0.6, 1.3};

    WHEN("Every pair of coefficients extrapolates a field and the field reflected with and without its sign swapped") {
      const int twoNodeMismatches   = countReflectedFaceMismatches<2>(TwoNodeLeft, TwoNodeRight);
      const int threeNodeMismatches = countReflectedFaceMismatches<3>(ThreeNodeLeft, ThreeNodeRight);
      const int fourNodeMismatches  = countReflectedFaceMismatches<4>(FourNodeLeft, FourNodeRight);
      const int fiveNodeMismatches  = countReflectedFaceMismatches<5>(FiveNodeLeft, FiveNodeRight);

      THEN("The lower face of every reflected field is the upper face of the field and the other way round") {
        VALIDATE_EQUALS(twoNodeMismatches, 0);
        VALIDATE_EQUALS(threeNodeMismatches, 0);
        VALIDATE_EQUALS(fourNodeMismatches, 0);
        VALIDATE_EQUALS(fiveNodeMismatches, 0);
      }
    }
  }
}

UNIT_TEST("The face nodes run with the lowest tangential axis fastest") {
  GIVEN("A field with its own value at every cell node and unit coefficient vectors, the right one mirrored") {
    double field[CellNodes * NumberOfData];
    for (int cellNode = 0; cellNode < CellNodes; cellNode++) {
      for (int entry = 0; entry < NumberOfData; entry++) {
        field[cellNode * NumberOfData + entry] = 1.0 + cellNode + 0.125 * entry;
      }
    }

    double leftCoefficients[NodesPerAxis][NodesPerAxis]  = {};
    double rightCoefficients[NodesPerAxis][NodesPerAxis] = {};
    for (int markedIndex = 0; markedIndex < NodesPerAxis; markedIndex++) {
      leftCoefficients[markedIndex][markedIndex]                     = 1.0;
      rightCoefficients[markedIndex][NodesPerAxis - 1 - markedIndex] = 1.0;
    }

    WHEN("The test extrapolates the field to both faces per direction with the coefficients of every marked index") {
      double lowerFaces[DIMENSIONS][NodesPerAxis][FaceNodes * NumberOfData] = {};
      double upperFaces[DIMENSIONS][NodesPerAxis][FaceNodes * NumberOfData] = {};
      for (int direction = 0; direction < DIMENSIONS; direction++) {
        for (int markedIndex = 0; markedIndex < NodesPerAxis; markedIndex++) {
          exahype2::dg::extrapolateNodalFieldToFacePair<
            NodesPerAxis,
            NumberOfData,
            NumberOfData,
            double,
            double,
            double
          >(lowerFaces[direction][markedIndex],
            upperFaces[direction][markedIndex],
            field,
            leftCoefficients[markedIndex],
            rightCoefficients[markedIndex],
            direction);
        }
      }

      THEN("Every face node equals the value of the cell node at the marked position of its line") {
        for (int direction = 0; direction < DIMENSIONS; direction++) {
          for (int markedIndex = 0; markedIndex < NodesPerAxis; markedIndex++) {
            for (int faceNode = 0; faceNode < FaceNodes; faceNode++) {
              const int lowerCellNode = getCellNodeInAxisOrder(direction, faceNode, markedIndex);
              const int upperCellNode = getCellNodeInAxisOrder(direction, faceNode, NodesPerAxis - 1 - markedIndex);
              for (int entry = 0; entry < NumberOfData; entry++) {
                VALIDATE_NUMERICAL_EQUALS(
                  lowerFaces[direction][markedIndex][faceNode * NumberOfData + entry],
                  field[lowerCellNode * NumberOfData + entry],
                  direction,
                  markedIndex,
                  faceNode
                );
                VALIDATE_NUMERICAL_EQUALS(
                  upperFaces[direction][markedIndex][faceNode * NumberOfData + entry],
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
    }
  }
}

UNIT_TEST("The extrapolation of a float field accumulates the faces in double precision") {
  GIVEN("Per direction, a float field with 1024 at the first node of every line and 1/16384 at the other nodes") {
    // Half the small value is half the distance between half the large value and its float
    // neighbour. A sum in float drops it and a sum in double keeps it.
    constexpr double LargeValue = 1024.0;
    constexpr double SmallValue = 0.00006103515625;

    float fields[DIMENSIONS][CellNodes * NumberOfData];
    for (int direction = 0; direction < DIMENSIONS; direction++) {
      for (int faceNode = 0; faceNode < FaceNodes; faceNode++) {
        for (int normalIndex = 0; normalIndex < NodesPerAxis; normalIndex++) {
          const int  cellNode = getCellNodeInAxisOrder(direction, faceNode, normalIndex);
          const auto value    = static_cast<float>((normalIndex == 0) ? LargeValue : SmallValue);
          for (int entry = 0; entry < NumberOfData; entry++) {
            fields[direction][cellNode * NumberOfData + entry] = value;
          }
        }
      }
    }

    WHEN("The test extrapolates every field with the weight one half on the first node and on one other node") {
      const double leftCoefficients[NodesPerAxis]                   = {0.5, 0.5, 0.0};
      const double rightCoefficients[NodesPerAxis]                  = {0.5, 0.0, 0.5};
      double       lowerFaces[DIMENSIONS][FaceNodes * NumberOfData] = {};
      double       upperFaces[DIMENSIONS][FaceNodes * NumberOfData] = {};
      for (int direction = 0; direction < DIMENSIONS; direction++) {
        exahype2::dg::extrapolateNodalFieldToFacePair<NodesPerAxis, NumberOfData, NumberOfData, float, double, double>(
          lowerFaces[direction],
          upperFaces[direction],
          fields[direction],
          leftCoefficients,
          rightCoefficients,
          direction
        );
      }

      THEN("Every face node equals half the large value plus half the small value") {
        const double expected = 0.5 * LargeValue + 0.5 * SmallValue;
        for (int direction = 0; direction < DIMENSIONS; direction++) {
          for (int entry = 0; entry < FaceNodes * NumberOfData; entry++) {
            VALIDATE_EQUALS(lowerFaces[direction][entry], expected, direction, entry);
            VALIDATE_EQUALS(upperFaces[direction][entry], expected, direction, entry);
          }
        }
      }
    }
  }
}

UNIT_TEST("Threads that divide the items of the face extrapolation write the faces of the paired extrapolation") {
  GIVEN("A field per direction that rises with its entry index and the values of the basis at both ends") {
    constexpr int Threads    = 3;
    constexpr int FieldSize  = CellNodes * NumberOfData;
    constexpr int FaceValues = FaceNodes * NumberOfData;
    double        field[DIMENSIONS * FieldSize];
    for (int entry = 0; entry < DIMENSIONS * FieldSize; entry++) {
      field[entry] = 1.0 + 0.5 * entry + 0.01 * entry * entry;
    }
    const EndCoefficients coefficients;

    WHEN("The paired extrapolation writes both faces of every direction and three threads write every face") {
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

      THEN("The threads write the bits of the paired extrapolation into every face") {
        for (int face = 0; face < peano4::FacesPerCell; face++) {
          for (int entry = 0; entry < FaceValues; entry++) {
            VALIDATE_EQUALS(faceValues[face][entry], pairedFaces[face][entry], face, entry);
          }
        }
      }
    }
  }
}

UNIT_TEST("Line i of an axis runs through face node i of the faces normal to the axis") {
  GIVEN("The nodes of every line of every axis in the order of the axis") {
    THEN("The first node of every line plus the stride of its axis runs through the cell nodes of that line") {
      for (int axis = 0; axis < DIMENSIONS; axis++) {
        for (int line = 0; line < FaceNodes; line++) {
          for (int normalIndex = 0; normalIndex < NodesPerAxis; normalIndex++) {
            VALIDATE_EQUALS(
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
  }
}

UNIT_TEST("The tensor-product projection with the identity leaves 1D data unchanged") {
  GIVEN("The identity of two nodes and a 1D field of two values at each of two nodes") {
    constexpr int                           Dofs          = 2;
    constexpr int                           ValuesPerDof  = 2;
    constexpr auto                          MatrixEntries = static_cast<std::size_t>(Dofs) * Dofs;
    constexpr auto                          FieldEntries  = static_cast<std::size_t>(Dofs) * ValuesPerDof;
    const std::array<double, MatrixEntries> identity{1.0, 0.0, 0.0, 1.0};
    const std::array<double, FieldEntries>  input{1.0, 2.0, 3.0, 4.0};

    WHEN("The test projects the field with the identity") {
      std::array<double, FieldEntries> output{};
      exahype2::dg::projectTensorProduct<1, Dofs, Dofs, ValuesPerDof, ProjectionRule::PairedProducts>(
        input.data(),
        output.data(),
        identity.data()
      );

      THEN("Every output entry equals its input entry") {
        for (int index = 0; index < Dofs * ValuesPerDof; index++) {
          VALIDATE_NUMERICAL_EQUALS(output[index], input[index]);
        }
      }
    }
  }
}

UNIT_TEST("The tensor-product projection with the identity leaves 2D data unchanged") {
  GIVEN("The identity of two nodes and a 2D field of one value at each of four nodes") {
    constexpr int                           Dofs          = 2;
    constexpr auto                          MatrixEntries = static_cast<std::size_t>(Dofs) * Dofs;
    const std::array<double, MatrixEntries> identity{1.0, 0.0, 0.0, 1.0};
    const std::array<double, MatrixEntries> input{1.0, 2.0, 3.0, 4.0};

    WHEN("The test projects the field with the identity") {
      std::array<double, MatrixEntries> output{};
      exahype2::dg::projectTensorProduct<2, Dofs, Dofs, 1, ProjectionRule::PairedProducts>(
        input.data(),
        output.data(),
        identity.data()
      );

      THEN("Every output entry equals its input entry") {
        for (int index = 0; index < Dofs * Dofs; index++) {
          VALIDATE_NUMERICAL_EQUALS(output[index], input[index]);
        }
      }
    }
  }
}

UNIT_TEST("The tensor-product projection with the identity leaves 3D data unchanged") {
  GIVEN("The identity of two nodes and a 3D field of one value at each of eight nodes") {
    constexpr int                           Dofs          = 2;
    constexpr auto                          MatrixEntries = static_cast<std::size_t>(Dofs) * Dofs;
    constexpr auto                          CellEntries   = static_cast<std::size_t>(Dofs) * Dofs * Dofs;
    const std::array<double, MatrixEntries> identity{1.0, 0.0, 0.0, 1.0};
    const std::array<double, CellEntries>   input{1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0};

    WHEN("The test projects the field with the identity") {
      std::array<double, CellEntries> output{};
      exahype2::dg::projectTensorProduct<3, Dofs, Dofs, 1, ProjectionRule::PairedProducts>(
        input.data(),
        output.data(),
        identity.data()
      );

      THEN("Every output entry equals its input entry") {
        for (int index = 0; index < Dofs * Dofs * Dofs; index++) {
          VALIDATE_NUMERICAL_EQUALS(output[index], input[index]);
        }
      }
    }
  }
}

UNIT_TEST("The tensor-product projection with a matrix whose columns sum to one preserves a constant field") {
  GIVEN("A matrix of two nodes whose columns sum to one and a 2D field of seven at every node") {
    constexpr int                           Dofs          = 2;
    constexpr double                        Constant      = 7.0;
    constexpr auto                          MatrixEntries = static_cast<std::size_t>(Dofs) * Dofs;
    const std::array<double, MatrixEntries> columnStochasticMatrix{0.25, 0.75, 0.75, 0.25};
    const std::array<double, MatrixEntries> input{Constant, Constant, Constant, Constant};

    WHEN("The paired products and the butterfly project the field with the matrix") {
      std::array<double, MatrixEntries> pairedOutput{};
      std::array<double, MatrixEntries> butterflyOutput{};
      exahype2::dg::projectTensorProduct<2, Dofs, Dofs, 1, ProjectionRule::PairedProducts>(
        input.data(),
        pairedOutput.data(),
        columnStochasticMatrix.data()
      );
      exahype2::dg::projectTensorProduct<2, Dofs, Dofs, 1, ProjectionRule::Butterfly>(
        input.data(),
        butterflyOutput.data(),
        columnStochasticMatrix.data()
      );

      THEN("Every output entry of both rules is seven") {
        for (int index = 0; index < Dofs * Dofs; index++) {
          VALIDATE_NUMERICAL_EQUALS(pairedOutput[index], Constant, index);
          VALIDATE_NUMERICAL_EQUALS(butterflyOutput[index], Constant, index);
        }
      }
    }
  }
}

UNIT_TEST("The projection of a field with two swapped axes is the projection with these axes swapped bitwise") {
  GIVEN("A mirror-symmetric projection from three onto four nodes per axis and fields that are not dyadic") {
    WHEN("Both projection rules project every field in 2D and 3D and the field with every pair of axes swapped") {
      const int pairedMismatchesIn2D    = countSwappedProjectionMismatches<2, ProjectionRule::PairedProducts>();
      const int pairedMismatchesIn3D    = countSwappedProjectionMismatches<3, ProjectionRule::PairedProducts>();
      const int butterflyMismatchesIn2D = countSwappedProjectionMismatches<2, ProjectionRule::Butterfly>();
      const int butterflyMismatchesIn3D = countSwappedProjectionMismatches<3, ProjectionRule::Butterfly>();

      THEN("Every value of the swapped projection has the bits of its value of the projection") {
        VALIDATE_EQUALS(pairedMismatchesIn2D, 0);
        VALIDATE_EQUALS(pairedMismatchesIn3D, 0);
        VALIDATE_EQUALS(butterflyMismatchesIn2D, 0);
        VALIDATE_EQUALS(butterflyMismatchesIn3D, 0);
      }
    }
  }
}

UNIT_TEST("The projection of a reflected field with a negated second value is the reflected, negated projection") {
  GIVEN("A mirror-symmetric projection from three onto four nodes per axis and fields that are not dyadic") {
    WHEN("Both projection rules project every field in 2D and 3D and the field reflected along every axis") {
      const int pairedMismatchesIn2D    = countReflectedProjectionMismatches<2, ProjectionRule::PairedProducts>();
      const int pairedMismatchesIn3D    = countReflectedProjectionMismatches<3, ProjectionRule::PairedProducts>();
      const int butterflyMismatchesIn2D = countReflectedProjectionMismatches<2, ProjectionRule::Butterfly>();
      const int butterflyMismatchesIn3D = countReflectedProjectionMismatches<3, ProjectionRule::Butterfly>();

      THEN("Every value of the reflected projection has the bits of its value of the projection, the second negated") {
        VALIDATE_EQUALS(pairedMismatchesIn2D, 0);
        VALIDATE_EQUALS(pairedMismatchesIn3D, 0);
        VALIDATE_EQUALS(butterflyMismatchesIn2D, 0);
        VALIDATE_EQUALS(butterflyMismatchesIn3D, 0);
      }
    }
  }
}

UNIT_TEST("Both projection rules return the same projection up to rounding in 2D and 3D") {
  GIVEN("A mirror-symmetric projection from three onto four nodes per axis and fields that are not dyadic") {
    const std::vector<double> inputIn2D = getProjectionInput<2>();
    const std::vector<double> inputIn3D = getProjectionInput<3>();

    WHEN("The paired products and the butterfly project both fields") {
      const std::vector<double> pairedIn2D    = project<2, ProjectionRule::PairedProducts>(inputIn2D);
      const std::vector<double> butterflyIn2D = project<2, ProjectionRule::Butterfly>(inputIn2D);
      const std::vector<double> pairedIn3D    = project<3, ProjectionRule::PairedProducts>(inputIn3D);
      const std::vector<double> butterflyIn3D = project<3, ProjectionRule::Butterfly>(inputIn3D);

      THEN("The butterfly returns the paired products within rounding") {
        VALIDATE_NUMERICAL_BUFFERS_EQUALS(butterflyIn2D, pairedIn2D);
        VALIDATE_NUMERICAL_BUFFERS_EQUALS(butterflyIn3D, pairedIn3D);
      }
    }
  }
}

UNIT_TEST("Every split of the lines or of the values of each stage among threads projects the bits of one thread") {
  GIVEN("A mirror-symmetric projection from three onto four nodes per axis and fields that are not dyadic") {
    WHEN("1, 2, 3, 7 and 32 threads split every stage of both rules in 2D and 3D by lines and by values of lines") {
      const int mismatches[8] = {
        countSplitProjectionMismatches<2, ProjectionRule::PairedProducts, ProjectionItem::Line>(),
        countSplitProjectionMismatches<3, ProjectionRule::PairedProducts, ProjectionItem::Line>(),
        countSplitProjectionMismatches<2, ProjectionRule::Butterfly, ProjectionItem::Line>(),
        countSplitProjectionMismatches<3, ProjectionRule::Butterfly, ProjectionItem::Line>(),
        countSplitProjectionMismatches<2, ProjectionRule::PairedProducts, ProjectionItem::ValueOfLine>(),
        countSplitProjectionMismatches<3, ProjectionRule::PairedProducts, ProjectionItem::ValueOfLine>(),
        countSplitProjectionMismatches<2, ProjectionRule::Butterfly, ProjectionItem::ValueOfLine>(),
        countSplitProjectionMismatches<3, ProjectionRule::Butterfly, ProjectionItem::ValueOfLine>()
      };

      THEN("Every split writes the bits of one thread") {
        for (int split = 0; split < 8; split++) {
          VALIDATE_EQUALS(mismatches[split], 0, split);
        }
      }
    }
  }
}

UNIT_TEST("Every Lagrange basis function is one at its own node and zero at the others and the basis sums to one") {
  GIVEN("The point 0.3 inside the unit interval, 0.2 below its centre") {
    constexpr double Point = 0.3;

    WHEN("The test evaluates every basis function of the four matrix nodes at every node and at the point") {
      double valueAtNode[MatrixNodesPerAxis][MatrixNodesPerAxis];
      double partitionOfUnity = 0.0;
      for (int basisFunction = 0; basisFunction < MatrixNodesPerAxis; basisFunction++) {
        for (int node = 0; node < MatrixNodesPerAxis; node++) {
          valueAtNode[basisFunction][node] = exahype2::dg::getLagrangeBasisFunction1D<MatrixNodesPerAxis>(
            MatrixNodes,
            basisFunction,
            MatrixNodes[node] - 0.5
          );
        }
        partitionOfUnity +=
          exahype2::dg::getLagrangeBasisFunction1D<MatrixNodesPerAxis>(MatrixNodes, basisFunction, Point - 0.5);
      }

      THEN("Each function is one at its own node and zero at the others and all values at the point sum to one") {
        for (int basisFunction = 0; basisFunction < MatrixNodesPerAxis; basisFunction++) {
          for (int node = 0; node < MatrixNodesPerAxis; node++) {
            const double value    = valueAtNode[basisFunction][node];
            const double expected = node == basisFunction ? 1.0 : 0.0;
            VALIDATE_NUMERICAL_EQUALS(value, expected, basisFunction, node);
          }
        }
        VALIDATE_NUMERICAL_EQUALS(partitionOfUnity, 1.0);
      }
    }
  }
}

UNIT_TEST("The Lagrange basis functions reproduce a linear function of the offset from the centre of the interval") {
  GIVEN("Nine offsets from -0.5 to 0.5 from the centre of the unit interval") {
    WHEN("The basis functions of the four matrix nodes weight the node offsets at every offset") {
      double linear[9] = {};
      for (int sample = 0; sample <= 8; sample++) {
        const double offsetFromCentre = 0.125 * sample - 0.5;
        for (int basisFunction = 0; basisFunction < MatrixNodesPerAxis; basisFunction++) {
          linear[sample] +=
            exahype2::dg::getLagrangeBasisFunction1D<MatrixNodesPerAxis>(MatrixNodes, basisFunction, offsetFromCentre)
            * (MatrixNodes[basisFunction] - 0.5);
        }
      }

      THEN("Every weighted sum returns its offset") {
        for (int sample = 0; sample <= 8; sample++) {
          const double offsetFromCentre = 0.125 * sample - 0.5;
          VALIDATE_NUMERICAL_EQUALS(linear[sample], offsetFromCentre, sample);
        }
      }
    }
  }
}

UNIT_TEST("On nodes that mirror exactly at one half the mirror basis function at the negated offset keeps the bits") {
  GIVEN("Two to five nodes whose lower half is one minus the upper half and six offsets from the centre") {
    WHEN("Every basis function takes every offset and its mirror function takes the negated offset") {
      const int twoNodeMismatches   = countMirroredBasisFunctionMismatches<2>();
      const int threeNodeMismatches = countMirroredBasisFunctionMismatches<3>();
      const int fourNodeMismatches  = countMirroredBasisFunctionMismatches<4>();
      const int fiveNodeMismatches  = countMirroredBasisFunctionMismatches<5>();

      THEN("Both return the same bits for every node count") {
        VALIDATE_EQUALS(twoNodeMismatches, 0);
        VALIDATE_EQUALS(threeNodeMismatches, 0);
        VALIDATE_EQUALS(fourNodeMismatches, 0);
        VALIDATE_EQUALS(fiveNodeMismatches, 0);
      }
    }
  }
}

UNIT_TEST("The interpolation matrices evaluate the coarse polynomial at the child nodes") {
  GIVEN("A cubic polynomial at the four coarse nodes") {
    double coarseValues[MatrixNodesPerAxis];
    for (int coarse = 0; coarse < MatrixNodesPerAxis; coarse++) {
      coarseValues[coarse] = getCubicValue(MatrixNodes[coarse]);
    }

    WHEN("The interpolation matrix of every child multiplies the coarse values") {
      double interpolated[SubdivisionFactor][MatrixNodesPerAxis] = {};
      for (int child = 0; child < SubdivisionFactor; child++) {
        for (int fine = 0; fine < MatrixNodesPerAxis; fine++) {
          for (int coarse = 0; coarse < MatrixNodesPerAxis; coarse++) {
            interpolated[child][fine] += Interpolation[getIndex(child, fine, coarse)] * coarseValues[coarse];
          }
        }
      }

      THEN("Every fine value is the cubic at the position of its fine node within the coarse cell") {
        for (int child = 0; child < SubdivisionFactor; child++) {
          for (int fine = 0; fine < MatrixNodesPerAxis; fine++) {
            const double fineNode = (MatrixNodes[fine] + child) / SubdivisionFactor;
            VALIDATE_NUMERICAL_EQUALS(interpolated[child][fine], getCubicValue(fineNode), child, fine);
          }
        }
      }
    }
  }
}

UNIT_TEST("The restriction matrices are the interpolation matrices transposed and divided by the subdivision factor") {
  GIVEN("The interpolation and the restriction matrices of the four matrix nodes") {
    THEN("Every restriction entry equals the transposed interpolation entry over the subdivision factor") {
      for (int child = 0; child < SubdivisionFactor; child++) {
        for (int coarse = 0; coarse < MatrixNodesPerAxis; coarse++) {
          for (int fine = 0; fine < MatrixNodesPerAxis; fine++) {
            VALIDATE_NUMERICAL_EQUALS(
              Restriction[getIndex(child, coarse, fine)],
              Interpolation[getIndex(child, fine, coarse)] / SubdivisionFactor,
              child,
              coarse,
              fine
            );
          }
        }
      }
    }
  }
}

UNIT_TEST("The weighted restriction of children that all store the constant one yields one at every coarse node") {
  GIVEN("Children that store the constant one at every fine node") {
    constexpr double FineValue = 1.0;

    WHEN("The weighted restriction matrices of every child restrict the fine values onto every coarse node") {
      double restricted[MatrixNodesPerAxis] = {};
      for (int coarse = 0; coarse < MatrixNodesPerAxis; coarse++) {
        for (int child = 0; child < SubdivisionFactor; child++) {
          for (int fine = 0; fine < MatrixNodesPerAxis; fine++) {
            restricted[coarse] += WeightedRestriction[getIndex(child, coarse, fine)] * FineValue;
          }
        }
      }

      THEN("Every coarse node equals one") {
        for (int coarse = 0; coarse < MatrixNodesPerAxis; coarse++) {
          VALIDATE_NUMERICAL_EQUALS(restricted[coarse], 1.0, coarse);
        }
      }
    }
  }
}

UNIT_TEST("The interpolation and both restriction matrices are mirror-symmetric to the last bit") {
  GIVEN("The interpolation, the restriction and the weighted restriction matrices of the four matrix nodes") {
    THEN("Every entry equals the entry of the mirrored child, row and column bitwise") {
      for (int child = 0; child < SubdivisionFactor; child++) {
        for (int row = 0; row < MatrixNodesPerAxis; row++) {
          for (int column = 0; column < MatrixNodesPerAxis; column++) {
            VALIDATE_EQUALS(
              Interpolation[getIndex(child, row, column)],
              Interpolation[getMirrorIndex(child, row, column)],
              child
            );
            VALIDATE_EQUALS(
              Restriction[getIndex(child, row, column)],
              Restriction[getMirrorIndex(child, row, column)],
              child
            );
            VALIDATE_EQUALS(
              WeightedRestriction[getIndex(child, row, column)],
              WeightedRestriction[getMirrorIndex(child, row, column)],
              child
            );
          }
        }
      }
    }
  }
}

UNIT_TEST("Averaging the mirror pairs keeps symmetric entries and replaces an asymmetric pair by its mean") {
  GIVEN("A copy of the interpolation matrices and zero matrices with one and three at two mirrored corners") {
    auto matrices = Interpolation;

    exahype2::dg::AMRMatrices1D<MatrixNodesPerAxis> perturbed{};
    perturbed[getIndex(0, 0, 0)]                                                               = 1.0;
    perturbed[getIndex(SubdivisionFactor - 1, MatrixNodesPerAxis - 1, MatrixNodesPerAxis - 1)] = 3.0;

    WHEN("The test averages the mirror pairs of both") {
      exahype2::dg::averageMirrorPairs<MatrixNodesPerAxis>(matrices);
      exahype2::dg::averageMirrorPairs<MatrixNodesPerAxis>(perturbed);

      THEN("The copy keeps every entry of the interpolation matrices and both corners equal two") {
        VALIDATE(matrices == Interpolation);
        VALIDATE_EQUALS(perturbed[getIndex(0, 0, 0)], 2.0);
        VALIDATE_EQUALS(
          perturbed[getIndex(SubdivisionFactor - 1, MatrixNodesPerAxis - 1, MatrixNodesPerAxis - 1)],
          2.0
        );
      }
    }
  }
}

UNIT_TEST("The transpose per child swaps the node indices and returns the matrices when applied twice") {
  GIVEN("The weighted restriction matrices of the four matrix nodes") {
    WHEN("The test transposes the matrices per child once and twice") {
      const auto transposed      = exahype2::dg::transposePerChild<MatrixNodesPerAxis>(WeightedRestriction);
      const auto transposedTwice = exahype2::dg::transposePerChild<MatrixNodesPerAxis>(transposed);

      THEN("The transpose swaps the row and the column of every child and the second transpose restores them") {
        for (int child = 0; child < SubdivisionFactor; child++) {
          for (int row = 0; row < MatrixNodesPerAxis; row++) {
            for (int column = 0; column < MatrixNodesPerAxis; column++) {
              VALIDATE_EQUALS(
                transposed[getIndex(child, column, row)],
                WeightedRestriction[getIndex(child, row, column)],
                child,
                row,
                column
              );
            }
          }
        }
        VALIDATE(transposedTwice == WeightedRestriction);
      }
    }
  }
}

UNIT_TEST("The matrices of order zero copy the value into every child and average the children") {
  GIVEN("One node at the centre of the unit interval with the weight one") {
    constexpr double Node{0.5};
    constexpr double Weight{1.0};

    WHEN("The test builds the interpolation, the restriction and the weighted restriction matrices of this node") {
      const auto interpolation = exahype2::dg::getInterpolationMatrix1D<1>(&Node);
      const auto restriction   = exahype2::dg::getRestrictionMatrix1D<1>(&Node);
      const auto weighted      = exahype2::dg::getWeightedRestrictionMatrix1D<1>(&Node, &Weight);

      THEN("The interpolation equals one and both restrictions equal one over the subdivision factor per child") {
        for (int child = 0; child < SubdivisionFactor; child++) {
          VALIDATE_EQUALS(interpolation[child], 1.0);
          VALIDATE_EQUALS(restriction[child], 1.0 / SubdivisionFactor);
          VALIDATE_EQUALS(weighted[child], 1.0 / SubdivisionFactor);
        }
      }
    }
  }
}

UNIT_TEST("Every subface takes the coarse face values at its own node positions") {
  GIVEN("The transfer operators and a coarse face with a linear profile") {
    const exahype2::dg::unittests::TransferOperators operators;
    double                                           coarseFace[Setup::FaceNodes * Setup::ValuesPerNode];
    exahype2::dg::unittests::fillLinearTransferProfile<Setup::FaceNodes>(coarseFace, DIMENSIONS - 1, -1);

    WHEN("The test interpolates the coarse face onto every subface") {
      double fineFaces[peano4::SubfacesPerFace][Setup::FaceNodes * Setup::ValuesPerNode];
      for (int subface = 0; subface < peano4::SubfacesPerFace; subface++) {
        exahype2::dg::interpolateFace<Setup::NodesPerAxis, Setup::ValuesPerNode>(
          coarseFace,
          fineFaces[subface],
          operators.prolongation.data(),
          subface
        );
      }

      THEN("Every subface contains the linear profile at the positions of its nodes") {
        for (int subface = 0; subface < peano4::SubfacesPerFace; subface++) {
          for (int node = 0; node < Setup::FaceNodes; node++) {
            double coordinates[DIMENSIONS];
            exahype2::dg::unittests::setTransferCoordinatesOf(coordinates, DIMENSIONS - 1, node, subface);
            for (int value = 0; value < Setup::ValuesPerNode; value++) {
              VALIDATE_NUMERICAL_EQUALS(
                fineFaces[subface][node * Setup::ValuesPerNode + value],
                exahype2::dg::unittests::getLinearTransferProfile(coordinates, DIMENSIONS - 1, value),
                subface,
                node,
                value
              );
            }
          }
        }
      }
    }
  }
}

UNIT_TEST("Every child takes the coarse state at its own node positions") {
  GIVEN("The transfer operators and a coarse cell with a linear state") {
    const exahype2::dg::unittests::TransferOperators operators;
    double                                           coarseCell[Setup::CellNodes * Setup::ValuesPerNode];
    exahype2::dg::unittests::fillLinearTransferProfile<Setup::CellNodes>(coarseCell, DIMENSIONS, -1);

    WHEN("The test interpolates the coarse cell onto every child") {
      double fineCells[Setup::ChildrenPerCell][Setup::CellNodes * Setup::ValuesPerNode] = {};
      for (int child = 0; child < Setup::ChildrenPerCell; child++) {
        interpolateEveryNode(coarseCell, fineCells[child], operators.prolongation.data(), child);
      }

      THEN("Every child contains the linear state at the positions of its nodes") {
        for (int child = 0; child < Setup::ChildrenPerCell; child++) {
          for (int node = 0; node < Setup::CellNodes; node++) {
            double coordinates[DIMENSIONS];
            exahype2::dg::unittests::setTransferCoordinatesOf(coordinates, DIMENSIONS, node, child);
            for (int value = 0; value < Setup::ValuesPerNode; value++) {
              VALIDATE_NUMERICAL_EQUALS(
                fineCells[child][node * Setup::ValuesPerNode + value],
                exahype2::dg::unittests::getLinearTransferProfile(coordinates, DIMENSIONS, value),
                child,
                node,
                value
              );
            }
          }
        }
      }
    }
  }
}

UNIT_TEST("The interpolation of a constant face and a constant cell returns their bits on every subface and child") {
  GIVEN("The transfer operators, a constant coarse face and a constant coarse cell") {
    const exahype2::dg::unittests::TransferOperators operators;
    double                                           coarseFace[Setup::FaceNodes * Setup::ValuesPerNode];
    double                                           coarseCell[Setup::CellNodes * Setup::ValuesPerNode];
    exahype2::dg::unittests::fillTransferConstant<Setup::FaceNodes>(coarseFace);
    exahype2::dg::unittests::fillTransferConstant<Setup::CellNodes>(coarseCell);

    WHEN("The test interpolates the face onto every subface and the cell onto every child") {
      double fineFaces[peano4::SubfacesPerFace][Setup::FaceNodes * Setup::ValuesPerNode];
      double fineCells[Setup::ChildrenPerCell][Setup::CellNodes * Setup::ValuesPerNode] = {};
      for (int subface = 0; subface < peano4::SubfacesPerFace; subface++) {
        exahype2::dg::interpolateFace<Setup::NodesPerAxis, Setup::ValuesPerNode>(
          coarseFace,
          fineFaces[subface],
          operators.prolongation.data(),
          subface
        );
      }
      for (int child = 0; child < Setup::ChildrenPerCell; child++) {
        interpolateEveryNode(coarseCell, fineCells[child], operators.prolongation.data(), child);
      }

      THEN("Every subface and every child contains the constant of every value bitwise") {
        for (int subface = 0; subface < peano4::SubfacesPerFace; subface++) {
          for (int node = 0; node < Setup::FaceNodes; node++) {
            for (int value = 0; value < Setup::ValuesPerNode; value++) {
              VALIDATE_EQUALS(
                fineFaces[subface][node * Setup::ValuesPerNode + value],
                exahype2::dg::unittests::getTransferConstantOf(value),
                subface,
                node,
                value
              );
            }
          }
        }
        for (int child = 0; child < Setup::ChildrenPerCell; child++) {
          for (int node = 0; node < Setup::CellNodes; node++) {
            for (int value = 0; value < Setup::ValuesPerNode; value++) {
              VALIDATE_EQUALS(
                fineCells[child][node * Setup::ValuesPerNode + value],
                exahype2::dg::unittests::getTransferConstantOf(value),
                child,
                node,
                value
              );
            }
          }
        }
      }
    }
  }
}

UNIT_TEST("The coarse face takes the values the subfaces represent at its own node positions") {
  GIVEN("The transfer operators and subfaces with a linear profile over the coarse face") {
    const exahype2::dg::unittests::TransferOperators operators;

    double        fineFaces[peano4::SubfacesPerFace][Setup::FaceNodes * Setup::ValuesPerNode];
    const double* fineFacePointers[peano4::SubfacesPerFace];
    for (int subface = 0; subface < peano4::SubfacesPerFace; subface++) {
      exahype2::dg::unittests::fillLinearTransferProfile<Setup::FaceNodes>(fineFaces[subface], DIMENSIONS - 1, subface);
      fineFacePointers[subface] = fineFaces[subface];
    }

    WHEN("The test restricts the subfaces onto the coarse face") {
      double coarseFace[Setup::FaceNodes * Setup::ValuesPerNode];
      exahype2::dg::restrictFace<Setup::NodesPerAxis, Setup::ValuesPerNode>(
        fineFacePointers,
        coarseFace,
        operators.restriction.data()
      );

      THEN("The coarse face contains the linear profile at the positions of its nodes") {
        for (int node = 0; node < Setup::FaceNodes; node++) {
          double coordinates[DIMENSIONS];
          exahype2::dg::unittests::setTransferCoordinatesOf(coordinates, DIMENSIONS - 1, node, -1);
          for (int value = 0; value < Setup::ValuesPerNode; value++) {
            VALIDATE_NUMERICAL_EQUALS(
              coarseFace[node * Setup::ValuesPerNode + value],
              exahype2::dg::unittests::getLinearTransferProfile(coordinates, DIMENSIONS - 1, value),
              node,
              value
            );
          }
        }
      }
    }
  }
}

UNIT_TEST("The coarse cell takes the state the children represent at its own node positions") {
  GIVEN("The transfer operators and children with a linear state over the coarse cell") {
    const exahype2::dg::unittests::TransferOperators operators;

    double        fineCells[Setup::ChildrenPerCell][Setup::CellNodes * Setup::ValuesPerNode];
    const double* fineCellPointers[Setup::ChildrenPerCell];
    for (int child = 0; child < Setup::ChildrenPerCell; child++) {
      exahype2::dg::unittests::fillLinearTransferProfile<Setup::CellNodes>(fineCells[child], DIMENSIONS, child);
      fineCellPointers[child] = fineCells[child];
    }

    WHEN("The test restricts the children onto the coarse cell") {
      double coarseCell[Setup::CellNodes * Setup::ValuesPerNode];
      restrictEveryNode(fineCellPointers, coarseCell, operators.restriction.data());

      THEN("The coarse cell contains the linear state at the positions of its nodes") {
        for (int node = 0; node < Setup::CellNodes; node++) {
          double coordinates[DIMENSIONS];
          exahype2::dg::unittests::setTransferCoordinatesOf(coordinates, DIMENSIONS, node, -1);
          for (int value = 0; value < Setup::ValuesPerNode; value++) {
            VALIDATE_NUMERICAL_EQUALS(
              coarseCell[node * Setup::ValuesPerNode + value],
              exahype2::dg::unittests::getLinearTransferProfile(coordinates, DIMENSIONS, value),
              node,
              value
            );
          }
        }
      }
    }
  }
}

UNIT_TEST("The restriction and the interpolation of a negated field return the negated field bitwise") {
  GIVEN("The transfer operators, subfaces and children that are not dyadic and their negations") {
    constexpr int                                    FaceValues = Setup::FaceNodes * Setup::ValuesPerNode;
    constexpr int                                    CellValues = Setup::CellNodes * Setup::ValuesPerNode;
    const exahype2::dg::unittests::TransferOperators operators;
    double                                           fineFaces[peano4::SubfacesPerFace][FaceValues];
    double                                           negatedFineFaces[peano4::SubfacesPerFace][FaceValues];
    double                                           fineCells[Setup::ChildrenPerCell][CellValues];
    double                                           negatedFineCells[Setup::ChildrenPerCell][CellValues];
    const double*                                    fineFacePointers[peano4::SubfacesPerFace];
    const double*                                    negatedFineFacePointers[peano4::SubfacesPerFace];
    const double*                                    fineCellPointers[Setup::ChildrenPerCell];
    const double*                                    negatedFineCellPointers[Setup::ChildrenPerCell];
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

    WHEN("Both fields restrict onto a coarse face and cell and these interpolate onto the second subface and child") {
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

      THEN("Every restricted and every interpolated value of the negated field is the negated value bitwise") {
        for (int entry = 0; entry < FaceValues; entry++) {
          VALIDATE_EQUALS(negatedCoarseFace[entry], -coarseFace[entry], entry);
          VALIDATE_EQUALS(negatedFineFace[entry], -fineFace[entry], entry);
        }
        for (int entry = 0; entry < CellValues; entry++) {
          VALIDATE_EQUALS(negatedCoarseCell[entry], -coarseCell[entry], entry);
          VALIDATE_EQUALS(negatedFineCell[entry], -fineCell[entry], entry);
        }
      }
    }
  }
}

UNIT_TEST("The restriction of constant subfaces and children returns their bits on the coarse face and cell") {
  GIVEN("The transfer operators, a constant fine face for every subface and a constant cell for every child") {
    const exahype2::dg::unittests::TransferOperators operators;
    double                                           fineFace[Setup::FaceNodes * Setup::ValuesPerNode];
    double                                           fineCell[Setup::CellNodes * Setup::ValuesPerNode];
    exahype2::dg::unittests::fillTransferConstant<Setup::FaceNodes>(fineFace);
    exahype2::dg::unittests::fillTransferConstant<Setup::CellNodes>(fineCell);
    const double* fineFacePointers[peano4::SubfacesPerFace];
    const double* fineCellPointers[Setup::ChildrenPerCell];
    for (int subface = 0; subface < peano4::SubfacesPerFace; subface++) {
      fineFacePointers[subface] = fineFace;
    }
    for (int child = 0; child < Setup::ChildrenPerCell; child++) {
      fineCellPointers[child] = fineCell;
    }

    WHEN("The test restricts the subfaces onto the coarse face and the children onto the coarse cell") {
      double coarseFace[Setup::FaceNodes * Setup::ValuesPerNode];
      double coarseCell[Setup::CellNodes * Setup::ValuesPerNode];
      exahype2::dg::restrictFace<Setup::NodesPerAxis, Setup::ValuesPerNode>(
        fineFacePointers,
        coarseFace,
        operators.restriction.data()
      );
      restrictEveryNode(fineCellPointers, coarseCell, operators.restriction.data());

      THEN("The coarse face and the coarse cell contain the constant of every value bitwise") {
        for (int node = 0; node < Setup::FaceNodes; node++) {
          for (int value = 0; value < Setup::ValuesPerNode; value++) {
            VALIDATE_EQUALS(
              coarseFace[node * Setup::ValuesPerNode + value],
              exahype2::dg::unittests::getTransferConstantOf(value),
              node,
              value
            );
          }
        }
        for (int node = 0; node < Setup::CellNodes; node++) {
          for (int value = 0; value < Setup::ValuesPerNode; value++) {
            VALIDATE_EQUALS(
              coarseCell[node * Setup::ValuesPerNode + value],
              exahype2::dg::unittests::getTransferConstantOf(value),
              node,
              value
            );
          }
        }
      }
    }
  }
}
