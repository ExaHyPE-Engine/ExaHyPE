// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "peano4/sfc/SpaceFillingCurve.h"

#include "peano4/Constants.h"
#include "peano4/Loop.h"
#include "peano4/sfc/curves/HilbertGrammar.h"
#include "peano4/sfc/curves/PeanoGrammar.h"
#include "tarch/testing/UnitTest.h"

namespace {
  using Leaf         = std::array<int, DIMENSIONS>;
  using PeanoCurve   = peano4::sfc::SpaceFillingCurve<peano4::sfc::PeanoGrammar>;
  using HilbertCurve = peano4::sfc::SpaceFillingCurve<peano4::sfc::HilbertGrammar>;

  /**
   * Appends the leaves of a subtree in curve order, with leaf coordinates
   * counted in units of the finest cells.
   */
  template <class Curve>
  void appendLeaves(const int state, const int depth, const Leaf& origin, std::vector<Leaf>& leaves) {
    if (depth == 0) {
      leaves.push_back(origin);
      return;
    }
    const int childWidth = peano4::power(Curve::SubdivisionFactor, depth - 1);
    for (const auto& child : Curve::getChildren(state)) {
      Leaf childOrigin = origin;
      for (int axis = 0; axis < DIMENSIONS; axis++) {
        childOrigin[axis] += child.position[axis] * childWidth;
      }
      appendLeaves<Curve>(child.state, depth - 1, childOrigin, leaves);
    }
  }

  template <class Curve>
  std::vector<Leaf> getLeavesOfDepth(const int depth) {
    std::vector<Leaf> leaves;
    appendLeaves<Curve>(Curve::RootState, depth, Leaf{}, leaves);
    return leaves;
  }

  /**
   * Advances a serpentine walk over a 3^d block. Axis 0 runs in its current direction. An axis that
   * reaches a face flips its direction and passes the step on to the next axis.
   */
  bool advanceSerpentine(Leaf& counter, std::bitset<DIMENSIONS>& positive) {
    for (int axis = 0; axis < DIMENSIONS; axis++) {
      if (positive[axis] and counter[axis] + 1 < 3) {
        counter[axis]++;
        return true;
      }
      if (not positive[axis] and counter[axis] > 0) {
        counter[axis]--;
        return true;
      }
      positive[axis] = not positive[axis];
    }
    return false;
  }

  /**
   * Appends the leaves of the Peano curve from its closed form. The walk along an axis runs in the
   * positive direction if the number of set flags of the other axes is even. The middle child along an
   * axis flips the flag of that axis.
   */
  void appendSerpentineLeaves(
    const std::bitset<DIMENSIONS>& evenFlags,
    const int                      depth,
    const Leaf&                    origin,
    std::vector<Leaf>&             leaves
  ) {
    if (depth == 0) {
      leaves.push_back(origin);
      return;
    }
    std::bitset<DIMENSIONS> positive;
    for (int axis = 0; axis < DIMENSIONS; axis++) {
      int flagsOfOtherAxes = 0;
      for (int other = 0; other < DIMENSIONS; other++) {
        flagsOfOtherAxes += (other != axis and evenFlags[other]) ? 1 : 0;
      }
      positive[axis] = flagsOfOtherAxes % 2 == 0;
    }
    const int childWidth = peano4::power(3, depth - 1);
    Leaf      counter;
    for (int axis = 0; axis < DIMENSIONS; axis++) {
      counter[axis] = positive[axis] ? 0 : 2;
    }
    do {
      std::bitset<DIMENSIONS> childFlags  = evenFlags;
      Leaf                    childOrigin = origin;
      for (int axis = 0; axis < DIMENSIONS; axis++) {
        childOrigin[axis] += counter[axis] * childWidth;
        if (counter[axis] == 1) {
          childFlags.flip(axis);
        }
      }
      appendSerpentineLeaves(childFlags, depth - 1, childOrigin, leaves);
    } while (advanceSerpentine(counter, positive));
  }

  int getLinearIndexOf(const Leaf& leaf, const int cellsPerAxis) {
    int result = 0;
    int stride = 1;
    for (int axis = 0; axis < DIMENSIONS; axis++) {
      result += leaf[axis] * stride;
      stride *= cellsPerAxis;
    }
    return result;
  }

  template <class Curve>
  void validateChildrenOfEveryState() {
    for (int state = 0; state < Curve::NumberOfStates; state++) {
      const auto& children = Curve::getChildren(state);
      for (int child = 0; child < Curve::ChildrenPerCell; child++) {
        VALIDATE(children[child].state < Curve::NumberOfStates, state, child);
        const auto position = children[child].getPositionWithinParent();
        for (int axis = 0; axis < DIMENSIONS; axis++) {
          VALIDATE_EQUALS(position(axis), children[child].position[axis], state, child, axis);
        }
        VALIDATE_EQUALS(
          peano4::dLinearised(position, Curve::SubdivisionFactor),
          children[child].linearisedPosition,
          state,
          child
        );
      }
    }
  }

