// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "PeriodicTopology.h"

#include <algorithm>
#include <cmath>
#include <sstream>

/**
 * Derive the integer cell index from an AutomatonState and the global domain
 * offset. Uses AutomatonState::x/h, which are always present, in contrast to
 * the debug-only GridVertex coordinates. Floating point and therefore only
 * used as a debug cross-check of the exact index carried through descend();
 * it asserts that the pre-round quotient is close to an integer.
 */
tarch::la::Vector<DIMENSIONS, int> peano4::grid::cellIndexFromState(
  const AutomatonState&                        state,
  const tarch::la::Vector<DIMENSIONS, double>& domainOffset
) {
  tarch::la::Vector<DIMENSIONS, int> result;
  for (int d = 0; d < DIMENSIONS; d++) {
    const double quotient = (state.x(d) - domainOffset(d)) / state.h(d);
    result(d)             = static_cast<int>(std::lround(quotient));
    assertion(std::abs(quotient - static_cast<double>(result(d))) < 1.0e-6, d, quotient, result(d), state.toString());
    assertion(result(d) >= 0 and result(d) < cellsPerAxis(state.level), d, result(d), state.level);
  }
  return result;
}

/**
 * Build the stamp for faceNumber of the periodic boundary cell at cellIndex.
 * Face numbers 0..DIMENSIONS-1 are the low faces, faceNumber-DIMENSIONS the
 * high faces, so the side follows from the face number; a debug assertion
 * cross-checks it against the cell index.
 */
peano4::grid::PeriodicStamp
peano4::grid::makeFaceStamp(int level, const tarch::la::Vector<DIMENSIONS, int>& cellIndex, int faceNumber) {
  assertion(faceNumber >= 0 and faceNumber < TWO_TIMES_D, faceNumber);
  const int  axis = faceNumber % DIMENSIONS;
  const auto side = faceNumber < DIMENSIONS ? PeriodicSide::Low : PeriodicSide::High;
  assertion(
    cellIndex(axis) == (side == PeriodicSide::Low ? 0 : cellsPerAxis(level) - 1),
    faceNumber,
    axis,
    cellIndex(axis),
    level
  );

  PeriodicStamp result;
  result.key.level  = level;
  result.key.entity = PeriodicEntity::Face;
  result.key.wrappedAxes.reset();
  result.key.wrappedAxes.set(axis);
  result.key.canonicalIndex       = cellIndex;
  result.key.canonicalIndex(axis) = 0;
  result.side                     = side;
  return result;
}

/**
 * Build the stamp for the periodic image of the vertex at vertexIndex that
 * wraps the given non-empty axis subset. A vertex on k periodic axes has
 * 2^k - 1 images, one per non-empty subset; each is stamped independently.
 * The side is the anchor axis' side, and wrapped axes fold to the phase
 * relative to the anchor (0 = same side, max = opposite side), so the two
 * partner images of a subset share a key while distinct diagonals stay
 * distinct.
 */
peano4::grid::PeriodicStamp peano4::grid::makeVertexStamp(
  int                                       level,
  const tarch::la::Vector<DIMENSIONS, int>& vertexIndex,
  const std::bitset<DIMENSIONS>&            wrappedAxes
) {
  assertion(wrappedAxes.any());
  const int cellsAlongAxis = cellsPerAxis(level);

  int anchor = -1;
  for (int d = 0; d < DIMENSIONS; d++) {
    if (wrappedAxes[d]) {
      anchor = d;
      break;
    }
  }

  PeriodicStamp result;
  result.key.level       = level;
  result.key.entity      = PeriodicEntity::Vertex;
  result.key.wrappedAxes = wrappedAxes;
  for (int d = 0; d < DIMENSIONS; d++) {
    if (wrappedAxes[d]) {
      assertion(vertexIndex(d) == 0 or vertexIndex(d) == cellsAlongAxis, d, vertexIndex(d), cellsAlongAxis, level);
      result.key.canonicalIndex(d) = (vertexIndex(d) == vertexIndex(anchor)) ? 0 : cellsAlongAxis;
    } else {
      assertion(vertexIndex(d) >= 0 and vertexIndex(d) <= cellsAlongAxis, d, vertexIndex(d), cellsAlongAxis, level);
      result.key.canonicalIndex(d) = vertexIndex(d);
    }
  }
  result.side = (vertexIndex(anchor) == 0) ? PeriodicSide::Low : PeriodicSide::High;
  return result;
}

void peano4::grid::packPeriodicStamp(const PeriodicStamp& stamp, int* out) {
  out[0] = stamp.key.level;
  out[1] = static_cast<int>(stamp.key.entity);
  out[2] = static_cast<int>(stamp.key.wrappedAxes.to_ulong());
  for (int d = 0; d < DIMENSIONS; d++) {
    out[3 + d] = stamp.key.canonicalIndex(d);
  }
  out[3 + DIMENSIONS] = static_cast<int>(stamp.side);
}

