// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#ifndef _PEANO4_GRID_H_
#define _PEANO4_GRID_H_


#include <vector>

#include "GridVertex.h"


namespace peano4 {
  /**
   * @namespace peano4::grid
   *
   * The grid namespace is Peano's core.
   *
   */
  namespace grid {
    /**
     * Forward declaration
     */
    class GridStatistics;
    class GridControlEvent;

    /**
     * The term clear() is not 100% correct, as the number of stationary traversals
     * is not reset to a dummy but instead incremented.
     */
    void clear( GridStatistics& statistics, bool isGlobalMasterTree );

    /**
     * Merge set such that many events next to each other are fused into one
     *
     * Take a set of grid control events and try to consolidate them, i.e.
     * remove overlaps and fuse individual requests. The consolidation
     * algorithm is pretty primitive:
     *
     * - We create a new (empty) result list.
     * - We iterate over the input list. 
     * - Per entry, we check it against all elements that have already been
     *   added ot the result set. 
     *   -- If the current one is adjacent to an element from the output set,
     *      and if both trigger the same kind of refinement, we enlarge the 
     *      element that is already in the result set and memorise that we 
     *      have "inserted" something into the output. We can immediately 
     *      terminate our run-through through the output.
     *   -- If a refinement entry in the input sequence overlaps with an 
     *      erase entry in the output (or the other way round), we keep the 
     *      refinement only. We can immediately terminate our run-through 
     *      through the output.
     *   -- If an entry from the input set has not triggered any modification
     *      of an entry in the output set, we append it to the output set.
     * - If changes have been made, i.e. if the output set is smaller than 
     *   the input set, then we rerun the algorithm recursively again. Due to
     *   mergers, four adjacent blocks might now form two bigger blocks which 
     *   in turn can be merged once again.
     *
     * <h2> Two boxes can be merged into one </h2>
     * 
     * The "next to each other" implementation is again simplistic: We
     * construct the bounding box around two events and compare its volume
     * to the sum of the volumes of the two elements. If the bounding box
     * volume is only around a few percent bigger than the sum, then we use
     * the boundary box to replace the original two elements.
     *
     * What I don't do in this algorithm is to take overlaps into account.
     * Indeed, the slack (percentage) accommodates slight overlaps. But the
     * code still might not recognise two elements if they massively overlap.
     */
    std::vector< GridControlEvent > merge( std::vector< GridControlEvent>  events, const double Tolerance = 0.1 );

    /**
     * Factory mechanism
     *
     * There are a few attributes which should have dummy values. There are
     * also a few attributes which are set later on throughout the traversal,
     * but I should initialise them here properly to ensure that valgrind's
     * memchecker doesn't complain.
     *
     * @image html grid_createVertex.png
     *
     * @param adjacentRanks  Typically the result from Spacetree::getAdjacentRanksForNewVertex(coarseGridVertices,vertexPositionWithin3x3Patch).
     */
    GridVertex createVertex(
      GridVertex::State                            state,
      const tarch::la::Vector<Dimensions,double>&  x,
      int                                          level,
      const tarch::la::Vector<TwoPowerD,int>&      adjacentRanks,
      bool                                         isNewFineGridVertex
    );
  }
}


peano4::grid::GridStatistics operator+( peano4::grid::GridStatistics lhs, peano4::grid::GridStatistics rhs );

#endif
