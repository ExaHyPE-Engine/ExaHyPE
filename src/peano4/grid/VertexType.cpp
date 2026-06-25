// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "VertexType.h"

#include "GridVertex.h"
#include "peano4/Loop.h"

std::ostream& operator<<(std::ostream& out, const peano4::grid::VertexType& type) {
  return (out << peano4::grid::toString(type));
}

std::string peano4::grid::toString(VertexType type) {
  switch (type) {
  case VertexType::New:
    return "New";
  case VertexType::Hanging:
    return "Hanging";
  case VertexType::Persistent:
    return "Persistent";
  case VertexType::Delete:
    return "Delete";
  }
  return "<undef>";
}

inline peano4::grid::VertexType mergeVertexTypes(peano4::grid::VertexType lhs, peano4::grid::VertexType rhs) {
  using namespace peano4::grid;
  constexpr VertexType MergeTable[4][4] = {
    {VertexType::New, VertexType::New, VertexType::Persistent, VertexType::Persistent},
    {VertexType::New, VertexType::Hanging, VertexType::Persistent, VertexType::Delete},
    {VertexType::Persistent, VertexType::Persistent, VertexType::Persistent, VertexType::Persistent},
    {VertexType::Persistent, VertexType::Delete, VertexType::Persistent, VertexType::Delete},
  };
  return MergeTable[static_cast<int>(lhs)][static_cast<int>(rhs)];
}

inline peano4::grid::VertexType getCoarseGridVertexType(const peano4::grid::GridVertex& coarseGridVertex) {
  using namespace peano4::grid;
  switch (coarseGridVertex.getState()) {
  case GridVertex::State::Refined:
  case GridVertex::State::EraseTriggered:
    return VertexType::Persistent;
  case GridVertex::State::Refining:
    return VertexType::New;
  case GridVertex::State::RefinementTriggered:
  case GridVertex::State::Delete:
  case GridVertex::State::New:
  case GridVertex::State::Unrefined:
  case GridVertex::State::HangingVertex:
    return VertexType::Hanging;
  case GridVertex::State::Erasing:
    return VertexType::Delete;
  }
  assertion(false);
  return VertexType::Hanging;
}

// Recursive fallback for higher dimensions
inline peano4::grid::VertexType getVertexType(
  peano4::grid::GridVertex           coarseGridVertices[TWO_POWER_D],
  tarch::la::Vector<DIMENSIONS, int> position,
  int                                dimension
) {
  if (dimension < 0) {
    int linearIndex = peano4::dLinearised(position, 2);
    return getCoarseGridVertexType(coarseGridVertices[linearIndex]);
  }

  if (position(dimension) == 0) {
    return getVertexType(coarseGridVertices, position, dimension - 1);
  } else if (position(dimension) == 3) {
    position(dimension) = 1;
    return getVertexType(coarseGridVertices, position, dimension - 1);
  }

  tarch::la::Vector<DIMENSIONS, int> positionCopy = position;
  position(dimension)                             = 0;
  peano4::grid::VertexType lhs                    = getVertexType(coarseGridVertices, position, dimension - 1);
  positionCopy(dimension)                         = 1;
  peano4::grid::VertexType rhs                    = getVertexType(coarseGridVertices, positionCopy, dimension - 1);
  return mergeVertexTypes(lhs, rhs);
}

