#include "GridTraversalEventGeneratorTest.h"

#include "peano4/grid/FaceType.h"
#include "peano4/grid/GridTraversalEventGenerator.h"
#include "peano4/Globals.h"
#include "peano4/utils/Loop.h"
#include "tarch/la/Vector.h"


tarch::logging::Log peano4::grid::tests::GridTraversalEventGeneratorTest::_log(
  "peano4::grid::tests::GridTraversalEventGeneratorTest"
);

peano4::grid::tests::GridTraversalEventGeneratorTest::GridTraversalEventGeneratorTest():
  TestCase("peano4::grid::tests::GridTraversalEventGeneratorTest") {}

void peano4::grid::tests::GridTraversalEventGeneratorTest::testAreFacesLocal2() {
// #if DIMENSIONS == 2
//   GridTraversalEventGenerator generator0(0);
//   GridTraversalEventGenerator generator1(1);

//   GridVertex vertices[TWO_POWER_D];

//   vertices[0].setState(GridVertex::State::HangingVertex);
//   vertices[0].adjacentRanks         = {0, 1, 0, 1};
//   vertices[0].backupOfAdjacentRanks = {0, 1, 0, 1};

//   vertices[1].setState(GridVertex::State::Unrefined);
//   vertices[1].adjacentRanks         = {1, 1, 1, 1};
//   vertices[1].backupOfAdjacentRanks = {1, 1, 1, 1};

//   vertices[2].setState(GridVertex::State::HangingVertex);
//   vertices[2].adjacentRanks         = {0, 1, 0, 1};
//   vertices[2].backupOfAdjacentRanks = {0, 1, 0, 1};

//   vertices[3].setState(GridVertex::State::Unrefined);
//   vertices[3].adjacentRanks         = {1, 1, 1, 1};
//   vertices[3].backupOfAdjacentRanks = {1, 1, 1, 1};

//   std::bitset<TWO_TIMES_D> result0 = generator0.areFacesLocal(
//     vertices,
//     SplitSpecification(),
//     std::set<int>(),
//     std::set<int>(),
//     std::set<int>()
//   );

//   validateWithParams1(not result0[0], result0);
//   validateWithParams1(not result0[1], result0);
//   validateWithParams1(not result0[2], result0);
//   validateWithParams1(not result0[3], result0);

//   std::bitset<TWO_TIMES_D> result1 = generator1.areFacesLocal(
//     vertices,
//     SplitSpecification(),
//     std::set<int>(),
//     std::set<int>(),
//     std::set<int>()
//   );

//   validateWithParams1(result1[0], result1);
//   validateWithParams1(result1[1], result1);
//   validateWithParams1(result1[2], result1);
//   validateWithParams1(result1[3], result1);
// #endif
}


void peano4::grid::tests::GridTraversalEventGeneratorTest::testAreFacesLocal1() {
// #if DIMENSIONS == 2
//   GridTraversalEventGenerator generator(1);

//   GridTraversalEvent event;
//   GridVertex         vertices[TWO_POWER_D];

//   vertices[0].setState(GridVertex::State::Unrefined);
//   vertices[0].adjacentRanks         = {0, 0, 1, 1};
//   vertices[0].backupOfAdjacentRanks = {0, 0, 1, 1};
//   vertices[1].setState(GridVertex::State::Unrefined);
//   vertices[1].adjacentRanks         = {0, -1, 1, -1};
//   vertices[1].backupOfAdjacentRanks = {0, -1, 1, -1};
//   vertices[2].setState(GridVertex::State::Unrefined);
//   vertices[2].adjacentRanks         = {1, 1, -1, -1};
//   vertices[2].backupOfAdjacentRanks = {1, 1, -1, -1};
//   vertices[3].setState(GridVertex::State::Unrefined);
//   vertices[3].adjacentRanks         = {1, -1, -1, -1};
//   vertices[3].backupOfAdjacentRanks = {1, -1, -1, -1};

//   std::bitset<TWO_TIMES_D>
//     result = generator.areFacesLocal(vertices, SplitSpecification(), std::set<int>(), std::set<int>(), std::set<int>());

//   validateWithParams1(result[0], result);
//   validateWithParams1(result[1], result);
//   validateWithParams1(result[2], result);
//   validateWithParams1(result[3], result);
// #endif
}


