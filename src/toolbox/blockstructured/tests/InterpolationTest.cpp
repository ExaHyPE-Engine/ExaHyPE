// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "InterpolationTest.h"

#include "peano4/Loop.h"
#include "peano4/grid/GridTraversalEvent.h"
#include "toolbox/blockstructured/AMRRoutines.h"

#include <algorithm>
#include <vector>

toolbox::blockstructured::tests::InterpolationTest::InterpolationTest() :
  TestCase("toolbox::blockstructured::tests::InterpolationTest") {}

void toolbox::blockstructured::tests::InterpolationTest::run() {
  testMethod(testInterpolateHaloLayer);
  testMethod(testInterpolateHaloLayerFromCoarseCell);
  testMethod(testInterpolateCell);
}

void toolbox::blockstructured::tests::InterpolationTest::testInterpolateHaloLayer() {
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
      std::fill_n(coarseGridFaceValues, FaceSize, 0.);
      peano4::dForEachExclusive(
        NumberOfDofsPerAxis,
        normal,
        side,
        [&](const tarch::la::Vector<DIMENSIONS, int>& volume) {
          const auto index = serialiseIndex(volume);
          for (int u = 0; u < NumberOfUnknowns; u++) {
            coarseGridFaceValues[index + u] = volume[u];
          }
        }
      );

      peano4::grid::GridTraversalEvent event;
      peano4::dForEach(3, [&](const tarch::la::Vector<DIMENSIONS, int>& subCellIndex) {
        event.setRelativePositionToFather(subCellIndex);
        auto faceMarker = event.getFaceMarker();
        faceMarker.select(normal + side * DIMENSIONS);

        if (not faceMarker.isInteriorFaceWithinPatch()) {
          std::fill_n(fineGridFaceValues, FaceSize, 0.);
          toolbox::blockstructured::interpolateHaloLayer(
            faceMarker,
            coarseGridFaceValues,
            fineGridFaceValues,
            side,
            NumberOfDofsPerAxis,
            NumberOfUnknowns
          );

          peano4::dForEachExclusive(
            NumberOfDofsPerAxis,
            normal,
            (1 - side),
            [&](const tarch::la::Vector<DIMENSIONS, int>& volume) {
              const auto index = serialiseIndex(volume);
              for (int u = 0; u < NumberOfUnknowns; u++) {
                validateNumericalEquals(fineGridFaceValues[index + u], 0.);
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
                const auto expectedValue =
                  (u == normal ? side : (NumberOfDofsPerAxis * subCellIndex[u] + volume[u]) / 3);
                validateNumericalEquals(fineGridFaceValues[index + u], expectedValue);
              }
            }
          );
        }
      });
    }
  }
}

