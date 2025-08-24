#include "STDVectorStackOverObjectsWithSmartPointersTest.h"
#include "GeneratedTestObject.h"


#include "peano4/stacks/STDVectorStackOverObjectsWithSmartPointers.h"


tarch::logging::Log peano4::stacks::tests::STDVectorStackOverObjectsWithSmartPointersTest::_log("peano4::stacks::tests::STDVectorStackOverObjectsWithSmartPointersTest");


#ifdef UseTestSpecificCompilerSettings
#pragma optimize("",off)
#endif


peano4::stacks::tests::STDVectorStackOverObjectsWithSmartPointersTest::STDVectorStackOverObjectsWithSmartPointersTest():
  TestCase( "peano4::stacks::tests::STDVectorStackOverObjectsWithSmartPointersTest" ) {
}


void peano4::stacks::tests::STDVectorStackOverObjectsWithSmartPointersTest::testFlattening() {
  GeneratedTestObject baselineObject;

  baselineObject.setId(23);
  baselineObject.setData(0,  0);
  baselineObject.setData(1, -1);
  baselineObject.setData(2, -2);
  baselineObject.setData(3, -3);

  GeneratedTestObject_Flattened flattenedObject = GeneratedTestObject::flatten(baselineObject);

  validateEquals( flattenedObject.getId(), baselineObject.getId() );
  validateEquals( flattenedObject.getData(0), baselineObject.getData(0) );
  validateEquals( flattenedObject.getData(1), baselineObject.getData(1) );
  validateEquals( flattenedObject.getData(2), baselineObject.getData(2) );
  validateEquals( flattenedObject.getData(3), baselineObject.getData(3) );

  GeneratedTestObject reconstructedObject = GeneratedTestObject::unflatten(flattenedObject);

  validateEquals( reconstructedObject.getId(), baselineObject.getId() );
  validateEquals( reconstructedObject.getData(0), baselineObject.getData(0) );
  validateEquals( reconstructedObject.getData(1), baselineObject.getData(1) );
  validateEquals( reconstructedObject.getData(2), baselineObject.getData(2) );
  validateEquals( reconstructedObject.getData(3), baselineObject.getData(3) );
}


void peano4::stacks::tests::STDVectorStackOverObjectsWithSmartPointersTest::testMPIExchangeOfFlattenedObject() {
  #if defined(Parallel)
  GeneratedTestObject_Flattened testObject;

  GeneratedTestObject_Flattened::initDatatype();

  const int Tag = 12;

  testObject.setId(23);
  testObject.setData(0,  0);
  testObject.setData(1, -1);
  testObject.setData(2, -2);
  testObject.setData(3, -3);

  if ( tarch::mpi::Rank::getInstance().getNumberOfRanks()>=2 and tarch::mpi::Rank::getInstance().getRank()==0) {
    GeneratedTestObject_Flattened::send(testObject, 1, Tag, tarch::mpi::Rank::getInstance().getCommunicator());
  }
  if ( tarch::mpi::Rank::getInstance().getNumberOfRanks()>=2 and tarch::mpi::Rank::getInstance().getRank()==1) {
    GeneratedTestObject_Flattened receivedObject;
    GeneratedTestObject_Flattened::receive(receivedObject, 0, Tag, tarch::mpi::Rank::getInstance().getCommunicator());

    validateEquals( receivedObject.getId(),    testObject.getId() );
    validateEquals( receivedObject.getData(0), testObject.getData(0) );
    validateEquals( receivedObject.getData(1), testObject.getData(1) );
    validateEquals( receivedObject.getData(2), testObject.getData(2) );
    validateEquals( receivedObject.getData(3), testObject.getData(3) );
  }
  #endif
}


