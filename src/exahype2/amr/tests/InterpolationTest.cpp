// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "InterpolationTest.h"

#include "exahype2/amr/AMRRoutines.h"
#include "peano4/Loop.h"
#include "peano4/grid/GridTraversalEvent.h"
#include "peano4/sfc/ActiveCurve.h"

namespace {
  constexpr int SubdivisionFactor = peano4::sfc::ActiveCurve::SubdivisionFactor;
} // namespace

exahype2::amr::tests::InterpolationTest::InterpolationTest() :
  TestCase("exahype2::amr::tests::InterpolationTest") {}

void exahype2::amr::tests::InterpolationTest::run() {
  testMethod(testInterpolateHaloLayer);
  testMethod(testInterpolateHaloLayerFromCoarseCell);
  testMethod(testInterpolateCell);
  testMethod(testInterpolateHaloLayerWithTangentialSlopeIsBitExactUnderReflection);
  testMethod(testInterpolateHaloLayerWithTangentialSlopeIsBitExactUnderTangentialSwap);
  testMethod(testInterpolateHaloLayerWithTangentialSlopePreservesFreeStream);
  testMethod(testInterpolateHaloLayerWithTangentialSlopeIsLinearExact);
}

/**
 * The fine ghost of a tangentially reflected coarse face equals the reflection of the
 * fine ghost of the original coarse face, bit for bit. A solution-driven AMR criterion
 * amplifies any last-bit asymmetry into diverging topology between mirror patches.
 */
void exahype2::amr::tests::InterpolationTest::testInterpolateHaloLayerWithTangentialSlopeIsBitExactUnderReflection() {
  constexpr int NumberOfUnknowns    = 3;
  constexpr int NumberOfDofsPerAxis = 6;

#if DIMENSIONS == 2
  constexpr int FaceSize = 2 * NumberOfUnknowns * NumberOfDofsPerAxis;
#else
  constexpr int FaceSize = 2 * NumberOfUnknowns * NumberOfDofsPerAxis * NumberOfDofsPerAxis;
#endif

  // A genuinely non-symmetric, non-affine tangential profile, so any asymmetry in the
  // slope stencil or its end clamping shows up in the last bits.
  auto profile = [](const tarch::la::Vector<DIMENSIONS, int>& tangential, int u) {
    double result = 0.0;
    for (int d = 0; d < DIMENSIONS; d++) {
      result += std::sin(0.7 * tangential(d) + 1.1 * u + 0.3 * d) + 0.23 * tangential(d) * tangential(d)
        + 0.013 * tangential(d);
    }
    return result;
  };

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
      for (int mirroredAxis = 0; mirroredAxis < DIMENSIONS; mirroredAxis++) {
        if (mirroredAxis == normal) {
          continue;
        }

        double coarseGridFaceValues[FaceSize];
        double mirroredCoarseGridFaceValues[FaceSize];
        std::fill_n(coarseGridFaceValues, FaceSize, 0.);
        std::fill_n(mirroredCoarseGridFaceValues, FaceSize, 0.);
        peano4::dForEachExclusive(
          NumberOfDofsPerAxis,
          normal,
          side,
          [&](const tarch::la::Vector<DIMENSIONS, int>& volume) {
            tarch::la::Vector<DIMENSIONS, int> mirroredVolume = volume;
            mirroredVolume(mirroredAxis)                      = NumberOfDofsPerAxis - 1 - volume(mirroredAxis);
            for (int u = 0; u < NumberOfUnknowns; u++) {
              // One evaluation feeds both arrays: A compiler may lower two calls on the same
              // arguments through different code paths whose transcendentals round apart, and
              // the reflection check requires bit-identical mirrored input by construction.
              const double value                                               = profile(volume, u);
              coarseGridFaceValues[serialiseIndex(volume) + u]                 = value;
              mirroredCoarseGridFaceValues[serialiseIndex(mirroredVolume) + u] = value;
            }
          }
        );

        peano4::grid::GridTraversalEvent event;
        peano4::dForEach(SubdivisionFactor, [&](const tarch::la::Vector<DIMENSIONS, int>& subCellIndex) {
          if (subCellIndex(normal) != (SubdivisionFactor - 1) * side) {
            return;
          }
          tarch::la::Vector<DIMENSIONS, int> mirroredSubCellIndex = subCellIndex;
          mirroredSubCellIndex(mirroredAxis)                      = SubdivisionFactor - 1 - subCellIndex(mirroredAxis);

          event.setRelativePositionToFather(subCellIndex);
          auto faceMarker = event.getFaceMarker();
          faceMarker.select(normal + side * DIMENSIONS);

          event.setRelativePositionToFather(mirroredSubCellIndex);
          auto mirroredFaceMarker = event.getFaceMarker();
          mirroredFaceMarker.select(normal + side * DIMENSIONS);

          double fineGridFaceValues[FaceSize];
          double mirroredFineGridFaceValues[FaceSize];
          std::fill_n(fineGridFaceValues, FaceSize, 0.);
          std::fill_n(mirroredFineGridFaceValues, FaceSize, 0.);

          exahype2::amr::interpolateHaloLayerWithTangentialSlope(
            faceMarker,
            coarseGridFaceValues,
            fineGridFaceValues,
            side,
            NumberOfDofsPerAxis,
            NumberOfUnknowns
          );
          exahype2::amr::interpolateHaloLayerWithTangentialSlope(
            mirroredFaceMarker,
            mirroredCoarseGridFaceValues,
            mirroredFineGridFaceValues,
            side,
            NumberOfDofsPerAxis,
            NumberOfUnknowns
          );

          peano4::dForEachExclusive(
            NumberOfDofsPerAxis,
            normal,
            side,
            [&](const tarch::la::Vector<DIMENSIONS, int>& volume) {
              tarch::la::Vector<DIMENSIONS, int> mirroredVolume = volume;
              mirroredVolume(mirroredAxis)                      = NumberOfDofsPerAxis - 1 - volume(mirroredAxis);
              for (int u = 0; u < NumberOfUnknowns; u++) {
                const double difference = fineGridFaceValues[serialiseIndex(volume) + u]
                  - mirroredFineGridFaceValues[serialiseIndex(mirroredVolume) + u];
                validateWithParams4(difference == 0.0, difference, normal, side, mirroredAxis);
              }
            }
          );
        });
      }
    }
  }
}

