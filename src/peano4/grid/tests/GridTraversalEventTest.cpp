// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "GridTraversalEventTest.h"

#include "peano4/Globals.h"
#include "peano4/dataview/CellMarker.h"
#include "peano4/dataview/FaceMarker.h"
#include "peano4/grid/GridTraversalEvent.h"
#include "tarch/la/Vector.h"

peano4::grid::tests::GridTraversalEventTest::GridTraversalEventTest() :
  TestCase("peano4::grid::tests::GridTraversalEventTest") {}

void peano4::grid::tests::GridTraversalEventTest::run() {
  testMethod(testDefaults);
  testMethod(testBitHelpers);
  testMethod(testGeometry);
  testMethod(testVertexMasks);
  testMethod(testFaceMasks);
  testMethod(testCellFlags);
  testMethod(testDataIndices);
  testMethod(testToString);
  testMethod(testCellMarkerCarriesTheCellFlags);
  testMethod(testFaceMarkerCarriesTheFaceAndTheCellFlags);
}

void peano4::grid::tests::GridTraversalEventTest::testDefaults() {
  const peano4::grid::GridTraversalEvent event{};
  validateEquals(event.level, 0);
  validateEquals(static_cast<int>(event.getHasBeenRefined()), 0);
  validateEquals(static_cast<int>(event.getWillBeRefined()), 0);
  validateEquals(static_cast<int>(event.getIsVertexLocal()), 0);
  validateEquals(static_cast<int>(event.getIsVertexHanging()), 0);
  validateEquals(static_cast<int>(event.getIsParentVertexLocal()), 0);
  validateEquals(static_cast<int>(event.getIsVertexParentOfSubtree()), 0);
  validateEquals(static_cast<int>(event.getIsVertexAdjacentToParallelDomainBoundary()), 0);
  validateEquals(static_cast<int>(event.getIsFaceLocal()), 0);
  validateEquals(static_cast<int>(event.getIsFaceHanging()), 0);
  validateEquals(static_cast<int>(event.getIsFaceAdjacentToParallelDomainBoundary()), 0);
  validateFalse(event.getIsCellLocal());
  validateFalse(event.getIsParentCellLocal());
  validateEquals(event.getIsAdjacentCellLocal(), 0u);
  validateEquals(event.getCellData(), -1);
  validateEquals(event.getCellPoolIndex(), -1);
  for (int vertexNumber = 0; vertexNumber < TWO_POWER_D; vertexNumber++) {
    validateEqualsWithParams1(event.getVertexDataFrom(vertexNumber), 0, vertexNumber);
    validateEqualsWithParams1(event.getVertexDataTo(vertexNumber), 0, vertexNumber);
    validateEqualsWithParams1(event.getVertexPoolIndex(vertexNumber), 0, vertexNumber);
  }
  for (int faceNumber = 0; faceNumber < TWO_TIMES_D; faceNumber++) {
    validateEqualsWithParams1(event.getFaceDataFrom(faceNumber), 0, faceNumber);
    validateEqualsWithParams1(event.getFaceDataTo(faceNumber), 0, faceNumber);
    validateEqualsWithParams1(event.getFacePoolIndex(faceNumber), 0, faceNumber);
  }
}

void peano4::grid::tests::GridTraversalEventTest::testBitHelpers() {
  std::uint8_t mask8 = 0;
  peano4::grid::GridTraversalEvent::bitSet8(mask8, 3, true);
  validateEquals(static_cast<int>(mask8), 8);
  validateTrue(peano4::grid::GridTraversalEvent::bitTest8(mask8, 3));
  validateFalse(peano4::grid::GridTraversalEvent::bitTest8(mask8, 2));
  peano4::grid::GridTraversalEvent::bitSet8(mask8, 7, true);
  validateEquals(static_cast<int>(mask8), 8 + 128);
  peano4::grid::GridTraversalEvent::bitSet8(mask8, 3, false);
  validateEquals(static_cast<int>(mask8), 128);
  peano4::grid::GridTraversalEvent::bitSet8(mask8, 3, false);
  validateEquals(static_cast<int>(mask8), 128);

  std::uint32_t mask32 = 0;
  peano4::grid::GridTraversalEvent::bitSet32(mask32, 31, true);
  validateEquals(mask32, 1u << 31);
  validateTrue(peano4::grid::GridTraversalEvent::bitTest32(mask32, 31));
  validateFalse(peano4::grid::GridTraversalEvent::bitTest32(mask32, 0));
  peano4::grid::GridTraversalEvent::bitSet32(mask32, 0, true);
  peano4::grid::GridTraversalEvent::bitSet32(mask32, 31, false);
  validateEquals(mask32, 1u);
}

