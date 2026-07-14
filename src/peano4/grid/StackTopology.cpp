// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "StackTopology.h"

#include "SpacetreeHelper.h"

#include "peano4/Loop.h"

bool peano4::grid::StackTopology::isInOutStack(int stackNumber) {
  return stackNumber == static_cast<int>(BaseStack::Input) or stackNumber == static_cast<int>(BaseStack::Output);
}

bool peano4::grid::StackTopology::isFaceExchangeStack(int stackNumber) {
  return stackNumber >= FaceExchangeStackOffset and stackNumber < PeriodicBoundaryExchangeStackOffset;
}

bool peano4::grid::StackTopology::isLocalExchangeStack(int stackNumber) {
  return stackNumber < TotalNumberOfLocalExchangeStacks;
}

int peano4::grid::StackTopology::mapPeriodicBoundaryExchangeOutputStackOntoInputStack(int outputStack) {
  const int firstPeriodicBCStack        = PeriodicBoundaryExchangeStackOffset;
  const int outputStackMinusOtherStacks = outputStack - firstPeriodicBCStack;
  const int mirroredStack               = TotalNumberOfPeriodicBoundaryExchangeStacks - outputStackMinusOtherStacks - 1;
  return mirroredStack + firstPeriodicBCStack;
}

// TODO: Move to SpacetreeHelper or create new class to handle periodic boundaries
std::bitset<2 * DIMENSIONS>
peano4::grid::StackTopology::getPeriodicBoundaryNumber(const std::array<int, TWO_POWER_D>& flags) {
  std::bitset<2 * DIMENSIONS> result;

  for (int d = 0; d < 2 * DIMENSIONS; d++) {
    bool allFlagsSet = true;
    peano4::dForEachCorner([&](const tarch::la::Vector<DIMENSIONS, int>& k, int kScalar) {
      tarch::la::Vector<DIMENSIONS, int> entry;
      entry                 = k;
      entry(d % DIMENSIONS) = d / DIMENSIONS;
      allFlagsSet &= flags[peano4::dLinearised(entry, 2)] == SpacetreeHelper::RankOfPeriodicBoundaryCondition;
    });
    if (allFlagsSet) {
      result.set(d);
    }
  }
  return result;
}

int peano4::grid::StackTopology::getOutputStackForPeriodicBoundaryExchange(int faceNumber) {
  tarch::la::Vector<DIMENSIONS, int> neighbour(1);
  int                                normal    = faceNumber % DIMENSIONS;
  bool                               lookRight = faceNumber < DIMENSIONS;
  neighbour(normal) += lookRight ? 1 : -1;
  int linearisedIndex = peano4::dLinearised(neighbour, 3);
  if (linearisedIndex >= THREE_POWER_D / 2) {
    linearisedIndex--;
  }
  const int baseIndex = PeriodicBoundaryExchangeStackOffset;
  return baseIndex + linearisedIndex;
}

std::set<peano4::grid::StackTopology::PeriodicBoundaryStackIdentifier>
peano4::grid::StackTopology::getOutputStacksForPeriodicBoundaryExchange(const std::array<int, TWO_POWER_D>& flags) {
  std::bitset<2 * DIMENSIONS> boundaryNumbers = getPeriodicBoundaryNumber(flags);

  std::unordered_set<std::bitset<2 * DIMENSIONS>> powerSet;
  for (int i = 0; i < 2 * DIMENSIONS; i++) {
    if (boundaryNumbers[i]) {
      std::bitset<2 * DIMENSIONS> newEntry(0);
      newEntry.set(i);
      powerSet.insert(newEntry);

      for (const auto& p : powerSet) {
        newEntry = p;
        newEntry.set(i);
        powerSet.insert(newEntry);
      }
    }
  }

  std::set<PeriodicBoundaryStackIdentifier> result;
  int                                       currentStackNumber = 0;
  peano4::dForEachStencil([&](const tarch::la::Vector<DIMENSIONS, int>& neighbour, int neighbourScalar) {
    if (neighbour != tarch::la::Vector<DIMENSIONS, int>(1)) {
      std::bitset<2 * DIMENSIONS> requiredEntryForThisDirection(0);
      std::bitset<DIMENSIONS>     direction(0);
      for (int d = 0; d < DIMENSIONS; d++) {
        if (neighbour(d) > 1) {
          requiredEntryForThisDirection[d] = true;
          direction[d]                     = true;
        }
        if (neighbour(d) < 1) {
          requiredEntryForThisDirection[d + DIMENSIONS] = true;
          direction[d]                                  = true;
        }
      }
      if (powerSet.count(requiredEntryForThisDirection) > 0) {
        PeriodicBoundaryStackIdentifier newEntry(
          PeriodicBoundaryExchangeStackOffset + currentStackNumber,
          direction.to_ulong()
        );
        assertion(
          isPeriodicBoundaryExchangeOutputStackNumber(newEntry.first),
          newEntry.first,
          newEntry.second,
          PeriodicBoundaryExchangeStackOffset,
          TotalNumberOfPeriodicBoundaryExchangeStacks
        );
        result.insert(newEntry);
      }
      currentStackNumber++;
    }
  });

  return result;
}

int peano4::grid::StackTopology::getPeriodicBoundaryExchangeInputStackNumberForOutputStack(int outputStackNumber) {
  assertion(isPeriodicBoundaryExchangeOutputStackNumber(outputStackNumber));
  int result = outputStackNumber + TotalNumberOfPeriodicBoundaryExchangeStacks / 2;
  return result;
}

peano4::grid::StackTopology::PeriodicBoundaryStackIdentifier
peano4::grid::StackTopology::getPeriodicBoundaryExchangeInputStackNumberForOutputStack(
  PeriodicBoundaryStackIdentifier outputStackIdentifier
) {
  assertion(isPeriodicBoundaryExchangeOutputStackNumber(outputStackIdentifier.first));
  PeriodicBoundaryStackIdentifier result;
  result.first  = outputStackIdentifier.first + TotalNumberOfPeriodicBoundaryExchangeStacks / 2;
  result.second = outputStackIdentifier.second;
  return result;
}

bool peano4::grid::StackTopology::isPeriodicBoundaryExchangeOutputStackNumber(int id) {
  // We assume the first half of the periodic stacks are outputs, and the second half are inputs.
  const bool periodicBoundary = id >= PeriodicBoundaryExchangeStackOffset
    and id < PeriodicBoundaryExchangeStackOffset + TotalNumberOfPeriodicBoundaryExchangeStacks / 2;
  return periodicBoundary;
}
