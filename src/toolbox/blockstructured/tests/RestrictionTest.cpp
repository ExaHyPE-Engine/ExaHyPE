// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "RestrictionTest.h"

#include "peano4/Loop.h"
#include "peano4/grid/GridTraversalEvent.h"
#include "toolbox/blockstructured/AMRRoutines.h"

toolbox::blockstructured::tests::RestrictionTest::RestrictionTest() :
  TestCase("toolbox::blockstructured::tests::RestrictionTest") {}

void toolbox::blockstructured::tests::RestrictionTest::run() {
  testMethod(testRestrictHaloLayer);
  testMethod(testRestrictCell);
}

void toolbox::blockstructured::tests::RestrictionTest::testRestrictHaloLayer() {
  // Unknowns == DIMENSIONS lets each axis carry a distinct linear ramp.
  constexpr int NumberOfUnknowns    = DIMENSIONS;
  constexpr int NumberOfDofsPerAxis = 7;

#if DIMENSIONS == 2
  constexpr int FaceSize = 2 * NumberOfUnknowns * NumberOfDofsPerAxis;
#else
  constexpr int FaceSize = 2 * NumberOfUnknowns * NumberOfDofsPerAxis * NumberOfDofsPerAxis;
#endif
  double coarseGridFaceValues[FaceSize];
  double fineGridFaceValues[FaceSize];

  for (int normal = 0; normal < DIMENSIONS; normal++) {
    auto serialiseIndex = [normal](tarch::la::Vector<DIMENSIONS, int> volume) {
      int base  = NumberOfUnknowns;
      int index = 0;
      for (int d = 0; d < DIMENSIONS; d++) {
        index += volume(d) * base;
        if (d == normal) {
          base *= 2;
        } else {
          base *= NumberOfDofsPerAxis;
        }
      }
      return index;
    };

    for (int side = 0; side < 2; side++) {
      // Pre-zero the coarse face: the averaging restriction accumulates onto it.
      std::fill_n(coarseGridFaceValues, FaceSize, 0.);

      peano4::grid::GridTraversalEvent event;
      peano4::dForEach(3, [&](const tarch::la::Vector<DIMENSIONS, int>& subCellIndex) {
        event.setRelativePositionToFather(subCellIndex);
        auto faceMarker = event.getFaceMarker();
        faceMarker.select(normal + side * DIMENSIONS);

        if (not faceMarker.isInteriorFaceWithinPatch()) {
          peano4::dForEachExclusive(
            NumberOfDofsPerAxis,
            normal,
            side,
            [&](const tarch::la::Vector<DIMENSIONS, int>& volume) {
              const auto index = serialiseIndex(volume);
              for (int u = 0; u < NumberOfUnknowns; u++) {
                fineGridFaceValues[index + u] = volume[u] + (u == normal ? 0 : NumberOfDofsPerAxis * subCellIndex[u]);
              }
            }
          );

          toolbox::blockstructured::restrictHaloLayer(
            faceMarker,
            coarseGridFaceValues,
            fineGridFaceValues,
            side,
            NumberOfDofsPerAxis,
            NumberOfUnknowns
          );
        }
      });

      peano4::dForEachExclusive(
        NumberOfDofsPerAxis,
        normal,
        (1 - side),
        [&](const tarch::la::Vector<DIMENSIONS, int>& volume) {
          const auto index = serialiseIndex(volume);
          for (int u = 0; u < NumberOfUnknowns; u++) {
            validateNumericalEquals(coarseGridFaceValues[index + u], 0.);
          }
        }
      );
      peano4::dForEachExclusive(
        NumberOfDofsPerAxis,
        normal,
        side,
        [&](const tarch::la::Vector<DIMENSIONS, int>& volume) {
          const auto index = serialiseIndex(volume);
          for (int u = 0; u < NumberOfUnknowns; u++) {
            const auto expectedValue = (u == normal ? side : (1 + 3 * volume[u]));
            validateNumericalEquals(coarseGridFaceValues[index + u], expectedValue);
          }
        }
      );
    }
  }
}

void toolbox::blockstructured::tests::RestrictionTest::testRestrictCell() {
  // Unknowns == DIMENSIONS lets each axis carry a distinct linear ramp.
  constexpr int NumberOfUnknowns    = DIMENSIONS;
  constexpr int NumberOfDofsPerAxis = 7;

#if DIMENSIONS == 2
  constexpr int CellSize = NumberOfUnknowns * NumberOfDofsPerAxis * NumberOfDofsPerAxis;
#else
  constexpr int CellSize = NumberOfUnknowns * NumberOfDofsPerAxis * NumberOfDofsPerAxis * NumberOfDofsPerAxis;
#endif
  double coarseGridCellValues[CellSize] = {0.};
  double fineGridCellValues[CellSize];

  peano4::grid::GridTraversalEvent event;
  peano4::dForEach(3, [&](const tarch::la::Vector<DIMENSIONS, int>& subCellIndex) {
    event.setRelativePositionToFather(subCellIndex);
    const auto cellMarker = event.getCellMarker();

    peano4::dForEach(NumberOfDofsPerAxis, [&](const tarch::la::Vector<DIMENSIONS, int>& volume) {
      const auto index = NumberOfUnknowns * peano4::dLinearised(volume, NumberOfDofsPerAxis);
      for (int u = 0; u < NumberOfUnknowns; u++) {
        fineGridCellValues[index + u] = volume[u] + NumberOfDofsPerAxis * subCellIndex[u];
      }
    });

    toolbox::blockstructured::restrictCell(
      cellMarker,
      coarseGridCellValues,
      fineGridCellValues,
      NumberOfDofsPerAxis,
      NumberOfUnknowns
    );
  });

  peano4::dForEach(NumberOfDofsPerAxis, [&](const tarch::la::Vector<DIMENSIONS, int>& volume) {
    const auto index = NumberOfUnknowns * peano4::dLinearised(volume, NumberOfDofsPerAxis);
    for (int u = 0; u < NumberOfUnknowns; u++) {
      validateNumericalEquals(coarseGridCellValues[index + u], (1 + 3 * volume[u]));
    }
  });
}
