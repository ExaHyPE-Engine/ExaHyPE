#include "StringTest.h"

#include "tarch/mpi/Rank.h"
#include "tarch/mpi/StringMessage.h"

tarch::mpi::tests::StringTest::StringTest() :
  TestCase("tarch::mpi::tests::StringTest") {}

void tarch::mpi::tests::StringTest::run() { testMethod(testSendReceive); }

void tarch::mpi::tests::StringTest::testSendReceive() {
#ifdef PEANO_WITH_MPI
  const std::string testString = "unittest";
  const int         tag        = tarch::mpi::Rank::reserveFreeTag("mpi::StringTest");
  if (tarch::mpi::Rank::getInstance().getNumberOfRanks() > 1 and tarch::mpi::Rank::getInstance().getRank() == 0) {
    StringMessage message;
    message.setData(testString);
    StringMessage::send(message, 1, tag, tarch::mpi::Rank::getInstance().getCommunicator());
  }

  if (tarch::mpi::Rank::getInstance().getNumberOfRanks() > 1 and tarch::mpi::Rank::getInstance().getRank() == 1) {
    StringMessage message;
    StringMessage::receive(message, 0, tag, tarch::mpi::Rank::getInstance().getCommunicator());
    validateEqualsWithParams2(message.getData(), testString, message.toString(), testString);
  }

  tarch::mpi::Rank::releaseTag(tag);
#endif
}
