// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "FaceMergesTest.h"

#include "exahype2/FaceMerges.h"
#include "peano4/Constants.h"
#include "peano4/dataview/FaceMarker.h"

namespace {
  constexpr int    DofsPerAxis      = 3;
  constexpr int    Overlap          = 2;
  constexpr int    NumberOfUnknowns = 2;
  constexpr int    OverlapCells     = 2 * Overlap * (DIMENSIONS == 2 ? DofsPerAxis : DofsPerAxis * DofsPerAxis);
  constexpr double Untouched        = -1.0;

  peano4::dataview::FaceMarker createMarker(const int faceNumber) {
    peano4::dataview::FaceMarker marker{};
    for (int axis = 0; axis < DIMENSIONS; axis++) {
      marker.h[axis]          = 1.0 / 3.0;
      marker.cellCentre[axis] = 0.5;
    }
    marker.cellIsLocal = true;
    marker.select(faceNumber);
    return marker;
  }

  /**
   * Returns the index along the face normal of the overlap cell that holds the value. The axes
   * below the normal run faster and span DofsPerAxis cells each.
   */
  int layerAlongNormal(const int value, const int normal) {
    int stride = 1;
    for (int axis = 0; axis < normal; axis++) {
      stride *= DofsPerAxis;
    }
    return (value / NumberOfUnknowns / stride) % (2 * Overlap);
  }
} // namespace

exahype2::tests::FaceMergesTest::FaceMergesTest() :
  tarch::tests::TestCase("exahype2::tests::FaceMergesTest") {}

void exahype2::tests::FaceMergesTest::run() {
  testMethod(testOverlappingVolumesCopyTheNeighbourLayers);
  testMethod(testProjectionHalvesCopyTheNeighbourHalf);
}

/**
 * Checks that every face copies the halo layers on the neighbour's side and keeps its own layers.
 */
void exahype2::tests::FaceMergesTest::testOverlappingVolumesCopyTheNeighbourLayers() {
  std::array<double, OverlapCells * NumberOfUnknowns> source{};
  for (int value = 0; value < OverlapCells * NumberOfUnknowns; value++) {
    source[value] = value + 1.0;
  }

  for (int faceNumber = 0; faceNumber < peano4::FacesPerCell; faceNumber++) {
    std::array<double, OverlapCells * NumberOfUnknowns> destination{};
    destination.fill(Untouched);
    exahype2::mergeOverlappingVolumes<DofsPerAxis, Overlap, NumberOfUnknowns>(
      createMarker(faceNumber),
      destination.data(),
      source.data()
    );

    const int normal              = faceNumber % DIMENSIONS;
    const int firstNeighbourLayer = faceNumber < DIMENSIONS ? 0 : Overlap;
    for (int value = 0; value < OverlapCells * NumberOfUnknowns; value++) {
      const int    layer              = layerAlongNormal(value, normal);
      const bool   belongsToNeighbour = layer >= firstNeighbourLayer and layer < firstNeighbourLayer + Overlap;
      const double expected           = belongsToNeighbour ? source[value] : Untouched;
      validateEqualsWithParams2(destination[value], expected, faceNumber, value);
    }
  }
}

/**
 * Checks that every face copies the projection half on the neighbour's side and keeps its own half.
 */
void exahype2::tests::FaceMergesTest::testProjectionHalvesCopyTheNeighbourHalf() {
  constexpr int                  Cardinality = 8;
  std::array<float, Cardinality> source{};
  for (int value = 0; value < Cardinality; value++) {
    source[value] = static_cast<float>(value + 1);
  }

  for (int faceNumber = 0; faceNumber < peano4::FacesPerCell; faceNumber++) {
    std::array<float, Cardinality> destination{};
    destination.fill(static_cast<float>(Untouched));
    exahype2::mergeProjectionHalves<Cardinality>(createMarker(faceNumber), destination.data(), source.data());

    const int neighbourSide = faceNumber < DIMENSIONS ? 0 : 1;
    for (int value = 0; value < Cardinality; value++) {
      const bool  belongsToNeighbour = value / (Cardinality / 2) == neighbourSide;
      const float expected           = belongsToNeighbour ? source[value] : static_cast<float>(Untouched);
      validateEqualsWithParams2(destination[value], expected, faceNumber, value);
    }
  }
}
