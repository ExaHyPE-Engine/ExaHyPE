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


void peano4::grid::tests::SpacetreeTest::testCreateLeaveCellTraversalEvent() {
  GridVertex vertices[TwoPowerD];
  dfor2(k)
    vertices[kScalar].setState( GridVertex::State::Refined );
    vertices[kScalar].setAdjacentRanks( tarch::la::Vector<TwoPowerD,int>(peano4::grid::Spacetree::InvalidRank) );
    vertices[kScalar].setHasBeenAntecessorOfRefinedVertexInPreviousTreeSweep(true);
    vertices[kScalar].setIsAntecessorOfRefinedVertexInCurrentTreeSweep(true);
    vertices[kScalar].setX(tarch::la::convertScalar<double>(k));
    vertices[kScalar].setLevel(0);
  }

  // AutomatonState(const int& level, const tarch::la::Vector<Dimensions,double>& x, const tarch::la::Vector<Dimensions,double>& h, const bool& inverted, const std::bitset<Dimensions>& evenFlags, const tarch::la::Vector<DimensionsTimesTwo,short int>& accessNumber);
  AutomatonState  state;
  state.setLevel( 0 );
  state.setX( 0.0 );
  state.setH( 1.0 );
  state.setInverted( false );
  state.setEvenFlags( 0 );

/*
  observer.enterCell(createLeaveCellTraversalEvent(
    vertices, fineGridVertices, fineGridStates[peano4::utils::dLinearised(k,3)], k
  ));
*/
}


void peano4::grid::tests::SpacetreeTest::testAreFacesLocal() {
  #if Dimensions==2
  Spacetree tree( {0.0,0.0}, {1.0,1.0} );
  tree._id = 1;

  GridTraversalEvent event;
  GridVertex vertices[TwoPowerD];

  vertices[0].setState( GridVertex::State::Unrefined );
  vertices[0].setAdjacentRanks( { 0, 0, 1, 1 } );
  vertices[0].setBackupOfAdjacentRanks( { 0, 0, 1, 1 } );
  vertices[1].setState( GridVertex::State::Unrefined );
  vertices[1].setAdjacentRanks( { 0, Spacetree::InvalidRank, 1, Spacetree::InvalidRank } );
  vertices[1].setBackupOfAdjacentRanks( { 0, Spacetree::InvalidRank, 1, Spacetree::InvalidRank } );
  vertices[2].setState( GridVertex::State::Unrefined );
  vertices[2].setAdjacentRanks( { 1, 1, Spacetree::InvalidRank, Spacetree::InvalidRank } );
  vertices[2].setBackupOfAdjacentRanks( { 1, 1, Spacetree::InvalidRank, Spacetree::InvalidRank } );
  vertices[3].setState( GridVertex::State::Unrefined );
  vertices[3].setAdjacentRanks( { 1, Spacetree::InvalidRank, Spacetree::InvalidRank, Spacetree::InvalidRank } );
  vertices[3].setBackupOfAdjacentRanks( { 1, Spacetree::InvalidRank, Spacetree::InvalidRank, Spacetree::InvalidRank } );

  std::bitset<TwoTimesD>  result = tree.areFacesLocal( vertices );

  validateWithParams1( result[0], result );
  validateWithParams1( result[1], result );
  validateWithParams1( result[2], result );
  validateWithParams1( result[3], result );
#endif
}