/**
 * Swapping the two tangential axes of a face commutes bitwise with the interpolation.
 * Two dimensions have a single tangential axis, so this test runs in three dimensions
 * only.
 */
void exahype2::amr::tests::InterpolationTest::
  testInterpolateHaloLayerWithTangentialSlopeIsBitExactUnderTangentialSwap() {
#if DIMENSIONS == 3
  constexpr int NumberOfUnknowns    = 3;
  constexpr int NumberOfDofsPerAxis = 6;
  constexpr int FaceSize            = 2 * NumberOfUnknowns * NumberOfDofsPerAxis * NumberOfDofsPerAxis;

  // A genuinely non-symmetric, non-affine profile, so the two tangential axes carry
  // distinct slopes and any rounding difference between the swap orders shows up.
  auto profile = [](const tarch::la::Vector<DIMENSIONS, int>& tangential, int u) {
    double result = 0.0;
    for (int d = 0; d < DIMENSIONS; d++) {
      result += std::sin(0.7 * tangential(d) + 1.1 * u + 0.3 * d) + 0.23 * tangential(d) * tangential(d)
        + 0.013 * tangential(d);
    }
    return result;
  };

  for (int normal = 0; normal < DIMENSIONS; normal++) {
    const int firstTangentialAxis  = normal == 0 ? 1 : 0;
    const int secondTangentialAxis = normal == 2 ? 1 : 2;

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
    auto swapTangentialAxes = [&](tarch::la::Vector<DIMENSIONS, int> volume) {
      const auto swapped           = volume(firstTangentialAxis);
      volume(firstTangentialAxis)  = volume(secondTangentialAxis);
      volume(secondTangentialAxis) = swapped;
      return volume;
    };

    for (int side = 0; side < 2; side++) {
      double coarseGridFaceValues[FaceSize];
      double swappedCoarseGridFaceValues[FaceSize];
      std::fill_n(coarseGridFaceValues, FaceSize, 0.);
      std::fill_n(swappedCoarseGridFaceValues, FaceSize, 0.);
      peano4::dForEachExclusive(
        NumberOfDofsPerAxis,
        normal,
        side,
        [&](const tarch::la::Vector<DIMENSIONS, int>& volume) {
          for (int u = 0; u < NumberOfUnknowns; u++) {
            // One evaluation feeds both arrays, so the swapped input equals the swap of the
            // original input to the bit.
            const double value                                                          = profile(volume, u);
            coarseGridFaceValues[serialiseIndex(volume) + u]                            = value;
            swappedCoarseGridFaceValues[serialiseIndex(swapTangentialAxes(volume)) + u] = value;
          }
        }
      );

      peano4::grid::GridTraversalEvent event;
      peano4::dForEach(SubdivisionFactor, [&](const tarch::la::Vector<DIMENSIONS, int>& subCellIndex) {
        if (subCellIndex(normal) != (SubdivisionFactor - 1) * side) {
          return;
        }
        event.setRelativePositionToFather(subCellIndex);
        auto faceMarker = event.getFaceMarker();
        faceMarker.select(normal + side * DIMENSIONS);

        event.setRelativePositionToFather(swapTangentialAxes(subCellIndex));
        auto swappedFaceMarker = event.getFaceMarker();
        swappedFaceMarker.select(normal + side * DIMENSIONS);

        double fineGridFaceValues[FaceSize];
        double swappedFineGridFaceValues[FaceSize];
        std::fill_n(fineGridFaceValues, FaceSize, 0.);
        std::fill_n(swappedFineGridFaceValues, FaceSize, 0.);

        exahype2::amr::interpolateHaloLayerWithTangentialSlope(
          faceMarker,
          coarseGridFaceValues,
          fineGridFaceValues,
          side,
          NumberOfDofsPerAxis,
          NumberOfUnknowns
        );
        exahype2::amr::interpolateHaloLayerWithTangentialSlope(
          swappedFaceMarker,
          swappedCoarseGridFaceValues,
          swappedFineGridFaceValues,
          side,
          NumberOfDofsPerAxis,
          NumberOfUnknowns
        );

        peano4::dForEachExclusive(
          NumberOfDofsPerAxis,
          normal,
          side,
          [&](const tarch::la::Vector<DIMENSIONS, int>& volume) {
            for (int u = 0; u < NumberOfUnknowns; u++) {
              const double difference = fineGridFaceValues[serialiseIndex(volume) + u]
                - swappedFineGridFaceValues[serialiseIndex(swapTangentialAxes(volume)) + u];
              validateWithParams3(difference == 0.0, difference, normal, side);
            }
          }
        );
      });
    }
  }
#endif
}