  /**
   * Checks that the leaves visit every cell of the lattice once and that every leaf is a face
   * neighbour of its predecessor.
   */
  void validateLeavesFormAHamiltonianPath(const std::vector<Leaf>& leaves, const int cellsPerAxis) {
    VALIDATE_EQUALS(static_cast<int>(leaves.size()), peano4::power(cellsPerAxis, DIMENSIONS));
    std::vector<bool> visited(leaves.size(), false);
    for (std::size_t leaf = 0; leaf < leaves.size(); leaf++) {
      const int index = getLinearIndexOf(leaves[leaf], cellsPerAxis);
      VALIDATE(not visited[index], leaf);
      visited[index] = true;
      if (leaf > 0) {
        int distance = 0;
        for (int axis = 0; axis < DIMENSIONS; axis++) {
          distance += std::abs(leaves[leaf][axis] - leaves[leaf - 1][axis]);
        }
        VALIDATE_EQUALS(distance, 1, leaf);
      }
    }
  }
} // namespace

UNIT_TEST("The children of every curve state name a valid state and lie at their linearised position") {
  GIVEN("The Peano curve and the Hilbert curve") {
    THEN("Every child names a valid state and its position within the parent agrees with its linearised form") {
      validateChildrenOfEveryState<PeanoCurve>();
      validateChildrenOfEveryState<HilbertCurve>();
    }
  }
}

UNIT_TEST("The leaves of every curve visit every cell once, each leaf a face neighbour of its predecessor") {
  GIVEN("The depth two") {
    constexpr int Depth = 2;

    WHEN("The function getLeavesOfDepth() lists the leaves of both curves of this depth in curve order") {
      const auto peanoLeaves   = getLeavesOfDepth<PeanoCurve>(Depth);
      const auto hilbertLeaves = getLeavesOfDepth<HilbertCurve>(Depth);

      THEN("Either list visits every cell of its lattice once and every leaf is a face neighbour of its predecessor") {
        validateLeavesFormAHamiltonianPath(peanoLeaves, peano4::power(PeanoCurve::SubdivisionFactor, Depth));
        validateLeavesFormAHamiltonianPath(hilbertLeaves, peano4::power(HilbertCurve::SubdivisionFactor, Depth));
      }
    }
  }
}

UNIT_TEST("The Peano curve visits its leaves in the order of the serpentine closed form") {
  GIVEN("The depth three and its leaves in the order of the serpentine closed form") {
    constexpr int     Depth = 3;
    std::vector<Leaf> expected;
    appendSerpentineLeaves(std::bitset<DIMENSIONS>{}, Depth, Leaf{}, expected);

    WHEN("The function getLeavesOfDepth() lists the leaves of the Peano curve of this depth in curve order") {
      const auto leaves = getLeavesOfDepth<PeanoCurve>(Depth);

      THEN("Every leaf of the curve equals the leaf of the closed form at its place") {
        VALIDATE_EQUALS(leaves.size(), expected.size());
        for (std::size_t leaf = 0; leaf < leaves.size(); leaf++) {
          for (int axis = 0; axis < DIMENSIONS; axis++) {
            VALIDATE_EQUALS(leaves[leaf][axis], expected[leaf][axis], leaf, axis);
          }
        }
      }
    }
  }
}

UNIT_TEST("The Hilbert curve visits the cells in the textbook order") {
  GIVEN("The depth two with the classic order in 2D and the depth one with the order of Bader, section 8.1.2, in 3D") {
#if DIMENSIONS == 2
    constexpr int              Depth = 2;
    const std::array<Leaf, 16> expected{
      {{0, 0},
       {1, 0},
       {1, 1},
       {0, 1},
       {0, 2},
       {0, 3},
       {1, 3},
       {1, 2},
       {2, 2},
       {2, 3},
       {3, 3},
       {3, 2},
       {3, 1},
       {2, 1},
       {2, 0},
       {3, 0}}
    };
#else
    constexpr int             Depth = 1;
    const std::array<Leaf, 8> expected{
      {{0, 0, 0}, {0, 1, 0}, {1, 1, 0}, {1, 0, 0}, {1, 0, 1}, {1, 1, 1}, {0, 1, 1}, {0, 0, 1}}
    };
#endif

    WHEN("The function getLeavesOfDepth() lists the leaves of the Hilbert curve of this depth in curve order") {
      const auto leaves = getLeavesOfDepth<HilbertCurve>(Depth);

      THEN("Every leaf of the curve equals the leaf of the textbook order at its place") {
        VALIDATE_EQUALS(leaves.size(), expected.size());
        for (std::size_t leaf = 0; leaf < leaves.size(); leaf++) {
          for (int axis = 0; axis < DIMENSIONS; axis++) {
            VALIDATE_EQUALS(leaves[leaf][axis], expected[leaf][axis], leaf, axis);
          }
        }
      }
    }
  }
}
