// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "RestrictionTest.h"

#include "exahype2/amr/AMRRoutines.h"
#include "peano4/Constants.h"
#include "peano4/Loop.h"
#include "peano4/grid/GridTraversalEvent.h"
#include "peano4/sfc/ActiveCurve.h"

namespace {
  constexpr int SubdivisionFactor = peano4::sfc::ActiveCurve::SubdivisionFactor;
  constexpr int ChildrenPerCell   = peano4::sfc::ActiveCurve::ChildrenPerCell;

  // A two-point Gauss-Legendre basis on the unit interval, the matrices a solver generates
  // for polynomial order one.
  constexpr int         NodesPerAxis  = 2;
  constexpr std::size_t MatrixEntries = static_cast<std::size_t>(SubdivisionFactor) * NodesPerAxis * NodesPerAxis;
  constexpr double      Node0         = 0.5 - 0.5 / 1.7320508075688772;
  constexpr double      Node1         = 0.5 + 0.5 / 1.7320508075688772;

  double shapeFunction(const int node, const double x) {
    return node == 0 ? (Node1 - x) / (Node1 - Node0) : (x - Node0) / (Node1 - Node0);
  }

  std::array<double, MatrixEntries> interpolationMatrix() {
    std::array<double, MatrixEntries> result{};
    for (int block = 0; block < SubdivisionFactor; block++) {
      for (int fine = 0; fine < NodesPerAxis; fine++) {
        const double fineNode = ((fine == 0 ? Node0 : Node1) + block) / SubdivisionFactor;
        for (int coarse = 0; coarse < NodesPerAxis; coarse++) {
          result[block * NodesPerAxis * NodesPerAxis + fine * NodesPerAxis + coarse] = shapeFunction(coarse, fineNode);
        }
      }
    }
    return result;
  }

  std::array<double, MatrixEntries> restrictionMatrix() {
    std::array<double, MatrixEntries> result{};
    for (int block = 0; block < SubdivisionFactor; block++) {
      for (int coarse = 0; coarse < NodesPerAxis; coarse++) {
        for (int fine = 0; fine < NodesPerAxis; fine++) {
          const double fineNode = ((fine == 0 ? Node0 : Node1) + block) / SubdivisionFactor;
          result[block * NodesPerAxis * NodesPerAxis + coarse * NodesPerAxis + fine] =
            shapeFunction(coarse, fineNode) / SubdivisionFactor;
        }
      }
    }
    return result;
  }

  constexpr std::array<double, NodesPerAxis> QuadratureWeights = {0.5, 0.5};

  double nodePosition(const int node) { return node == 0 ? Node0 : Node1; }

  // An affine field is representable in the order-one basis, so the interpolate-then-restrict
  // round trip has to reproduce it exactly up to roundoff.
  double affineField(const tarch::la::Vector<DIMENSIONS, double>& x) {
    double result = 1.0;
    for (int d = 0; d < DIMENSIONS; d++) {
      result += (2.0 + d) * x(d);
    }
    return result;
  }

  // The equivariance tests need the restriction operator mirror symmetric to the
  // last bit, so the lower half of the child intervals is computed and the upper
  // half is its mirror copy.
  std::array<double, MatrixEntries> inputMajorSymmetricRestrictionMatrix() {
    std::array<double, MatrixEntries> result{};
    constexpr int                     Stride              = NodesPerAxis * NodesPerAxis;
    constexpr int                     NumberOfLowerBlocks = (SubdivisionFactor + 1) / 2;
    for (int block = 0; block < NumberOfLowerBlocks; block++) {
      for (int fine = 0; fine < NodesPerAxis; fine++) {
        const double fineNode = ((fine == 0 ? Node0 : Node1) + block) / SubdivisionFactor;
        for (int coarse = 0; coarse < NodesPerAxis; coarse++) {
          result[block * Stride + fine * NodesPerAxis + coarse] = shapeFunction(coarse, fineNode) / SubdivisionFactor;
        }
      }
    }
    for (int block = 0; block < NumberOfLowerBlocks; block++) {
      for (int fine = 0; fine < NodesPerAxis; fine++) {
        for (int coarse = 0; coarse < NodesPerAxis; coarse++) {
          result
            [(SubdivisionFactor - 1 - block) * Stride + (NodesPerAxis - 1 - fine) * NodesPerAxis
             + (NodesPerAxis - 1 - coarse)] = result[block * Stride + fine * NodesPerAxis + coarse];
        }
      }
    }
    return result;
  }

  // The prolongation counterpart of the matrix above, input-major with entry
  // [coarse * NodesPerAxis + fine] and mirror symmetric to the last bit.
  std::array<double, MatrixEntries> inputMajorSymmetricProlongationMatrix() {
    std::array<double, MatrixEntries> result{};
    constexpr int                     Stride              = NodesPerAxis * NodesPerAxis;
    constexpr int                     NumberOfLowerBlocks = (SubdivisionFactor + 1) / 2;
    for (int block = 0; block < NumberOfLowerBlocks; block++) {
      for (int fine = 0; fine < NodesPerAxis; fine++) {
        const double fineNode = ((fine == 0 ? Node0 : Node1) + block) / SubdivisionFactor;
        for (int coarse = 0; coarse < NodesPerAxis; coarse++) {
          result[block * Stride + coarse * NodesPerAxis + fine] = shapeFunction(coarse, fineNode);
        }
      }
    }
    for (int block = 0; block < NumberOfLowerBlocks; block++) {
      for (int fine = 0; fine < NodesPerAxis; fine++) {
        for (int coarse = 0; coarse < NodesPerAxis; coarse++) {
          result
            [(SubdivisionFactor - 1 - block) * Stride + (NodesPerAxis - 1 - coarse) * NodesPerAxis
             + (NodesPerAxis - 1 - fine)] = result[block * Stride + coarse * NodesPerAxis + fine];
        }
      }
    }
    return result;
  }

  double pseudoRandomValue(const int child, const int node, const int unknown) {
    const int hash = (child * 131 + node * 17 + unknown * 7 + 3) % 1999;
    return static_cast<double>(hash - 999) / 997.0;
  }

  std::uint64_t bitPattern(const double value) { return std::bit_cast<std::uint64_t>(value); }

  constexpr int NumberOfSubfaces = peano4::power(SubdivisionFactor, DIMENSIONS - 1);

  /**
   * Returns one face-node index independently of the routines under test.
   * A face stores both halves, so the side carries the largest stride along the
   * normal and the tangential axes keep the patch's own extent.
   */
  int faceNodeIndex(
    const int normal,
    const int side,
    const int tangentialI,
    const int tangentialJ,
    const int numberOfDofsPerAxis
  ) {
    const int iStride          = normal == 0 ? 2 : 1;
    const int jStride          = normal == 2 ? numberOfDofsPerAxis : 2 * numberOfDofsPerAxis;
    const int rowOrSliceStride = normal == 1 ? numberOfDofsPerAxis : numberOfDofsPerAxis * numberOfDofsPerAxis;
    const int sideStride       = normal == 0 ? 1 : rowOrSliceStride;
    return side * sideStride + tangentialI * iStride + tangentialJ * jStride;
  }