void peano4::grid::tests::GridTraversalEventTest::testGeometry() {
  const tarch::la::Vector<DIMENSIONS, double> centre(0.25);
  const tarch::la::Vector<DIMENSIONS, double> width(0.5);
  peano4::grid::GridTraversalEvent            event{};
  event.level = 3;
  event.setX(centre);
  event.setH(width);
  validateEquals(event.level, 3);
  validateNumericalVectorEquals(event.getX(), centre);
  validateNumericalVectorEquals(event.getH(), width);

  event.setX(1, 0.75);
  event.setH(0, 0.125);
  validateNumericalEquals(event.getX(0), 0.25);
  validateNumericalEquals(event.getX(1), 0.75);
  validateNumericalEquals(event.getH(0), 0.125);
  validateNumericalEquals(event.getH(1), 0.5);

  event.setRelativePositionToFather(tarch::la::Vector<DIMENSIONS, int>(2));
  validateEquals(event.getRelativePositionToFather()(0), 2);
  validateEquals(event.getRelativePositionToFather()(1), 2);
  event.setRelativePositionToFather(0, 1);
  validateEquals(event.getRelativePositionToFather(0), 1);
  validateEquals(event.getRelativePositionToFather(1), 2);
}

void peano4::grid::tests::GridTraversalEventTest::testVertexMasks() {
  constexpr int                    AllVertices = (1 << TWO_POWER_D) - 1;
  peano4::grid::GridTraversalEvent event{};

  event.setHasBeenRefined(static_cast<std::uint8_t>(AllVertices));
  validateEquals(static_cast<int>(event.getHasBeenRefined()), AllVertices);
  event.setHasBeenRefined(0, false);
  validateFalse(event.getHasBeenRefined(0));
  validateTrue(event.getHasBeenRefined(1));
  validateEquals(static_cast<int>(event.getHasBeenRefined()), AllVertices - 1);

  event.setWillBeRefined(2, true);
  validateTrue(event.getWillBeRefined(2));
  validateFalse(event.getWillBeRefined(1));
  validateEquals(static_cast<int>(event.getWillBeRefined()), 4);
  event.setWillBeRefined(static_cast<std::uint8_t>(0));
  validateFalse(event.getWillBeRefined(2));

  event.setIsVertexLocal(1, true);
  validateTrue(event.getIsVertexLocal(1));
  validateEquals(static_cast<int>(event.getIsVertexLocal()), 2);
  event.setIsVertexLocal(static_cast<std::uint8_t>(AllVertices));
  validateEquals(static_cast<int>(event.getIsVertexLocal()), AllVertices);

  event.setIsVertexHanging(static_cast<std::uint8_t>(1));
  validateTrue(event.getIsVertexHanging(0));
  event.setIsVertexHanging(0, false);
  validateEquals(static_cast<int>(event.getIsVertexHanging()), 0);

  event.setIsParentVertexLocal(TWO_POWER_D - 1, true);
  validateTrue(event.getIsParentVertexLocal(TWO_POWER_D - 1));
  validateEquals(static_cast<int>(event.getIsParentVertexLocal()), 1 << (TWO_POWER_D - 1));
  event.setIsParentVertexLocal(static_cast<std::uint8_t>(0));
  validateFalse(event.getIsParentVertexLocal(TWO_POWER_D - 1));

  event.setIsVertexParentOfSubtree(static_cast<std::uint8_t>(3));
  validateTrue(event.getIsVertexParentOfSubtree(0));
  validateTrue(event.getIsVertexParentOfSubtree(1));
  event.setIsVertexParentOfSubtree(1, false);
  validateEquals(static_cast<int>(event.getIsVertexParentOfSubtree()), 1);

  event.setIsVertexAdjacentToParallelDomainBoundary(2, true);
  validateTrue(event.getIsVertexAdjacentToParallelDomainBoundary(2));
  validateEquals(static_cast<int>(event.getIsVertexAdjacentToParallelDomainBoundary()), 4);
  event.setIsVertexAdjacentToParallelDomainBoundary(static_cast<std::uint8_t>(AllVertices));
  validateEquals(static_cast<int>(event.getIsVertexAdjacentToParallelDomainBoundary()), AllVertices);

  validateEquals(static_cast<int>(event.getHasBeenRefined()), AllVertices - 1);
  validateEquals(static_cast<int>(event.getWillBeRefined()), 0);
  validateEquals(static_cast<int>(event.getIsVertexLocal()), AllVertices);
  validateEquals(static_cast<int>(event.getIsVertexHanging()), 0);
  validateEquals(static_cast<int>(event.getIsParentVertexLocal()), 0);
  validateEquals(static_cast<int>(event.getIsVertexParentOfSubtree()), 1);
}

