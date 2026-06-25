// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "PeanoCurve.h"

#include "AutomatonState.h"
#include "StackTopology.h"

bool peano4::grid::PeanoCurve::isTraversePositiveAlongAxis(const AutomatonState& state, int axis) {
  assertion(axis >= 0);
  assertion(axis < DIMENSIONS);
  bool evenParity = true; // true ⇔ even number of set flags seen so far
  for (int d = 0; d < DIMENSIONS; ++d) {
    if (d == axis) {
      continue;
    }
    if (state.getEvenFlag(d)) {
      evenParity = not evenParity;
    }
  }
  return evenParity ^ state.getInverted();
}

peano4::LoopDirection peano4::grid::PeanoCurve::getLoopDirection(const AutomatonState& state) {
  peano4::LoopDirection result;
  for (int i = 0; i < DIMENSIONS; i++) {
    result[i] = isTraversePositiveAlongAxis(state, i);
  }
  return result;
}

void peano4::grid::PeanoCurve::setExitFace(AutomatonState& cell, int axis) {
  if (isTraversePositiveAlongAxis(cell, axis)) {
    removeFaceAccessNumber(cell, axis + DIMENSIONS);
    setFaceAccessNumber(cell, axis + DIMENSIONS, 1);
  } else {
    removeFaceAccessNumber(cell, axis);
    setFaceAccessNumber(cell, axis, 1);
  }
}

void peano4::grid::PeanoCurve::setEntryFace(AutomatonState& cell, int axis) {
  if (isTraversePositiveAlongAxis(cell, axis)) {
    removeFaceAccessNumber(cell, axis);
    setFaceAccessNumber(cell, axis, -1);
  } else {
    removeFaceAccessNumber(cell, axis + DIMENSIONS);
    setFaceAccessNumber(cell, axis + DIMENSIONS, -1);
  }
}

void peano4::grid::PeanoCurve::invertEvenFlag(AutomatonState& cell, int axis) {
  assertion(axis >= 0);
  assertion(axis < DIMENSIONS);
  cell.flipEvenFlag(axis);
}

void peano4::grid::PeanoCurve::removeFaceAccessNumber(AutomatonState& cell, int face) {
  assertion(face >= 0);
  assertion(face < 2 * DIMENSIONS);

  auto&     accessNumber  = cell.accessNumber;
  const int oldFaceNumber = accessNumber[face];

  if (accessNumber[face] > 0) {
    for (int i = 0; i < 2 * DIMENSIONS; i++) {
      if (accessNumber[i] >= oldFaceNumber) {
        accessNumber[i]--;
      }
    }
  }
  if (accessNumber[face] < 0) {
    for (int i = 0; i < 2 * DIMENSIONS; i++) {
      if (accessNumber[i] <= oldFaceNumber) {
        accessNumber[i]++;
      }
    }
  }

  accessNumber[face] = 0;
}

std::bitset<DIMENSIONS> peano4::grid::PeanoCurve::getFirstVertexIndex(const AutomatonState& cell) {
  std::bitset<DIMENSIONS> result;
  std::bitset<DIMENSIONS> currentMask;

  for (int i = 0; i < DIMENSIONS; i++) {
    if (cell.getEvenFlag(i)) {
      currentMask[i] = true;
      currentMask.flip();
      result ^= currentMask;
      currentMask.flip();
      currentMask[i] = false;
    }
  }

  if (cell.getInverted()) {
    result.flip();
  }

  return result;
}

void peano4::grid::PeanoCurve::setFaceAccessNumber(AutomatonState& cell, int face, int value) {
  assertion(face >= 0);
  assertion(face < 2 * DIMENSIONS);
  assertion(value != 0);
  assertion(cell.accessNumber[face] == 0);

  auto& accessNumber = cell.accessNumber;

  if (value > 0) {
    for (int i = 0; i < 2 * DIMENSIONS; i++) {
      if (accessNumber[i] >= value) {
        accessNumber[i]++;
      }
    }
  } else {
    for (int i = 0; i < 2 * DIMENSIONS; i++) {
      if (accessNumber[i] <= value) {
        accessNumber[i]--;
      }
    }
  }

  accessNumber[face] = static_cast<short int>(value);
}


int peano4::grid::PeanoCurve::getInputStackNumber(const AutomatonState& state) {
  if (state.getInverted()) {
    return static_cast<int>(StackTopology::BaseStack::Output);
  } else {
    return static_cast<int>(StackTopology::BaseStack::Input);
  }
}

int peano4::grid::PeanoCurve::getOutputStackNumber(const AutomatonState& state) {
  if (state.getInverted()) {
    return static_cast<int>(StackTopology::BaseStack::Input);
  } else {
    return static_cast<int>(StackTopology::BaseStack::Output);
  }
}