  /** Returns the number of values in both halves of a face array. */
  constexpr int faceValueCount(const int numberOfDofsPerAxis, const int numberOfUnknowns) {
    return DIMENSIONS == 2 ? 2 * numberOfDofsPerAxis * numberOfUnknowns
                           : 2 * numberOfDofsPerAxis * numberOfDofsPerAxis * numberOfUnknowns;
  }
} // namespace

exahype2::amr::tests::RestrictionTest::RestrictionTest() :
  TestCase("exahype2::amr::tests::RestrictionTest") {}

void exahype2::amr::tests::RestrictionTest::run() {
  testMethod(testRestrictHaloLayer);
  testMethod(testRestrictCell);
  testMethod(testRestrictCellWithMatrixReproducesPolynomials);
  testMethod(testOrderInvariantProduct);
  testMethod(testRestrictCellFromChildrenReproducesRamp);
  testMethod(testRestrictCellFromChildrenWithMatrixReproducesPolynomials);
  testMethod(testRestrictCellFromChildrenIsSymmetryEquivariant);
  testMethod(testApplyTensorProductOrderInvariant);
  testMethod(testInterpolateFaceFromCoarse);
  testMethod(testRestrictFaceFromChildren);
  testMethod(testGatheredHaloRestrictionAveragesTheSubfaceRamp);
  testMethod(testGatheredHaloRestrictionIsStagingOrderInvariant);
  testMethod(testGatheredHaloRestrictionScalesAPartialSubfaceSet);
  testMethod(testGatheredHaloRestrictionRunsInSinglePrecision);
}

void exahype2::amr::tests::RestrictionTest::testOrderInvariantProduct() {
  const double factors[3]         = {0.7, -1.3e-5, 42.0};
  const int    permutations[6][3] = {
    {0, 1, 2},
    {0, 2, 1},
    {1, 0, 2},
    {1, 2, 0},
    {2, 0, 1},
    {2, 1, 0},
  };
  const auto reference = exahype2::amr::orderInvariantProduct(factors[0], factors[1], factors[2]);
  for (const auto& permutation : permutations) {
    validateEquals(
      bitPattern(
        exahype2::amr::orderInvariantProduct(factors[permutation[0]], factors[permutation[1]], factors[permutation[2]])
      ),
      bitPattern(reference)
    );
  }
  validateNumericalEquals(exahype2::amr::orderInvariantProduct(2.0, 3.0, 4.0), 24.0);
}

void exahype2::amr::tests::RestrictionTest::testRestrictCellFromChildrenReproducesRamp() {
  // NumberOfUnknowns == DIMENSIONS lets each axis carry a distinct linear ramp.
  constexpr int NumberOfUnknowns    = DIMENSIONS;
  constexpr int NumberOfDofsPerAxis = 7;

  int volumesPerCell = 1;
  for (int d = 0; d < DIMENSIONS; d++) {
    volumesPerCell *= NumberOfDofsPerAxis;
  }

  std::vector<std::vector<double>> children(ChildrenPerCell);
  std::vector<const double*>       childPointers(ChildrenPerCell);
  peano4::dForEach(SubdivisionFactor, [&](const tarch::la::Vector<DIMENSIONS, int>& child) {
    auto& values = children[peano4::dLinearised(child, SubdivisionFactor)];
    values.resize(static_cast<std::size_t>(volumesPerCell) * NumberOfUnknowns);
    peano4::dForEach(NumberOfDofsPerAxis, [&](const tarch::la::Vector<DIMENSIONS, int>& volume) {
      const auto index = NumberOfUnknowns * peano4::dLinearised(volume, NumberOfDofsPerAxis);
      for (int u = 0; u < NumberOfUnknowns; u++) {
        values[index + u] = volume[u] + NumberOfDofsPerAxis * child[u];
      }
    });
    childPointers[peano4::dLinearised(child, SubdivisionFactor)] = values.data();
  });

  std::vector<double> coarse(static_cast<std::size_t>(volumesPerCell) * NumberOfUnknowns, -1.0);
  exahype2::amr::restrictCellFromChildren(coarse.data(), childPointers.data(), NumberOfDofsPerAxis, NumberOfUnknowns);

  // Averaging the fine volumes of a linear ramp per axis yields the ramp's value in
  // the middle of the coarse volume, which is a multiple of one half and therefore
  // exact.
  peano4::dForEach(NumberOfDofsPerAxis, [&](const tarch::la::Vector<DIMENSIONS, int>& volume) {
    const auto index = NumberOfUnknowns * peano4::dLinearised(volume, NumberOfDofsPerAxis);
    for (int u = 0; u < NumberOfUnknowns; u++) {
      validateNumericalEquals(coarse[index + u], SubdivisionFactor * volume[u] + 0.5 * (SubdivisionFactor - 1));
    }
  });
}

void exahype2::amr::tests::RestrictionTest::testRestrictCellFromChildrenWithMatrixReproducesPolynomials() {
  constexpr int NumberOfUnknowns = 2;

  int nodesPerCell = 1;
  for (int d = 0; d < DIMENSIONS; d++) {
    nodesPerCell *= NodesPerAxis;
  }

  const auto                                   interpolation = interpolationMatrix();
  const auto                                   restriction   = inputMajorSymmetricRestrictionMatrix();
  std::array<const double*, SubdivisionFactor> restrictionMatrixPerInterval{};
  for (int interval = 0; interval < SubdivisionFactor; interval++) {
    restrictionMatrixPerInterval[interval] =
      restriction.data() + static_cast<std::ptrdiff_t>(interval) * NodesPerAxis * NodesPerAxis;
  }

  std::vector<double> coarse(static_cast<std::size_t>(nodesPerCell) * NumberOfUnknowns, 0.0);
  peano4::dForEach(NodesPerAxis, [&](const tarch::la::Vector<DIMENSIONS, int>& node) {
    tarch::la::Vector<DIMENSIONS, double> x;
    for (int d = 0; d < DIMENSIONS; d++) {
      x(d) = nodePosition(node(d));
    }
    const int index   = peano4::dLinearised(node, NodesPerAxis) * NumberOfUnknowns;
    coarse[index]     = affineField(x);
    coarse[index + 1] = 4.0;
  });

  peano4::grid::GridTraversalEvent event;
  std::vector<std::vector<double>> children(ChildrenPerCell);
  std::vector<const double*>       childPointers(ChildrenPerCell);
  peano4::dForEach(SubdivisionFactor, [&](const tarch::la::Vector<DIMENSIONS, int>& child) {
    event.setRelativePositionToFather(child);
    auto& fine = children[peano4::dLinearised(child, SubdivisionFactor)];
    fine.resize(static_cast<std::size_t>(nodesPerCell) * NumberOfUnknowns);
    exahype2::amr::interpolateCell(
      event.getCellMarker(),
      coarse.data(),
      fine.data(),
      interpolation.data(),
      NodesPerAxis,
      NumberOfUnknowns
    );
    childPointers[peano4::dLinearised(child, SubdivisionFactor)] = fine.data();
  });

  std::vector<double> restricted(static_cast<std::size_t>(nodesPerCell) * NumberOfUnknowns, 0.0);
  exahype2::amr::restrictCellFromChildren(
    restricted.data(),
    childPointers.data(),
    restrictionMatrixPerInterval.data(),
    NodesPerAxis,
    NumberOfUnknowns
  );

  for (std::size_t i = 0; i < coarse.size(); i++) {
    validateNumericalEqualsWithEps(restricted[i], coarse[i], 1e-12);
  }
}