void peano4::grid::tests::GridTraversalEventTest::testFaceMasks() {
  constexpr int                    AllFaces = (1 << TWO_TIMES_D) - 1;
  peano4::grid::GridTraversalEvent event{};

  event.setIsFaceLocal(static_cast<std::uint8_t>(AllFaces));
  validateEquals(static_cast<int>(event.getIsFaceLocal()), AllFaces);
  event.setIsFaceLocal(DIMENSIONS, false);
  validateFalse(event.getIsFaceLocal(DIMENSIONS));
  validateTrue(event.getIsFaceLocal(0));
  validateEquals(static_cast<int>(event.getIsFaceLocal()), AllFaces - (1 << DIMENSIONS));

  event.setIsFaceHanging(TWO_TIMES_D - 1, true);
  validateTrue(event.getIsFaceHanging(TWO_TIMES_D - 1));
  validateEquals(static_cast<int>(event.getIsFaceHanging()), 1 << (TWO_TIMES_D - 1));
  event.setIsFaceHanging(static_cast<std::uint8_t>(0));
  validateFalse(event.getIsFaceHanging(TWO_TIMES_D - 1));

  event.setIsFaceAdjacentToParallelDomainBoundary(1, true);
  validateTrue(event.getIsFaceAdjacentToParallelDomainBoundary(1));
  validateEquals(static_cast<int>(event.getIsFaceAdjacentToParallelDomainBoundary()), 2);
  event.setIsFaceAdjacentToParallelDomainBoundary(static_cast<std::uint8_t>(AllFaces));
  validateEquals(static_cast<int>(event.getIsFaceAdjacentToParallelDomainBoundary()), AllFaces);

  validateEquals(static_cast<int>(event.getIsFaceLocal()), AllFaces - (1 << DIMENSIONS));
  validateEquals(static_cast<int>(event.getIsFaceHanging()), 0);
}

void peano4::grid::tests::GridTraversalEventTest::testCellFlags() {
  constexpr int                    LastAdjacentCell = THREE_POWER_D - 1;
  peano4::grid::GridTraversalEvent event{};

  event.setIsCellLocal(true);
  validateTrue(event.getIsCellLocal());
  validateFalse(event.getIsParentCellLocal());
  event.setIsParentCellLocal(true);
  event.setIsCellLocal(false);
  validateFalse(event.getIsCellLocal());
  validateTrue(event.getIsParentCellLocal());

  event.setIsAdjacentCellLocal(LastAdjacentCell, true);
  validateTrue(event.getIsAdjacentCellLocal(LastAdjacentCell));
  validateFalse(event.getIsAdjacentCellLocal(0));
  validateEquals(event.getIsAdjacentCellLocal(), std::uint32_t{1} << LastAdjacentCell);
  event.setIsAdjacentCellLocal(std::uint32_t{7});
  validateTrue(event.getIsAdjacentCellLocal(0));
  validateFalse(event.getIsAdjacentCellLocal(LastAdjacentCell));
  event.setIsAdjacentCellLocal(1, false);
  validateEquals(event.getIsAdjacentCellLocal(), std::uint32_t{5});
}

void peano4::grid::tests::GridTraversalEventTest::testDataIndices() {
  peano4::grid::GridTraversalEvent event{};

  event.setCellData(5);
  validateEquals(event.getCellData(), 5);
  event.setCellPoolIndex(17);
  validateEquals(event.getCellPoolIndex(), 17);

  std::array<int, TWO_POWER_D> vertexData{};
  std::array<int, TWO_TIMES_D> faceData{};
  for (int vertexNumber = 0; vertexNumber < TWO_POWER_D; vertexNumber++) {
    vertexData[vertexNumber] = 10 + vertexNumber;
  }
  for (int faceNumber = 0; faceNumber < TWO_TIMES_D; faceNumber++) {
    faceData[faceNumber] = 20 + faceNumber;
  }
  event.setVertexDataFrom(vertexData);
  event.setVertexDataTo(vertexData);
  event.setFaceDataFrom(faceData);
  event.setFaceDataTo(faceData);
  validateTrue(event.getVertexDataFrom() == vertexData);
  validateTrue(event.getVertexDataTo() == vertexData);
  validateTrue(event.getFaceDataFrom() == faceData);
  validateTrue(event.getFaceDataTo() == faceData);

  event.setVertexDataFrom(0, -3);
  event.setVertexDataTo(1, -4);
  event.setFaceDataFrom(0, -5);
  event.setFaceDataTo(1, -6);
  validateEquals(event.getVertexDataFrom(0), -3);
  validateEquals(event.getVertexDataFrom(1), 11);
  validateEquals(event.getVertexDataTo(1), -4);
  validateEquals(event.getVertexDataTo(0), 10);
  validateEquals(event.getFaceDataFrom(0), -5);
  validateEquals(event.getFaceDataFrom(1), 21);
  validateEquals(event.getFaceDataTo(1), -6);
  validateEquals(event.getFaceDataTo(0), 20);

  event.setVertexPoolIndex(TWO_POWER_D - 1, 40);
  event.setFacePoolIndex(TWO_TIMES_D - 1, 41);
  validateEquals(event.getVertexPoolIndex(TWO_POWER_D - 1), 40);
  validateEquals(event.getVertexPoolIndex(0), 0);
  validateEquals(event.getFacePoolIndex(TWO_TIMES_D - 1), 41);
  validateEquals(event.getFacePoolIndex(0), 0);
}

