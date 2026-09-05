// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "peano4/Globals.h"
#include "tarch/Assertions.h"
#include "tarch/la/Vector.h"

namespace peano4::topology {
  class Tree;
} // namespace peano4::topology

/**
 * Names checkpointed cells by integers.
 *
 * A checkpoint stores one record per leaf cell, keyed by the cell's level and
 * its triadic Morton key: The base-three digits of the lattice position,
 * interleaved across the axes from the coarsest digit down. Cells below one
 * ancestor therefore carry the same leading digits, so the records of any
 * ancestor cell form one contiguous key range.
 *
 * Every level's records are sorted by that key and grouped into blocks: The
 * cells below one ancestor on the level's block level. A block is the unit a
 * resuming rank reads, so the block level caps how much of a level one read
 * pulls in.
 */
namespace peano4::checkpoint {
  /** The deepest block level; a coarser cell level is its own block level. */
  constexpr int BlockLevelBound = DIMENSIONS == 2 ? 5 : 3;

  INLINE_METHOD int blockLevelOfLevel(int cellLevel);

  INLINE_METHOD std::int64_t numberOfBlocksOnLevel(int cellLevel);

  INLINE_METHOD std::int64_t triadicMortonKey(const tarch::la::Vector<DIMENSIONS, int>& latticeIndex, int level);

  INLINE_METHOD tarch::la::Vector<DIMENSIONS, int> latticeIndexOfTriadicMortonKey(std::int64_t key, int level);

  /** Returns the lattice position of the cell's ancestor on ancestorLevel. */
  INLINE_METHOD tarch::la::Vector<DIMENSIONS, int> ancestorOfCell(
    const tarch::la::Vector<DIMENSIONS, int>& latticeIndex,
    int                                       cellLevel,
    int                                       ancestorLevel
  );

  /**
   * Appends the triadic Morton keys of every leaf of cellLevel below the
   * given block, in ascending key order. The tree's refinement-plan tables
   * answer which cells exist and which are refined, so the result is the same
   * on every rank.
   */
  void appendLeafKeysOfBlock(
    const peano4::topology::Tree&             tree,
    int                                       cellLevel,
    int                                       blockLevel,
    const tarch::la::Vector<DIMENSIONS, int>& blockLattice,
    std::vector<std::int64_t>&                keys
  );
} // namespace peano4::checkpoint

#include "peano4/checkpoint/Lattice.cpph"
