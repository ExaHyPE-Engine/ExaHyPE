#include "StringTest.h"
#include "../StringMessage.h"
#include "../Rank.h"


tarch::logging::Log tarch::mpi::tests::StringTest::_log( "tarch::mpi::tests::StringTest" );


tarch::mpi::tests::StringTest::StringTest():
  TestCase( "tarch::mpi::tests::StringTest" ) {
}


void tarch::mpi::tests::StringTest::run() {
  testMethod (testSendReceive);
}


void tarch::mpi::tests::StringTest::testSendReceive() {
  #ifdef Parallel
  const std::string testString = "unittest";
  if (
    tarch::mpi::Rank::getInstance().getNumberOfRanks()>1
    and
    tarch::mpi::Rank::getInstance().getRank()==0
  ) {
    StringMessage message;
    message.setData( testString );
    StringMessage::send( message, 1, 0, tarch::mpi::Rank::getInstance().getCommunicator() );
  }

  if (
    tarch::mpi::Rank::getInstance().getNumberOfRanks()>1
    and
    tarch::mpi::Rank::getInstance().getRank()==1
  ) {
    StringMessage message;
    StringMessage::receive( message, 0, 0, tarch::mpi::Rank::getInstance().getCommunicator() );
    validateEqualsWithParams2( message.getData(), testString, message.toString(), testString );
  }
  #endif
}
