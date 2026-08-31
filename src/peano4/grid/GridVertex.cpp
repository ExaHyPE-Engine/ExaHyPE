// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "GridVertex.h"

static_assert(
  std::is_standard_layout_v<peano4::grid::GridVertex>,
  "A grid vertex travels between ranks as raw bytes, so its layout has to be standard."
);
static_assert(
  std::is_trivially_copyable_v<peano4::grid::GridVertex>,
  "A grid vertex travels between ranks as raw bytes, so it has to be trivially copyable."
);

/** Returns whether the tree continues below every corner of one cell. */
bool peano4::grid::GridVertex::areAllVerticesRefined(const peano4::grid::GridVertex vertices[TWO_POWER_D]) {
  return getRefinedVertices(vertices).all();
}

/**
 * Returns one bit per vertex, set where the vertex is refined. A zero result
 * means the cell holding those vertices is a leaf.
 */
std::bitset<TWO_POWER_D> peano4::grid::GridVertex::getRefinedVertices(
  const peano4::grid::GridVertex vertices[TWO_POWER_D]
) {
  std::bitset<TWO_POWER_D> refinedVertices;
  for (int vertexNumber = 0; vertexNumber < TWO_POWER_D; vertexNumber++) {
    refinedVertices.set(vertexNumber, vertices[vertexNumber].getState() == peano4::grid::GridVertex::State::Refined);
  }
  return refinedVertices;
}

std::ostream& operator<<(std::ostream& out, const peano4::grid::GridVertex& vertex) {
  return (out << vertex.toString());
}

std::string peano4::grid::GridVertex::toString() const {
  std::ostringstream out;
  out << "GridVertex{State=" << static_cast<int>(state) << " (";

  switch (getState()) {
  case State::HangingVertex:
    out << "HangingVertex";
    break;
  case State::Unrefined:
    out << "Unrefined";
    break;
  case State::Refined:
    out << "Refined";
    break;
  }
  out << ")";

  out << ", Level=" << level;
#if PEANO_DEBUG
  out << ", Position=" << x;
#endif
  out << "}";

  return out.str();
}
