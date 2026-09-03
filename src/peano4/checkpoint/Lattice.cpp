// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "Lattice.h"

#include "peano4/topology/Tree.h"

namespace {
  void appendLeafKeys(
    const peano4::topology::Tree&             tree,
    const int                                 cellLevel,
    const int                                 level,
    const tarch::la::Vector<DIMENSIONS, int>& latticeIndex,
    std::vector<std::int64_t>&                keys
  ) {
    if (level == cellLevel) {
      if (not tree.isCellRefinedInPlan(level, latticeIndex)) {
        keys.push_back(peano4::checkpoint::triadicMortonKey(latticeIndex, level));
      }
      return;
    }
    // Every cell above the base depth is refined; beyond it the plan decides.
    if (level >= tree.getDepth() and not tree.isCellRefinedInPlan(level, latticeIndex)) {
      return;
    }
    // The child order matches the key's digit encoding, so the recursion emits
    // ascending keys.
    for (int child = 0; child < THREE_POWER_D; child++) {
      tarch::la::Vector<DIMENSIONS, int> childLattice(0);
      int                                childDigits = child;
      for (int axis = 0; axis < DIMENSIONS; axis++) {
        childLattice(axis) = latticeIndex(axis) * 3 + childDigits % 3;
        childDigits /= 3;
      }
      appendLeafKeys(tree, cellLevel, level + 1, childLattice, keys);
    }
  }
} // namespace

void peano4::checkpoint::appendLeafKeysOfBlock(
  const peano4::topology::Tree&             tree,
  const int                                 cellLevel,
  const int                                 blockLevel,
  const tarch::la::Vector<DIMENSIONS, int>& blockLattice,
  std::vector<std::int64_t>&                keys
) {
  assertion(blockLevel <= cellLevel, blockLevel, cellLevel);
  if (not tree.cellExistsInPlan(blockLevel, blockLattice)) {
    return;
  }
  appendLeafKeys(tree, cellLevel, blockLevel, blockLattice, keys);
}