void peano4::grid::tests::GridTraversalEventGeneratorTest::testGetFaceType() {
#if DIMENSIONS == 2
  // GridTraversalEventGenerator generator(2);

  // GridVertex coarseGridVertices[TWO_POWER_D];

  // coarseGridVertices[0].setState(GridVertex::State::Refining);
  // coarseGridVertices[1].setState(GridVertex::State::Refining);
  // coarseGridVertices[2].setState(GridVertex::State::Refining);
  // coarseGridVertices[3].setState(GridVertex::State::Refining);

  // auto result = getFaceType(coarseGridVertices, {2, 2}, 0);
  // validateWithParams1(result == FaceType::New, toString(result));

  // result = getFaceType(coarseGridVertices, {2, 2}, 1);
  // validateWithParams1(result == FaceType::New, toString(result));

  // result = getFaceType(coarseGridVertices, {2, 2}, 2);
  // validateWithParams1(result == FaceType::New, toString(result));

  // result = getFaceType(coarseGridVertices, {2, 2}, 3);
  // validateWithParams1(result == FaceType::New, toString(result));
#endif
}


void peano4::grid::tests::GridTraversalEventGeneratorTest::testCreateLeaveCellTraversalEvent1() {
  // GridVertex vertices[TWO_POWER_D];
  // dfor2(k) vertices[kScalar].setState(GridVertex::State::Refined);
  // vertices[kScalar].adjacentRanks = {-1};
  // vertices[kScalar].setFlag(GridVertex::HasBeenAntecessorOfRefinedVertexInPreviousTreeSweep, true);
  // vertices[kScalar].setFlag(GridVertex::IsAntecessorOfRefinedVertexInCurrentTreeSweep, true);
  // vertices[kScalar].level = 0;

  // AutomatonState(const int& level, const tarch::la::Vector<DIMENSIONS,double>& x, const
  // tarch::la::Vector<DIMENSIONS,double>& h, const bool& inverted, const std::bitset<DIMENSIONS>& evenFlags, const
  // tarch::la::Vector<TWO_TIMES_D,short int>& accessNumber);
  // AutomatonState state;
  // state.level = 0;
  // state.x     = {0.0};
  // state.h     = {1.0};
  // state.setInverted(false);

  /*
    observer.enterCell(createLeaveCellTraversalEvent(
      vertices, fineGridVertices, fineGridStates[peano4::utils::dLinearised(k,3)], k
    ));
  */
}


