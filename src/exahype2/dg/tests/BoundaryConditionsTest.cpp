// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "BoundaryConditionsTest.h"

#include "exahype2/dg/BoundaryConditions.h"
#include "peano4/Constants.h"

namespace {
  constexpr int    NumberOfDofsPerAxis        = 3;
  constexpr int    NumberOfUnknowns           = 2;
  constexpr int    NumberOfAuxiliaryVariables = 1;
  constexpr int    NumberOfData               = NumberOfUnknowns + NumberOfAuxiliaryVariables;
  constexpr int    NodesPerFace = (DIMENSIONS == 3) ? NumberOfDofsPerAxis * NumberOfDofsPerAxis : NumberOfDofsPerAxis;
  constexpr double TimeStamp    = 0.75;
  constexpr double QuadratureNodes[NumberOfDofsPerAxis] = {0.1, 0.5, 0.9};

  // Writes the inner state, the position and the time stamp into the outer
  // state, so a test can read back what the kernel handed over.
  struct RecordingSolver final {
    static void boundaryConditions(
      const double* NOALIAS const                                   insideState,
      double* NOALIAS const                                         outsideState,
      const tarch::la::Vector<DIMENSIONS, double>&                  position,
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& cellSize,
      const double                                                  timeStamp,
      const int                                                     direction
    ) {
      for (int value = 0; value < NumberOfData; value++) {
        outsideState[value] =
          2.0 * insideState[value] + position(direction) + 10.0 * position((direction + 1) % DIMENSIONS) + timeStamp;
      }
    }
  };

  tarch::la::Vector<DIMENSIONS, double> makeCellSize() {
    tarch::la::Vector<DIMENSIONS, double> cellSize;
    for (int axis = 0; axis < DIMENSIONS; axis++) {
      cellSize(axis) = 0.5 + 0.25 * axis;
    }
    return cellSize;
  }

  tarch::la::Vector<DIMENSIONS, double> makeFaceCentre(const int faceNumber) {
    const auto                            cellSize = makeCellSize();
    tarch::la::Vector<DIMENSIONS, double> faceCentre;
    for (int axis = 0; axis < DIMENSIONS; axis++) {
      faceCentre(axis) = 1.0 + axis;
    }
    faceCentre(faceNumber % DIMENSIONS) += (faceNumber < DIMENSIONS ? -0.5 : 0.5) * cellSize(faceNumber % DIMENSIONS);
    return faceCentre;
  }

  // Returns the position of a face node: The face centre along the normal and,
  // along the tangential axes in ascending order, the quadrature node of the
  // lexicographic face index.
  tarch::la::Vector<DIMENSIONS, double> nodePositionOf(const int faceNumber, const int faceNode) {
    const int  direction  = faceNumber % DIMENSIONS;
    const auto cellSize   = makeCellSize();
    const auto faceCentre = makeFaceCentre(faceNumber);
    auto       position   = faceCentre;
    int        remainder  = faceNode;
    for (int axis = 0; axis < DIMENSIONS; axis++) {
      if (axis != direction) {
        position(axis) = faceCentre(axis) + (QuadratureNodes[remainder % NumberOfDofsPerAxis] - 0.5) * cellSize(axis);
        remainder /= NumberOfDofsPerAxis;
      }
    }
    return position;
  }

  template <typename StoragePrecision>
  void fillInnerHalf(StoragePrecision (&faceState)[2 * NodesPerFace * NumberOfData], const int innerSide) {
    for (int entry = 0; entry < 2 * NodesPerFace * NumberOfData; entry++) {
      faceState[entry] = static_cast<StoragePrecision>(-1.0);
    }
    for (int entry = 0; entry < NodesPerFace * NumberOfData; entry++) {
      faceState[innerSide * NodesPerFace * NumberOfData + entry] = static_cast<StoragePrecision>(0.25 * entry);
    }
  }
} // namespace

exahype2::dg::tests::BoundaryConditionsTest::BoundaryConditionsTest() :
  tarch::tests::TestCase("exahype2::dg::tests::BoundaryConditionsTest") {}

void exahype2::dg::tests::BoundaryConditionsTest::run() {
  testMethod(testTheOuterHalfIsFilledFromTheInnerHalf);
  testMethod(testEveryNodeSeesItsOwnPositionOnTheFace);
  testMethod(testANarrowerStorageIsStagedThroughTheComputePrecision);
}

