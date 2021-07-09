#include "GridTraversalEventGeneratorTest.h"
#include "../GridTraversalEventGenerator.h"

#include "peano4/utils/Globals.h"
#include "peano4/utils/Loop.h"

#include "tarch/la/Vector.h"


tarch::logging::Log peano4::grid::tests::GridTraversalEventGeneratorTest::_log("peano4::grid::tests::GridTraversalEventGeneratorTest");


#ifdef UseTestSpecificCompilerSettings
#pragma optimize("",off)
#endif


peano4::grid::tests::GridTraversalEventGeneratorTest::GridTraversalEventGeneratorTest():
  TestCase( "peano4::grid::tests::GridTraversalEventGeneratorTest" ) {
}



void peano4::grid::tests::GridTraversalEventGeneratorTest::testAreFacesLocal2() {
  #if Dimensions==2
  GridTraversalEventGenerator generator0(0);

  GridVertex  vertices[TwoPowerD];

  vertices[0].setState( GridVertex::State::HangingVertex );
  vertices[0].setAdjacentRanks( { 0, 1, 0, 1 } );
  vertices[0].setBackupOfAdjacentRanks( { 0, 1, 0, 1 } );

  vertices[1].setState( GridVertex::State::Unrefined );
  vertices[1].setAdjacentRanks( { 1, 1, 1, 1 } );
  vertices[1].setBackupOfAdjacentRanks( { 1, 1, 1, 1 } );

  vertices[0].setState( GridVertex::State::HangingVertex );
  vertices[0].setAdjacentRanks( { 0, 1, 0, 1 } );
  vertices[0].setBackupOfAdjacentRanks( { 0, 1, 0, 1 } );

  vertices[0].setState( GridVertex::State::Unrefined );
  vertices[0].setAdjacentRanks( { 1, 1, 1, 1 } );
  vertices[0].setBackupOfAdjacentRanks( { 1, 1, 1, 1 } );

  std::bitset<TwoTimesD> result0 = generator0.areFacesLocal(
    vertices,
    SplitSpecification(),
    std::set<int>(),
    std::set<int>(),
    std::set<int>()
  );

  validateWithParams1(     result0[0], result );
  validateWithParams1( not result0[1], result );
  validateWithParams1( not result0[2], result );
  validateWithParams1( not result0[3], result );

  std::bitset<TwoTimesD> result1 = generator1.areFacesLocal(
    vertices,
    SplitSpecification(),
    std::set<int>(),
    std::set<int>(),
    std::set<int>()
  );

  validateWithParams1( not result1[0], result );
  validateWithParams1(     result1[1], result );
  validateWithParams1(     result1[2], result );
  validateWithParams1(     result1[3], result );
  #endif
}


void peano4::grid::tests::GridTraversalEventGeneratorTest::testAreFacesLocal1() {
  #if Dimensions==2
  GridTraversalEventGenerator generator(1);

  GridTraversalEvent event;
  GridVertex vertices[TwoPowerD];

  vertices[0].setState( GridVertex::State::Unrefined );
  vertices[0].setAdjacentRanks( { 0, 0, 1, 1 } );
  vertices[0].setBackupOfAdjacentRanks( { 0, 0, 1, 1 } );
  vertices[1].setState( GridVertex::State::Unrefined );
  vertices[1].setAdjacentRanks( { 0, InvalidRank, 1, InvalidRank } );
  vertices[1].setBackupOfAdjacentRanks( { 0, InvalidRank, 1, InvalidRank } );
  vertices[2].setState( GridVertex::State::Unrefined );
  vertices[2].setAdjacentRanks( { 1, 1, InvalidRank, InvalidRank } );
  vertices[2].setBackupOfAdjacentRanks( { 1, 1, InvalidRank, InvalidRank } );
  vertices[3].setState( GridVertex::State::Unrefined );
  vertices[3].setAdjacentRanks( { 1, InvalidRank, InvalidRank, InvalidRank } );
  vertices[3].setBackupOfAdjacentRanks( { 1, InvalidRank, InvalidRank, InvalidRank } );

  std::bitset<TwoTimesD>  result = generator.areFacesLocal( vertices, SplitSpecification(), std::set<int>(), std::set<int>(), std::set<int>() );

  validateWithParams1( result[0], result );
  validateWithParams1( result[1], result );
  validateWithParams1( result[2], result );
  validateWithParams1( result[3], result );
  #endif
}


void peano4::grid::tests::GridTraversalEventGeneratorTest::testGetFaceType() {
  #if Dimensions==2
  GridTraversalEventGenerator generator( 2 );

  GridVertex coarseGridVertices[TwoPowerD];

  coarseGridVertices[0].setState( GridVertex::State::Refining );
  coarseGridVertices[1].setState( GridVertex::State::Refining );
  coarseGridVertices[2].setState( GridVertex::State::Refining );
  coarseGridVertices[3].setState( GridVertex::State::Refining );

  auto result = generator.getFaceType( coarseGridVertices, {2,2}, 0 );
  validateWithParams1( result==FaceType::New, toString(result) );

  result = generator.getFaceType( coarseGridVertices, {2,2}, 1 );
  validateWithParams1( result==FaceType::New, toString(result) );

  result = generator.getFaceType( coarseGridVertices, {2,2}, 2 );
  validateWithParams1( result==FaceType::New, toString(result) );

  result = generator.getFaceType( coarseGridVertices, {2,2}, 3 );
  validateWithParams1( result==FaceType::New, toString(result) );
  #endif
}


