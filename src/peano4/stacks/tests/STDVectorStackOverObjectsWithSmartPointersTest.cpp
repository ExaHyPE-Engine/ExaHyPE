#include "STDVectorStackOverObjectsWithSmartPointersTest.h"
#include "GeneratedTestObject.h"


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

  testMethod(  testFlattening );
  testMethod(  testMPIExchangeOfFlattenedObject );
  testMethod(  testMPIExchangeOfSingleObject );

  logTraceOut( "run()" );
}


#ifdef UseTestSpecificCompilerSettings
#pragma optimize("",on)
#endif