int peano4::grid::PeanoCurve::getVertexReadStackNumber(
  const AutomatonState&          state,
  const std::bitset<DIMENSIONS>& vertex
) {
  const auto& a = state.accessNumber;

  int bestVal  = -2 * DIMENSIONS - 1; // Largest negative
  int bestFace = -1;
  int bestDir  = -1;

  UNROLL(DIMENSIONS)
  for (int d = 0; d < DIMENSIONS; ++d) {
    const int face = vertex[d] ? d + DIMENSIONS : d;
    const int v    = a[face];
    if ((v < 0) and (v > bestVal)) {
      bestVal  = v;
      bestFace = face;
      bestDir  = d;
    }
  }

  int result = -1;
  if (bestFace < 0) {
    result = getInputStackNumber(state);
  } else {
    if (state.getEvenFlag(bestDir)) {
      bestFace += (bestFace < DIMENSIONS) ? DIMENSIONS : -DIMENSIONS;
    }
    result = static_cast<int>(StackTopology::BaseStack::NumberOfStacks) + bestFace;
  }

  assertion(result > static_cast<int>(StackTopology::BaseStack::Call), result);
  return result;
}

int peano4::grid::PeanoCurve::getVertexWriteStackNumber(
  const AutomatonState&          cell,
  const std::bitset<DIMENSIONS>& vertex
) {
  const auto& a = cell.accessNumber;

  int bestVal  = 2 * DIMENSIONS + 1; // Smallest positive
  int bestFace = -1;
  int bestDir  = -1;

  UNROLL(DIMENSIONS)
  for (int d = 0; d < DIMENSIONS; ++d) {
    const int face = vertex[d] ? d + DIMENSIONS : d;
    const int v    = a[face];
    if ((v > 0) and (v < bestVal)) {
      bestVal  = v;
      bestFace = face;
      bestDir  = d;
    }
  }

  int result = -1;
  if (bestFace < 0) {
    result = getOutputStackNumber(cell);
  } else {
    if (cell.getEvenFlag(bestDir)) {
      bestFace += (bestFace < DIMENSIONS) ? DIMENSIONS : -DIMENSIONS;
    }
    result = static_cast<int>(StackTopology::BaseStack::NumberOfStacks) + bestFace;
  }

  assertion(result > static_cast<int>(StackTopology::BaseStack::Call), result);
  return result;
}

int peano4::grid::PeanoCurve::getFaceNumberAlongCurve(const AutomatonState& cell, int face) {
  if (cell.getInverted()) {
    face = 2 * DIMENSIONS - 1 - face;
  }
  int normal = face % DIMENSIONS;
  int result = face;
  for (int d = 0; d < DIMENSIONS; d++) {
    if (d != normal) {
      if (cell.getEvenFlag(d) and not cell.getInverted()) {
        result += DIMENSIONS;
      } else if (cell.getEvenFlag(d) and cell.getInverted()) {
        result += DIMENSIONS;
      }
    }
  }

  result = result % (2 * DIMENSIONS);
  return result;
}

int peano4::grid::PeanoCurve::getFaceReadStackNumber(const AutomatonState& state, int face) {
  int result    = getInputStackNumber(state);
  int direction = face % DIMENSIONS;

  if (state.accessNumber[face] < 0) {
    int newResult = face;
    if (state.getInverted()) {
      newResult += DIMENSIONS;
    }
    if (state.getEvenFlag(direction)) {
      newResult += DIMENSIONS;
    }
    newResult = newResult % (2 * DIMENSIONS);
    result    = newResult + static_cast<int>(StackTopology::BaseStack::NumberOfStacks);
  }

  assertion(result > static_cast<int>(StackTopology::BaseStack::Call), result);
  return result;
}

int peano4::grid::PeanoCurve::getFaceWriteStackNumber(const AutomatonState& cell, int face) {
  int result    = getOutputStackNumber(cell);
  int direction = face % DIMENSIONS;

  if (cell.accessNumber[face] > 0) {
    int newResult = face;
    if (cell.getInverted()) {
      newResult += DIMENSIONS;
    }
    if (cell.getEvenFlag(direction)) {
      newResult += DIMENSIONS;
    }
    newResult = newResult % (2 * DIMENSIONS);
    result    = newResult + static_cast<int>(StackTopology::BaseStack::NumberOfStacks);
  }

  assertion(result > static_cast<int>(StackTopology::BaseStack::Call), result);
  return result;
}

int peano4::grid::PeanoCurve::getCellReadStackNumber(const AutomatonState& state) { return getInputStackNumber(state); }

int peano4::grid::PeanoCurve::getCellWriteStackNumber(const AutomatonState& state) {
  return getOutputStackNumber(state);
}
