// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "SpaceFillingCurveTest.h"

#include "peano4/sfc/SpaceFillingCurve.h"
#include "peano4/sfc/curves/HilbertGrammar.h"
#include "peano4/sfc/curves/PeanoGrammar.h"

namespace {
  using Leaf         = std::array<int, DIMENSIONS>;
  using PeanoCurve   = peano4::sfc::SpaceFillingCurve<peano4::sfc::PeanoGrammar>;
  using HilbertCurve = peano4::sfc::SpaceFillingCurve<peano4::sfc::HilbertGrammar>;

  int power(const int base, const int exponent) {
    int result = 1;
    for (int i = 0; i < exponent; i++) {
      result *= base;
    }
    return result;
  }

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
    const int childWidth = power(Curve::SubdivisionFactor, depth - 1);
    for (const auto& child : Curve::children(state)) {
      Leaf childOrigin = origin;
      for (int axis = 0; axis < DIMENSIONS; axis++) {
        childOrigin[axis] += child.position[axis] * childWidth;
      }
      appendLeaves<Curve>(child.state, depth - 1, childOrigin, leaves);
    }
  }

  template <class Curve>
  std::vector<Leaf> leavesOf(const int depth) {
    std::vector<Leaf> leaves;
    appendLeaves<Curve>(Curve::RootState, depth, Leaf{}, leaves);
    return leaves;
  }

  /**
   * Advances a serpentine walk over a 3^d patch: Axis 0 runs in its current
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
    const int childWidth = power(3, depth - 1);
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

void peano4::sfc::tests::SpaceFillingCurveTest::run() {
  testMethod(testChildrenMatchTheDerivedTable);
  testMethod(testLeafSequenceIsAHamiltonianPath);
  testMethod(testCurveParameterFollowsTheLeafOrder);
  testMethod(testPeanoMatchesTheSerpentineClosedForm);
  testMethod(testHilbertVisitsTheTextbookOrder);
}

template <class Curve>
void peano4::sfc::tests::SpaceFillingCurveTest::validateChildrenMatchTheDerivedTable() {
  constexpr auto Table = Curve::deriveChildOrder();
  static_assert(Table.size() == Curve::NumberOfStates);
  for (int state = 0; state < Curve::NumberOfStates; state++) {
    const auto& children = Curve::children(state);
    for (int child = 0; child < Curve::ChildrenPerCell; child++) {
      validateEqualsWithParams2(
        children[child].linearisedPosition,
        Table[state][child].linearisedPosition,
        state,
        child
      );
      validateEqualsWithParams2(children[child].state, Table[state][child].state, state, child);
      validateWithParams2(children[child].state < Curve::NumberOfStates, state, child);
      const auto position           = children[child].positionWithinParent();
      int        linearisedPosition = 0;
      int        stride             = 1;
      for (int axis = 0; axis < DIMENSIONS; axis++) {
        validateEqualsWithParams3(position(axis), children[child].position[axis], state, child, axis);
        linearisedPosition += position(axis) * stride;
        stride *= Curve::SubdivisionFactor;
      }
      validateEqualsWithParams2(linearisedPosition, children[child].linearisedPosition, state, child);
    }
  }
}

template <class Curve>
void peano4::sfc::tests::SpaceFillingCurveTest::validateLeafSequenceIsAHamiltonianPath() {
  constexpr int Depth        = 2;
  const int     cellsPerAxis = power(Curve::SubdivisionFactor, Depth);
  const auto    leaves       = leavesOf<Curve>(Depth);
  validateEquals(static_cast<int>(leaves.size()), power(cellsPerAxis, DIMENSIONS));
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

void peano4::sfc::tests::SpaceFillingCurveTest::testChildrenMatchTheDerivedTable() {
  validateChildrenMatchTheDerivedTable<PeanoCurve>();
  validateChildrenMatchTheDerivedTable<HilbertCurve>();
}

void peano4::sfc::tests::SpaceFillingCurveTest::testLeafSequenceIsAHamiltonianPath() {
  validateLeafSequenceIsAHamiltonianPath<PeanoCurve>();
  validateLeafSequenceIsAHamiltonianPath<HilbertCurve>();
}

/**
 * The leaves of a regular grid in curve order receive the parameters
 * i / ChildrenPerCell^depth, a coarser cell receives the parameter of its
 * first leaf, and the root receives zero.
 */
template <class Curve>
void peano4::sfc::tests::SpaceFillingCurveTest::validateCurveParameterFollowsTheLeafOrder() {
  constexpr int    Depth                  = 3;
  constexpr double Tolerance              = 1e-12;
  const int        cellsPerAxis           = power(Curve::SubdivisionFactor, Depth);
  const int        leavesBelowACoarseCell = power(Curve::ChildrenPerCell, Depth - 1);
  const auto       leaves                 = leavesOf<Curve>(Depth);
  const auto       numberOfLeaves         = static_cast<double>(leaves.size());
  for (std::size_t leaf = 0; leaf < leaves.size(); leaf++) {
    tarch::la::Vector<DIMENSIONS, double> centre;
    for (int axis = 0; axis < DIMENSIONS; axis++) {
      centre(axis) = (leaves[leaf][axis] + 0.5) / cellsPerAxis;
    }
    validateNumericalEqualsWithEpsWithParams1(
      Curve::curveParameter(centre, Depth),
      static_cast<double>(leaf) / numberOfLeaves,
      Tolerance,
      leaf
    );
    const auto coarseCell = leaf / leavesBelowACoarseCell;
    validateNumericalEqualsWithEpsWithParams1(
      Curve::curveParameter(centre, 1),
      static_cast<double>(coarseCell) / Curve::ChildrenPerCell,
      Tolerance,
      leaf
    );
    validateNumericalEquals(Curve::curveParameter(centre, 0), 0.0);
  }
}

void peano4::sfc::tests::SpaceFillingCurveTest::testCurveParameterFollowsTheLeafOrder() {
  validateCurveParameterFollowsTheLeafOrder<PeanoCurve>();
  validateCurveParameterFollowsTheLeafOrder<HilbertCurve>();
}

void peano4::sfc::tests::SpaceFillingCurveTest::testPeanoMatchesTheSerpentineClosedForm() {
  constexpr int     Depth  = 3;
  const auto        leaves = leavesOf<PeanoCurve>(Depth);
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
 * In two dimensions the second iteration runs through the 4x4 cells in the
 * classic Hilbert order. In three dimensions the first iteration runs through
 * the octants along the base pattern of Bader, Sect. 8.1.2, which enters at
 * the origin and leaves along axis 2.
 */
void peano4::sfc::tests::SpaceFillingCurveTest::testHilbertVisitsTheTextbookOrder() {
#if DIMENSIONS == 2
  const auto                 leaves = leavesOf<HilbertCurve>(2);
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
  const auto                leaves = leavesOf<HilbertCurve>(1);
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