void toolbox::blockstructured::tests::InterpolationTest::testInterpolateHaloLayerFromCoarseCell() {
  // Unknowns == DIMENSIONS lets each axis carry a distinct linear ramp.
  constexpr int NumberOfUnknowns = DIMENSIONS;

  // A size divisible by 3 (6, not 7) is needed: the coarse source layer along the
  // normal depends on side only when 3 divides the patch size, so a non-divisible
  // size would leave that side dependence untested.
  for (const auto numberOfDofsPerAxis : {6, 7}) {
#if DIMENSIONS == 2
    const int faceSize = 2 * NumberOfUnknowns * numberOfDofsPerAxis;
    const int cellSize = NumberOfUnknowns * numberOfDofsPerAxis * numberOfDofsPerAxis;
#else
    const int faceSize = 2 * NumberOfUnknowns * numberOfDofsPerAxis * numberOfDofsPerAxis;
    const int cellSize = NumberOfUnknowns * numberOfDofsPerAxis * numberOfDofsPerAxis * numberOfDofsPerAxis;
#endif
    std::vector<double> coarseGridCellValues(cellSize);
    std::vector<double> fineGridFaceValues(faceSize);

    peano4::dForEach(numberOfDofsPerAxis, [&](const tarch::la::Vector<DIMENSIONS, int>& volume) {
      const auto index = NumberOfUnknowns * peano4::dLinearised(volume, numberOfDofsPerAxis);
      for (int u = 0; u < NumberOfUnknowns; u++) {
        coarseGridCellValues[index + u] = volume[u];
      }
    });

    for (int normal = 0; normal < DIMENSIONS; normal++) {
      auto serialiseFaceIndex = [normal, numberOfDofsPerAxis](tarch::la::Vector<DIMENSIONS, int> volume) {
        int base  = NumberOfUnknowns;
        int index = 0;
        for (int d = 0; d < DIMENSIONS; d++) {
          index += volume(d) * base;
          if (d == normal) {
            base *= 2;
          } else {
            base *= numberOfDofsPerAxis;
          }
        }
        return index;
      };

      for (int side = 0; side < 2; side++) {
        peano4::grid::GridTraversalEvent event;
        peano4::dForEach(3, [&](const tarch::la::Vector<DIMENSIONS, int>& subCellIndex) {
          event.setRelativePositionToFather(subCellIndex);
          auto faceMarker = event.getFaceMarker();
          faceMarker.select(normal + side * DIMENSIONS);

          if (faceMarker.isInteriorFaceWithinPatch()) {
            std::fill_n(fineGridFaceValues.data(), faceSize, 0.);
            toolbox::blockstructured::interpolateHaloLayerFromCoarseCell(
              faceMarker,
              coarseGridCellValues.data(),
              fineGridFaceValues.data(),
              side,
              numberOfDofsPerAxis,
              NumberOfUnknowns
            );

            peano4::dForEachExclusive(
              numberOfDofsPerAxis,
              normal,
              (1 - side),
              [&](const tarch::la::Vector<DIMENSIONS, int>& volume) {
                const auto index = serialiseFaceIndex(volume);
                for (int u = 0; u < NumberOfUnknowns; u++) {
                  validateNumericalEquals(fineGridFaceValues[index + u], 0.);
                }
              }
            );
            // With value==position the expected value equals the coarse source
            // index (side dependent along the normal, see the routine).
            const auto subCellPositionAlongNormal = faceMarker.getRelativePositionWithinFatherCell()[normal];
            peano4::dForEachExclusive(
              numberOfDofsPerAxis,
              normal,
              side,
              [&](const tarch::la::Vector<DIMENSIONS, int>& volume) {
                const auto index = serialiseFaceIndex(volume);
                for (int u = 0; u < NumberOfUnknowns; u++) {
                  const auto expectedValue = (u == normal)
                    ? (numberOfDofsPerAxis * subCellPositionAlongNormal - (1 - side)) / 3
                    : (numberOfDofsPerAxis * faceMarker.getRelativePositionWithinFatherCell()[u] + volume[u]) / 3;
                  validateNumericalEquals(fineGridFaceValues[index + u], expectedValue);
                }
              }
            );
          }
        });
      }
    }
  }
}

void toolbox::blockstructured::tests::InterpolationTest::testInterpolateCell() {
  // Unknowns == DIMENSIONS lets each axis carry a distinct linear ramp.
  constexpr int NumberOfUnknowns    = DIMENSIONS;
  constexpr int NumberOfDofsPerAxis = 7;

#if DIMENSIONS == 2
  constexpr int CellSize = NumberOfDofsPerAxis * NumberOfDofsPerAxis * NumberOfUnknowns;
#else
  constexpr int CellSize = NumberOfDofsPerAxis * NumberOfDofsPerAxis * NumberOfDofsPerAxis * NumberOfUnknowns;
#endif
  double coarseGridCellValues[CellSize];
  double fineGridCellValues[CellSize];

  peano4::dForEach(NumberOfDofsPerAxis, [&](const tarch::la::Vector<DIMENSIONS, int>& volume) {
    const auto index = NumberOfUnknowns * peano4::dLinearised(volume, NumberOfDofsPerAxis);
    for (int u = 0; u < NumberOfUnknowns; u++) {
      coarseGridCellValues[index + u] = volume[u];
    }
  });

  peano4::grid::GridTraversalEvent event;
  peano4::dForEach(3, [&](const tarch::la::Vector<DIMENSIONS, int>& subCellIndex) {
    event.setRelativePositionToFather(subCellIndex);
    const auto cellMarker = event.getCellMarker();

    toolbox::blockstructured::interpolateCell(
      cellMarker,
      coarseGridCellValues,
      fineGridCellValues,
      NumberOfDofsPerAxis,
      NumberOfUnknowns
    );

    peano4::dForEach(NumberOfDofsPerAxis, [&](const tarch::la::Vector<DIMENSIONS, int>& volume) {
      const auto index = NumberOfUnknowns * peano4::dLinearised(volume, NumberOfDofsPerAxis);
      for (int u = 0; u < NumberOfUnknowns; u++) {
        validateNumericalEqualsWithParams4(
          fineGridCellValues[index + u],
          (NumberOfDofsPerAxis * subCellIndex[u] + volume[u]) / 3,
          subCellIndex,
          index,
          volume,
          u
        );
      }
    });
  });
}