void exahype2::amr::tests::RestrictionTest::testRestrictCellFromChildrenIsSymmetryEquivariant() {
  constexpr int NumberOfUnknowns = 2;

  int nodesPerCell = 1;
  for (int d = 0; d < DIMENSIONS; d++) {
    nodesPerCell *= NodesPerAxis;
  }

  const auto                                   restriction = inputMajorSymmetricRestrictionMatrix();
  std::array<const double*, SubdivisionFactor> restrictionMatrixPerInterval{};
  for (int interval = 0; interval < SubdivisionFactor; interval++) {
    restrictionMatrixPerInterval[interval] =
      restriction.data() + static_cast<std::ptrdiff_t>(interval) * NodesPerAxis * NodesPerAxis;
  }

  const auto reflectAxis0 = [](const tarch::la::Vector<DIMENSIONS, int>& index, const int range) {
    auto result = index;
    result(0)   = range - 1 - index(0);
    return result;
  };
  const auto swapAxes01 = [](const tarch::la::Vector<DIMENSIONS, int>& index, [[maybe_unused]] const int range) {
    auto result = index;
    result(0)   = index(1);
    result(1)   = index(0);
    return result;
  };
  using Transform = tarch::la::Vector<DIMENSIONS, int> (*)(const tarch::la::Vector<DIMENSIONS, int>& index, int range);
  const Transform transforms[2] = {reflectAxis0, swapAxes01};

  for (const auto transform : transforms) {
    std::vector<std::vector<double>> children(ChildrenPerCell);
    std::vector<std::vector<double>> mirroredChildren(ChildrenPerCell);
    std::vector<const double*>       childPointers(ChildrenPerCell);
    std::vector<const double*>       mirroredChildPointers(ChildrenPerCell);
    for (int child = 0; child < ChildrenPerCell; child++) {
      children[child].resize(static_cast<std::size_t>(nodesPerCell) * NumberOfUnknowns);
      mirroredChildren[child].resize(static_cast<std::size_t>(nodesPerCell) * NumberOfUnknowns);
    }
    peano4::dForEach(SubdivisionFactor, [&](const tarch::la::Vector<DIMENSIONS, int>& child) {
      peano4::dForEach(NodesPerAxis, [&](const tarch::la::Vector<DIMENSIONS, int>& node) {
        const auto sourceIndex = peano4::dLinearised(node, NodesPerAxis) * NumberOfUnknowns;
        const auto targetIndex = peano4::dLinearised(transform(node, NodesPerAxis), NodesPerAxis) * NumberOfUnknowns;
        for (int u = 0; u < NumberOfUnknowns; u++) {
          const auto value = pseudoRandomValue(
            peano4::dLinearised(child, SubdivisionFactor),
            peano4::dLinearised(node, NodesPerAxis),
            u
          );
          children[peano4::dLinearised(child, SubdivisionFactor)][sourceIndex + u] = value;
          mirroredChildren[peano4::dLinearised(transform(child, SubdivisionFactor), SubdivisionFactor)]
                          [targetIndex + u] = value;
        }
      });
    });
    for (int child = 0; child < ChildrenPerCell; child++) {
      childPointers[child]         = children[child].data();
      mirroredChildPointers[child] = mirroredChildren[child].data();
    }

    // The averaging and the mass-lumped restriction both have to map mirrored
    // children onto the bitwise mirrored coarse cell.
    std::vector<double> coarse(static_cast<std::size_t>(nodesPerCell) * NumberOfUnknowns, 0.0);
    std::vector<double> mirroredCoarse(static_cast<std::size_t>(nodesPerCell) * NumberOfUnknowns, 0.0);
    exahype2::amr::restrictCellFromChildren(coarse.data(), childPointers.data(), NodesPerAxis, NumberOfUnknowns);
    exahype2::amr::restrictCellFromChildren(
      mirroredCoarse.data(),
      mirroredChildPointers.data(),
      NodesPerAxis,
      NumberOfUnknowns
    );
    peano4::dForEach(NodesPerAxis, [&](const tarch::la::Vector<DIMENSIONS, int>& node) {
      const auto sourceIndex = peano4::dLinearised(node, NodesPerAxis) * NumberOfUnknowns;
      const auto targetIndex = peano4::dLinearised(transform(node, NodesPerAxis), NodesPerAxis) * NumberOfUnknowns;
      for (int u = 0; u < NumberOfUnknowns; u++) {
        validateEquals(bitPattern(mirroredCoarse[targetIndex + u]), bitPattern(coarse[sourceIndex + u]));
      }
    });

    std::fill(coarse.begin(), coarse.end(), 0.0);
    std::fill(mirroredCoarse.begin(), mirroredCoarse.end(), 0.0);
    exahype2::amr::restrictCellFromChildren(
      coarse.data(),
      childPointers.data(),
      restrictionMatrixPerInterval.data(),
      NodesPerAxis,
      NumberOfUnknowns
    );
    exahype2::amr::restrictCellFromChildren(
      mirroredCoarse.data(),
      mirroredChildPointers.data(),
      restrictionMatrixPerInterval.data(),
      NodesPerAxis,
      NumberOfUnknowns
    );
    peano4::dForEach(NodesPerAxis, [&](const tarch::la::Vector<DIMENSIONS, int>& node) {
      const auto sourceIndex = peano4::dLinearised(node, NodesPerAxis) * NumberOfUnknowns;
      const auto targetIndex = peano4::dLinearised(transform(node, NodesPerAxis), NodesPerAxis) * NumberOfUnknowns;
      for (int u = 0; u < NumberOfUnknowns; u++) {
        validateEquals(bitPattern(mirroredCoarse[targetIndex + u]), bitPattern(coarse[sourceIndex + u]));
      }
    });
  }
}

