// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "RuntimeTest.h"

#include "tarch/mpi/Rank.h"
#include "tarch/tasking/Runtime.h"

tarch::tasking::tests::RuntimeTest::RuntimeTest() :
  tarch::tests::TestCase("tarch::tasking::tests::RuntimeTest") {}

void tarch::tasking::tests::RuntimeTest::run() {
  testMethod(testRuntimeProperties);
  testMethod(testWaitPolicyOfRanksOnOneNode);
}

void tarch::tasking::tests::RuntimeTest::testWaitPolicyOfRanksOnOneNode() {
  const auto& rank = tarch::mpi::Rank::getInstance();
  if (rank.getNumberOfRanks() > rank.getNumberOfNodes()) {
    const char* const waitPolicy = std::getenv("OMP_WAIT_POLICY");
    validate(waitPolicy != nullptr);
  }
}

void tarch::tasking::tests::RuntimeTest::testRuntimeProperties() {
  validate(tarch::tasking::Runtime::getInstance().isInitialised());
  validate(tarch::tasking::Runtime::getInstance().getNumberOfThreads() > 0);
  validate(tarch::tasking::Runtime::getInstance().getCoreNumber() >= 0);
  validate(tarch::tasking::getNumberOfUnmaskedThreads() > 0);

  const auto unmaskedThreads = tarch::tasking::printUnmaskedThreads();
  validate(not unmaskedThreads.empty());

  const int threadNumber = tarch::tasking::Runtime::getInstance().getThreadNumber();
  validate(threadNumber >= 0);
  validate(threadNumber < tarch::tasking::Runtime::getInstance().getNumberOfThreads());
}