/**
 * A constant coarse field must produce a constant fine ghost exactly (free-stream
 * preservation).
 */
void exahype2::amr::tests::InterpolationTest::testInterpolateHaloLayerWithTangentialSlopePreservesFreeStream() {
  constexpr int    NumberOfUnknowns    = 4;
  constexpr int    NumberOfDofsPerAxis = 5;
  constexpr double Constant            = 0.123456789;

#if DIMENSIONS == 2
  constexpr int FaceSize = 2 * NumberOfUnknowns * NumberOfDofsPerAxis;
#else
  constexpr int FaceSize = 2 * NumberOfUnknowns * NumberOfDofsPerAxis * NumberOfDofsPerAxis;
#endif
  double coarseGridFaceValues[FaceSize];
  double fineGridFaceValues[FaceSize];
  std::fill_n(coarseGridFaceValues, FaceSize, Constant);

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
      peano4::grid::GridTraversalEvent event;
      peano4::dForEach(SubdivisionFactor, [&](const tarch::la::Vector<DIMENSIONS, int>& subCellIndex) {
        if (subCellIndex(normal) != (SubdivisionFactor - 1) * side) {
          return;
        }
        event.setRelativePositionToFather(subCellIndex);
        auto faceMarker = event.getFaceMarker();
        faceMarker.select(normal + side * DIMENSIONS);

        std::fill_n(fineGridFaceValues, FaceSize, 0.);
        exahype2::amr::interpolateHaloLayerWithTangentialSlope(
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
          side,
          [&](const tarch::la::Vector<DIMENSIONS, int>& volume) {
            for (int u = 0; u < NumberOfUnknowns; u++) {
              const double difference = fineGridFaceValues[serialiseIndex(volume) + u] - Constant;
              validateWithParams3(difference == 0.0, difference, normal, side);
            }
          }
        );
      });
    }
  }
}

