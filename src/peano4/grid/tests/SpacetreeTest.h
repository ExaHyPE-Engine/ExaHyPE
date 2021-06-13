// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#ifndef _PEANO4_GRID_TESTS_SPACETREE_TEST_H_
#define _PEANO4_GRID_TESTS_SPACETREE_TEST_H_

#include "tarch/tests/TestCase.h"
#include "tarch/logging/Log.h"
#include "tarch/tests/TestCase.h"


namespace peano4 {
  namespace grid {
    namespace tests {
      class SpacetreeTest;
    }
  }
}


class peano4::grid::tests::SpacetreeTest: public tarch::tests::TestCase {
  private:
    /**
     * Logging device
     */
    static tarch::logging::Log _log;

    void testRestrictToCoarseGrid();



    /**
     * Test case which I extracted from a bug report:
     *
<pre>
21759300951  00:00:21     rank:0       core:0       trace        peano4::grid::Spacetree::getAdjacentDomainIds(...)                  out:neighbour:0 (file:peano4/grid/Spacetree.cpp,line:1253)
PreviousTreeSweep=0,isAntecessorOfRefinedVertexInCurrentTreeSweep=0,numberOfAdjacentRefinedLocalCells=0,x=[0.777778,0.481481],level=3),splittingIsConsideredLocal:1,joiningIsConsideredLocal:1 (file:peano4/grid/Spacetree.cpp,line:95)
21758461048  00:00:21     rank:0       core:0       trace        peano4::grid::Spacetree::isVertexAdjacentToLocalSpacetree(...)               out:result:1 (file:peano4/grid/Spacetree.cpp,line:108)
21758489494  00:00:21     rank:0       core:0       trace        peano4::grid::Spacetree::isVertexAdjacentToLocalSpacetree(...)               out:result:1 (file:peano4/grid/Spacetree.cpp,line:108)
</pre>
     *
     * with the following parameters:
<pre>
    vertex.toString():(state=New,      adjacentRanks=[1,1,1,1],backupOfAdjacentRanks=[0,0,0,0],hasBeenAntecessorOfRefinedVertexInPreviousTreeSweep=0,isAntecessorOfRefinedVertexInCurrentTreeSweep=0,numberOfAdjacentRefinedLocalCells=1684632135,x=[0.777778,0.444444],level=3),splittingIsConsideredLocal:1,joiningIsConsideredLocal:1 (file:peano4/grid/Spacetree.cpp,line:95)
    vertex.toString():(state=Unrefined,adjacentRanks=[1,1,1,1],backupOfAdjacentRanks=[1,1,1,1],hasBeenAntecessorOfRefinedVertexInPreviousTreeSweep=0,isAntecessorOfRefinedVertexInCurrentTreeSweep=0,numberOfAdjacentRefinedLocalCells=0,         x=[0.814815,0.444444],level=3),splittingIsConsideredLocal:1,joiningIsConsideredLocal:1 (file:peano4/grid/Spacetree.cpp,line:95)
    vertex.toString():(state=Unrefined,adjacentRanks=[1,1,1,1],backupOfAdjacentRanks=[1,1,1,1],hasBeenAntecessorOfRefinedVertexInPreviousTreeSweep=0,isAntecessorOfRefinedVertexInCurrentTreeSweep=0,numberOfAdjacentRefinedLocalCells=0,         x=[0.777778,0.481481],level=3),splittingIsConsideredLocal:1,joiningIsConsideredLocal:0 (file:peano4/grid/Spacetree.cpp,line:95)
    vertex.toString():(state=Unrefined,adjacentRanks=[1,1,1,1],backupOfAdjacentRanks=[1,1,1,1],hasBeenAntecessorOfRefinedVertexInPreviousTreeSweep=0,isAntecessorOfRefinedVertexInCurrentTreeSweep=0,numberOfAdjacentRefinedLocalCells=0,         x=[0.814815,0.481481],level=3),splittingIsConsideredLocal:1,joiningIsConsideredLocal:1 (file:peano4/grid/Spacetree.cpp,line:95)
</pre>
     * face number 1 on tree 1. The code yields a neighbour 0:
     *
     */
    void testGetAdjacentDomainIds();

  public:
    SpacetreeTest();
    void run() override;
};


#endif