void peano4::stacks::tests::STDVectorStackOverObjectsWithSmartPointersTest::testStack() {
  #if defined(Parallel)
  const int Tag = 12;

  GeneratedTestObject::initDatatype();

  peano4::stacks::STDVectorStackOverObjectsWithSmartPointers<GeneratedTestObject> stack;

  GeneratedTestObject testObject0;
  GeneratedTestObject testObject1;
  GeneratedTestObject testObject2;

  testObject0.setId(230);
  testObject0.setData(0,  00);
  testObject0.setData(1, -10);
  testObject0.setData(2, -20);
  testObject0.setData(3, -30);

  testObject1.setId(231);
  testObject1.setData(0,  01);
  testObject1.setData(1, -11);
  testObject1.setData(2, -21);
  testObject1.setData(3, -31);

  testObject2.setId(232);
  testObject2.setData(0,  02);
  testObject2.setData(1, -12);
  testObject2.setData(2, -22);
  testObject2.setData(3, -32);

  if ( tarch::mpi::Rank::getInstance().getNumberOfRanks()>=2 and tarch::mpi::Rank::getInstance().getRank()==0) {
    stack.push(testObject0);
    stack.push(testObject1);
    stack.push(testObject2);

    stack.startSend(peano4::grid::TraversalObserver::SendReceiveContext::BoundaryExchange, 1, Tag, tarch::mpi::Rank::getInstance().getCommunicator());
    while ( not stack.tryToFinishSendOrReceive() ) {}
  }
  if ( tarch::mpi::Rank::getInstance().getNumberOfRanks()>=2 and tarch::mpi::Rank::getInstance().getRank()==1) {
    stack.startReceive(peano4::grid::TraversalObserver::SendReceiveContext::BoundaryExchange, 0, Tag, tarch::mpi::Rank::getInstance().getCommunicator(), 3);
    while ( not stack.tryToFinishSendOrReceive() ) {}

    GeneratedTestObject receivedObject2 = stack.pop();
    GeneratedTestObject receivedObject1 = stack.pop();
    GeneratedTestObject receivedObject0 = stack.pop();

    validateEquals( receivedObject0.getId(),    testObject0.getId() );
    validateEquals( receivedObject0.getData(0), testObject0.getData(0) );
    validateEquals( receivedObject0.getData(1), testObject0.getData(1) );
    validateEquals( receivedObject0.getData(2), testObject0.getData(2) );
    validateEquals( receivedObject0.getData(3), testObject0.getData(3) );

    validateEquals( receivedObject1.getId(),    testObject1.getId() );
    validateEquals( receivedObject1.getData(0), testObject1.getData(0) );
    validateEquals( receivedObject1.getData(1), testObject1.getData(1) );
    validateEquals( receivedObject1.getData(2), testObject1.getData(2) );
    validateEquals( receivedObject1.getData(3), testObject1.getData(3) );

    validateEquals( receivedObject2.getId(),    testObject2.getId() );
    validateEquals( receivedObject2.getData(0), testObject2.getData(0) );
    validateEquals( receivedObject2.getData(1), testObject2.getData(1) );
    validateEquals( receivedObject2.getData(2), testObject2.getData(2) );
    validateEquals( receivedObject2.getData(3), testObject2.getData(3) );
  }
  #endif
}


void peano4::stacks::tests::STDVectorStackOverObjectsWithSmartPointersTest::testMPIExchangeOfSingleObject() {
  #if defined(Parallel)
  GeneratedTestObject testObject;

  GeneratedTestObject::initDatatype();

  const int Tag = 12;

  testObject.setId(23);
  testObject.setData(0,  0);
  testObject.setData(1, -1);
  testObject.setData(2, -2);
  testObject.setData(3, -3);

  if ( tarch::mpi::Rank::getInstance().getNumberOfRanks()>=2 and tarch::mpi::Rank::getInstance().getRank()==0) {
    GeneratedTestObject::send(testObject, 1, Tag, tarch::mpi::Rank::getInstance().getCommunicator());
  }
  if ( tarch::mpi::Rank::getInstance().getNumberOfRanks()>=2 and tarch::mpi::Rank::getInstance().getRank()==1) {
    GeneratedTestObject receivedObject;
    GeneratedTestObject::receive(receivedObject, 0, Tag, tarch::mpi::Rank::getInstance().getCommunicator());

    validateEquals( receivedObject.getId(),    testObject.getId() );
    validateEquals( receivedObject.getData(0), testObject.getData(0) );
    validateEquals( receivedObject.getData(1), testObject.getData(1) );
    validateEquals( receivedObject.getData(2), testObject.getData(2) );
    validateEquals( receivedObject.getData(3), testObject.getData(3) );
  }
  #endif
}


void peano4::stacks::tests::STDVectorStackOverObjectsWithSmartPointersTest::run() {
  logTraceIn( "run()" );

  testMethod( testFlattening );
  testMethod( testMPIExchangeOfFlattenedObject );
  testMethod( testMPIExchangeOfSingleObject );
  testMethod( testStack );

  logTraceOut( "run()" );
}


#ifdef UseTestSpecificCompilerSettings
#pragma optimize("",on)
#endif
