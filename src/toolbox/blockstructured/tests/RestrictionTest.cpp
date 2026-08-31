// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "RestrictionTest.h"

#include "peano4/Loop.h"
#include "peano4/grid/GridTraversalEvent.h"
#include "toolbox/blockstructured/AMRRoutines.h"

namespace {
  // A two-point Gauss-Legendre basis on the unit interval, the matrices a solver generates
  // for polynomial order one.
  constexpr int    NodesPerAxis = 2;
  constexpr double Node0        = 0.5 - 0.5 / 1.7320508075688772;
  constexpr double Node1        = 0.5 + 0.5 / 1.7320508075688772;

  double shapeFunction(const int node, const double x) {
    return node == 0 ? (Node1 - x) / (Node1 - Node0) : (x - Node0) / (Node1 - Node0);
  }

  std::array<double, 3 * NodesPerAxis * NodesPerAxis> interpolationMatrix() {
    std::array<double, 3 * NodesPerAxis * NodesPerAxis> result{};
    for (int block = 0; block < 3; block++) {
      for (int fine = 0; fine < NodesPerAxis; fine++) {
        const double fineNode = ((fine == 0 ? Node0 : Node1) + block) / 3.0;
        for (int coarse = 0; coarse < NodesPerAxis; coarse++) {
          result[block * NodesPerAxis * NodesPerAxis + fine * NodesPerAxis + coarse] = shapeFunction(coarse, fineNode);
        }
      }
    }
    return result;
  }