/**
 * A linear coarse tangential profile is reconstructed exactly in every fine sub-patch,
 * including the two end sub-patches whose stencils reach the patch boundary and fall
 * back to a one-sided difference.
 */
void exahype2::amr::tests::InterpolationTest::testInterpolateHaloLayerWithTangentialSlopeIsLinearExact() {
  constexpr int    NumberOfUnknowns    = 1;
  constexpr int    NumberOfDofsPerAxis = 6;
  constexpr double Slope               = 0.37;
  constexpr double Intercept           = 1.9;

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

    for (int tangentialAxis = 0; tangentialAxis < DIMENSIONS; tangentialAxis++) {
      if (tangentialAxis == normal) {
        continue;
      }

      constexpr int Side = 0;
      std::fill_n(coarseGridFaceValues, FaceSize, 0.);
      peano4::dForEachExclusive(
        NumberOfDofsPerAxis,
        normal,
        Side,
        [&](const tarch::la::Vector<DIMENSIONS, int>& volume) {
          coarseGridFaceValues[serialiseIndex(volume)] = Slope * volume(tangentialAxis) + Intercept;
        }
      );

      // The sub-patch positions cover the interior central difference as well as
      // the one-sided difference at both patch ends, so every fine volume must
      // reproduce the linear profile.
      for (int subPatch = 0; subPatch < SubdivisionFactor; subPatch++) {
        peano4::grid::GridTraversalEvent event;
        event.setRelativePositionToFather(tarch::la::Vector<DIMENSIONS, int>(subPatch));
        auto faceMarker = event.getFaceMarker();
        faceMarker.select(normal + Side * DIMENSIONS);

        std::fill_n(fineGridFaceValues, FaceSize, 0.);
        exahype2::amr::interpolateHaloLayerWithTangentialSlope(
          faceMarker,
          coarseGridFaceValues,
          fineGridFaceValues,
          Side,
          NumberOfDofsPerAxis,
          NumberOfUnknowns
        );

        peano4::dForEachExclusive(
          NumberOfDofsPerAxis,
          normal,
          Side,
          [&](const tarch::la::Vector<DIMENSIONS, int>& volume) {
            const int    globalIndex = volume(tangentialAxis) + NumberOfDofsPerAxis * subPatch;
            const int    coarseCell  = globalIndex / SubdivisionFactor;
            const int    position    = globalIndex % SubdivisionFactor;
            const double expected =
              Slope * coarseCell + Intercept + Slope * (position - 0.5 * (SubdivisionFactor - 1)) / SubdivisionFactor;
            validateNumericalEqualsWithParams4(
              fineGridFaceValues[serialiseIndex(volume)],
              expected,
              normal,
              tangentialAxis,
              subPatch,
              volume
            );
          }
        );
      }
    }
  }
}

