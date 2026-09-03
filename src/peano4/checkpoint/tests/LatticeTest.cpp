// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "LatticeTest.h"

#include "peano4/checkpoint/Lattice.h"
#include "peano4/topology/Tree.h"

peano4::checkpoint::tests::LatticeTest::LatticeTest() :
  TestCase("peano4::checkpoint::tests::LatticeTest") {}

void peano4::checkpoint::tests::LatticeTest::testTriadicMortonKeyRoundTrip() {
  constexpr int Level        = 4;
  constexpr int CellsPerAxis = 81;
  for (int cell = 0; cell < 500; cell++) {
    tarch::la::Vector<DIMENSIONS, int> latticeIndex(0);
    int                                remainder = cell * 131;
    for (int axis = 0; axis < DIMENSIONS; axis++) {
      latticeIndex(axis) = remainder % CellsPerAxis;
      remainder /= 3;
    }
    const std::int64_t key = triadicMortonKey(latticeIndex, Level);
    validateWithParams2(latticeIndexOfTriadicMortonKey(key, Level) == latticeIndex, key, latticeIndex(0));
  }

  const tarch::la::Vector<DIMENSIONS, int> origin(0);
  validateEquals(triadicMortonKey(origin, 0), 0);
  validateEquals(triadicMortonKey(origin, 5), 0);
}

void peano4::checkpoint::tests::LatticeTest::testAncestorKeysArePrefixes() {
  constexpr int                      Level = 3;
  tarch::la::Vector<DIMENSIONS, int> cell(0);
  for (int axis = 0; axis < DIMENSIONS; axis++) {
    cell(axis) = 7 + axis * 5;
  }
  const std::int64_t key = triadicMortonKey(cell, Level);
  for (int ancestorLevel = 0; ancestorLevel <= Level; ancestorLevel++) {
    const auto   ancestor    = ancestorOfCell(cell, Level, ancestorLevel);
    std::int64_t shiftedKey  = key;
    for (int digit = ancestorLevel; digit < Level; digit++) {
      shiftedKey /= THREE_POWER_D;
    }
    validateEqualsWithParams1(triadicMortonKey(ancestor, ancestorLevel), shiftedKey, ancestorLevel);
  }
}

void peano4::checkpoint::tests::LatticeTest::testLeafEnumerationOnRegularGrid() {
  // A regular depth-2 grid without refinement flags: Every level-2 cell is a
  // leaf, so a level-1 block enumerates its 3^d children in key order.
  const tarch::la::Vector<DIMENSIONS, double> offset(0.0);
  const tarch::la::Vector<DIMENSIONS, double> width(1.0);
  peano4::topology::Tree                      tree(0, offset, width);
  tree.buildRegularGrid(2);

  tarch::la::Vector<DIMENSIONS, int> block(0);
  for (int axis = 0; axis < DIMENSIONS; axis++) {
    block(axis) = axis == 0 ? 2 : 1;
  }
  std::vector<std::int64_t> keys;
  appendLeafKeysOfBlock(tree, 2, 1, block, keys);

  validateEquals(keys.size(), THREE_POWER_D);
  const std::int64_t firstKey = triadicMortonKey(block, 1) * THREE_POWER_D;
  for (int leaf = 0; leaf < THREE_POWER_D; leaf++) {
    validateEqualsWithParams1(keys[leaf], firstKey + leaf, leaf);
  }

  // The whole domain as one level-0 block enumerates every level-2 cell.
  keys.clear();
  appendLeafKeysOfBlock(tree, 2, 0, tarch::la::Vector<DIMENSIONS, int>(0), keys);
  validateEquals(keys.size(), static_cast<std::size_t>(THREE_POWER_D) * THREE_POWER_D);
  validate(std::is_sorted(keys.begin(), keys.end()));
}

void peano4::checkpoint::tests::LatticeTest::run() {
  testMethod(testTriadicMortonKeyRoundTrip);
  testMethod(testAncestorKeysArePrefixes);
  testMethod(testLeafEnumerationOnRegularGrid);
}