void peano4::grid::tests::SpacetreeTest::testCreateNeighbourExchangeLists() {
  #if Dimensions==2
/*
  Spacetree tree( {0.0,0.0}, {1.0,1.0} );

  GridTraversalEvent event;
  GridVertex vertices[TwoPowerD];

  vertices[0].setState( GridVertex::State::Unrefined );
  vertices[0].setAdjacentRanks( { Spacetree::InvalidRank, 1, Spacetree::InvalidRank, 0 } );
  vertices[0].setBackupOfAdjacentRanks( { Spacetree::InvalidRank, 1, Spacetree::InvalidRank, 0 } );
  vertices[1].setState( GridVertex::State::Unrefined );
  vertices[1].setAdjacentRanks( { 1, 1, 0, 1 } );
  vertices[1].setBackupOfAdjacentRanks( { 1, 1, 0, 1 } );
  vertices[2].setState( GridVertex::State::Unrefined );
  vertices[2].setAdjacentRanks( { Spacetree::InvalidRank, 0, Spacetree::InvalidRank, 0 } );
  vertices[2].setBackupOfAdjacentRanks( { Spacetree::InvalidRank, 0, Spacetree::InvalidRank, 0 } );
  vertices[3].setState( GridVertex::State::Unrefined );
  vertices[3].setAdjacentRanks( { 0, 1, 0, 0 } );
  vertices[3].setBackupOfAdjacentRanks( { 0, 1, 0, 0 } );

  event.setIsVertexLocal( 0, true );
  event.setIsVertexLocal( 1, true );
  event.setIsVertexLocal( 2, true );
  event.setIsVertexLocal( 3, true );
  event.setIsFaceLocal( 0, true );
  event.setIsFaceLocal( 1, true );
  event.setIsFaceLocal( 2, true );
  event.setIsFaceLocal( 3, true );

  // enter cell; no load balancing going on
  tree.createNeighbourExchangeLists( vertices, event, true );

  validateEqualsWithParams1( event.getExchangeFaceData(0), TraversalObserver::NoData, event.toString() );
  validateEqualsWithParams1( event.getExchangeFaceData(1), 1,                         event.toString() );
  validateEqualsWithParams1( event.getExchangeFaceData(2), 1,                         event.toString() );
  validateEqualsWithParams1( event.getExchangeFaceData(3), TraversalObserver::NoData, event.toString() );

  vertices[0].setState( GridVertex::State::Unrefined );
  vertices[0].setAdjacentRanks( { Spacetree::InvalidRank, 0, Spacetree::InvalidRank, 1 } );
  vertices[0].setBackupOfAdjacentRanks( { Spacetree::InvalidRank, 0, Spacetree::InvalidRank, 1 } );
  vertices[1].setState( GridVertex::State::Unrefined );
  vertices[1].setAdjacentRanks( { 0, 0, 1, 0 } );
  vertices[1].setBackupOfAdjacentRanks( { 0, 0, 1, 0 } );
  vertices[2].setState( GridVertex::State::Unrefined );
  vertices[2].setAdjacentRanks( { Spacetree::InvalidRank, 1, Spacetree::InvalidRank, 1 } );
  vertices[2].setBackupOfAdjacentRanks( { Spacetree::InvalidRank, 1, Spacetree::InvalidRank, 1 } );
  vertices[3].setState( GridVertex::State::Unrefined );
  vertices[3].setAdjacentRanks( { 1, 0, 1, 1 } );
  vertices[3].setBackupOfAdjacentRanks( { 1, 0, 1, 1 } );

  event.setIsVertexLocal( 0, true );
  event.setIsVertexLocal( 1, true );
  event.setIsVertexLocal( 2, false );
  event.setIsVertexLocal( 3, true );
  event.setIsFaceLocal( 0, false );
  event.setIsFaceLocal( 1, true );
  event.setIsFaceLocal( 2, true );
  event.setIsFaceLocal( 3, false );

  // enter cell; just triggered the split
  tree._splitTriggered.insert( std::pair<int,int>(1,1) );
  tree.createNeighbourExchangeLists( vertices, event, true );

  validateEqualsWithParams1( event.getExchangeFaceData(0), TraversalObserver::NoData, event.toString() );
  validateEqualsWithParams1( event.getExchangeFaceData(1), TraversalObserver::NoData, event.toString() );
  validateEqualsWithParams1( event.getExchangeFaceData(2), TraversalObserver::NoData, event.toString() );
  validateEqualsWithParams1( event.getExchangeFaceData(3), TraversalObserver::NoData, event.toString() );

  tree.createNeighbourExchangeLists( vertices, event, false );

  validateEqualsWithParams1( event.getExchangeFaceData(0), TraversalObserver::NoData, event.toString() );
  validateEqualsWithParams1( event.getExchangeFaceData(1), TraversalObserver::NoData, event.toString() );
  validateEqualsWithParams1( event.getExchangeFaceData(2), TraversalObserver::NoData, event.toString() );
  validateEqualsWithParams1( event.getExchangeFaceData(3), TraversalObserver::NoData, event.toString() );

  // enter cell; just triggered the split
  tree._splitTriggered.clear();
  tree._splitting.insert( 1 );
  tree.createNeighbourExchangeLists( vertices, event, true );

  validateEqualsWithParams1( event.getExchangeFaceData(0), TraversalObserver::NoData, event.toString() );
  validateEqualsWithParams1( event.getExchangeFaceData(1), TraversalObserver::NoData, event.toString() );
  validateEqualsWithParams1( event.getExchangeFaceData(2), TraversalObserver::NoData, event.toString() );
  validateEqualsWithParams1( event.getExchangeFaceData(3), TraversalObserver::NoData, event.toString() );

  tree.createNeighbourExchangeLists( vertices, event, false );

  validateEqualsWithParams1( event.getExchangeFaceData(0), TraversalObserver::NoData, event.toString() );
  validateEqualsWithParams1( event.getExchangeFaceData(1), 1,                         event.toString() );
  validateEqualsWithParams1( event.getExchangeFaceData(2), 1,                         event.toString() );
  validateEqualsWithParams1( event.getExchangeFaceData(3), TraversalObserver::NoData, event.toString() );
*/

#endif
}


