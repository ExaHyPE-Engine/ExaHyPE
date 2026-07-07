// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "AutomatonState.h"

#include "peano4/Globals.h"
#include "tarch/Assertions.h"
#include "tarch/la/Vector.h"

#include <bitset>
#include <cstdint>
#include <map>
#include <optional>
#include <string>
#include <type_traits>
#include <vector>

namespace peano4::grid {
  struct PeriodicKey;
  struct PeriodicStamp;
  struct PeriodicDescriptor;

  /**
   * Which side of a periodic axis (subset) a boundary entity sits on.
   *
   * For a face this is low (cell index 0) vs high (cell index max). For a
   * multi-axis (corner) vertex image it is the side of the lowest-indexed
   * wrapped axis, the anchor; see makeVertexStamp().
   */
  enum class PeriodicSide : std::int8_t { Low = 0, High = 1 };

  /**
   * Discriminates face keys from vertex keys. A face key folds a cell index
   * and a vertex key folds a vertex index, so without this tag the two could
   * collide on the same integer tuple.
   */
  enum class PeriodicEntity : std::int8_t { Face = 0, Vertex = 1 };

  INLINE_METHOD constexpr PeriodicSide opposite(PeriodicSide side);

  INLINE_METHOD constexpr int toIndex(PeriodicSide side);

  INLINE_METHOD constexpr int cellsPerAxis(int level);

  INLINE_METHOD tarch::la::Vector<DIMENSIONS, int> childCellIndex(
    const tarch::la::Vector<DIMENSIONS, int>& parentCellIndex,
    const tarch::la::Vector<DIMENSIONS, int>& childPositionInParent
  );

  INLINE_METHOD tarch::la::Vector<DIMENSIONS, int>
  vertexIndexOfCellVertex(const tarch::la::Vector<DIMENSIONS, int>& cellIndex, int vertexNumber);

  tarch::la::Vector<DIMENSIONS, int>
  cellIndexFromState(const AutomatonState& state, const tarch::la::Vector<DIMENSIONS, double>& domainOffset);

  PeriodicStamp makeFaceStamp(int level, const tarch::la::Vector<DIMENSIONS, int>& cellIndex, int faceNumber);

  PeriodicStamp makeVertexStamp(
    int                                       level,
    const tarch::la::Vector<DIMENSIONS, int>& vertexIndex,
    const std::bitset<DIMENSIONS>&            wrappedAxes
  );

  /**
   * Number of ints a stamp respectively descriptor packs into for the
   * cross-rank exchange: level, entity, wrappedAxes, the DIMENSIONS index
   * entries, and side; a descriptor adds the tree id. All integer, so both
   * travel as plain MPI_INT blocks without a custom datatype.
   */
  constexpr int PackedPeriodicStampSize      = 4 + DIMENSIONS;
  constexpr int PackedPeriodicDescriptorSize = PackedPeriodicStampSize + 1;

  void               packPeriodicStamp(const PeriodicStamp& stamp, int* out);
  PeriodicStamp      unpackPeriodicStamp(const int* in);
  void               packPeriodicDescriptor(const PeriodicDescriptor& descriptor, int* out);
  PeriodicDescriptor unpackPeriodicDescriptor(const int* in);

  int periodicKeyBroker(const PeriodicKey& key, int numberOfBrokers);

  /**
   * Per key and side, the sorted, duplicate-free tree ids that touch that side
   * of the wrap. Data produced at one side is routed to every tree registered
   * for the opposite side.
   */
  using PeriodicRouting = std::map<PeriodicKey, std::array<std::vector<int>, 2>>;

  PeriodicRouting buildPeriodicRouting(
    const std::vector<PeriodicDescriptor>& producers,
    const std::vector<PeriodicDescriptor>& consumers
  );
} // namespace peano4::grid

/**
 * Tree-independent integer identity of a periodic boundary entity: a face or
 * one vertex image. The two entities on opposite sides of the same wrap share
 * a key, as the wrapped-axis position is folded to a canonical value, while
 * everything else (level, entity kind, wrapped axes, transverse position)
 * keeps them apart. Purely integer, no coordinates.
 */
struct peano4::grid::PeriodicKey {
  int                                level  = -1;
  PeriodicEntity                     entity = PeriodicEntity::Face;
  std::bitset<DIMENSIONS>            wrappedAxes;
  tarch::la::Vector<DIMENSIONS, int> canonicalIndex;

  bool               operator==(const PeriodicKey& rhs) const;
  INLINE_METHOD bool operator!=(const PeriodicKey& rhs) const;
  bool               operator<(const PeriodicKey& rhs) const;
  std::string        toString() const;
};

/**
 * A key plus the geometric side it was computed on. The stamp travels with
 * every periodic payload: the producer pushes it in lockstep with the data,
 * the routing sends the payload to the opposite side's trees, and the reader
 * pops the entry whose key equals its own and whose side is the opposite of
 * its own.
 */
struct peano4::grid::PeriodicStamp {
  PeriodicKey  key;
  PeriodicSide side = PeriodicSide::Low;

  INLINE_METHOD bool matchesPartnerOf(const PeriodicStamp& readerStamp) const;

  std::string toString() const;
};

/**
 * A stamp plus the recording tree. Every tree records one descriptor per
 * periodic boundary entity it produces (sends) and one per entity it consumes
 * (reads) each traversal; SpacetreeSet resolves them into the PeriodicRouting.
 */
struct peano4::grid::PeriodicDescriptor {
  PeriodicStamp stamp;
  int           treeId = -1;

  std::string toString() const;
};

namespace peano4::grid {
  /**
   * A periodic payload that has to travel to a partner tree on another rank.
   * Staged during the same-rank periodic pass and shipped by
   * SpacetreeSet::exchangeCrossRankPeriodicData(), which sends the stamp and
   * deposit stack alongside the payload so the receiving rank deposits it
   * where the partner's stamp-matched receive reads.
   */
  template <class Payload>
  struct CrossRankPeriodicEntry {
    int           sendTree;
    int           recvTree;
    PeriodicStamp stamp;
    int           depositStack;
    Payload       data;
  };

  /**
   * Remove and return the entry of a stamped periodic stack that is the
   * wrapped partner of readerStamp (equal key, opposite side), or
   * std::nullopt if none is present. Data stack and stamp list stay parallel
   * via a swap-with-last before popping. Finding nothing is legitimate on the
   * first sweep after a regrid, when the partner side had not produced data
   * yet; the caller then skips the merge for this sweep.
   */
  template <class StackType>
  std::optional<std::decay_t<decltype(std::declval<StackType&>().pop())>>
  popPeriodicEntryMatching(StackType& stack, std::vector<PeriodicStamp>& stamps, const PeriodicStamp& readerStamp) {
    assertion(stack.size() == stamps.size(), stack.size(), stamps.size(), readerStamp.toString());
    for (std::size_t i = 0; i < stamps.size(); i++) {
      if (stamps[i].matchesPartnerOf(readerStamp)) {
        const std::size_t last = stamps.size() - 1;
        if (i != last) {
          std::swap(stack[i], stack[last]);
          std::swap(stamps[i], stamps[last]);
        }
        stamps.pop_back();
        return std::optional<std::decay_t<decltype(std::declval<StackType&>().pop())>>(stack.pop());
      }
    }
    return std::nullopt;
  }
} // namespace peano4::grid

#include "PeriodicTopology.cpph"