void exahype2::amr::tests::InterpolationTest::testInterpolateHaloLayer() {
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
      peano4::dForEach(SubdivisionFactor, [&](const tarch::la::Vector<DIMENSIONS, int>& subCellIndex) {
        event.setRelativePositionToFather(subCellIndex);
        auto faceMarker = event.getFaceMarker();
        faceMarker.select(normal + side * DIMENSIONS);

        if (not faceMarker.isInteriorFaceWithinPatch()) {
          std::fill_n(fineGridFaceValues, FaceSize, 0.);
          exahype2::amr::interpolateHaloLayer(
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
                  (u == normal ? side : (NumberOfDofsPerAxis * subCellIndex[u] + volume[u]) / SubdivisionFactor);
                validateNumericalEquals(fineGridFaceValues[index + u], expectedValue);
              }
            }
          );
        }
      });
    }
  }
}

void exahype2::amr::tests::InterpolationTest::testInterpolateHaloLayerFromCoarseCell() {
  // Unknowns == DIMENSIONS lets each axis carry a distinct linear ramp.
  constexpr int NumberOfUnknowns = DIMENSIONS;

  // The coarse source layer along the normal depends on side only when the
  // subdivision factor divides the patch size, so one divisible size is needed
  // to exercise that side dependence and one non-divisible size covers the rest.
  for (const auto numberOfDofsPerAxis : {2 * SubdivisionFactor, 2 * SubdivisionFactor + 1}) {
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
        peano4::dForEach(SubdivisionFactor, [&](const tarch::la::Vector<DIMENSIONS, int>& subCellIndex) {
          event.setRelativePositionToFather(subCellIndex);
          auto faceMarker = event.getFaceMarker();
          faceMarker.select(normal + side * DIMENSIONS);

          if (faceMarker.isInteriorFaceWithinPatch()) {
            std::fill_n(fineGridFaceValues.data(), faceSize, 0.);
            exahype2::amr::interpolateHaloLayerFromCoarseCell(
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
            // Each coarse value equals its own position, so the expected value equals
            // the coarse source index, which depends on side along the normal.
            const auto subCellPositionAlongNormal = faceMarker.getRelativePositionWithinFatherCell()[normal];
            peano4::dForEachExclusive(
              numberOfDofsPerAxis,
              normal,
              side,
              [&](const tarch::la::Vector<DIMENSIONS, int>& volume) {
                const auto index = serialiseFaceIndex(volume);
                for (int u = 0; u < NumberOfUnknowns; u++) {
                  const auto expectedValue = (u == normal)
                    ? (numberOfDofsPerAxis * subCellPositionAlongNormal - (1 - side)) / SubdivisionFactor
                    : (numberOfDofsPerAxis * faceMarker.getRelativePositionWithinFatherCell()[u] + volume[u])
                      / SubdivisionFactor;
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

void exahype2::amr::tests::InterpolationTest::testInterpolateCell() {
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
  peano4::dForEach(SubdivisionFactor, [&](const tarch::la::Vector<DIMENSIONS, int>& subCellIndex) {
    event.setRelativePositionToFather(subCellIndex);
    const auto cellMarker = event.getCellMarker();

    exahype2::amr::interpolateCell(
      cellMarker,
      coarseGridCellValues,
      fineGridCellValues,
      NumberOfDofsPerAxis,
      NumberOfUnknowns
    );

    peano4::dForEach(NumberOfDofsPerAxis, [&](const tarch::la::Vector<DIMENSIONS, int>& volume) {
      const auto index = NumberOfUnknowns * peano4::dLinearised(volume, NumberOfDofsPerAxis);
      for (int u = 0; u < NumberOfUnknowns; u++) {
        // interpolateCell copies the coarse volume that contains the fine volume, so the division truncates.
        const int coarseVolumeCoordinate = (NumberOfDofsPerAxis * subCellIndex[u] + volume[u]) / SubdivisionFactor;
        validateNumericalEqualsWithParams4(
          fineGridCellValues[index + u],
          static_cast<double>(coarseVolumeCoordinate),
          subCellIndex,
          index,
          volume,
          u
        );
      }
    });
  });
}