inline peano4::grid::VertexType getVertexType(
  peano4::grid::GridVertex                  coarseGridVertices[TWO_POWER_D],
  const tarch::la::Vector<DIMENSIONS, int>& position
) {
  using namespace peano4::grid;

#if DIMENSIONS == 2 or DIMENSIONS == 3
  if (
    coarseGridVertices[0].getState() == GridVertex::State::Refined
    and coarseGridVertices[1].getState() == GridVertex::State::Refined
    and coarseGridVertices[2].getState() == GridVertex::State::Refined
    and coarseGridVertices[3].getState() == GridVertex::State::Refined
#if DIMENSIONS == 3
    and coarseGridVertices[4].getState() == GridVertex::State::Refined
    and coarseGridVertices[5].getState() == GridVertex::State::Refined
    and coarseGridVertices[6].getState() == GridVertex::State::Refined
    and coarseGridVertices[7].getState() == GridVertex::State::Refined
#endif
  ) {
    return VertexType::Persistent;
  }

#if DIMENSIONS == 2
  // Clamp pos into [0..3]
  assertion(position(0) >= 0 and position(0) <= 3);
  assertion(position(1) >= 0 and position(1) <= 3);

  // Precompute the 4 coarse-corner types once.
  VertexType c00 = getCoarseGridVertexType(coarseGridVertices[peano4::dLinearised({0, 0}, 2)]);
  VertexType c01 = getCoarseGridVertexType(coarseGridVertices[peano4::dLinearised({0, 1}, 2)]);
  VertexType c10 = getCoarseGridVertexType(coarseGridVertices[peano4::dLinearised({1, 0}, 2)]);
  VertexType c11 = getCoarseGridVertexType(coarseGridVertices[peano4::dLinearised({1, 1}, 2)]);

  const int x = position(0), y = position(1);

  // Corners: map directly to one coarse vertex.
  if ((x == 0 or x == 3) and (y == 0 or y == 3)) {
    const int ix = (x == 0 ? 0 : 1);
    const int iy = (y == 0 ? 0 : 1);
    return (ix == 0 ? (iy == 0 ? c00 : c01) : (iy == 0 ? c10 : c11));
  }

  // Edges: merge two coarse vertices.
  if (x == 0 or x == 3) {
    const int  ix = (x == 0 ? 0 : 1);
    VertexType v0 = (ix == 0 ? c00 : c10);
    VertexType v1 = (ix == 0 ? c01 : c11);
    return mergeVertexTypes(v0, v1);
  }
  if (y == 0 or y == 3) {
    const int  iy = (y == 0 ? 0 : 1);
    VertexType v0 = (iy == 0 ? c00 : c01);
    VertexType v1 = (iy == 0 ? c10 : c11);
    return mergeVertexTypes(v0, v1);
  }

  // Interior: combine all four coarse vertices
  return mergeVertexTypes(mergeVertexTypes(c00, c01), mergeVertexTypes(c10, c11));
#elif DIMENSIONS == 3
  assertion(position(0) >= 0 and position(0) <= 3);
  assertion(position(1) >= 0 and position(1) <= 3);
  assertion(position(2) >= 0 and position(2) <= 3);

  const int x = position(0), y = position(1), z = position(2);

  // Precompute the 8 coarse-corner types once.
  auto C = [&](int ix, int iy, int iz) -> VertexType {
    return getCoarseGridVertexType(coarseGridVertices[peano4::dLinearised({ix, iy, iz}, 2)]);
  };
  VertexType c000 = C(0, 0, 0), c001 = C(0, 0, 1), c010 = C(0, 1, 0), c011 = C(0, 1, 1), c100 = C(1, 0, 0),
             c101 = C(1, 0, 1), c110 = C(1, 1, 0), c111 = C(1, 1, 1);

  // Corners: one coarse vertex.
  if ((x == 0 or x == 3) and (y == 0 or y == 3) and (z == 0 or z == 3)) {
    const int ix = (x == 0 ? 0 : 1);
    const int iy = (y == 0 ? 0 : 1);
    const int iz = (z == 0 ? 0 : 1);
    if (ix == 0) {
      return (iy == 0 ? (iz == 0 ? c000 : c001) : (iz == 0 ? c010 : c011));
    } else {
      return (iy == 0 ? (iz == 0 ? c100 : c101) : (iz == 0 ? c110 : c111));
    }
  }

  // Edges: exactly two fixed at 0/3 -> merge two coarse vertices.
  if ((x == 0 or x == 3) and (y == 0 or y == 3)) {
    const int  ix = (x == 0 ? 0 : 1);
    const int  iy = (y == 0 ? 0 : 1);
    VertexType v0 = (ix ? (iy ? c110 : c100) : (iy ? c010 : c000));
    VertexType v1 = (ix ? (iy ? c111 : c101) : (iy ? c011 : c001));
    return mergeVertexTypes(v0, v1);
  }
  if ((x == 0 or x == 3) and (z == 0 or z == 3)) {
    const int  ix = (x == 0 ? 0 : 1);
    const int  iz = (z == 0 ? 0 : 1);
    VertexType v0 = (ix ? (iz ? c101 : c100) : (iz ? c001 : c000));
    VertexType v1 = (ix ? (iz ? c111 : c110) : (iz ? c011 : c010));
    return mergeVertexTypes(v0, v1);
  }
  if ((y == 0 or y == 3) and (z == 0 or z == 3)) {
    const int  iy = (y == 0 ? 0 : 1);
    const int  iz = (z == 0 ? 0 : 1);
    VertexType v0 = (iy ? (iz ? c011 : c010) : (iz ? c001 : c000));
    VertexType v1 = (iy ? (iz ? c111 : c110) : (iz ? c101 : c100));
    return mergeVertexTypes(v0, v1);
  }

  // Faces: exactly one fixed at 0/3 -> merge four coarse vertices.
  if (x == 0 or x == 3) {
    const int  ix = (x == 0 ? 0 : 1);
    VertexType v0 = (ix ? c100 : c000);
    VertexType v1 = (ix ? c101 : c001);
    VertexType v2 = (ix ? c110 : c010);
    VertexType v3 = (ix ? c111 : c011);
    return mergeVertexTypes(mergeVertexTypes(v0, v1), mergeVertexTypes(v2, v3));
  }
  if (y == 0 or y == 3) {
    const int  iy = (y == 0 ? 0 : 1);
    VertexType v0 = (iy ? c010 : c000);
    VertexType v1 = (iy ? c011 : c001);
    VertexType v2 = (iy ? c110 : c100);
    VertexType v3 = (iy ? c111 : c101);
    return mergeVertexTypes(mergeVertexTypes(v0, v1), mergeVertexTypes(v2, v3));
  }
  if (z == 0 or z == 3) {
    const int  iz = (z == 0 ? 0 : 1);
    VertexType v0 = (iz ? c001 : c000);
    VertexType v1 = (iz ? c011 : c010);
    VertexType v2 = (iz ? c101 : c100);
    VertexType v3 = (iz ? c111 : c110);
    return mergeVertexTypes(mergeVertexTypes(v0, v1), mergeVertexTypes(v2, v3));
  }

  // Interior: combine all 8
  return mergeVertexTypes(
    mergeVertexTypes(mergeVertexTypes(c000, c001), mergeVertexTypes(c010, c011)),
    mergeVertexTypes(mergeVertexTypes(c100, c101), mergeVertexTypes(c110, c111))
  );
#endif
#else
  return ::getVertexType(coarseGridVertices, position, DIMENSIONS - 1);
#endif
}