void peano4::grid::tests::SpacetreeTest::testCreateEnterCellTraversalEvent1() {
  #if Dimensions==2
  Spacetree tree( {0.0,0.0}, {1.0,1.0} );
  tree._id = 2;

  GridVertex coarseGridVertices[TwoPowerD];
  GridVertex fineGridVertices[TwoPowerD];

  coarseGridVertices[0].setState( GridVertex::State::Refined );
  coarseGridVertices[1].setState( GridVertex::State::Refined );
  coarseGridVertices[2].setState( GridVertex::State::Refined );
  coarseGridVertices[3].setState( GridVertex::State::Refined );

  fineGridVertices[0].setState( GridVertex::State::Unrefined );
  fineGridVertices[0].setAdjacentRanks( { 3, 3, 3, 3 } );
  fineGridVertices[0].setBackupOfAdjacentRanks( { 3, 3, 3, 3 } );
  fineGridVertices[0].setX( {0.62963,0.407407} );

  fineGridVertices[1].setState( GridVertex::State::Unrefined );
  fineGridVertices[1].setAdjacentRanks( { 3, 2, 3, 2 } );
  fineGridVertices[1].setBackupOfAdjacentRanks( { 3, 2, 3, 2 } );
  fineGridVertices[1].setX( {0.666667,0.407407} );

  fineGridVertices[2].setState( GridVertex::State::New );
  fineGridVertices[2].setAdjacentRanks( { 3, 3, 3, 3 } );
  fineGridVertices[2].setBackupOfAdjacentRanks( { 0, 0, 0, 0 } );
  fineGridVertices[2].setX( {0.62963,0.444444} );

  fineGridVertices[3].setState( GridVertex::State::New );
  fineGridVertices[3].setAdjacentRanks( { 3, 2, 3, 2 } );
  fineGridVertices[3].setBackupOfAdjacentRanks( { 0, 0, 0, 0 } );
  fineGridVertices[3].setX( {0.666667,0.444444} );

  coarseGridVertices[0].setState( GridVertex::State::Refining );
  coarseGridVertices[0].setAdjacentRanks( { 3, 3, 3, 3 } );

  coarseGridVertices[1].setState( GridVertex::State::Refining );
  coarseGridVertices[1].setAdjacentRanks( { 3, 2, 3, 2 } );

  coarseGridVertices[2].setState( GridVertex::State::Refining );
  coarseGridVertices[2].setAdjacentRanks( { 3, 3, 3, 3 } );

  coarseGridVertices[3].setState( GridVertex::State::Refining );
  coarseGridVertices[3].setAdjacentRanks( { 3, 2, 3, 2 } );

  AutomatonState state(
    3,                            // level,
	{0.62963,0.407407},           // x
	{0.037037,0.037037},          // h
	true,                         // inverted
	std::bitset<Dimensions>(3),   // even flags = 11
	{-1,-2,2,1}
  );
  tarch::la::Vector<Dimensions,int> relativePositionToFather = {2,2};

  GridTraversalEvent event = tree.createEnterCellTraversalEvent(coarseGridVertices,fineGridVertices,state,relativePositionToFather);

  validateEqualsWithParams1( event.getFaceDataFrom(0),                                                      4, event.toString() );
  validateEqualsWithParams1( event.getFaceDataFrom(1),                                                      3, event.toString() );
  validateEqualsWithParams1( event.getFaceDataFrom(2), TraversalObserver::CreateOrDestroyPersistentGridEntity, event.toString() );
  validateEqualsWithParams1( event.getFaceDataFrom(3), TraversalObserver::CreateOrDestroyPersistentGridEntity, event.toString() );
  #endif
}


void peano4::grid::tests::SpacetreeTest::testGetFaceType() {
  #if Dimensions==2
  Spacetree tree( {0.0,0.0}, {1.0,1.0} );
  tree._id = 2;

  GridVertex coarseGridVertices[TwoPowerD];

  coarseGridVertices[0].setState( GridVertex::State::Refining );
  coarseGridVertices[1].setState( GridVertex::State::Refining );
  coarseGridVertices[2].setState( GridVertex::State::Refining );
  coarseGridVertices[3].setState( GridVertex::State::Refining );

  auto result = tree.getFaceType( coarseGridVertices, {2,2}, 0 );
  validateWithParams1( result==Spacetree::FaceType::New, Spacetree::toString(result) );

  result = tree.getFaceType( coarseGridVertices, {2,2}, 1 );
  validateWithParams1( result==Spacetree::FaceType::New, Spacetree::toString(result) );

  result = tree.getFaceType( coarseGridVertices, {2,2}, 2 );
  validateWithParams1( result==Spacetree::FaceType::New, Spacetree::toString(result) );

  result = tree.getFaceType( coarseGridVertices, {2,2}, 3 );
  validateWithParams1( result==Spacetree::FaceType::New, Spacetree::toString(result) );
  #endif
}


void peano4::grid::tests::SpacetreeTest::testCreateEnterCellTraversalEvent2() {
  #if Dimensions==2
  Spacetree tree( {0.0,0.0}, {1.0,1.0} );
  tree._id = 2;

  GridVertex coarseGridVertices[TwoPowerD];
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
  testMethod( testCreateLeaveCellTraversalEvent );
  testMethod( testCreateNeighbourExchangeLists );
  testMethod( testAreFacesLocal );
  testMethod( testGetFaceType );
  testMethod( testCreateEnterCellTraversalEvent1 );
  testMethod( testCreateEnterCellTraversalEvent2 );
  testMethod( testGetAdjacentDomainIds );
}


#ifdef UseTestSpecificCompilerSettings
#pragma optimize("",on)
#endif
