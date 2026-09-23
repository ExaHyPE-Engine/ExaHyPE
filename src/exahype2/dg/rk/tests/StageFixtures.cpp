// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "StageFixtures.h"

exahype2::dg::rk::tests::TestSolver::TestSolver() :
  Solver(1.0) {}

void exahype2::dg::rk::tests::TestSolver::startSimulation() {}

void exahype2::dg::rk::tests::TestSolver::finishSimulation() {}

void exahype2::dg::rk::tests::TestSolver::setTimeStepSize(const double timeStepSize) {
  setFixedTimeStepSize(timeStepSize);
}

exahype2::dg::rk::tests::AdaptiveTestSolver::AdaptiveTestSolver() :
  Solver(1.0) {}

void exahype2::dg::rk::tests::AdaptiveTestSolver::startSimulation() {}

void exahype2::dg::rk::tests::AdaptiveTestSolver::finishSimulation() {}

double exahype2::dg::rk::tests::AdaptiveTestSolver::minTerminalTime() const { return 10.0; }

void exahype2::dg::rk::tests::Quadrature::initialise() {
  for (int faceNode = 0; faceNode < NodesPerFace; faceNode++) {
    QuadratureWeightsOnFace[faceNode] = 1.0;
  }
  for (int cellNode = 0; cellNode < NodesPerCell; cellNode++) {
    InverseQuadratureWeightsInCell[cellNode] = 1.0;
  }
}

void exahype2::dg::rk::tests::DGMatrices::initialise() {
  for (int node = 0; node < NumberOfDofsPerAxis; node++) {
    MassMatrixDiagonal1D[node]       = 1.0;
    BasisFunctionValuesLeft1D[node]  = node == 0 ? 1.0 : 0.0;
    BasisFunctionValuesRight1D[node] = node == NumberOfDofsPerAxis - 1 ? 1.0 : 0.0;
  }
  for (int entry = 0; entry < NumberOfDofsPerAxis * NumberOfDofsPerAxis; entry++) {
    StiffnessOperator1D[entry]  = 0.0;
    DerivativeOperator1D[entry] = 0.0;
  }
  for (int entry = 0; entry < TransferMatrixEntries; entry++) {
    ProlongationMatrix1D[entry]        = 0.0;
    InterpolationMatrix1D[entry]       = 0.0;
    WeightedRestrictionMatrix1D[entry] = 0.0;
  }
  for (int interval = 0; interval < SubdivisionFactor; interval++) {
    for (int node = 0; node < NumberOfDofsPerAxis; node++) {
      const int entry = interval * NumberOfDofsPerAxis * NumberOfDofsPerAxis + node * NumberOfDofsPerAxis + node;
      ProlongationMatrix1D[entry]        = 1.0 + interval;
      InterpolationMatrix1D[entry]       = 1.0;
      WeightedRestrictionMatrix1D[entry] = 1.0 / SubdivisionFactor;
    }
  }
}

tarch::la::Vector<DIMENSIONS, int> exahype2::dg::rk::tests::nodeIndexOf(const int cellNode) {
  tarch::la::Vector<DIMENSIONS, int> nodeIndex;
  int                                remainder = cellNode;
  for (int axis = 0; axis < DIMENSIONS; axis++) {
    nodeIndex(axis) = remainder % NumberOfDofsPerAxis;
    remainder /= NumberOfDofsPerAxis;
  }
  return nodeIndex;
}

tarch::la::Vector<DIMENSIONS, int> exahype2::dg::rk::tests::positionOfChild(const int child) {
  tarch::la::Vector<DIMENSIONS, int> position;
  int                                remainder = child;
  for (int axis = 0; axis < DIMENSIONS; axis++) {
    position(axis) = remainder % SubdivisionFactor;
    remainder /= SubdivisionFactor;
  }
  return position;
}

tarch::la::Vector<DIMENSIONS, double> exahype2::dg::rk::tests::makeCellSize() {
  tarch::la::Vector<DIMENSIONS, double> cellSize;
  for (int axis = 0; axis < DIMENSIONS; axis++) {
    cellSize(axis) = 0.5 + 0.25 * axis;
  }
  return cellSize;
}

/**
 * Returns the cell node that sits on the line through faceNode along the normal of direction,
 * where the lowest tangential axis of a face runs fastest.
 */
int exahype2::dg::rk::tests::cellNodeOnFaceLine(const int direction, const int faceNode, const int normalIndex) {
  int stride = 1;
  for (int axis = 0; axis < direction; axis++) {
    stride *= NumberOfDofsPerAxis;
  }
  return (faceNode / stride * NumberOfDofsPerAxis + normalIndex) * stride + faceNode % stride;
}

peano4::dataview::CellMarker exahype2::dg::rk::tests::makeCellMarker() {
  peano4::dataview::CellMarker marker;
  marker.level  = 1;
  marker.h      = makeCellSize();
  marker.centre = tarch::la::Vector<DIMENSIONS, double>(1.0);
  marker.flags  = peano4::dataview::CellMarker::IsLocal;
  return marker;
}

peano4::dataview::CellMarker exahype2::dg::rk::tests::makeChildCellMarker(const int child) {
  peano4::dataview::CellMarker marker     = makeCellMarker();
  marker.relativePositionWithinFatherCell = positionOfChild(child);
  return marker;
}

peano4::dataview::FaceMarker exahype2::dg::rk::tests::makeFaceMarker(const int faceNumber) {
  peano4::dataview::FaceMarker marker;
  marker.h          = makeCellSize();
  marker.cellCentre = tarch::la::Vector<DIMENSIONS, double>(1.0);
  marker.select(faceNumber);
  return marker;
}

/**
 * Returns the marker of the subface that sits at the subface index below the coarse face, where
 * the normal axis carries the position of the cell within its parent.
 */
peano4::dataview::FaceMarker exahype2::dg::rk::tests::makeSubfaceMarker(const int faceNumber, const int subface) {
  peano4::dataview::FaceMarker marker    = makeFaceMarker(faceNumber);
  const int                    normal    = faceNumber % DIMENSIONS;
  int                          remainder = subface;
  for (int axis = 0; axis < DIMENSIONS; axis++) {
    if (axis != normal) {
      marker.relativePositionWithinFatherCell(axis) = remainder % SubdivisionFactor;
      remainder /= SubdivisionFactor;
    }
  }
  marker.relativePositionWithinFatherCell(normal) = faceNumber < DIMENSIONS ? 0 : SubdivisionFactor - 1;
  return marker;
}
