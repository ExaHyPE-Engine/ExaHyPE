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
    void testCreateLeaveCellTraversalEvent();
    void testCreateNeighbourExchangeLists();
    void testAreFacesLocal();

    /**
     * Test face state identification
     *
     * @image html Spacetree_testCreateEnterCellTraversalEvent.png
     */
    void testGetFaceType();

    /**
     * Test to find out where faces go to and come from
     *
     *
     * @image html Spacetree_testCreateEnterCellTraversalEvent.png
     *
     *
     * This examples stems from the following (buggy) output:
     *
     * <pre>
18465645121  00:00:18     rank:0       core:0       trace        peano4::grid::Spacetree::isVertexAdjacentToLocalSpacetree(...)               in:vertex.toString():(state=Unrefined,adjacentRanks=[3,3,3,3],backupOfAdjacentRanks=[3,3,3,3],hasBeenAntecessorOfRefinedVertexInPreviousTreeSweep=0,isAntecessorOfRefinedVertexInCurrentTreeSweep=0,numberOfAdjacentRefinedLocalCells=0,x=[0.62963,0.407407],level=3),splittingIsConsideredLocal:1,joiningIsConsideredLocal:1 (file:peano4/grid/Spacetree.cpp,line:95)
18465660468  00:00:18     rank:0       core:0       trace        peano4::grid::Spacetree::isVertexAdjacentToLocalSpacetree(...)               out:result:0 (file:peano4/grid/Spacetree.cpp,line:108)
18465677543  00:00:18     rank:0       core:0       trace        peano4::grid::Spacetree::isVertexAdjacentToLocalSpacetree(...)               in:vertex.toString():(state=Unrefined,adjacentRanks=[3,2,3,2],backupOfAdjacentRanks=[3,2,3,2],hasBeenAntecessorOfRefinedVertexInPreviousTreeSweep=0,isAntecessorOfRefinedVertexInCurrentTreeSweep=0,numberOfAdjacentRefinedLocalCells=0,x=[0.666667,0.407407],level=3),splittingIsConsideredLocal:1,joiningIsConsideredLocal:1 (file:peano4/grid/Spacetree.cpp,line:95)
18465692370  00:00:18     rank:0       core:0       trace        peano4::grid::Spacetree::isVertexAdjacentToLocalSpacetree(...)               out:result:1 (file:peano4/grid/Spacetree.cpp,line:108)
18465709288  00:00:18     rank:0       core:0       trace        peano4::grid::Spacetree::isVertexAdjacentToLocalSpacetree(...)               in:vertex.toString():(state=New,adjacentRanks=[3,3,3,3],backupOfAdjacentRanks=[0,0,0,0],hasBeenAntecessorOfRefinedVertexInPreviousTreeSweep=0,isAntecessorOfRefinedVertexInCurrentTreeSweep=0,numberOfAdjacentRefinedLocalCells=1684632135,x=[0.62963,0.444444],level=3),splittingIsConsideredLocal:1,joiningIsConsideredLocal:1 (file:peano4/grid/Spacetree.cpp,line:95)
18465723996  00:00:18     rank:0       core:0       trace        peano4::grid::Spacetree::isVertexAdjacentToLocalSpacetree(...)               out:result:0 (file:peano4/grid/Spacetree.cpp,line:108)
18465740694  00:00:18     rank:0       core:0       trace        peano4::grid::Spacetree::isVertexAdjacentToLocalSpacetree(...)               in:vertex.toString():(state=New,adjacentRanks=[3,2,3,2],backupOfAdjacentRanks=[0,0,0,0],hasBeenAntecessorOfRefinedVertexInPreviousTreeSweep=0,isAntecessorOfRefinedVertexInCurrentTreeSweep=0,numberOfAdjacentRefinedLocalCells=1684632135,x=[0.666667,0.444444],level=3),splittingIsConsideredLocal:1,joiningIsConsideredLocal:1 (file:peano4/grid/Spacetree.cpp,line:95)
18466102574  00:00:18     rank:0       core:0       trace        peano4::grid::Spacetree::isVertexAdjacentToLocalSpacetree(...)               out:result:1 (file:peano4/grid/Spacetree.cpp,line:108)
18466130181  00:00:18     rank:0       core:0       trace        peano4::grid::Spacetree::createGenericCellTraversalEvent(...)             out:event.toString():(x=[0.648148,0.425926],h=[0.037037,0.037037],isRefined=0000,isVertexLocal=1010,isFaceLocal=0100,isCellLocal=0,vertexDataFrom=[0,0,0,0],vertexDataTo=[0,0,0,0],faceDataFrom=[0,0,0,0],faceDataTo=[0,0,0,0],cellData=2,relativePositionToFather=[2,2]) (file:peano4/grid/Spacetree.cpp,line:1735)
18466167134  00:00:18     rank:0       core:0       trace        peano4::grid::Spacetree::createEnterCellTraversalEvent(...)           out:state.toString():(level=3,x=[0.62963,0.407407],h=[0.037037,0.037037],inverted=1,evenFlags=11,accessNumber=[-1,-2,2,1]),event.toString():(x=[0.648148,0.425926],h=[0.037037,0.037037],isRefined=0000,isVertexLocal=1010,isFaceLocal=0100,isCellLocal=0,vertexDataFrom=[5,6,5,-2],vertexDataTo=[0,1,2,3],faceDataFrom=[4,3,-2,2],faceDataTo=[1,0,3,2],cellData=2,relativePositionToFather=[2,2]),_id:2 (file:peano4/grid/Spacetree.cpp,line:1826)
18466200951  00:00:18     rank:0       core:0       trace        peano4::grid::Spacetree::receiveAndMergeUserDataAtHorizontalBoundary(...)           in:state.toString():(level=3,x=[0.62963,0.407407],h=[0.037037,0.037037],inverted=1,evenFlags=11,accessNumber=[-1,-2,2,1]),enterCellTraversalEvent.toString():(x=[0.648148,0.425926],h=[0.037037,0.037037],isRefined=0000,isVertexLocal=1010,isFaceLocal=0100,isCellLocal=0,vertexDataFrom=[5,6,5,-2],vertexDataTo=[0,1,2,3],faceDataFrom=[4,3,-2,2],faceDataTo=[1,0,3,2],cellData=2,relativePositionToFather=[2,2]),_id:2 (file:peano4/grid/Spacetree.cpp,line:1832)
18466219530  00:00:18     rank:0       core:0       trace        peano4::grid::Spacetree::getAdjacentDomainIds(...)                  in:faceNumber:2,calledByReceivingProcess:1,_id:2 (file:peano4/grid/Spacetree.cpp,line:1194)
18466237899  00:00:18     rank:0       core:0       trace        peano4::grid::Spacetree::getAdjacentDomainIds(...)                  out:neighbour:0 (file:peano4/grid/Spacetree.cpp,line:1261)
18466259586  00:00:18     rank:0       core:0       info         peano4::grid::Spacetree::receiveAndMergeUserDataAtHorizontalBoundary(...)             receive and merge 3th face on stack 2 of tree 2 (relative position=0) with neighbour 0. Local position in cell=2, state=(level=3,x=[0.62963,0.407407],h=[0.037037,0.037037],inverted=1,evenFlags=11,accessNumber=[-1,-2,2,1]), inFaceStack=2
18466347886  00:00:18     rank:0       core:0       trace        examples::exahype2::euler::observers::CreateGrid::receiveAndMergeFaceHorizontally(...)             in:event.toString():(x=[0.648148,0.425926],h=[0.037037,0.037037],isRefined=0000,isVertexLocal=1010,isFaceLocal=0100,isCellLocal=0,vertexDataFrom=[5,6,5,-2],vertexDataTo=[0,1,2,3],faceDataFrom=[4,3,-2,2],faceDataTo=[1,0,3,2],cellData=2,relativePositionToFather=[2,2]),positionWithinCell:2,inOutStack:2,relativePositionOnInOutStack:0,fromTree:0,marker.toString():(x=[0.648148,0.425926],h=[0.037037,0.037037],select=2,is-cell-local=0),_spacetreeId:2 (file:observers/CreateGrid.cpp,line:1302)
assertion in file observers/CreateGrid.cpp, line 1313 failed: tarch::la::equals( data.getDebugX(),incomingData.getDebugX() )
parameter data.getDebugX(): [0.666667,0.425926]
parameter incomingData.getDebugX(): [0.833333,0.333333]
parameter data.getDebugX(): [0.666667,0.425926]
parameter incomingData.getDebugX(): [0.833333,0.333333]
parameter fromTree: 0
parameter fromStack: 34
parameter inOutStack: 2
parameter relativePositionOnInOutStack: 0
parameter marker.toString(): (x=[0.648148,0.425926],h=[0.037037,0.037037],select=2,is-cell-local=0)
parameter _spacetreeId: 2
peano4: observers/CreateGrid.cpp:1313: virtual void examples::exahype2::euler::observers::CreateGrid::receiveAndMergeFaceHorizontally(const peano4::grid::GridTraversalEvent&, int, int, int, int): Assertion `false' failed.
Aborted (core dumped)
  </pre>
     *
     * We run through this cell from left bottom to top right and this cell is
     * created while we span this whole third level from scratch. That is, all
     * grid elements are new. Indeed, only the bottom two vertices are unrefined,
     * as they have been created by another cell before (the one below), while
     * the top two vertices are new. The left top one has been created before
     * (through the left neighbour), so the right top vertex is the one which is
     * actually to be created by this event.
     *
     * The vertex indices in the output above are correct. Only one vertex (number
     * three) is to be created. The flags for the faces are however wrong: The
     * left and bottom one indeed are to be taken from temporary stacks, as we've
     * visited their other adjacent neighbour/cell before. But the top one is to
     * be created (correct) as well as the right one (wrong in dump above).
     *
     */
    void testCreateEnterCellTraversalEvent1();


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
    face number 1 on tree 1. The code yields a neighbour 0:
    *
    */
    void testGetAdjacentDomainIds();

    void testCreateEnterCellTraversalEvent2();

  public:
    SpacetreeTest();
    void run() override;
};


#endif