void peano4::grid::getVertexTypes(
  GridVertex                                coarseGridVertices[TWO_POWER_D],
  const tarch::la::Vector<DIMENSIONS, int>& position,
  VertexType                                result[TWO_POWER_D]
) {
  for (int i = 0; i < TWO_POWER_D; ++i) {
    std::bitset<DIMENSIONS> offset(i);
    result[i] = getVertexType(coarseGridVertices, position + tarch::la::Vector<DIMENSIONS, int>(offset));
  }
}

inline peano4::grid::VertexType getCoarseGridVertexTypeFromPreviousTreeSweep(
  const peano4::grid::GridVertex& coarseGridVertex
) {
  using namespace peano4::grid;
  return coarseGridVertex.getFlag(GridVertex::HasBeenVertexTypeHangingInPreviousTreeSweep)
           ? VertexType::Hanging
           : VertexType::Persistent;
}

inline peano4::grid::VertexType getVertexTypeFromPreviousTreeSweep(
  peano4::grid::GridVertex                  coarseGridVertices[TWO_POWER_D],
  const tarch::la::Vector<DIMENSIONS, int>& position
) {
  using namespace peano4::grid;

#if DIMENSIONS == 2
  assertion(position(0) >= 0 and position(0) <= 3);
  assertion(position(1) >= 0 and position(1) <= 3);

  VertexType c00 = getCoarseGridVertexTypeFromPreviousTreeSweep(
    coarseGridVertices[peano4::dLinearised({0, 0}, 2)]
  );
  VertexType c01 = getCoarseGridVertexTypeFromPreviousTreeSweep(
    coarseGridVertices[peano4::dLinearised({0, 1}, 2)]
  );
  VertexType c10 = getCoarseGridVertexTypeFromPreviousTreeSweep(
    coarseGridVertices[peano4::dLinearised({1, 0}, 2)]
  );
  VertexType c11 = getCoarseGridVertexTypeFromPreviousTreeSweep(
    coarseGridVertices[peano4::dLinearised({1, 1}, 2)]
  );

  const int x = position(0), y = position(1);

  if ((x == 0 or x == 3) and (y == 0 or y == 3)) {
    const int ix = (x == 0 ? 0 : 1);
    const int iy = (y == 0 ? 0 : 1);
    return (ix == 0 ? (iy == 0 ? c00 : c01) : (iy == 0 ? c10 : c11));
  }

  if (x == 0 or x == 3) {
    const int  ix = (x == 0 ? 0 : 1);
    VertexType v0 = (ix == 0 ? c00 : c10);
    VertexType v1 = (ix == 0 ? c01 : c11);
    return mergeVertexTypes(v0, v1);
  }
  if (y == 0 or y == 3) {
    const int  iy = (y == 0 ? 0 : 1);
    VertexType v0 = (iy == 0 ? c00 : c01);
    VertexType v1 = (iy == 0 ? c10 : c11);
    return mergeVertexTypes(v0, v1);
  }

  return mergeVertexTypes(mergeVertexTypes(c00, c01), mergeVertexTypes(c10, c11));
#elif DIMENSIONS == 3
  assertion(position(0) >= 0 and position(0) <= 3);
  assertion(position(1) >= 0 and position(1) <= 3);
  assertion(position(2) >= 0 and position(2) <= 3);

  const int x = position(0), y = position(1), z = position(2);

  auto C = [&](int ix, int iy, int iz) -> VertexType {
    return getCoarseGridVertexTypeFromPreviousTreeSweep(
      coarseGridVertices[peano4::dLinearised({ix, iy, iz}, 2)]
    );
  };
  VertexType c000 = C(0, 0, 0), c001 = C(0, 0, 1), c010 = C(0, 1, 0), c011 = C(0, 1, 1), c100 = C(1, 0, 0),
             c101 = C(1, 0, 1), c110 = C(1, 1, 0), c111 = C(1, 1, 1);

  if ((x == 0 or x == 3) and (y == 0 or y == 3) and (z == 0 or z == 3)) {
    const int ix = (x == 0 ? 0 : 1);
    const int iy = (y == 0 ? 0 : 1);
    const int iz = (z == 0 ? 0 : 1);
    if (ix == 0) {
      return (iy == 0 ? (iz == 0 ? c000 : c001) : (iz == 0 ? c010 : c011));
    } else {
      return (iy == 0 ? (iz == 0 ? c100 : c101) : (iz == 0 ? c110 : c111));
    }
  }

  if ((x == 0 or x == 3) and (y == 0 or y == 3)) {
    const int  ix = (x == 0 ? 0 : 1);
    const int  iy = (y == 0 ? 0 : 1);
    VertexType v0 = (ix ? (iy ? c110 : c100) : (iy ? c010 : c000));
    VertexType v1 = (ix ? (iy ? c111 : c101) : (iy ? c011 : c001));
    return mergeVertexTypes(v0, v1);
  }
  if ((x == 0 or x == 3) and (z == 0 or z == 3)) {
    const int  ix = (x == 0 ? 0 : 1);
    const int  iz = (z == 0 ? 0 : 1);
    VertexType v0 = (ix ? (iz ? c101 : c100) : (iz ? c001 : c000));
    VertexType v1 = (ix ? (iz ? c111 : c110) : (iz ? c011 : c010));
    return mergeVertexTypes(v0, v1);
  }
  if ((y == 0 or y == 3) and (z == 0 or z == 3)) {
    const int  iy = (y == 0 ? 0 : 1);
    const int  iz = (z == 0 ? 0 : 1);
    VertexType v0 = (iy ? (iz ? c011 : c010) : (iz ? c001 : c000));
    VertexType v1 = (iy ? (iz ? c111 : c110) : (iz ? c101 : c100));
    return mergeVertexTypes(v0, v1);
  }

  if (x == 0 or x == 3) {
    const int  ix = (x == 0 ? 0 : 1);
    VertexType v0 = (ix ? c100 : c000);
    VertexType v1 = (ix ? c101 : c001);
    VertexType v2 = (ix ? c110 : c010);
    VertexType v3 = (ix ? c111 : c011);
    return mergeVertexTypes(mergeVertexTypes(v0, v1), mergeVertexTypes(v2, v3));
  }
  if (y == 0 or y == 3) {
    const int  iy = (y == 0 ? 0 : 1);
    VertexType v0 = (iy ? c010 : c000);
    VertexType v1 = (iy ? c011 : c001);
    VertexType v2 = (iy ? c110 : c100);
    VertexType v3 = (iy ? c111 : c101);
    return mergeVertexTypes(mergeVertexTypes(v0, v1), mergeVertexTypes(v2, v3));
  }
  if (z == 0 or z == 3) {
    const int  iz = (z == 0 ? 0 : 1);
    VertexType v0 = (iz ? c001 : c000);
    VertexType v1 = (iz ? c011 : c010);
    VertexType v2 = (iz ? c101 : c100);
    VertexType v3 = (iz ? c111 : c110);
    return mergeVertexTypes(mergeVertexTypes(v0, v1), mergeVertexTypes(v2, v3));
  }

  return mergeVertexTypes(
    mergeVertexTypes(mergeVertexTypes(c000, c001), mergeVertexTypes(c010, c011)),
    mergeVertexTypes(mergeVertexTypes(c100, c101), mergeVertexTypes(c110, c111))
  );
#else
  // Recursive fallback for higher dimensions
  auto getVertexTypeRec = [&](auto&& self, tarch::la::Vector<DIMENSIONS, int> pos, int dim) -> VertexType {
    if (dim < 0) {
      const int linearIndex = peano4::dLinearised(pos, 2);
      return getCoarseGridVertexTypeFromPreviousTreeSweep(coarseGridVertices[linearIndex]);
    }
    if (pos(dim) == 0) {
      return self(self, pos, dim - 1);
    } else if (pos(dim) == 3) {
      pos(dim) = 1;
      return self(self, pos, dim - 1);
    }
    tarch::la::Vector<DIMENSIONS, int> posCopy = pos;
    pos(dim)                                   = 0;
    VertexType lhs                             = self(self, pos, dim - 1);
    posCopy(dim)                               = 1;
    VertexType rhs                             = self(self, posCopy, dim - 1);
    return mergeVertexTypes(lhs, rhs);
  };
  return getVertexTypeRec(getVertexTypeRec, position, DIMENSIONS - 1);
#endif
}

void peano4::grid::getVertexTypesFromPreviousTreeSweep(
  GridVertex                                coarseGridVertices[TWO_POWER_D],
  const tarch::la::Vector<DIMENSIONS, int>& position,
  VertexType                                result[TWO_POWER_D]
) {
  for (int i = 0; i < TWO_POWER_D; ++i) {
    std::bitset<DIMENSIONS> offset(i);
    result[i] = getVertexTypeFromPreviousTreeSweep(
      coarseGridVertices,
      position + tarch::la::Vector<DIMENSIONS, int>(offset)
    );
  }
}