/*
void peano4::grid::tests::GridTraversalEventGeneratorTest::testCreateNeighbourExchangeLists() {
  #if DIMENSIONS==2
  GridTraversalEventGenerator generator(1);

  GridTraversalEvent event;
  GridVertex vertices[TWO_POWER_D];

  vertices[0].setState( GridVertex::State::Unrefined );
  vertices[0].setAdjacentRanks( { -1, 1, -1, 0 } );
  vertices[0].setBackupOfAdjacentRanks( { -1, 1, -1, 0 } );
  vertices[1].setState( GridVertex::State::Unrefined );
  vertices[1].setAdjacentRanks( { 1, 1, 0, 1 } );
  vertices[1].setBackupOfAdjacentRanks( { 1, 1, 0, 1 } );
  vertices[2].setState( GridVertex::State::Unrefined );
  vertices[2].setAdjacentRanks( { -1, 0, -1, 0 } );
  vertices[2].setBackupOfAdjacentRanks( { -1, 0, -1, 0 } );
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

  validateEqualsWithParams1( event.getExchangeFaceData(0), GridTraversalObserver::NoData, event.toString() );
  validateEqualsWithParams1( event.getExchangeFaceData(1), 1,                         event.toString() );
  validateEqualsWithParams1( event.getExchangeFaceData(2), 1,                         event.toString() );
  validateEqualsWithParams1( event.getExchangeFaceData(3), GridTraversalObserver::NoData, event.toString() );

  vertices[0].setState( GridVertex::State::Unrefined );
  vertices[0].setAdjacentRanks( { -1, 0, -1, 1 } );
  vertices[0].setBackupOfAdjacentRanks( { -1, 0, -1, 1 } );
  vertices[1].setState( GridVertex::State::Unrefined );
  vertices[1].setAdjacentRanks( { 0, 0, 1, 0 } );
  vertices[1].setBackupOfAdjacentRanks( { 0, 0, 1, 0 } );
  vertices[2].setState( GridVertex::State::Unrefined );
  vertices[2].setAdjacentRanks( { -1, 1, -1, 1 } );
  vertices[2].setBackupOfAdjacentRanks( { -1, 1, -1, 1 } );
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

  validateEqualsWithParams1( event.getExchangeFaceData(0), GridTraversalObserver::NoData, event.toString() );
  validateEqualsWithParams1( event.getExchangeFaceData(1), GridTraversalObserver::NoData, event.toString() );
  validateEqualsWithParams1( event.getExchangeFaceData(2), GridTraversalObserver::NoData, event.toString() );
  validateEqualsWithParams1( event.getExchangeFaceData(3), GridTraversalObserver::NoData, event.toString() );

  generator.createNeighbourExchangeLists( vertices, event, false );

  validateEqualsWithParams1( event.getExchangeFaceData(0), GridTraversalObserver::NoData, event.toString() );
  validateEqualsWithParams1( event.getExchangeFaceData(1), GridTraversalObserver::NoData, event.toString() );
  validateEqualsWithParams1( event.getExchangeFaceData(2), GridTraversalObserver::NoData, event.toString() );
  validateEqualsWithParams1( event.getExchangeFaceData(3), GridTraversalObserver::NoData, event.toString() );

  // enter cell; just triggered the split
  generator._splitTriggered.clear();
  generator._splitting.insert( 1 );
  generator.createNeighbourExchangeLists( vertices, event, true );

  validateEqualsWithParams1( event.getExchangeFaceData(0), GridTraversalObserver::NoData, event.toString() );
  validateEqualsWithParams1( event.getExchangeFaceData(1), GridTraversalObserver::NoData, event.toString() );
  validateEqualsWithParams1( event.getExchangeFaceData(2), GridTraversalObserver::NoData, event.toString() );
  validateEqualsWithParams1( event.getExchangeFaceData(3), GridTraversalObserver::NoData, event.toString() );

  generator.createNeighbourExchangeLists( vertices, event, false );

  validateEqualsWithParams1( event.getExchangeFaceData(0), GridTraversalObserver::NoData, event.toString() );
  validateEqualsWithParams1( event.getExchangeFaceData(1), 1,                         event.toString() );
  validateEqualsWithParams1( event.getExchangeFaceData(2), 1,                         event.toString() );
  validateEqualsWithParams1( event.getExchangeFaceData(3), GridTraversalObserver::NoData, event.toString() );
  #endif
}
*/


void peano4::grid::tests::GridTraversalEventGeneratorTest::testCreateEnterCellTraversalEvent1() {
  /*
    #if DIMENSIONS==2
    Spacetree tree( {0.0,0.0}, {1.0,1.0} );
    tree._id = 2;

    GridVertex coarseGridVertices[TWO_POWER_D];
    GridVertex fineGridVertices[TWO_POWER_D];

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
          std::bitset<DIMENSIONS>(3),   // even flags = 11
          {-1,-2,2,1}
    );
    tarch::la::Vector<DIMENSIONS,int> relativePositionToFather = {2,2};

    GridTraversalEvent event =
    tree.createEnterCellTraversalEvent(coarseGridVertices,fineGridVertices,state,relativePositionToFather);

    validateEqualsWithParams1( event.getFaceDataFrom(0),                                                      4,
    event.toString() ); validateEqualsWithParams1( event.getFaceDataFrom(1), 3, event.toString() );
    validateEqualsWithParams1( event.getFaceDataFrom(2), GridTraversalObserver::CreateOrDestroyPersistentGridEntity,
    event.toString() ); validateEqualsWithParams1( event.getFaceDataFrom(3),
    GridTraversalObserver::CreateOrDestroyPersistentGridEntity, event.toString() ); #endif
  */
}