/**
 * Verifies that a coarse node of the gathered restriction comes out as the mean
 * of the fine nodes it covers, on both halves and every normal.
 */
void exahype2::amr::tests::RestrictionTest::testGatheredHaloRestrictionAveragesTheSubfaceRamp() {
  constexpr int    NumberOfUnknowns    = 2;
  constexpr int    NumberOfDofsPerAxis = 2;
  constexpr int    FaceSize            = faceValueCount(NumberOfDofsPerAxis, NumberOfUnknowns);
  constexpr int    ValuesPerSubface    = DIMENSIONS == 2 ? NumberOfDofsPerAxis * NumberOfUnknowns
                                                         : NumberOfDofsPerAxis * NumberOfDofsPerAxis * NumberOfUnknowns;
  constexpr double CoveredNodeCentre   = 0.5 * (SubdivisionFactor - 1);

  for (int normal = 0; normal < DIMENSIONS; normal++) {
    for (int side = 0; side < 2; side++) {
      exahype2::amr::ChildDataStage<double> stage(NumberOfSubfaces, ValuesPerSubface);
      std::vector<double>                   coarseGridFaceValues(FaceSize, -1.0);
      const auto* const                     coarseFaceKey = coarseGridFaceValues.data();

      peano4::grid::GridTraversalEvent event;
      peano4::dForEach(SubdivisionFactor, [&](const tarch::la::Vector<DIMENSIONS, int>& subCellIndex) {
        event.setRelativePositionToFather(subCellIndex);
        auto faceMarker = event.getFaceMarker();
        faceMarker.select(normal + side * DIMENSIONS);
        if (faceMarker.isInteriorFaceWithinPatch()) {
          return;
        }

        const auto subfacePosition = faceMarker.getRelativePositionWithinFatherFace();
        const int  subfaceI        = subfacePosition[0];
#if DIMENSIONS == 3
        const int subfaceJ = subfacePosition[1];
#else
        const int subfaceJ = 0;
#endif

        std::vector<double> fineGridFaceValues(FaceSize, 0.0);
#if DIMENSIONS == 3
        for (int fineJ = 0; fineJ < NumberOfDofsPerAxis; fineJ++) {
#else
        const int fineJ = 0;
        {
#endif
          for (int fineI = 0; fineI < NumberOfDofsPerAxis; fineI++) {
            const auto index = faceNodeIndex(normal, side, fineI, fineJ, NumberOfDofsPerAxis) * NumberOfUnknowns;
            fineGridFaceValues[index]     = subfaceI * NumberOfDofsPerAxis + fineI;
            fineGridFaceValues[index + 1] = subfaceJ * NumberOfDofsPerAxis + fineJ;
          }
        }

        exahype2::amr::stageHaloLayerForGatheredRestriction(
          faceMarker,
          stage,
          coarseFaceKey,
          fineGridFaceValues.data(),
          side,
          NumberOfDofsPerAxis,
          NumberOfUnknowns
        );
      });

      validateEquals(stage.frameTag(coarseFaceKey), side);
      exahype2::amr::restrictHaloLayerFromStagedSubfaces(
        stage,
        coarseFaceKey,
        coarseGridFaceValues.data(),
        normal,
        NumberOfDofsPerAxis,
        NumberOfUnknowns
      );
      validate(not stage.hasFrame(coarseFaceKey));

#if DIMENSIONS == 3
      for (int coarseJ = 0; coarseJ < NumberOfDofsPerAxis; coarseJ++) {
#else
      const int coarseJ = 0;
      {
#endif
        for (int coarseI = 0; coarseI < NumberOfDofsPerAxis; coarseI++) {
          const auto index = faceNodeIndex(normal, side, coarseI, coarseJ, NumberOfDofsPerAxis) * NumberOfUnknowns;
          validateNumericalEquals(coarseGridFaceValues[index], SubdivisionFactor * coarseI + CoveredNodeCentre);
#if DIMENSIONS == 3
          validateNumericalEquals(coarseGridFaceValues[index + 1], SubdivisionFactor * coarseJ + CoveredNodeCentre);
#endif
          // The opposite half belongs to the neighbouring cell and stays as the
          // caller left it.
          const auto oppositeIndex =
            faceNodeIndex(normal, 1 - side, coarseI, coarseJ, NumberOfDofsPerAxis) * NumberOfUnknowns;
          validateNumericalEquals(coarseGridFaceValues[oppositeIndex], -1.0);
        }
      }
    }
  }
}

/**
 * Verifies that staging one subface set forwards and backwards restricts to
 * bitwise identical halves.
 */
void exahype2::amr::tests::RestrictionTest::testGatheredHaloRestrictionIsStagingOrderInvariant() {
  constexpr int NumberOfUnknowns    = 1;
  constexpr int NumberOfDofsPerAxis = 2;
  constexpr int FaceSize            = faceValueCount(NumberOfDofsPerAxis, NumberOfUnknowns);
  constexpr int ValuesPerSubface    = DIMENSIONS == 2 ? NumberOfDofsPerAxis * NumberOfUnknowns
                                                      : NumberOfDofsPerAxis * NumberOfDofsPerAxis * NumberOfUnknowns;
  constexpr int Normal              = DIMENSIONS - 1;
  constexpr int Side                = 1;

  const auto restrictInStagingOrder = [&](const bool stageBackwards) {
    exahype2::amr::ChildDataStage<double> stage(NumberOfSubfaces, ValuesPerSubface);
    std::vector<double>                   coarseGridFaceValues(FaceSize, 0.0);
    const auto* const                     coarseFaceKey = coarseGridFaceValues.data();

    for (int subfaceNumber = 0; subfaceNumber < NumberOfSubfaces; subfaceNumber++) {
      const int   stagedSubface = stageBackwards ? NumberOfSubfaces - 1 - subfaceNumber : subfaceNumber;
      auto* const stagedValues  = stage.claim(coarseFaceKey, stagedSubface, Side);
      for (int value = 0; value < ValuesPerSubface; value++) {
        // Values of very different magnitude expose a plain running sum, whose
        // result would follow the order the addends arrive in.
        stagedValues[value] = std::pow(-8.0, stagedSubface - 4) * (value + 1);
      }
    }

    exahype2::amr::restrictHaloLayerFromStagedSubfaces(
      stage,
      coarseFaceKey,
      coarseGridFaceValues.data(),
      Normal,
      NumberOfDofsPerAxis,
      NumberOfUnknowns
    );
    return coarseGridFaceValues;
  };

  const auto forwardHalf  = restrictInStagingOrder(false);
  const auto backwardHalf = restrictInStagingOrder(true);
  for (int value = 0; value < FaceSize; value++) {
    validateEquals(bitPattern(backwardHalf[value]), bitPattern(forwardHalf[value]));
  }
}

/**
 * Verifies that an absent subface adds nothing to a coarse node and leaves the
 * scale of the restriction untouched.
 */
void exahype2::amr::tests::RestrictionTest::testGatheredHaloRestrictionScalesAPartialSubfaceSet() {
  constexpr int NumberOfUnknowns    = 1;
  constexpr int NumberOfDofsPerAxis = 2;
  constexpr int FaceSize            = faceValueCount(NumberOfDofsPerAxis, NumberOfUnknowns);
  constexpr int ValuesPerSubface    = DIMENSIONS == 2 ? NumberOfDofsPerAxis * NumberOfUnknowns
                                                      : NumberOfDofsPerAxis * NumberOfDofsPerAxis * NumberOfUnknowns;
  constexpr int Normal              = 0;
  constexpr int Side                = 0;
  constexpr int AbsentSubface       = 1;

  exahype2::amr::ChildDataStage<double> stage(NumberOfSubfaces, ValuesPerSubface);
  std::vector<double>                   coarseGridFaceValues(FaceSize, 0.0);
  const auto* const                     coarseFaceKey = coarseGridFaceValues.data();

  for (int subfaceNumber = 0; subfaceNumber < NumberOfSubfaces; subfaceNumber++) {
    if (subfaceNumber == AbsentSubface) {
      continue;
    }
    auto* const stagedValues = stage.claim(coarseFaceKey, subfaceNumber, Side);
    std::fill_n(stagedValues, ValuesPerSubface, 1.0);
  }
  validate(not stage.hasChild(coarseFaceKey, AbsentSubface));

  exahype2::amr::restrictHaloLayerFromStagedSubfaces(
    stage,
    coarseFaceKey,
    coarseGridFaceValues.data(),
    Normal,
    NumberOfDofsPerAxis,
    NumberOfUnknowns
  );

  // A coarse node adds one scaled contribution per staged fine node it covers, and
  // a fine node of the absent subface adds nothing.
  constexpr double RestrictionScale = 1.0 / NumberOfSubfaces;
  const auto       subfaceOf        = [](const int coarseNode, const int offset) {
    return (SubdivisionFactor * coarseNode + offset) / NumberOfDofsPerAxis;
  };
#if DIMENSIONS == 3
  for (int coarseJ = 0; coarseJ < NumberOfDofsPerAxis; coarseJ++) {
#else
  const int coarseJ = 0;
  {
#endif
    for (int coarseI = 0; coarseI < NumberOfDofsPerAxis; coarseI++) {
      double expected = 0.0;
#if DIMENSIONS == 3
      for (int offsetJ = 0; offsetJ < SubdivisionFactor; offsetJ++) {
#else
      const int offsetJ = 0;
      {
#endif
        for (int offsetI = 0; offsetI < SubdivisionFactor; offsetI++) {
          if (subfaceOf(coarseJ, offsetJ) * SubdivisionFactor + subfaceOf(coarseI, offsetI) != AbsentSubface) {
            expected += RestrictionScale;
          }
        }
      }
      const auto index = faceNodeIndex(Normal, Side, coarseI, coarseJ, NumberOfDofsPerAxis) * NumberOfUnknowns;
      validateNumericalEquals(coarseGridFaceValues[index], expected);
    }
  }
}

/**
 * Exercises the gathered path end to end for single-precision patch data.
 */
void exahype2::amr::tests::RestrictionTest::testGatheredHaloRestrictionRunsInSinglePrecision() {
  constexpr int NumberOfUnknowns    = 1;
  constexpr int NumberOfDofsPerAxis = 2;
  constexpr int FaceSize            = faceValueCount(NumberOfDofsPerAxis, NumberOfUnknowns);
  constexpr int ValuesPerSubface    = DIMENSIONS == 2 ? NumberOfDofsPerAxis * NumberOfUnknowns
                                                      : NumberOfDofsPerAxis * NumberOfDofsPerAxis * NumberOfUnknowns;
  constexpr int Normal              = 0;
  constexpr int Side                = 1;

  exahype2::amr::ChildDataStage<float> stage(NumberOfSubfaces, ValuesPerSubface);
  std::vector<float>                   coarseGridFaceValues(FaceSize, 0.0F);
  const auto* const                    coarseFaceKey = coarseGridFaceValues.data();

  for (int subfaceNumber = 0; subfaceNumber < NumberOfSubfaces; subfaceNumber++) {
    auto* const stagedValues = stage.claim(coarseFaceKey, subfaceNumber, Side);
    std::fill_n(stagedValues, ValuesPerSubface, 3.0F);
  }

  exahype2::amr::restrictHaloLayerFromStagedSubfaces(
    stage,
    coarseFaceKey,
    coarseGridFaceValues.data(),
    Normal,
    NumberOfDofsPerAxis,
    NumberOfUnknowns
  );

  // A constant field restricts to itself, whatever the scale and the precision.
#if DIMENSIONS == 3
  for (int coarseJ = 0; coarseJ < NumberOfDofsPerAxis; coarseJ++) {
#else
  const int coarseJ = 0;
  {
#endif
    for (int coarseI = 0; coarseI < NumberOfDofsPerAxis; coarseI++) {
      const auto index = faceNodeIndex(Normal, Side, coarseI, coarseJ, NumberOfDofsPerAxis) * NumberOfUnknowns;
      validateNumericalEquals(static_cast<double>(coarseGridFaceValues[index]), 3.0);
    }
  }
}

void exahype2::amr::tests::RestrictionTest::testApplyTensorProductOrderInvariant() {
  constexpr int NumberOfUnknowns = 2;

  int nodesPerCell = 1;
  for (int d = 0; d < DIMENSIONS; d++) {
    nodesPerCell *= NodesPerAxis;
  }

  std::vector<double> source(static_cast<std::size_t>(nodesPerCell) * NumberOfUnknowns);
  for (std::size_t i = 0; i < source.size(); i++) {
    source[i] = pseudoRandomValue(0, static_cast<int>(i), 1);
  }

  // The identity operator reproduces the source to the last bit.
  std::array<double, static_cast<std::size_t>(NodesPerAxis * NodesPerAxis)> identity{};
  for (int i = 0; i < NodesPerAxis; i++) {
    identity[i * NodesPerAxis + i] = 1.0;
  }
  const double* identityPerAxis[DIMENSIONS];
  for (int d = 0; d < DIMENSIONS; d++) {
    identityPerAxis[d] = identity.data();
  }
  std::vector<double> target(source.size(), -1.0);
  exahype2::amr::applyTensorProductOrderInvariant(
    target.data(),
    source.data(),
    identityPerAxis,
    NodesPerAxis,
    NodesPerAxis,
    NumberOfUnknowns
  );
  for (std::size_t i = 0; i < source.size(); i++) {
    validateEquals(bitPattern(target[i]), bitPattern(source[i]));
  }

  // With one matrix on every axis an axis swap of the source swaps the target
  // to the last bit.
  const auto    restriction = inputMajorSymmetricRestrictionMatrix();
  const double* matrixPerAxis[DIMENSIONS];
  for (int d = 0; d < DIMENSIONS; d++) {
    matrixPerAxis[d] = restriction.data() + static_cast<std::ptrdiff_t>(NodesPerAxis * NodesPerAxis);
  }
  std::vector<double> swappedSource(source.size());
  peano4::dForEach(NodesPerAxis, [&](const tarch::la::Vector<DIMENSIONS, int>& node) {
    auto swapped = node;
    swapped(0)   = node(1);
    swapped(1)   = node(0);
    for (int u = 0; u < NumberOfUnknowns; u++) {
      swappedSource[peano4::dLinearised(swapped, NodesPerAxis) * NumberOfUnknowns + u] =
        source[peano4::dLinearised(node, NodesPerAxis) * NumberOfUnknowns + u];
    }
  });
  std::vector<double> swappedTarget(source.size(), -1.0);
  exahype2::amr::applyTensorProductOrderInvariant(
    target.data(),
    source.data(),
    matrixPerAxis,
    NodesPerAxis,
    NodesPerAxis,
    NumberOfUnknowns
  );
  exahype2::amr::applyTensorProductOrderInvariant(
    swappedTarget.data(),
    swappedSource.data(),
    matrixPerAxis,
    NodesPerAxis,
    NodesPerAxis,
    NumberOfUnknowns
  );
  peano4::dForEach(NodesPerAxis, [&](const tarch::la::Vector<DIMENSIONS, int>& node) {
    auto swapped = node;
    swapped(0)   = node(1);
    swapped(1)   = node(0);
    for (int u = 0; u < NumberOfUnknowns; u++) {
      validateEquals(
        bitPattern(swappedTarget[peano4::dLinearised(swapped, NodesPerAxis) * NumberOfUnknowns + u]),
        bitPattern(target[peano4::dLinearised(node, NodesPerAxis) * NumberOfUnknowns + u])
      );
    }
  });
}

void exahype2::amr::tests::RestrictionTest::testInterpolateFaceFromCoarse() {
  constexpr int NumberOfUnknowns = 2;
  constexpr int Stride           = NodesPerAxis * NodesPerAxis;
#if DIMENSIONS == 2
  constexpr int NodesPerFace = NodesPerAxis;
#else
  constexpr int NodesPerFace = NodesPerAxis * NodesPerAxis;
#endif

  const auto prolongation = inputMajorSymmetricProlongationMatrix();

  // An affine tangential field lies in the order-one basis, so every subface
  // reproduces it at its own node positions up to roundoff.
  std::array<double, static_cast<std::size_t>(NodesPerFace * NumberOfUnknowns)> coarse{};
  for (int node = 0; node < NodesPerFace; node++) {
    for (int u = 0; u < NumberOfUnknowns; u++) {
#if DIMENSIONS == 2
      coarse[node * NumberOfUnknowns + u] = 1.0 + u + 2.0 * nodePosition(node);
#else
      coarse[node * NumberOfUnknowns + u] =
        1.0 + u + 2.0 * nodePosition(node % NodesPerAxis) + 3.0 * nodePosition(node / NodesPerAxis);
#endif
    }
  }

#if DIMENSIONS == 2
  for (int subface = 0; subface < SubdivisionFactor; subface++) {
    const double* prolongationMatrixPerAxis[1] = {prolongation.data() + static_cast<std::ptrdiff_t>(subface * Stride)};
    std::array<double, static_cast<std::size_t>(NodesPerFace * NumberOfUnknowns)> fine{};
    exahype2::amr::interpolateFaceFromCoarse(
      fine.data(),
      coarse.data(),
      prolongationMatrixPerAxis,
      NodesPerAxis,
      NumberOfUnknowns
    );
    for (int node = 0; node < NodesPerFace; node++) {
      const double finePosition = (nodePosition(node) + subface) / SubdivisionFactor;
      for (int u = 0; u < NumberOfUnknowns; u++) {
        validateNumericalEquals(fine[node * NumberOfUnknowns + u], 1.0 + u + 2.0 * finePosition);
      }
    }
  }

  // Mirroring the coarse half along the tangential axis interpolates the
  // opposite subface to the bitwise mirrored values.
  std::array<double, static_cast<std::size_t>(NodesPerFace * NumberOfUnknowns)> randomCoarse{};
  std::array<double, static_cast<std::size_t>(NodesPerFace * NumberOfUnknowns)> mirroredCoarse{};
  for (int node = 0; node < NodesPerFace; node++) {
    for (int u = 0; u < NumberOfUnknowns; u++) {
      randomCoarse[node * NumberOfUnknowns + u]                        = pseudoRandomValue(0, node, u);
      mirroredCoarse[(NodesPerFace - 1 - node) * NumberOfUnknowns + u] = pseudoRandomValue(0, node, u);
    }
  }
  for (int subface = 0; subface < SubdivisionFactor; subface++) {
    const double* prolongationMatrixPerAxis[1] = {prolongation.data() + static_cast<std::ptrdiff_t>(subface * Stride)};
    const double* mirroredMatrixPerAxis[1]     = {
      prolongation.data() + static_cast<std::ptrdiff_t>((SubdivisionFactor - 1 - subface) * Stride)
    };
    std::array<double, static_cast<std::size_t>(NodesPerFace * NumberOfUnknowns)> fine{};
    std::array<double, static_cast<std::size_t>(NodesPerFace * NumberOfUnknowns)> mirroredFine{};
    exahype2::amr::interpolateFaceFromCoarse(
      fine.data(),
      randomCoarse.data(),
      prolongationMatrixPerAxis,
      NodesPerAxis,
      NumberOfUnknowns
    );
    exahype2::amr::interpolateFaceFromCoarse(
      mirroredFine.data(),
      mirroredCoarse.data(),
      mirroredMatrixPerAxis,
      NodesPerAxis,
      NumberOfUnknowns
    );
    for (int node = 0; node < NodesPerFace; node++) {
      for (int u = 0; u < NumberOfUnknowns; u++) {
        validateEquals(
          bitPattern(mirroredFine[(NodesPerFace - 1 - node) * NumberOfUnknowns + u]),
          bitPattern(fine[node * NumberOfUnknowns + u])
        );
      }
    }
  }
#else
  for (int subfaceOuter = 0; subfaceOuter < SubdivisionFactor; subfaceOuter++) {
    for (int subfaceInner = 0; subfaceInner < SubdivisionFactor; subfaceInner++) {
      const double* subfaceMatrixPerAxis[2] = {
        prolongation.data() + static_cast<std::ptrdiff_t>(subfaceInner * Stride),
        prolongation.data() + static_cast<std::ptrdiff_t>(subfaceOuter * Stride)
      };
      std::array<double, static_cast<std::size_t>(NodesPerFace * NumberOfUnknowns)> subface{};
      exahype2::amr::interpolateFaceFromCoarse(
        subface.data(),
        coarse.data(),
        subfaceMatrixPerAxis,
        NodesPerAxis,
        NumberOfUnknowns
      );
      for (int fineOuter = 0; fineOuter < NodesPerAxis; fineOuter++) {
        for (int fineInner = 0; fineInner < NodesPerAxis; fineInner++) {
          const double innerPosition = (nodePosition(fineInner) + subfaceInner) / SubdivisionFactor;
          const double outerPosition = (nodePosition(fineOuter) + subfaceOuter) / SubdivisionFactor;
          for (int u = 0; u < NumberOfUnknowns; u++) {
            validateNumericalEquals(
              subface[(fineOuter * NodesPerAxis + fineInner) * NumberOfUnknowns + u],
              1.0 + u + 2.0 * innerPosition + 3.0 * outerPosition
            );
          }
        }
      }
    }
  }

  // Swapping the tangential axes of the coarse half and the per-axis operators
  // interpolates the transposed subface to the bitwise transposed values.
  const double* prolongationMatrixPerAxis[2] = {prolongation.data(), prolongation.data() + Stride};
  std::array<double, static_cast<std::size_t>(NodesPerFace * NumberOfUnknowns)> randomCoarse{};
  std::array<double, static_cast<std::size_t>(NodesPerFace * NumberOfUnknowns)> transposedCoarse{};
  for (int outer = 0; outer < NodesPerAxis; outer++) {
    for (int inner = 0; inner < NodesPerAxis; inner++) {
      for (int u = 0; u < NumberOfUnknowns; u++) {
        randomCoarse[(outer * NodesPerAxis + inner) * NumberOfUnknowns + u] =
          pseudoRandomValue(1, outer * NodesPerAxis + inner, u);
        transposedCoarse[(inner * NodesPerAxis + outer) * NumberOfUnknowns + u] =
          pseudoRandomValue(1, outer * NodesPerAxis + inner, u);
      }
    }
  }
  const double* swappedMatrixPerAxis[2] = {prolongation.data() + Stride, prolongation.data()};
  std::array<double, static_cast<std::size_t>(NodesPerFace * NumberOfUnknowns)> randomFine{};
  std::array<double, static_cast<std::size_t>(NodesPerFace * NumberOfUnknowns)> transposedFine{};
  exahype2::amr::interpolateFaceFromCoarse(
    randomFine.data(),
    randomCoarse.data(),
    prolongationMatrixPerAxis,
    NodesPerAxis,
    NumberOfUnknowns
  );
  exahype2::amr::interpolateFaceFromCoarse(
    transposedFine.data(),
    transposedCoarse.data(),
    swappedMatrixPerAxis,
    NodesPerAxis,
    NumberOfUnknowns
  );
  for (int fineOuter = 0; fineOuter < NodesPerAxis; fineOuter++) {
    for (int fineInner = 0; fineInner < NodesPerAxis; fineInner++) {
      for (int u = 0; u < NumberOfUnknowns; u++) {
        validateEquals(
          bitPattern(transposedFine[(fineInner * NodesPerAxis + fineOuter) * NumberOfUnknowns + u]),
          bitPattern(randomFine[(fineOuter * NodesPerAxis + fineInner) * NumberOfUnknowns + u])
        );
      }
    }
  }
#endif
}

void exahype2::amr::tests::RestrictionTest::testRestrictFaceFromChildren() {
  constexpr int NumberOfUnknowns = 2;
#if DIMENSIONS == 2
  constexpr int NodesPerFace = NodesPerAxis;
#else
  constexpr int NodesPerFace = NodesPerAxis * NodesPerAxis;
#endif

  const auto                                   restriction = inputMajorSymmetricRestrictionMatrix();
  std::array<const double*, SubdivisionFactor> restrictionMatrixPerInterval{};
  for (int interval = 0; interval < SubdivisionFactor; interval++) {
    restrictionMatrixPerInterval[interval] =
      restriction.data() + static_cast<std::ptrdiff_t>(interval) * NodesPerAxis * NodesPerAxis;
  }

  std::vector<std::vector<double>> subfaces(NumberOfSubfaces);
  std::vector<const double*>       subfacePointers(NumberOfSubfaces);
  for (int subface = 0; subface < NumberOfSubfaces; subface++) {
    subfaces[subface].resize(static_cast<std::size_t>(NodesPerFace) * NumberOfUnknowns);
    for (int node = 0; node < NodesPerFace; node++) {
      for (int u = 0; u < NumberOfUnknowns; u++) {
        subfaces[subface][node * NumberOfUnknowns + u] = pseudoRandomValue(subface, node, u);
      }
    }
    subfacePointers[subface] = subfaces[subface].data();
  }

  // An absent subface contributes exactly what a zero-valued one contributes.
  std::vector<double> zeroValues(static_cast<std::size_t>(NodesPerFace) * NumberOfUnknowns, 0.0);
  std::vector<double> coarseWithZeroChild(static_cast<std::size_t>(NodesPerFace) * NumberOfUnknowns, -1.0);
  std::vector<double> coarseWithAbsentChild(static_cast<std::size_t>(NodesPerFace) * NumberOfUnknowns, -1.0);
  auto                pointers = subfacePointers;
  pointers[1]                  = zeroValues.data();
  exahype2::amr::restrictFaceFromChildren(
    coarseWithZeroChild.data(),
    pointers.data(),
    restrictionMatrixPerInterval.data(),
    NodesPerAxis,
    NumberOfUnknowns
  );
  pointers[1] = nullptr;
  exahype2::amr::restrictFaceFromChildren(
    coarseWithAbsentChild.data(),
    pointers.data(),
    restrictionMatrixPerInterval.data(),
    NodesPerAxis,
    NumberOfUnknowns
  );
  for (std::size_t i = 0; i < coarseWithZeroChild.size(); i++) {
    validateEquals(bitPattern(coarseWithAbsentChild[i]), bitPattern(coarseWithZeroChild[i]));
  }

  // Reversing the subfaces and the nodes along every tangential axis mirrors the
  // coarse half to the last bit.
  std::vector<std::vector<double>> mirroredSubfaces(NumberOfSubfaces);
  std::vector<const double*>       mirroredPointers(NumberOfSubfaces);
  for (int subface = 0; subface < NumberOfSubfaces; subface++) {
    mirroredSubfaces[subface].resize(static_cast<std::size_t>(NodesPerFace) * NumberOfUnknowns);
  }
  for (int subface = 0; subface < NumberOfSubfaces; subface++) {
    for (int node = 0; node < NodesPerFace; node++) {
      for (int u = 0; u < NumberOfUnknowns; u++) {
        mirroredSubfaces[NumberOfSubfaces - 1 - subface][(NodesPerFace - 1 - node) * NumberOfUnknowns + u] =
          subfaces[subface][node * NumberOfUnknowns + u];
      }
    }
    mirroredPointers[subface] = mirroredSubfaces[subface].data();
  }
  std::vector<double> coarse(static_cast<std::size_t>(NodesPerFace) * NumberOfUnknowns, -1.0);
  std::vector<double> mirroredCoarse(static_cast<std::size_t>(NodesPerFace) * NumberOfUnknowns, -1.0);
  exahype2::amr::restrictFaceFromChildren(
    coarse.data(),
    subfacePointers.data(),
    restrictionMatrixPerInterval.data(),
    NodesPerAxis,
    NumberOfUnknowns
  );
  exahype2::amr::restrictFaceFromChildren(
    mirroredCoarse.data(),
    mirroredPointers.data(),
    restrictionMatrixPerInterval.data(),
    NodesPerAxis,
    NumberOfUnknowns
  );
  for (int node = 0; node < NodesPerFace; node++) {
    for (int u = 0; u < NumberOfUnknowns; u++) {
      validateEquals(
        bitPattern(mirroredCoarse[(NodesPerFace - 1 - node) * NumberOfUnknowns + u]),
        bitPattern(coarse[node * NumberOfUnknowns + u])
      );
    }
  }
}

void exahype2::amr::tests::RestrictionTest::testRestrictCellWithMatrixReproducesPolynomials() {
  constexpr int NumberOfUnknowns = 2;

  int nodesPerCell = 1;
  for (int d = 0; d < DIMENSIONS; d++) {
    nodesPerCell *= NodesPerAxis;
  }

  const auto interpolation = interpolationMatrix();
  const auto restriction   = restrictionMatrix();

  std::vector<double> coarse(static_cast<std::size_t>(nodesPerCell) * NumberOfUnknowns, 0.0);
  peano4::dForEach(NodesPerAxis, [&](const tarch::la::Vector<DIMENSIONS, int>& node) {
    tarch::la::Vector<DIMENSIONS, double> x;
    for (int d = 0; d < DIMENSIONS; d++) {
      x(d) = nodePosition(node(d));
    }
    const int index   = peano4::dLinearised(node, NodesPerAxis) * NumberOfUnknowns;
    coarse[index]     = affineField(x);
    coarse[index + 1] = 4.0;
  });

  // The interpolate-then-restrict round trip over all children is the identity for every
  // field the coarse basis represents.
  peano4::grid::GridTraversalEvent event;
  std::vector<double>              restricted(static_cast<std::size_t>(nodesPerCell) * NumberOfUnknowns, 0.0);
  peano4::dForEach(SubdivisionFactor, [&](const tarch::la::Vector<DIMENSIONS, int>& child) {
    event.setRelativePositionToFather(child);
    const auto cellMarker = event.getCellMarker();

    std::vector<double> fine(static_cast<std::size_t>(nodesPerCell) * NumberOfUnknowns, 0.0);
    exahype2::amr::interpolateCell(
      cellMarker,
      coarse.data(),
      fine.data(),
      interpolation.data(),
      NodesPerAxis,
      NumberOfUnknowns
    );

    peano4::dForEach(NodesPerAxis, [&](const tarch::la::Vector<DIMENSIONS, int>& node) {
      tarch::la::Vector<DIMENSIONS, double> x;
      for (int d = 0; d < DIMENSIONS; d++) {
        x(d) = (nodePosition(node(d)) + child(d)) / SubdivisionFactor;
      }
      const int index = peano4::dLinearised(node, NodesPerAxis) * NumberOfUnknowns;
      validateNumericalEqualsWithEps(fine[index], affineField(x), 1e-12);
      validateNumericalEqualsWithEps(fine[index + 1], 4.0, 1e-12);
    });

    exahype2::amr::restrictCell(
      cellMarker,
      restricted.data(),
      fine.data(),
      restriction.data(),
      QuadratureWeights.data(),
      NodesPerAxis,
      NumberOfUnknowns
    );
  });

  for (std::size_t i = 0; i < coarse.size(); i++) {
    validateNumericalEqualsWithEps(restricted[i], coarse[i], 1e-12);
  }
}

void exahype2::amr::tests::RestrictionTest::testRestrictHaloLayer() {
  // NumberOfUnknowns == DIMENSIONS lets each axis carry a distinct linear ramp.
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
      // The averaging restriction accumulates onto the coarse face, which starts at zero.
      std::fill_n(coarseGridFaceValues, FaceSize, 0.);

      peano4::grid::GridTraversalEvent event;
      peano4::dForEach(SubdivisionFactor, [&](const tarch::la::Vector<DIMENSIONS, int>& subCellIndex) {
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

          exahype2::amr::restrictHaloLayer(
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
            const double expectedValue =
              (u == normal) ? side : SubdivisionFactor * volume[u] + 0.5 * (SubdivisionFactor - 1);
            validateNumericalEquals(coarseGridFaceValues[index + u], expectedValue);
          }
        }
      );
    }
  }
}

void exahype2::amr::tests::RestrictionTest::testRestrictCell() {
  // NumberOfUnknowns == DIMENSIONS lets each axis carry a distinct linear ramp.
  constexpr int NumberOfUnknowns    = DIMENSIONS;
  constexpr int NumberOfDofsPerAxis = 7;

#if DIMENSIONS == 2
  constexpr int CellSize = NumberOfUnknowns * NumberOfDofsPerAxis * NumberOfDofsPerAxis;
#else
  constexpr int CellSize = NumberOfUnknowns * NumberOfDofsPerAxis * NumberOfDofsPerAxis * NumberOfDofsPerAxis;
#endif
  double coarseGridCellValues[CellSize] = {0.};
  double fineGridCellValues[CellSize]   = {0.};

  peano4::grid::GridTraversalEvent event;
  peano4::dForEach(SubdivisionFactor, [&](const tarch::la::Vector<DIMENSIONS, int>& subCellIndex) {
    event.setRelativePositionToFather(subCellIndex);
    const auto cellMarker = event.getCellMarker();

    peano4::dForEach(NumberOfDofsPerAxis, [&](const tarch::la::Vector<DIMENSIONS, int>& volume) {
      const auto index = NumberOfUnknowns * peano4::dLinearised(volume, NumberOfDofsPerAxis);
      for (int u = 0; u < NumberOfUnknowns; u++) {
        fineGridCellValues[index + u] = volume[u] + NumberOfDofsPerAxis * subCellIndex[u];
      }
    });

    exahype2::amr::restrictCell(
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
      validateNumericalEquals(
        coarseGridCellValues[index + u],
        SubdivisionFactor * volume[u] + 0.5 * (SubdivisionFactor - 1)
      );
    }
  });
}