void exahype2::dg::tests::BoundaryConditionsTest::testTheOuterHalfIsFilledFromTheInnerHalf() {
  for (int faceNumber = 0; faceNumber < peano4::FacesPerCell; faceNumber++) {
    // The cell behind a lower face wrote the upper half, and the other way round.
    const int innerSide = faceNumber < DIMENSIONS ? 1 : 0;
    const int outerSide = 1 - innerSide;
    double    faceState[2 * NodesPerFace * NumberOfData];
    fillInnerHalf(faceState, innerSide);

    exahype2::dg::applyBoundaryConditions<
      RecordingSolver,
      NumberOfDofsPerAxis,
      NumberOfUnknowns,
      NumberOfAuxiliaryVariables,
      double,
      double>(faceState, QuadratureNodes, makeFaceCentre(faceNumber), makeCellSize(), TimeStamp, faceNumber);

    for (int faceNode = 0; faceNode < NodesPerFace; faceNode++) {
      const auto position = nodePositionOf(faceNumber, faceNode);
      for (int value = 0; value < NumberOfData; value++) {
        const int    innerEntry = (innerSide * NodesPerFace + faceNode) * NumberOfData + value;
        const int    outerEntry = (outerSide * NodesPerFace + faceNode) * NumberOfData + value;
        const double expected   = 2.0 * faceState[innerEntry] + position(faceNumber % DIMENSIONS)
          + 10.0 * position((faceNumber % DIMENSIONS + 1) % DIMENSIONS) + TimeStamp;
        validateNumericalEqualsWithParams2(
          faceState[innerEntry],
          0.25 * (faceNode * NumberOfData + value),
          faceNumber,
          faceNode
        );
        validateNumericalEqualsWithParams2(faceState[outerEntry], expected, faceNumber, faceNode);
      }
    }
  }
}

void exahype2::dg::tests::BoundaryConditionsTest::testEveryNodeSeesItsOwnPositionOnTheFace() {
  for (int faceNumber = 0; faceNumber < peano4::FacesPerCell; faceNumber++) {
    const int innerSide                                  = faceNumber < DIMENSIONS ? 1 : 0;
    const int outerSide                                  = 1 - innerSide;
    double    faceState[2 * NodesPerFace * NumberOfData] = {};

    exahype2::dg::applyBoundaryConditions<
      RecordingSolver,
      NumberOfDofsPerAxis,
      NumberOfUnknowns,
      NumberOfAuxiliaryVariables,
      double,
      double>(faceState, QuadratureNodes, makeFaceCentre(faceNumber), makeCellSize(), 0.0, faceNumber);

    // A zero inner state leaves the position alone in the outer state, and the
    // normal coordinate of every node is the face centre's.
    const int direction = faceNumber % DIMENSIONS;
    for (int faceNode = 0; faceNode < NodesPerFace; faceNode++) {
      const auto   position   = nodePositionOf(faceNumber, faceNode);
      const int    outerEntry = (outerSide * NodesPerFace + faceNode) * NumberOfData;
      const double recorded   = faceState[outerEntry];
      validateNumericalEqualsWithParams2(
        recorded,
        position(direction) + 10.0 * position((direction + 1) % DIMENSIONS),
        faceNumber,
        faceNode
      );
      validateNumericalEqualsWithParams2(
        position(direction),
        makeFaceCentre(faceNumber)(direction),
        faceNumber,
        faceNode
      );
    }
  }
}

void exahype2::dg::tests::BoundaryConditionsTest::testANarrowerStorageIsStagedThroughTheComputePrecision() {
  constexpr int FaceNumber = DIMENSIONS;
  constexpr int InnerSide  = 0;
  constexpr int OuterSide  = 1;

  double faceState[2 * NodesPerFace * NumberOfData];
  float  narrowFaceState[2 * NodesPerFace * NumberOfData];
  fillInnerHalf(faceState, InnerSide);
  fillInnerHalf(narrowFaceState, InnerSide);

  exahype2::dg::applyBoundaryConditions<
    RecordingSolver,
    NumberOfDofsPerAxis,
    NumberOfUnknowns,
    NumberOfAuxiliaryVariables,
    double,
    double>(faceState, QuadratureNodes, makeFaceCentre(FaceNumber), makeCellSize(), TimeStamp, FaceNumber);
  exahype2::dg::applyBoundaryConditions<
    RecordingSolver,
    NumberOfDofsPerAxis,
    NumberOfUnknowns,
    NumberOfAuxiliaryVariables,
    float,
    double>(narrowFaceState, QuadratureNodes, makeFaceCentre(FaceNumber), makeCellSize(), TimeStamp, FaceNumber);

  // The inner values are exact in float, so the staged result is the double
  // result rounded once.
  for (int entry = 0; entry < NodesPerFace * NumberOfData; entry++) {
    const int outerEntry = OuterSide * NodesPerFace * NumberOfData + entry;
    validateNumericalEqualsWithParams1(
      static_cast<double>(narrowFaceState[outerEntry]),
      static_cast<double>(static_cast<float>(faceState[outerEntry])),
      entry
    );
  }
}