void peano4::grid::tests::GridTraversalEventGeneratorTest::testCreateGenericCellTraversalEvent1() {
// #if DIMENSIONS == 2
//   GridTraversalEventGenerator generator(0);

//   GridVertex coarseGridVertices[TWO_POWER_D];
//   GridVertex fineGridVertices[TWO_POWER_D];

//   fineGridVertices[0].setState(GridVertex::State::HangingVertex);
//   fineGridVertices[0].adjacentRanks         = {0, 0, 0, 0};
//   fineGridVertices[0].backupOfAdjacentRanks = {0, 0, 0, 0};

//   fineGridVertices[1].setState(GridVertex::State::Unrefined);
//   fineGridVertices[1].adjacentRanks         = {0, 0, 0, 0};
//   fineGridVertices[1].backupOfAdjacentRanks = {0, 0, 0, 0};

//   fineGridVertices[2].setState(GridVertex::State::HangingVertex);
//   fineGridVertices[2].adjacentRanks         = {0, 0, 0, 0};
//   fineGridVertices[2].backupOfAdjacentRanks = {0, 0, 0, 0};

//   fineGridVertices[3].setState(GridVertex::State::Unrefined);
//   fineGridVertices[3].adjacentRanks         = {0, 0, 0, 0};
//   fineGridVertices[3].backupOfAdjacentRanks = {0, 0, 0, 0};

//   coarseGridVertices[0].setState(GridVertex::State::Unrefined);
//   coarseGridVertices[0].adjacentRanks         = {0, 0, 0, 0};
//   coarseGridVertices[0].backupOfAdjacentRanks = {0, 0, 0, 0};

//   coarseGridVertices[1].setState(GridVertex::State::Refined);
//   coarseGridVertices[1].adjacentRanks         = {0, 0, 0, 0};
//   coarseGridVertices[1].backupOfAdjacentRanks = {0, 0, 0, 0};

//   coarseGridVertices[2].setState(GridVertex::State::Unrefined);
//   coarseGridVertices[2].adjacentRanks         = {0, 0, 0, 0};
//   coarseGridVertices[2].backupOfAdjacentRanks = {0, 0, 0, 0};

//   coarseGridVertices[3].setState(GridVertex::State::Refined);
//   coarseGridVertices[3].adjacentRanks         = {0, 0, 0, 0};
//   coarseGridVertices[3].backupOfAdjacentRanks = {0, 0, 0, 0};

//   AutomatonState state;
//   state.level = 4;
//   state.x     = {0.37037, 0.358025};
//   state.h     = {0.0123457, 0.0123457};
//   state.setInverted(true);
//   state.setEvenFlag(0, false);
//   state.setEvenFlag(1, true);
//   state.accessNumber[0] = -1;
//   state.accessNumber[1] = 2;
//   state.accessNumber[2] = 1;
//   state.accessNumber[3] = 3;

//   tarch::la::Vector<DIMENSIONS, int> relativePositionToFather = {0, 2};

//   GridTraversalEvent event = generator.createGenericTraversalEvent(
//     coarseGridVertices,
//     fineGridVertices,
//     state,
//     SplitSpecification(),
//     std::set<int>(),
//     std::set<int>(),
//     std::set<int>(),
//     relativePositionToFather,
//     true
//   );

//   validateEqualsWithParams1(event.getIsVertexLocal(0), true, event.toString());
//   validateEqualsWithParams1(event.getIsVertexLocal(1), true, event.toString());
//   validateEqualsWithParams1(event.getIsVertexLocal(2), true, event.toString());
//   validateEqualsWithParams1(event.getIsVertexLocal(3), true, event.toString());

//   validateEqualsWithParams1(event.getIsFaceLocal(0), true, event.toString());
//   validateEqualsWithParams1(event.getIsFaceLocal(1), true, event.toString());
//   validateEqualsWithParams1(event.getIsFaceLocal(2), true, event.toString());
//   validateEqualsWithParams1(event.getIsFaceLocal(3), true, event.toString());
// #endif
}


void peano4::grid::tests::GridTraversalEventGeneratorTest::run() {
  testMethod(testCreateGenericCellTraversalEvent1);
  testMethod(testAreFacesLocal1);
  testMethod(testAreFacesLocal2);
  testMethod(testGetFaceType);
  testMethod(testCreateEnterCellTraversalEvent1);
  testMethod(testCreateLeaveCellTraversalEvent1);
}