void peano4::grid::tests::GridTraversalEventTest::testToString() {
  peano4::grid::GridTraversalEvent event{};
  event.setX(tarch::la::Vector<DIMENSIONS, double>(0.5));
  event.setH(tarch::la::Vector<DIMENSIONS, double>(0.25));

  const std::string description = event.toString();
  validateTrue(description.find("GridTraversalEvent{") != std::string::npos);
  validateTrue(description.find("x=[0.5,0.5") != std::string::npos);
  validateTrue(description.find("h=[0.25,0.25") != std::string::npos);

  std::ostringstream stream;
  stream << event;
  validateEquals(stream.str(), description);
}

void peano4::grid::tests::GridTraversalEventTest::testCellMarkerCarriesTheCellFlags() {
  peano4::grid::GridTraversalEvent event{};
  event.level                    = 2;
  event.x                        = tarch::la::Vector<DIMENSIONS, double>(0.5);
  event.h                        = tarch::la::Vector<DIMENSIONS, double>(0.25);
  event.relativePositionToFather = tarch::la::Vector<DIMENSIONS, int>(1);
  event.setHasBeenRefined(0, true);
  event.setIsCellLocal(true);

  const auto marker = event.getCellMarker();
  validateEquals(marker.level, 2);
  validateNumericalVectorEquals(marker.getX(), event.x);
  validateNumericalVectorEquals(marker.getH(), event.h);
  validateEquals(marker.getRelativePositionWithinFatherCell()(0), 1);
  validateTrue(marker.hasBeenRefined());
  validateFalse(marker.willBeRefined());
  validateTrue(marker.isLocal());
  validateFalse(marker.isParentLocal());

  event.setHasBeenRefined(0, false);
  event.setWillBeRefined(TWO_POWER_D - 1, true);
  event.setIsParentCellLocal(true);
  const auto refiningMarker = event.getCellMarker();
  validateFalse(refiningMarker.hasBeenRefined());
  validateTrue(refiningMarker.willBeRefined());
  validateTrue(refiningMarker.isParentLocal());
}

void peano4::grid::tests::GridTraversalEventTest::testFaceMarkerCarriesTheFaceAndTheCellFlags() {
  // Vertex 0 alone is refined, so the cell has children while only the faces
  // through vertex 0, the ones with a negative normal, hold a refined vertex.
  peano4::grid::GridTraversalEvent event{};
  event.x = tarch::la::Vector<DIMENSIONS, double>(0.5);
  event.h = tarch::la::Vector<DIMENSIONS, double>(1.0);
  event.setHasBeenRefined(0, true);
  event.setIsCellLocal(true);
  event.setIsFaceLocal(1, true);
  event.setIsFaceHanging(DIMENSIONS, true);

  auto marker = event.getFaceMarker();
  validateTrue(marker.cellIsLocal);
  for (int faceNumber = 0; faceNumber < TWO_TIMES_D; faceNumber++) {
    const bool holdsVertexZero = faceNumber < DIMENSIONS;
    const bool isLocalFace     = faceNumber == 1;
    const bool isHangingFace   = faceNumber == DIMENSIONS;
    validateEqualsWithParams1(marker.hasBeenRefined(faceNumber), holdsVertexZero, faceNumber);
    validateWithParams1(not marker.willBeRefined(faceNumber), faceNumber);
    validateEqualsWithParams1(marker.isLocal(faceNumber), isLocalFace, faceNumber);
    validateEqualsWithParams1(marker.isHanging(faceNumber), isHangingFace, faceNumber);
  }

  marker.select(DIMENSIONS);
  validateEquals(marker.getSelectedFaceNumber(), DIMENSIONS);
  validateFalse(marker.hasBeenRefined());
  validateNumericalEquals(marker.getX()(0), 1.0);

  // The last vertex alone will be refined, so only the faces with a positive
  // normal hold a refined vertex.
  event.setWillBeRefined(TWO_POWER_D - 1, true);
  const auto refiningMarker = event.getFaceMarker();
  for (int faceNumber = 0; faceNumber < TWO_TIMES_D; faceNumber++) {
    const bool holdsLastVertex = faceNumber >= DIMENSIONS;
    validateEqualsWithParams1(refiningMarker.willBeRefined(faceNumber), holdsLastVertex, faceNumber);
  }
}
