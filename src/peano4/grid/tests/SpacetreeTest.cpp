#include "SpacetreeTest.h"
#include "../Spacetree.h"
#include "../GridTraversalEvent.h"
#include "../TraversalObserver.h"

#include "peano4/utils/Globals.h"
#include "peano4/utils/Loop.h"

#include "tarch/la/Vector.h"


tarch::logging::Log peano4::grid::tests::SpacetreeTest::_log("peano4::grid::tests::SpacetreeTest");


#ifdef UseTestSpecificCompilerSettings
#pragma optimize("",off)
#endif


peano4::grid::tests::SpacetreeTest::SpacetreeTest():
  TestCase( "peano4::grid::tests::SpacetreeTest" ) {
}


void peano4::grid::tests::SpacetreeTest::testRestrictToCoarseGrid() {
  #if Dimensions==2
  tarch::la::Vector<Dimensions,int>  positionInPatch;
  tarch::la::Vector<Dimensions,int>  coarsePosition;

  positionInPatch = {0,0};
  coarsePosition  = {0,0};
  validateWithParams2( Spacetree::restrictToCoarseGrid(coarsePosition,positionInPatch), coarsePosition, positionInPatch );

  positionInPatch = {0,3};
  coarsePosition  = {0,1};
  validateWithParams2( Spacetree::restrictToCoarseGrid(coarsePosition,positionInPatch), coarsePosition, positionInPatch );

  positionInPatch = {3,3};
  coarsePosition  = {1,1};
  validateWithParams2( Spacetree::restrictToCoarseGrid(coarsePosition,positionInPatch), coarsePosition, positionInPatch );

  positionInPatch = {2,2};
  coarsePosition  = {1,1};
  validateWithParams2( Spacetree::restrictToCoarseGrid(coarsePosition,positionInPatch), coarsePosition, positionInPatch );

  positionInPatch = {2,3};
  coarsePosition  = {1,1};
  validateWithParams2( Spacetree::restrictToCoarseGrid(coarsePosition,positionInPatch), coarsePosition, positionInPatch );

  positionInPatch = {0,2};
  coarsePosition  = {1,1};
  validateWithParams2( !Spacetree::restrictToCoarseGrid(coarsePosition,positionInPatch), coarsePosition, positionInPatch );

  positionInPatch = {0,2};
  coarsePosition  = {0,0};
  validateWithParams2( Spacetree::restrictToCoarseGrid(coarsePosition,positionInPatch), coarsePosition, positionInPatch );
  #endif
}


void peano4::grid::tests::SpacetreeTest::testGetAdjacentDomainIds() {
  #if Dimensions==2
  Spacetree tree( {0.0,0.0}, {1.0,1.0} );
  tree._id = 1;

  GridVertex fineGridVertices[TwoPowerD];

  fineGridVertices[0].setState( GridVertex::State::New );
  fineGridVertices[0].setAdjacentRanks( {1,1,1,1} );
  fineGridVertices[0].setBackupOfAdjacentRanks( {0,0,0,0} );

  fineGridVertices[1].setState( GridVertex::State::Unrefined );
  fineGridVertices[1].setAdjacentRanks( {1,1,1,1} );
  fineGridVertices[1].setBackupOfAdjacentRanks( {1,1,1,1} );

  fineGridVertices[2].setState( GridVertex::State::Unrefined );
  fineGridVertices[2].setAdjacentRanks( {1,1,1,1} );
  fineGridVertices[2].setBackupOfAdjacentRanks( {1,1,1,1} );

  fineGridVertices[3].setState( GridVertex::State::Unrefined );
  fineGridVertices[3].setAdjacentRanks( {1,1,1,1} );
  fineGridVertices[3].setBackupOfAdjacentRanks( {1,1,1,1} );

/*
  vertex.toString():(state=New,      adjacentRanks=[1,1,1,1],backupOfAdjacentRanks=[0,0,0,0],hasBeenAntecessorOfRefinedVertexInPreviousTreeSweep=0,isAntecessorOfRefinedVertexInCurrentTreeSweep=0,numberOfAdjacentRefinedLocalCells=1684632135,x=[0.777778,0.444444],level=3),splittingIsConsideredLocal:1,joiningIsConsideredLocal:1 (file:peano4/grid/Spacetree.cpp,line:95)
  vertex.toString():(state=Unrefined,adjacentRanks=[1,1,1,1],backupOfAdjacentRanks=[1,1,1,1],hasBeenAntecessorOfRefinedVertexInPreviousTreeSweep=0,isAntecessorOfRefinedVertexInCurrentTreeSweep=0,numberOfAdjacentRefinedLocalCells=0,         x=[0.814815,0.444444],level=3),splittingIsConsideredLocal:1,joiningIsConsideredLocal:1 (file:peano4/grid/Spacetree.cpp,line:95)
  vertex.toString():(state=Unrefined,adjacentRanks=[1,1,1,1],backupOfAdjacentRanks=[1,1,1,1],hasBeenAntecessorOfRefinedVertexInPreviousTreeSweep=0,isAntecessorOfRefinedVertexInCurrentTreeSweep=0,numberOfAdjacentRefinedLocalCells=0,         x=[0.777778,0.481481],level=3),splittingIsConsideredLocal:1,joiningIsConsideredLocal:0 (file:peano4/grid/Spacetree.cpp,line:95)
  vertex.toString():(state=Unrefined,adjacentRanks=[1,1,1,1],backupOfAdjacentRanks=[1,1,1,1],hasBeenAntecessorOfRefinedVertexInPreviousTreeSweep=0,isAntecessorOfRefinedVertexInCurrentTreeSweep=0,numberOfAdjacentRefinedLocalCells=0,         x=[0.814815,0.481481],level=3),splittingIsConsideredLocal:1,joiningIsConsideredLocal:1 (file:peano4/grid/Spacetree.cpp,line:95)
*/

  int result = tree.getNeighbourTrees( fineGridVertices, 0, true );
  validateEquals( result, -1 );

  result = tree.getNeighbourTrees( fineGridVertices, 1, true );
  validateEquals( result, -1 );

  result = tree.getNeighbourTrees( fineGridVertices, 2, true );
  validateEquals( result, -1 );

  result = tree.getNeighbourTrees( fineGridVertices, 3, true );
  validateEquals( result, -1 );
  #endif
}


void peano4::grid::tests::SpacetreeTest::run() {
  testMethod( testRestrictToCoarseGrid );
  testMethod( testGetAdjacentDomainIds );
}


#ifdef UseTestSpecificCompilerSettings
#pragma optimize("",on)
#endif