peano4::grid::PeriodicStamp peano4::grid::unpackPeriodicStamp(const int* in) {
  PeriodicStamp stamp;
  stamp.key.level       = in[0];
  stamp.key.entity      = static_cast<PeriodicEntity>(in[1]);
  stamp.key.wrappedAxes = std::bitset<DIMENSIONS>(static_cast<unsigned long>(in[2]));
  for (int d = 0; d < DIMENSIONS; d++) {
    stamp.key.canonicalIndex(d) = in[3 + d];
  }
  stamp.side = static_cast<PeriodicSide>(in[3 + DIMENSIONS]);
  return stamp;
}

void peano4::grid::packPeriodicDescriptor(const PeriodicDescriptor& descriptor, int* out) {
  packPeriodicStamp(descriptor.stamp, out);
  out[PackedPeriodicStampSize] = descriptor.treeId;
}

peano4::grid::PeriodicDescriptor peano4::grid::unpackPeriodicDescriptor(const int* in) {
  PeriodicDescriptor descriptor;
  descriptor.stamp  = unpackPeriodicStamp(in);
  descriptor.treeId = in[PackedPeriodicStampSize];
  return descriptor;
}

/**
 * Deterministic broker rank for a key. Every rank computes the same broker
 * for the same key from the key's integers alone, so all records of one
 * periodic entity meet on one rank during the rendezvous discovery. The
 * mixing spreads consecutive indices over the ranks to balance the brokers.
 */
int peano4::grid::periodicKeyBroker(const PeriodicKey& key, int numberOfBrokers) {
  assertion(numberOfBrokers > 0, numberOfBrokers);
  std::uint64_t hash = 0x9e3779b97f4a7c15ull;
  auto mix = [&hash](std::uint64_t value) {
    hash ^= value + 0x9e3779b97f4a7c15ull + (hash << 6) + (hash >> 2);
  };
  mix(static_cast<std::uint64_t>(key.level));
  mix(static_cast<std::uint64_t>(key.entity));
  mix(key.wrappedAxes.to_ullong());
  for (int d = 0; d < DIMENSIONS; d++) {
    mix(static_cast<std::uint64_t>(key.canonicalIndex(d)));
  }
  return static_cast<int>(hash % static_cast<std::uint64_t>(numberOfBrokers));
}

/**
 * Resolve the recorded descriptors into the routing table. The table unions
 * producer and consumer records per (key, side): producers predict who reads
 * the partner side next sweep, and the consumer records cover regrid
 * transitions in which the reader of a side is not its producer. A key with
 * only one side present yields an entry whose other side is empty, so its
 * data has no target and is dropped, never mis-routed.
 */
peano4::grid::PeriodicRouting peano4::grid::buildPeriodicRouting(
  const std::vector<PeriodicDescriptor>& producers,
  const std::vector<PeriodicDescriptor>& consumers
) {
  PeriodicRouting routing;

  auto insert = [&routing](const PeriodicDescriptor& descriptor) {
    routing[descriptor.stamp.key][toIndex(descriptor.stamp.side)].push_back(descriptor.treeId);
  };
  for (const auto& descriptor : producers) {
    insert(descriptor);
  }
  for (const auto& descriptor : consumers) {
    insert(descriptor);
  }

  for (auto& [key, sides] : routing) {
    for (auto& trees : sides) {
      std::sort(trees.begin(), trees.end());
      trees.erase(std::unique(trees.begin(), trees.end()), trees.end());
    }
  }
  return routing;
}

bool peano4::grid::PeriodicKey::operator==(const PeriodicKey& rhs) const {
  if (level != rhs.level or entity != rhs.entity or wrappedAxes != rhs.wrappedAxes) {
    return false;
  }
  for (int d = 0; d < DIMENSIONS; d++) {
    if (canonicalIndex(d) != rhs.canonicalIndex(d)) {
      return false;
    }
  }
  return true;
}

bool peano4::grid::PeriodicKey::operator<(const PeriodicKey& rhs) const {
  if (level != rhs.level) {
    return level < rhs.level;
  }
  if (entity != rhs.entity) {
    return static_cast<int>(entity) < static_cast<int>(rhs.entity);
  }
  if (wrappedAxes.to_ulong() != rhs.wrappedAxes.to_ulong()) {
    return wrappedAxes.to_ulong() < rhs.wrappedAxes.to_ulong();
  }
  for (int d = 0; d < DIMENSIONS; d++) {
    if (canonicalIndex(d) != rhs.canonicalIndex(d)) {
      return canonicalIndex(d) < rhs.canonicalIndex(d);
    }
  }
  return false;
}

std::string peano4::grid::PeriodicKey::toString() const {
  std::ostringstream out;
  out << "(level=" << level << ",entity=" << (entity == PeriodicEntity::Face ? "face" : "vertex")
      << ",wrap=" << wrappedAxes << ",idx=" << canonicalIndex << ")";
  return out.str();
}

std::string peano4::grid::PeriodicStamp::toString() const {
  std::ostringstream out;
  out << "(" << key.toString() << "," << (side == PeriodicSide::Low ? "low" : "high") << ")";
  return out.str();
}

std::string peano4::grid::PeriodicDescriptor::toString() const {
  std::ostringstream out;
  out << "(" << stamp.toString() << ",tree=" << treeId << ")";
  return out.str();
}
