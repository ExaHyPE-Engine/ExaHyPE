// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "SpaceFillingCurveTest.h"

#include "peano4/Constants.h"
#include "peano4/Loop.h"
#include "peano4/sfc/SpaceFillingCurve.h"
#include "peano4/sfc/curves/HilbertGrammar.h"
#include "peano4/sfc/curves/PeanoGrammar.h"

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
   * Advances a serpentine walk over a 3^d block: Axis 0 runs in its current
   * direction, and an axis that reaches a face flips its direction and carries
   * over to the next axis.
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
   * Appends the leaves of the Peano curve from its closed form: One even flag
   * per axis, the walk along an axis is positive if an even number of the
   * other axes' flags is set, and the middle child along an axis flips that
   * axis' flag.
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

  int linearise(const Leaf& leaf, const int cellsPerAxis) {
    int result = 0;
    int stride = 1;
    for (int axis = 0; axis < DIMENSIONS; axis++) {
      result += leaf[axis] * stride;
      stride *= cellsPerAxis;
    }
    return result;
  }
} // namespace

peano4::sfc::tests::SpaceFillingCurveTest::SpaceFillingCurveTest() :
  TestCase("peano4::sfc::tests::SpaceFillingCurveTest") {}

void
peano4::sfc::tests::SpaceFillingCurveTest::run() {
  testMethod(testChildrenMatchTheDerivedTable);
  testMethod(testLeafSequenceIsAHamiltonianPath);
  testMethod(testPeanoMatchesTheSerpentineClosedForm);
  testMethod(testHilbertVisitsTheTextbookOrder);
}

template <class Curve>
void
peano4::sfc::tests::SpaceFillingCurveTest::validateChildrenMatchTheDerivedTable() {
  constexpr auto Table = Curve::deriveChildOrder();
  static_assert(Table.size() == Curve::NumberOfStates);
  for (int state = 0; state < Curve::NumberOfStates; state++) {
    const auto& children = Curve::getChildren(state);
    for (int child = 0; child < Curve::ChildrenPerCell; child++) {
      validateEqualsWithParams2(
        children[child].linearisedPosition,
        Table[state][child].linearisedPosition,
        state,
        child
      );
      validateEqualsWithParams2(children[child].state, Table[state][child].state, state, child);
      validateWithParams2(children[child].state < Curve::NumberOfStates, state, child);
      const auto position = children[child].getPositionWithinParent();
      for (int axis = 0; axis < DIMENSIONS; axis++) {
        validateEqualsWithParams3(position(axis), children[child].position[axis], state, child, axis);
      }
      validateEqualsWithParams2(
        peano4::dLinearised(position, Curve::SubdivisionFactor),
        children[child].linearisedPosition,
        state,
        child
      );
    }
  }
}

template <class Curve>
void
peano4::sfc::tests::SpaceFillingCurveTest::validateLeafSequenceIsAHamiltonianPath() {
  constexpr int Depth        = 2;
  const int     cellsPerAxis = peano4::power(Curve::SubdivisionFactor, Depth);
  const auto    leaves       = getLeavesOfDepth<Curve>(Depth);
  validateEquals(static_cast<int>(leaves.size()), peano4::power(cellsPerAxis, DIMENSIONS));
  std::vector<bool> visited(leaves.size(), false);
  for (std::size_t leaf = 0; leaf < leaves.size(); leaf++) {
    const int index = linearise(leaves[leaf], cellsPerAxis);
    validateWithParams1(not visited[index], leaf);
    visited[index] = true;
    if (leaf > 0) {
      int distance = 0;
      for (int axis = 0; axis < DIMENSIONS; axis++) {
        distance += std::abs(leaves[leaf][axis] - leaves[leaf - 1][axis]);
      }
      validateEqualsWithParams1(distance, 1, leaf);
    }
  }
}

void
peano4::sfc::tests::SpaceFillingCurveTest::testChildrenMatchTheDerivedTable() {
  validateChildrenMatchTheDerivedTable<PeanoCurve>();
  validateChildrenMatchTheDerivedTable<HilbertCurve>();
}

void
peano4::sfc::tests::SpaceFillingCurveTest::testLeafSequenceIsAHamiltonianPath() {
  validateLeafSequenceIsAHamiltonianPath<PeanoCurve>();
  validateLeafSequenceIsAHamiltonianPath<HilbertCurve>();
}

void
peano4::sfc::tests::SpaceFillingCurveTest::testPeanoMatchesTheSerpentineClosedForm() {
  constexpr int     Depth  = 3;
  const auto        leaves = getLeavesOfDepth<PeanoCurve>(Depth);
  std::vector<Leaf> expected;
  appendSerpentineLeaves(std::bitset<DIMENSIONS>{}, Depth, Leaf{}, expected);
  validateEquals(leaves.size(), expected.size());
  for (std::size_t leaf = 0; leaf < leaves.size(); leaf++) {
    for (int axis = 0; axis < DIMENSIONS; axis++) {
      validateEqualsWithParams2(leaves[leaf][axis], expected[leaf][axis], leaf, axis);
    }
  }
}

/**
 * The second iteration in 2D visits the 4x4 cells in the classic Hilbert order, and the first
 * iteration in 3D visits the octants along the base pattern of Bader, Sect. 8.1.2.
 */
void
peano4::sfc::tests::SpaceFillingCurveTest::testHilbertVisitsTheTextbookOrder() {
#if DIMENSIONS == 2
  const auto                 leaves = getLeavesOfDepth<HilbertCurve>(2);
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
  const auto                leaves = getLeavesOfDepth<HilbertCurve>(1);
  const std::array<Leaf, 8> expected{
    {{0, 0, 0}, {0, 1, 0}, {1, 1, 0}, {1, 0, 0}, {1, 0, 1}, {1, 1, 1}, {0, 1, 1}, {0, 0, 1}}
  };
#endif
  validateEquals(leaves.size(), expected.size());
  for (std::size_t leaf = 0; leaf < leaves.size(); leaf++) {
    for (int axis = 0; axis < DIMENSIONS; axis++) {
      validateEqualsWithParams2(leaves[leaf][axis], expected[leaf][axis], leaf, axis);
    }
  }
}
