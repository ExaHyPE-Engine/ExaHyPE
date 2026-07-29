// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/CompilerSpecificSettings.h"
#include "tarch/mpi/Rank.h"

namespace peano4::parallel {
  class TreeIdMapping;
} // namespace peano4::parallel

/**
 * Global tree numbering. A tree ID interleaves the ranks: the ID modulo the
 * number of ranks is the hosting rank and the quotient is the local slot, so
 * both directions are pure arithmetic. The singleton part tracks which local
 * slots host a tree, hands out free slots, and takes them back.
 */
class peano4::parallel::TreeIdMapping final {
public:
  ~TreeIdMapping()                               = default;
  TreeIdMapping(const TreeIdMapping&)            = delete;
  TreeIdMapping(TreeIdMapping&&)                 = delete;
  TreeIdMapping& operator=(const TreeIdMapping&) = delete;
  TreeIdMapping& operator=(TreeIdMapping&&)      = delete;

  static TreeIdMapping& getInstance();

  static INLINE_METHOD int getLocalTreeId(const int treeId);
  static INLINE_METHOD int getGlobalTreeId(const int localTreeId);

  /**
   * Rank hosting the given tree.
   */
  static INLINE_METHOD int getRank(const int treeId);

  static INLINE_METHOD bool isGlobalMaster(const int treeId);

  void init();

  int getNumberOfRegisteredTrees() const;

  /**
   * Reserve one tree ID on this rank. This convenience operation delegates to
   * reserveIds().
   */
  int reserveId();

  /**
   * Reserve several tree IDs on this rank through one scan over the local slots.
   */
  std::vector<int> reserveIds(const int numberOfIds);

  void deregisterId(const int treeId);

private:
  TreeIdMapping() = default;

  static TreeIdMapping _singleton;

  static INLINE_METHOD int getId(const int rank, const int localTreeId);

  void registerId(const int treeId);

  /**
   * One bit per local tree slot, set while the slot hosts a tree. The slots of a
   * rank form the dense key space behind the global tree IDs, so occupancy is a
   * bitset rather than a map.
   */
  std::bitset<MAX_SPACETREES_PER_RANK> _occupiedTreeSlots;
};

#include "TreeIdMapping.cpph"