void peano4::grid::tests::GridTraversalEventGeneratorTest::testCreateLeaveCellTraversalEvent1() {
  GridVertex vertices[TwoPowerD];
  dfor2(k)
    vertices[kScalar].setState( GridVertex::State::Refined );
    vertices[kScalar].setAdjacentRanks( tarch::la::Vector<TwoPowerD,int>(peano4::grid::InvalidRank) );
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


/*
void peano4::grid::tests::GridTraversalEventGeneratorTest::testCreateNeighbourExchangeLists() {
  #if Dimensions==2
  GridTraversalEventGenerator generator(1);

  GridTraversalEvent event;
  GridVertex vertices[TwoPowerD];

  vertices[0].setState( GridVertex::State::Unrefined );
  vertices[0].setAdjacentRanks( { InvalidRank, 1, InvalidRank, 0 } );
  vertices[0].setBackupOfAdjacentRanks( { InvalidRank, 1, InvalidRank, 0 } );
  vertices[1].setState( GridVertex::State::Unrefined );
  vertices[1].setAdjacentRanks( { 1, 1, 0, 1 } );
  vertices[1].setBackupOfAdjacentRanks( { 1, 1, 0, 1 } );
  vertices[2].setState( GridVertex::State::Unrefined );
  vertices[2].setAdjacentRanks( { InvalidRank, 0, InvalidRank, 0 } );
  vertices[2].setBackupOfAdjacentRanks( { InvalidRank, 0, InvalidRank, 0 } );
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
  generator.createNeighbourExchangeLists( vertices, event, true );

  validateEqualsWithParams1( event.getExchangeFaceData(0), TraversalObserver::NoData, event.toString() );
  validateEqualsWithParams1( event.getExchangeFaceData(1), 1,                         event.toString() );
  validateEqualsWithParams1( event.getExchangeFaceData(2), 1,                         event.toString() );
  validateEqualsWithParams1( event.getExchangeFaceData(3), TraversalObserver::NoData, event.toString() );

  vertices[0].setState( GridVertex::State::Unrefined );
  vertices[0].setAdjacentRanks( { InvalidRank, 0, InvalidRank, 1 } );
  vertices[0].setBackupOfAdjacentRanks( { InvalidRank, 0, InvalidRank, 1 } );
  vertices[1].setState( GridVertex::State::Unrefined );
  vertices[1].setAdjacentRanks( { 0, 0, 1, 0 } );
  vertices[1].setBackupOfAdjacentRanks( { 0, 0, 1, 0 } );
  vertices[2].setState( GridVertex::State::Unrefined );
  vertices[2].setAdjacentRanks( { InvalidRank, 1, InvalidRank, 1 } );
  vertices[2].setBackupOfAdjacentRanks( { InvalidRank, 1, InvalidRank, 1 } );
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
  generator._splitTriggered.insert( std::pair<int,int>(1,1) );
  generator.createNeighbourExchangeLists( vertices, event, true );

  validateEqualsWithParams1( event.getExchangeFaceData(0), TraversalObserver::NoData, event.toString() );
  validateEqualsWithParams1( event.getExchangeFaceData(1), TraversalObserver::NoData, event.toString() );
  validateEqualsWithParams1( event.getExchangeFaceData(2), TraversalObserver::NoData, event.toString() );
  validateEqualsWithParams1( event.getExchangeFaceData(3), TraversalObserver::NoData, event.toString() );

  generator.createNeighbourExchangeLists( vertices, event, false );

  validateEqualsWithParams1( event.getExchangeFaceData(0), TraversalObserver::NoData, event.toString() );
  validateEqualsWithParams1( event.getExchangeFaceData(1), TraversalObserver::NoData, event.toString() );
  validateEqualsWithParams1( event.getExchangeFaceData(2), TraversalObserver::NoData, event.toString() );
  validateEqualsWithParams1( event.getExchangeFaceData(3), TraversalObserver::NoData, event.toString() );

  // enter cell; just triggered the split
  generator._splitTriggered.clear();
  generator._splitting.insert( 1 );
  generator.createNeighbourExchangeLists( vertices, event, true );

  validateEqualsWithParams1( event.getExchangeFaceData(0), TraversalObserver::NoData, event.toString() );
  validateEqualsWithParams1( event.getExchangeFaceData(1), TraversalObserver::NoData, event.toString() );
  validateEqualsWithParams1( event.getExchangeFaceData(2), TraversalObserver::NoData, event.toString() );
  validateEqualsWithParams1( event.getExchangeFaceData(3), TraversalObserver::NoData, event.toString() );

  generator.createNeighbourExchangeLists( vertices, event, false );

  validateEqualsWithParams1( event.getExchangeFaceData(0), TraversalObserver::NoData, event.toString() );
  validateEqualsWithParams1( event.getExchangeFaceData(1), 1,                         event.toString() );
  validateEqualsWithParams1( event.getExchangeFaceData(2), 1,                         event.toString() );
  validateEqualsWithParams1( event.getExchangeFaceData(3), TraversalObserver::NoData, event.toString() );
  #endif
}
*/


void peano4::grid::tests::GridTraversalEventGeneratorTest::testCreateEnterCellTraversalEvent1() {
/*
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
*/
}


void peano4::grid::tests::GridTraversalEventGeneratorTest::testCreateEnterCellTraversalEvent2() {
  #if Dimensions==2
  #endif
}


void peano4::grid::tests::GridTraversalEventGeneratorTest::run() {
  testMethod( testAreFacesLocal1 );
  testMethod( testAreFacesLocal2 );
  testMethod( testGetFaceType );
  testMethod( testCreateEnterCellTraversalEvent1 );
  testMethod( testCreateEnterCellTraversalEvent2 );
  testMethod( testCreateLeaveCellTraversalEvent1 );
}


#ifdef UseTestSpecificCompilerSettings
#pragma optimize("",on)
#endif