  std::array<double, 3 * NodesPerAxis * NodesPerAxis> restrictionMatrix() {
    std::array<double, 3 * NodesPerAxis * NodesPerAxis> result{};
    for (int block = 0; block < 3; block++) {
      for (int coarse = 0; coarse < NodesPerAxis; coarse++) {
        for (int fine = 0; fine < NodesPerAxis; fine++) {
          const double fineNode = ((fine == 0 ? Node0 : Node1) + block) / 3.0;
          result[block * NodesPerAxis * NodesPerAxis + coarse * NodesPerAxis + fine] =
            shapeFunction(coarse, fineNode) / 3.0;
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
  // last bit, so the upper half is a copy of the lower half.
  std::array<double, 3 * NodesPerAxis * NodesPerAxis> inputMajorSymmetricRestrictionMatrix() {
    std::array<double, 3 * NodesPerAxis * NodesPerAxis> result{};
    constexpr int                                       Stride = NodesPerAxis * NodesPerAxis;
    for (int block = 0; block < 2; block++) {
      for (int fine = 0; fine < NodesPerAxis; fine++) {
        const double fineNode = ((fine == 0 ? Node0 : Node1) + block) / 3.0;
        for (int coarse = 0; coarse < NodesPerAxis; coarse++) {
          result[block * Stride + fine * NodesPerAxis + coarse] = shapeFunction(coarse, fineNode) / 3.0;
        }
      }
    }
    for (int block = 0; block < 2; block++) {
      for (int fine = 0; fine < NodesPerAxis; fine++) {
        for (int coarse = 0; coarse < NodesPerAxis; coarse++) {
          result[(2 - block) * Stride + (NodesPerAxis - 1 - fine) * NodesPerAxis + (NodesPerAxis - 1 - coarse)] =
            result[block * Stride + fine * NodesPerAxis + coarse];
        }
      }
    }
    return result;
  }

  // The prolongation counterpart of the matrix above, input-major with entry
  // [coarse * NodesPerAxis + fine] and mirror symmetric to the last bit.
  std::array<double, 3 * NodesPerAxis * NodesPerAxis> inputMajorSymmetricProlongationMatrix() {
    std::array<double, 3 * NodesPerAxis * NodesPerAxis> result{};
    constexpr int                                       Stride = NodesPerAxis * NodesPerAxis;
    for (int block = 0; block < 2; block++) {
      for (int fine = 0; fine < NodesPerAxis; fine++) {
        const double fineNode = ((fine == 0 ? Node0 : Node1) + block) / 3.0;
        for (int coarse = 0; coarse < NodesPerAxis; coarse++) {
          result[block * Stride + coarse * NodesPerAxis + fine] = shapeFunction(coarse, fineNode);
        }
      }
    }
    for (int block = 0; block < 2; block++) {
      for (int fine = 0; fine < NodesPerAxis; fine++) {
        for (int coarse = 0; coarse < NodesPerAxis; coarse++) {
          result[(2 - block) * Stride + (NodesPerAxis - 1 - coarse) * NodesPerAxis + (NodesPerAxis - 1 - fine)] =
            result[block * Stride + coarse * NodesPerAxis + fine];
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

  constexpr int NumberOfSubfaces = DIMENSIONS == 2 ? 3 : 9;

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
    const int iStride    = normal == 0 ? 2 : 1;
    const int jStride    = normal == 2 ? numberOfDofsPerAxis : 2 * numberOfDofsPerAxis;
    const int sideStride = normal == 0 ? 1
      : normal == 1                    ? numberOfDofsPerAxis
                                       : numberOfDofsPerAxis * numberOfDofsPerAxis;
    return side * sideStride + tangentialI * iStride + tangentialJ * jStride;
  }

  /** Returns the number of values in both halves of a face array. */
  constexpr int faceValueCount(const int numberOfDofsPerAxis, const int numberOfUnknowns) {
    return DIMENSIONS == 2 ? 2 * numberOfDofsPerAxis * numberOfUnknowns
                           : 2 * numberOfDofsPerAxis * numberOfDofsPerAxis * numberOfUnknowns;
  }
} // namespace

toolbox::blockstructured::tests::RestrictionTest::RestrictionTest() :
  TestCase("toolbox::blockstructured::tests::RestrictionTest") {}

void toolbox::blockstructured::tests::RestrictionTest::run() {
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

void toolbox::blockstructured::tests::RestrictionTest::testOrderInvariantProduct() {
  const double factors[3]         = {0.7, -1.3e-5, 42.0};
  const int    permutations[6][3] = {
    {0, 1, 2},
    {0, 2, 1},
    {1, 0, 2},
    {1, 2, 0},
    {2, 0, 1},
    {2, 1, 0},
  };
  const auto reference = toolbox::blockstructured::orderInvariantProduct(factors[0], factors[1], factors[2]);
  for (const auto& permutation : permutations) {
    validateEquals(
      bitPattern(
        toolbox::blockstructured::orderInvariantProduct(
          factors[permutation[0]],
          factors[permutation[1]],
          factors[permutation[2]]
        )
      ),
      bitPattern(reference)
    );
  }
  validateNumericalEquals(toolbox::blockstructured::orderInvariantProduct(2.0, 3.0, 4.0), 24.0);
}

void toolbox::blockstructured::tests::RestrictionTest::testRestrictCellFromChildrenReproducesRamp() {
  // Unknowns == DIMENSIONS lets each axis carry a distinct linear ramp.
  constexpr int NumberOfUnknowns    = DIMENSIONS;
  constexpr int NumberOfDofsPerAxis = 7;

  int volumesPerCell = 1;
  for (int d = 0; d < DIMENSIONS; d++) {
    volumesPerCell *= NumberOfDofsPerAxis;
  }

  std::vector<std::vector<double>> children(THREE_POWER_D);
  std::vector<const double*>       childPointers(THREE_POWER_D);
  peano4::dForEach(3, [&](const tarch::la::Vector<DIMENSIONS, int>& child) {
    auto& values = children[peano4::dLinearised(child, 3)];
    values.resize(static_cast<std::size_t>(volumesPerCell) * NumberOfUnknowns);
    peano4::dForEach(NumberOfDofsPerAxis, [&](const tarch::la::Vector<DIMENSIONS, int>& volume) {
      const auto index = NumberOfUnknowns * peano4::dLinearised(volume, NumberOfDofsPerAxis);
      for (int u = 0; u < NumberOfUnknowns; u++) {
        values[index + u] = volume[u] + NumberOfDofsPerAxis * child[u];
      }
    });
    childPointers[peano4::dLinearised(child, 3)] = values.data();
  });

  std::vector<double> coarse(static_cast<std::size_t>(volumesPerCell) * NumberOfUnknowns, -1.0);
  toolbox::blockstructured::restrictCellFromChildren(
    coarse.data(),
    childPointers.data(),
    NumberOfDofsPerAxis,
    NumberOfUnknowns
  );

  // Averaging the three fine volumes of a linear ramp per axis yields the middle
  // fine value, which is an integer and therefore exact.
  peano4::dForEach(NumberOfDofsPerAxis, [&](const tarch::la::Vector<DIMENSIONS, int>& volume) {
    const auto index = NumberOfUnknowns * peano4::dLinearised(volume, NumberOfDofsPerAxis);
    for (int u = 0; u < NumberOfUnknowns; u++) {
      validateNumericalEquals(coarse[index + u], 3.0 * volume[u] + 1.0);
    }
  });
}

void toolbox::blockstructured::tests::RestrictionTest::testRestrictCellFromChildrenWithMatrixReproducesPolynomials() {
  constexpr int Unknowns = 2;

  int nodesPerCell = 1;
  for (int d = 0; d < DIMENSIONS; d++) {
    nodesPerCell *= NodesPerAxis;
  }

  const auto    interpolation                   = interpolationMatrix();
  const auto    restriction                     = inputMajorSymmetricRestrictionMatrix();
  const double* restrictionMatrixPerInterval[3] = {
    restriction.data(),
    restriction.data() + NodesPerAxis * NodesPerAxis,
    restriction.data() + 2 * NodesPerAxis * NodesPerAxis,
  };

  std::vector<double> coarse(static_cast<std::size_t>(nodesPerCell) * Unknowns, 0.0);
  peano4::dForEach(NodesPerAxis, [&](const tarch::la::Vector<DIMENSIONS, int>& node) {
    tarch::la::Vector<DIMENSIONS, double> x;
    for (int d = 0; d < DIMENSIONS; d++) {
      x(d) = nodePosition(node(d));
    }
    const int index   = peano4::dLinearised(node, NodesPerAxis) * Unknowns;
    coarse[index]     = affineField(x);
    coarse[index + 1] = 4.0;
  });

  peano4::grid::GridTraversalEvent event;
  std::vector<std::vector<double>> children(THREE_POWER_D);
  std::vector<const double*>       childPointers(THREE_POWER_D);
  peano4::dForEach(3, [&](const tarch::la::Vector<DIMENSIONS, int>& child) {
    event.setRelativePositionToFather(child);
    auto& fine = children[peano4::dLinearised(child, 3)];
    fine.resize(static_cast<std::size_t>(nodesPerCell) * Unknowns);
    toolbox::blockstructured::interpolateCell(
      event.getCellMarker(),
      coarse.data(),
      fine.data(),
      interpolation.data(),
      NodesPerAxis,
      Unknowns
    );
    childPointers[peano4::dLinearised(child, 3)] = fine.data();
  });

  std::vector<double> restricted(static_cast<std::size_t>(nodesPerCell) * Unknowns, 0.0);
  toolbox::blockstructured::restrictCellFromChildren(
    restricted.data(),
    childPointers.data(),
    restrictionMatrixPerInterval,
    NodesPerAxis,
    Unknowns
  );

  for (std::size_t i = 0; i < coarse.size(); i++) {
    validateNumericalEqualsWithEps(restricted[i], coarse[i], 1e-12);
  }
}

void toolbox::blockstructured::tests::RestrictionTest::testRestrictCellFromChildrenIsSymmetryEquivariant() {
  constexpr int Unknowns = 2;

  int nodesPerCell = 1;
  for (int d = 0; d < DIMENSIONS; d++) {
    nodesPerCell *= NodesPerAxis;
  }

  const auto    restriction                     = inputMajorSymmetricRestrictionMatrix();
  const double* restrictionMatrixPerInterval[3] = {
    restriction.data(),
    restriction.data() + NodesPerAxis * NodesPerAxis,
    restriction.data() + 2 * NodesPerAxis * NodesPerAxis,
  };

  const auto reflectAxis0 = [](const tarch::la::Vector<DIMENSIONS, int>& index, const int range) {
    auto result = index;
    result(0)   = range - 1 - index(0);
    return result;
  };
  const auto swapAxes01 = [](const tarch::la::Vector<DIMENSIONS, int>& index, const int) {
    auto result = index;
    result(0)   = index(1);
    result(1)   = index(0);
    return result;
  };
  using Transform = tarch::la::Vector<DIMENSIONS, int> (*)(const tarch::la::Vector<DIMENSIONS, int>&, const int);
  const Transform transforms[2] = {reflectAxis0, swapAxes01};

  for (const auto transform : transforms) {
    std::vector<std::vector<double>> children(THREE_POWER_D);
    std::vector<std::vector<double>> mirroredChildren(THREE_POWER_D);
    std::vector<const double*>       childPointers(THREE_POWER_D);
    std::vector<const double*>       mirroredChildPointers(THREE_POWER_D);
    for (int child = 0; child < THREE_POWER_D; child++) {
      children[child].resize(static_cast<std::size_t>(nodesPerCell) * Unknowns);
      mirroredChildren[child].resize(static_cast<std::size_t>(nodesPerCell) * Unknowns);
    }
    peano4::dForEach(3, [&](const tarch::la::Vector<DIMENSIONS, int>& child) {
      peano4::dForEach(NodesPerAxis, [&](const tarch::la::Vector<DIMENSIONS, int>& node) {
        const auto sourceIndex = peano4::dLinearised(node, NodesPerAxis) * Unknowns;
        const auto targetIndex = peano4::dLinearised(transform(node, NodesPerAxis), NodesPerAxis) * Unknowns;
        for (int u = 0; u < Unknowns; u++) {
          const auto value =
            pseudoRandomValue(peano4::dLinearised(child, 3), peano4::dLinearised(node, NodesPerAxis), u);
          children[peano4::dLinearised(child, 3)][sourceIndex + u]                       = value;
          mirroredChildren[peano4::dLinearised(transform(child, 3), 3)][targetIndex + u] = value;
        }
      });
    });
    for (int child = 0; child < THREE_POWER_D; child++) {
      childPointers[child]         = children[child].data();
      mirroredChildPointers[child] = mirroredChildren[child].data();
    }

    // The averaging and the mass-lumped restriction both have to map mirrored
    // children onto the bitwise mirrored coarse cell.
    std::vector<double> coarse(static_cast<std::size_t>(nodesPerCell) * Unknowns, 0.0);
    std::vector<double> mirroredCoarse(static_cast<std::size_t>(nodesPerCell) * Unknowns, 0.0);
    toolbox::blockstructured::restrictCellFromChildren(coarse.data(), childPointers.data(), NodesPerAxis, Unknowns);
    toolbox::blockstructured::restrictCellFromChildren(
      mirroredCoarse.data(),
      mirroredChildPointers.data(),
      NodesPerAxis,
      Unknowns
    );
    peano4::dForEach(NodesPerAxis, [&](const tarch::la::Vector<DIMENSIONS, int>& node) {
      const auto sourceIndex = peano4::dLinearised(node, NodesPerAxis) * Unknowns;
      const auto targetIndex = peano4::dLinearised(transform(node, NodesPerAxis), NodesPerAxis) * Unknowns;
      for (int u = 0; u < Unknowns; u++) {
        validateEquals(bitPattern(mirroredCoarse[targetIndex + u]), bitPattern(coarse[sourceIndex + u]));
      }
    });

    std::fill(coarse.begin(), coarse.end(), 0.0);
    std::fill(mirroredCoarse.begin(), mirroredCoarse.end(), 0.0);
    toolbox::blockstructured::restrictCellFromChildren(
      coarse.data(),
      childPointers.data(),
      restrictionMatrixPerInterval,
      NodesPerAxis,
      Unknowns
    );
    toolbox::blockstructured::restrictCellFromChildren(
      mirroredCoarse.data(),
      mirroredChildPointers.data(),
      restrictionMatrixPerInterval,
      NodesPerAxis,
      Unknowns
    );
    peano4::dForEach(NodesPerAxis, [&](const tarch::la::Vector<DIMENSIONS, int>& node) {
      const auto sourceIndex = peano4::dLinearised(node, NodesPerAxis) * Unknowns;
      const auto targetIndex = peano4::dLinearised(transform(node, NodesPerAxis), NodesPerAxis) * Unknowns;
      for (int u = 0; u < Unknowns; u++) {
        validateEquals(bitPattern(mirroredCoarse[targetIndex + u]), bitPattern(coarse[sourceIndex + u]));
      }
    });
  }
}

/**
 * Verifies that a coarse node of the gathered restriction comes out as the mean
 * of the three fine nodes it covers, on both halves and every normal.
 */
void toolbox::blockstructured::tests::RestrictionTest::testGatheredHaloRestrictionAveragesTheSubfaceRamp() {
  constexpr int NumberOfUnknowns    = 2;
  constexpr int NumberOfDofsPerAxis = 2;
  constexpr int FaceSize            = faceValueCount(NumberOfDofsPerAxis, NumberOfUnknowns);
  constexpr int ValuesPerSubface    = DIMENSIONS == 2 ? NumberOfDofsPerAxis * NumberOfUnknowns
                                                      : NumberOfDofsPerAxis * NumberOfDofsPerAxis * NumberOfUnknowns;

  for (int normal = 0; normal < DIMENSIONS; normal++) {
    for (int side = 0; side < 2; side++) {
      toolbox::blockstructured::ChildDataStage<double> stage(NumberOfSubfaces, ValuesPerSubface);
      std::vector<double>                              coarseGridFaceValues(FaceSize, -1.0);
      const auto* const                                coarseFaceKey = coarseGridFaceValues.data();

      peano4::grid::GridTraversalEvent event;
      peano4::dForEach(3, [&](const tarch::la::Vector<DIMENSIONS, int>& subCellIndex) {
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

        toolbox::blockstructured::stageHaloLayerForGatheredRestriction(
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
      toolbox::blockstructured::restrictHaloLayerFromStagedSubfaces(
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
          validateNumericalEquals(coarseGridFaceValues[index], 3.0 * coarseI + 1.0);
#if DIMENSIONS == 3
          validateNumericalEquals(coarseGridFaceValues[index + 1], 3.0 * coarseJ + 1.0);
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
void toolbox::blockstructured::tests::RestrictionTest::testGatheredHaloRestrictionIsStagingOrderInvariant() {
  constexpr int NumberOfUnknowns    = 1;
  constexpr int NumberOfDofsPerAxis = 2;
  constexpr int FaceSize            = faceValueCount(NumberOfDofsPerAxis, NumberOfUnknowns);
  constexpr int ValuesPerSubface    = DIMENSIONS == 2 ? NumberOfDofsPerAxis * NumberOfUnknowns
                                                      : NumberOfDofsPerAxis * NumberOfDofsPerAxis * NumberOfUnknowns;
  constexpr int Normal              = DIMENSIONS - 1;
  constexpr int Side                = 1;

  const auto restrictInStagingOrder = [&](const bool stageBackwards) {
    toolbox::blockstructured::ChildDataStage<double> stage(NumberOfSubfaces, ValuesPerSubface);
    std::vector<double>                              coarseGridFaceValues(FaceSize, 0.0);
    const auto* const                                coarseFaceKey = coarseGridFaceValues.data();

    for (int subfaceNumber = 0; subfaceNumber < NumberOfSubfaces; subfaceNumber++) {
      const int   stagedSubface = stageBackwards ? NumberOfSubfaces - 1 - subfaceNumber : subfaceNumber;
      auto* const stagedValues  = stage.claim(coarseFaceKey, stagedSubface, Side);
      for (int value = 0; value < ValuesPerSubface; value++) {
        // Values of very different magnitude expose a plain running sum, whose
        // result would follow the order the addends arrive in.
        stagedValues[value] = std::pow(-8.0, stagedSubface - 4) * (value + 1);
      }
    }

    toolbox::blockstructured::restrictHaloLayerFromStagedSubfaces(
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
void toolbox::blockstructured::tests::RestrictionTest::testGatheredHaloRestrictionScalesAPartialSubfaceSet() {
  constexpr int NumberOfUnknowns    = 1;
  constexpr int NumberOfDofsPerAxis = 2;
  constexpr int FaceSize            = faceValueCount(NumberOfDofsPerAxis, NumberOfUnknowns);
  constexpr int ValuesPerSubface    = DIMENSIONS == 2 ? NumberOfDofsPerAxis * NumberOfUnknowns
                                                      : NumberOfDofsPerAxis * NumberOfDofsPerAxis * NumberOfUnknowns;
  constexpr int Normal              = 0;
  constexpr int Side                = 0;
  constexpr int AbsentSubface       = 1;

  toolbox::blockstructured::ChildDataStage<double> stage(NumberOfSubfaces, ValuesPerSubface);
  std::vector<double>                              coarseGridFaceValues(FaceSize, 0.0);
  const auto* const                                coarseFaceKey = coarseGridFaceValues.data();

  for (int subfaceNumber = 0; subfaceNumber < NumberOfSubfaces; subfaceNumber++) {
    if (subfaceNumber == AbsentSubface) {
      continue;
    }
    auto* const stagedValues = stage.claim(coarseFaceKey, subfaceNumber, Side);
    std::fill_n(stagedValues, ValuesPerSubface, 1.0);
  }
  validate(not stage.hasChild(coarseFaceKey, AbsentSubface));

  toolbox::blockstructured::restrictHaloLayerFromStagedSubfaces(
    stage,
    coarseFaceKey,
    coarseGridFaceValues.data(),
    Normal,
    NumberOfDofsPerAxis,
    NumberOfUnknowns
  );

  // Every coarse node of the first column draws one of its three tangential
  // addends from the absent subface, so it loses exactly that one addend.
  constexpr double RestrictionScale = DIMENSIONS == 2 ? 1.0 / 3.0 : 1.0 / 9.0;
  constexpr int    AddendsPerNode   = DIMENSIONS == 2 ? 3 : 9;
  double           restrictedTotal  = 0.0;
#if DIMENSIONS == 3
  for (int coarseJ = 0; coarseJ < NumberOfDofsPerAxis; coarseJ++) {
#else
  const int coarseJ = 0;
  {
#endif
    for (int coarseI = 0; coarseI < NumberOfDofsPerAxis; coarseI++) {
      const auto index = faceNodeIndex(Normal, Side, coarseI, coarseJ, NumberOfDofsPerAxis) * NumberOfUnknowns;
      validate(coarseGridFaceValues[index] <= AddendsPerNode * RestrictionScale);
      validate(coarseGridFaceValues[index] > 0.0);
      restrictedTotal += coarseGridFaceValues[index];
    }
  }
  // The staged subfaces carry one unit per fine node, so the restricted half
  // sums to the staged node count scaled once.
  const double stagedNodeCount = static_cast<double>(NumberOfSubfaces - 1) * ValuesPerSubface;
  validateNumericalEquals(restrictedTotal, stagedNodeCount * RestrictionScale);
}

/**
 * Exercises the gathered path end to end for single-precision patch data.
 */
void toolbox::blockstructured::tests::RestrictionTest::testGatheredHaloRestrictionRunsInSinglePrecision() {
  constexpr int NumberOfUnknowns    = 1;
  constexpr int NumberOfDofsPerAxis = 2;
  constexpr int FaceSize            = faceValueCount(NumberOfDofsPerAxis, NumberOfUnknowns);
  constexpr int ValuesPerSubface    = DIMENSIONS == 2 ? NumberOfDofsPerAxis * NumberOfUnknowns
                                                      : NumberOfDofsPerAxis * NumberOfDofsPerAxis * NumberOfUnknowns;
  constexpr int Normal              = 0;
  constexpr int Side                = 1;

  toolbox::blockstructured::ChildDataStage<float> stage(NumberOfSubfaces, ValuesPerSubface);
  std::vector<float>                              coarseGridFaceValues(FaceSize, 0.0F);
  const auto* const                               coarseFaceKey = coarseGridFaceValues.data();

  for (int subfaceNumber = 0; subfaceNumber < NumberOfSubfaces; subfaceNumber++) {
    auto* const stagedValues = stage.claim(coarseFaceKey, subfaceNumber, Side);
    std::fill_n(stagedValues, ValuesPerSubface, 3.0F);
  }

  toolbox::blockstructured::restrictHaloLayerFromStagedSubfaces(
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

void toolbox::blockstructured::tests::RestrictionTest::testApplyTensorProductOrderInvariant() {
  constexpr int Unknowns = 2;

  int nodesPerCell = 1;
  for (int d = 0; d < DIMENSIONS; d++) {
    nodesPerCell *= NodesPerAxis;
  }

  std::vector<double> source(static_cast<std::size_t>(nodesPerCell) * Unknowns);
  for (std::size_t i = 0; i < source.size(); i++) {
    source[i] = pseudoRandomValue(0, static_cast<int>(i), 1);
  }

  // The identity operator reproduces the source to the last bit.
  std::array<double, NodesPerAxis * NodesPerAxis> identity{};
  for (int i = 0; i < NodesPerAxis; i++) {
    identity[i * NodesPerAxis + i] = 1.0;
  }
  const double* identityPerAxis[DIMENSIONS];
  for (int d = 0; d < DIMENSIONS; d++) {
    identityPerAxis[d] = identity.data();
  }
  std::vector<double> target(source.size(), -1.0);
  toolbox::blockstructured::applyTensorProductOrderInvariant(
    target.data(),
    source.data(),
    identityPerAxis,
    NodesPerAxis,
    NodesPerAxis,
    Unknowns
  );
  for (std::size_t i = 0; i < source.size(); i++) {
    validateEquals(bitPattern(target[i]), bitPattern(source[i]));
  }

  // With one matrix on every axis an axis swap of the source swaps the target
  // to the last bit.
  const auto    restriction = inputMajorSymmetricRestrictionMatrix();
  const double* matrixPerAxis[DIMENSIONS];
  for (int d = 0; d < DIMENSIONS; d++) {
    matrixPerAxis[d] = restriction.data() + NodesPerAxis * NodesPerAxis;
  }
  std::vector<double> swappedSource(source.size());
  peano4::dForEach(NodesPerAxis, [&](const tarch::la::Vector<DIMENSIONS, int>& node) {
    auto swapped = node;
    swapped(0)   = node(1);
    swapped(1)   = node(0);
    for (int u = 0; u < Unknowns; u++) {
      swappedSource[peano4::dLinearised(swapped, NodesPerAxis) * Unknowns + u] =
        source[peano4::dLinearised(node, NodesPerAxis) * Unknowns + u];
    }
  });
  std::vector<double> swappedTarget(source.size(), -1.0);
  toolbox::blockstructured::applyTensorProductOrderInvariant(
    target.data(),
    source.data(),
    matrixPerAxis,
    NodesPerAxis,
    NodesPerAxis,
    Unknowns
  );
  toolbox::blockstructured::applyTensorProductOrderInvariant(
    swappedTarget.data(),
    swappedSource.data(),
    matrixPerAxis,
    NodesPerAxis,
    NodesPerAxis,
    Unknowns
  );
  peano4::dForEach(NodesPerAxis, [&](const tarch::la::Vector<DIMENSIONS, int>& node) {
    auto swapped = node;
    swapped(0)   = node(1);
    swapped(1)   = node(0);
    for (int u = 0; u < Unknowns; u++) {
      validateEquals(
        bitPattern(swappedTarget[peano4::dLinearised(swapped, NodesPerAxis) * Unknowns + u]),
        bitPattern(target[peano4::dLinearised(node, NodesPerAxis) * Unknowns + u])
      );
    }
  });
}

void toolbox::blockstructured::tests::RestrictionTest::testInterpolateFaceFromCoarse() {
  constexpr int Unknowns = 2;
  constexpr int Stride   = NodesPerAxis * NodesPerAxis;
#if DIMENSIONS == 2
  constexpr int NodesPerFace = NodesPerAxis;
#else
  constexpr int NodesPerFace = NodesPerAxis * NodesPerAxis;
#endif

  const auto prolongation = inputMajorSymmetricProlongationMatrix();

  // An affine tangential field lies in the order-one basis, so every subface
  // reproduces it at its own node positions up to roundoff.
  std::array<double, NodesPerFace * Unknowns> coarse{};
  for (int node = 0; node < NodesPerFace; node++) {
    for (int u = 0; u < Unknowns; u++) {
#if DIMENSIONS == 2
      coarse[node * Unknowns + u] = 1.0 + u + 2.0 * nodePosition(node);
#else
      coarse[node * Unknowns + u] =
        1.0 + u + 2.0 * nodePosition(node % NodesPerAxis) + 3.0 * nodePosition(node / NodesPerAxis);
#endif
    }
  }

#if DIMENSIONS == 2
  for (int subface = 0; subface < 3; subface++) {
    const double*                               prolongationMatrixPerAxis[1] = {prolongation.data() + subface * Stride};
    std::array<double, NodesPerFace * Unknowns> fine{};
    toolbox::blockstructured::interpolateFaceFromCoarse(
      fine.data(),
      coarse.data(),
      prolongationMatrixPerAxis,
      NodesPerAxis,
      Unknowns
    );
    for (int node = 0; node < NodesPerFace; node++) {
      const double finePosition = (nodePosition(node) + subface) / 3.0;
      for (int u = 0; u < Unknowns; u++) {
        validateNumericalEquals(fine[node * Unknowns + u], 1.0 + u + 2.0 * finePosition);
      }
    }
  }

  // Mirroring the coarse half along the tangential axis interpolates the
  // opposite subface to the bitwise mirrored values.
  std::array<double, NodesPerFace * Unknowns> randomCoarse{};
  std::array<double, NodesPerFace * Unknowns> mirroredCoarse{};
  for (int node = 0; node < NodesPerFace; node++) {
    for (int u = 0; u < Unknowns; u++) {
      randomCoarse[node * Unknowns + u]                        = pseudoRandomValue(0, node, u);
      mirroredCoarse[(NodesPerFace - 1 - node) * Unknowns + u] = pseudoRandomValue(0, node, u);
    }
  }
  for (int subface = 0; subface < 3; subface++) {
    const double* prolongationMatrixPerAxis[1] = {prolongation.data() + subface * Stride};
    const double* mirroredMatrixPerAxis[1]     = {prolongation.data() + (2 - subface) * Stride};
    std::array<double, NodesPerFace * Unknowns> fine{};
    std::array<double, NodesPerFace * Unknowns> mirroredFine{};
    toolbox::blockstructured::interpolateFaceFromCoarse(
      fine.data(),
      randomCoarse.data(),
      prolongationMatrixPerAxis,
      NodesPerAxis,
      Unknowns
    );
    toolbox::blockstructured::interpolateFaceFromCoarse(
      mirroredFine.data(),
      mirroredCoarse.data(),
      mirroredMatrixPerAxis,
      NodesPerAxis,
      Unknowns
    );
    for (int node = 0; node < NodesPerFace; node++) {
      for (int u = 0; u < Unknowns; u++) {
        validateEquals(
          bitPattern(mirroredFine[(NodesPerFace - 1 - node) * Unknowns + u]),
          bitPattern(fine[node * Unknowns + u])
        );
      }
    }
  }
#else
  const double* prolongationMatrixPerAxis[2] = {prolongation.data(), prolongation.data() + Stride};
  std::array<double, NodesPerFace * Unknowns> fine{};
  toolbox::blockstructured::interpolateFaceFromCoarse(
    fine.data(),
    coarse.data(),
    prolongationMatrixPerAxis,
    NodesPerAxis,
    Unknowns
  );
  for (int fineOuter = 0; fineOuter < NodesPerAxis; fineOuter++) {
    for (int fineInner = 0; fineInner < NodesPerAxis; fineInner++) {
      const double innerPosition = nodePosition(fineInner) / 3.0;
      const double outerPosition = (nodePosition(fineOuter) + 1.0) / 3.0;
      for (int u = 0; u < Unknowns; u++) {
        validateNumericalEquals(
          fine[(fineOuter * NodesPerAxis + fineInner) * Unknowns + u],
          1.0 + u + 2.0 * innerPosition + 3.0 * outerPosition
        );
      }
    }
  }

  // Swapping the tangential axes of the coarse half and the per-axis operators
  // interpolates the transposed subface to the bitwise transposed values.
  std::array<double, NodesPerFace * Unknowns> randomCoarse{};
  std::array<double, NodesPerFace * Unknowns> transposedCoarse{};
  for (int outer = 0; outer < NodesPerAxis; outer++) {
    for (int inner = 0; inner < NodesPerAxis; inner++) {
      for (int u = 0; u < Unknowns; u++) {
        randomCoarse[(outer * NodesPerAxis + inner) * Unknowns + u] =
          pseudoRandomValue(1, outer * NodesPerAxis + inner, u);
        transposedCoarse[(inner * NodesPerAxis + outer) * Unknowns + u] =
          pseudoRandomValue(1, outer * NodesPerAxis + inner, u);
      }
    }
  }
  const double* swappedMatrixPerAxis[2] = {prolongation.data() + Stride, prolongation.data()};
  std::array<double, NodesPerFace * Unknowns> randomFine{};
  std::array<double, NodesPerFace * Unknowns> transposedFine{};
  toolbox::blockstructured::interpolateFaceFromCoarse(
    randomFine.data(),
    randomCoarse.data(),
    prolongationMatrixPerAxis,
    NodesPerAxis,
    Unknowns
  );
  toolbox::blockstructured::interpolateFaceFromCoarse(
    transposedFine.data(),
    transposedCoarse.data(),
    swappedMatrixPerAxis,
    NodesPerAxis,
    Unknowns
  );
  for (int fineOuter = 0; fineOuter < NodesPerAxis; fineOuter++) {
    for (int fineInner = 0; fineInner < NodesPerAxis; fineInner++) {
      for (int u = 0; u < Unknowns; u++) {
        validateEquals(
          bitPattern(transposedFine[(fineInner * NodesPerAxis + fineOuter) * Unknowns + u]),
          bitPattern(randomFine[(fineOuter * NodesPerAxis + fineInner) * Unknowns + u])
        );
      }
    }
  }
#endif
}

void toolbox::blockstructured::tests::RestrictionTest::testRestrictFaceFromChildren() {
  constexpr int Unknowns = 2;
#if DIMENSIONS == 2
  constexpr int NumberOfSubfaces = 3;
  constexpr int NodesPerFace     = NodesPerAxis;
#else
  constexpr int NumberOfSubfaces = 9;
  constexpr int NodesPerFace     = NodesPerAxis * NodesPerAxis;
#endif

  const auto    restriction                     = inputMajorSymmetricRestrictionMatrix();
  const double* restrictionMatrixPerInterval[3] = {
    restriction.data(),
    restriction.data() + NodesPerAxis * NodesPerAxis,
    restriction.data() + 2 * NodesPerAxis * NodesPerAxis,
  };

  std::vector<std::vector<double>> subfaces(NumberOfSubfaces);
  std::vector<const double*>       subfacePointers(NumberOfSubfaces);
  for (int subface = 0; subface < NumberOfSubfaces; subface++) {
    subfaces[subface].resize(static_cast<std::size_t>(NodesPerFace) * Unknowns);
    for (int node = 0; node < NodesPerFace; node++) {
      for (int u = 0; u < Unknowns; u++) {
        subfaces[subface][node * Unknowns + u] = pseudoRandomValue(subface, node, u);
      }
    }
    subfacePointers[subface] = subfaces[subface].data();
  }

  // An absent subface contributes exactly what a zero-valued one contributes.
  std::vector<double> zeroValues(static_cast<std::size_t>(NodesPerFace) * Unknowns, 0.0);
  std::vector<double> coarseWithZeroChild(static_cast<std::size_t>(NodesPerFace) * Unknowns, -1.0);
  std::vector<double> coarseWithAbsentChild(static_cast<std::size_t>(NodesPerFace) * Unknowns, -1.0);
  auto                pointers = subfacePointers;
  pointers[1]                  = zeroValues.data();
  toolbox::blockstructured::restrictFaceFromChildren(
    coarseWithZeroChild.data(),
    pointers.data(),
    restrictionMatrixPerInterval,
    NodesPerAxis,
    Unknowns
  );
  pointers[1] = nullptr;
  toolbox::blockstructured::restrictFaceFromChildren(
    coarseWithAbsentChild.data(),
    pointers.data(),
    restrictionMatrixPerInterval,
    NodesPerAxis,
    Unknowns
  );
  for (std::size_t i = 0; i < coarseWithZeroChild.size(); i++) {
    validateEquals(bitPattern(coarseWithAbsentChild[i]), bitPattern(coarseWithZeroChild[i]));
  }

#if DIMENSIONS == 2
  // Mirroring the subfaces along the tangential axis mirrors the coarse half to
  // the last bit.
  std::vector<std::vector<double>> mirroredSubfaces(NumberOfSubfaces);
  std::vector<const double*>       mirroredPointers(NumberOfSubfaces);
  for (int subface = 0; subface < NumberOfSubfaces; subface++) {
    mirroredSubfaces[subface].resize(static_cast<std::size_t>(NodesPerFace) * Unknowns);
  }
  for (int subface = 0; subface < NumberOfSubfaces; subface++) {
    for (int node = 0; node < NodesPerFace; node++) {
      for (int u = 0; u < Unknowns; u++) {
        mirroredSubfaces[NumberOfSubfaces - 1 - subface][(NodesPerFace - 1 - node) * Unknowns + u] =
          subfaces[subface][node * Unknowns + u];
      }
    }
    mirroredPointers[subface] = mirroredSubfaces[subface].data();
  }
  std::vector<double> coarse(static_cast<std::size_t>(NodesPerFace) * Unknowns, -1.0);
  std::vector<double> mirroredCoarse(static_cast<std::size_t>(NodesPerFace) * Unknowns, -1.0);
  toolbox::blockstructured::restrictFaceFromChildren(
    coarse.data(),
    subfacePointers.data(),
    restrictionMatrixPerInterval,
    NodesPerAxis,
    Unknowns
  );
  toolbox::blockstructured::restrictFaceFromChildren(
    mirroredCoarse.data(),
    mirroredPointers.data(),
    restrictionMatrixPerInterval,
    NodesPerAxis,
    Unknowns
  );
  for (int node = 0; node < NodesPerFace; node++) {
    for (int u = 0; u < Unknowns; u++) {
      validateEquals(
        bitPattern(mirroredCoarse[(NodesPerFace - 1 - node) * Unknowns + u]),
        bitPattern(coarse[node * Unknowns + u])
      );
    }
  }
#endif
}

void toolbox::blockstructured::tests::RestrictionTest::testRestrictCellWithMatrixReproducesPolynomials() {
  constexpr int Unknowns = 2;

  int nodesPerCell = 1;
  for (int d = 0; d < DIMENSIONS; d++) {
    nodesPerCell *= NodesPerAxis;
  }

  const auto interpolation = interpolationMatrix();
  const auto restriction   = restrictionMatrix();

  std::vector<double> coarse(static_cast<std::size_t>(nodesPerCell) * Unknowns, 0.0);
  peano4::dForEach(NodesPerAxis, [&](const tarch::la::Vector<DIMENSIONS, int>& node) {
    tarch::la::Vector<DIMENSIONS, double> x;
    for (int d = 0; d < DIMENSIONS; d++) {
      x(d) = nodePosition(node(d));
    }
    const int index   = peano4::dLinearised(node, NodesPerAxis) * Unknowns;
    coarse[index]     = affineField(x);
    coarse[index + 1] = 4.0;
  });

  // The interpolate-then-restrict round trip over all children is the identity for every
  // field the coarse basis represents.
  peano4::grid::GridTraversalEvent event;
  std::vector<double>              restricted(static_cast<std::size_t>(nodesPerCell) * Unknowns, 0.0);
  peano4::dForEach(3, [&](const tarch::la::Vector<DIMENSIONS, int>& child) {
    event.setRelativePositionToFather(child);
    const auto cellMarker = event.getCellMarker();

    std::vector<double> fine(static_cast<std::size_t>(nodesPerCell) * Unknowns, 0.0);
    toolbox::blockstructured::interpolateCell(
      cellMarker,
      coarse.data(),
      fine.data(),
      interpolation.data(),
      NodesPerAxis,
      Unknowns
    );

    peano4::dForEach(NodesPerAxis, [&](const tarch::la::Vector<DIMENSIONS, int>& node) {
      tarch::la::Vector<DIMENSIONS, double> x;
      for (int d = 0; d < DIMENSIONS; d++) {
        x(d) = (nodePosition(node(d)) + child(d)) / 3.0;
      }
      const int index = peano4::dLinearised(node, NodesPerAxis) * Unknowns;
      validateNumericalEqualsWithEps(fine[index], affineField(x), 1e-12);
      validateNumericalEqualsWithEps(fine[index + 1], 4.0, 1e-12);
    });

    toolbox::blockstructured::restrictCell(
      cellMarker,
      restricted.data(),
      fine.data(),
      restriction.data(),
      QuadratureWeights.data(),
      NodesPerAxis,
      Unknowns
    );
  });

  for (std::size_t i = 0; i < coarse.size(); i++) {
    validateNumericalEqualsWithEps(restricted[i], coarse[i], 1e-12);
  }
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
      // The averaging restriction accumulates onto the coarse face, which starts at zero.
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
