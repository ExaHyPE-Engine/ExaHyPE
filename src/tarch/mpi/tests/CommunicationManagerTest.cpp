// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "CommunicationManagerTest.h"

#include "tarch/mpi/AbstractCommunicationManager.h"
#include "tarch/mpi/Rank.h"
#include "tarch/mpi/SerialCommunicationManager.h"
#include "tarch/mpi/ThreadedCommunicationManager.h"

tarch::mpi::tests::CommunicationManagerTest::CommunicationManagerTest() :
  tarch::tests::TestCase("tarch::mpi::tests::CommunicationManagerTest") {}

void
tarch::mpi::tests::CommunicationManagerTest::run() {
  testMethod(testSerialProgressRunsEveryPollerOnce);
  testMethod(testSerialFinishPendingWorkRunsUntilIdle);
  testMethod(testThreadedManagerPollsWithoutProgressCalls);
  testMethod(testThreadedFinishPendingWorkWaitsForAnIdlePass);
  testMethod(testThreadedFinishPendingWorkAfterStopRunsInline);
  testMethod(testDedicatedThreadNeedsAPeerAndAConfiguration);
}

void
tarch::mpi::tests::CommunicationManagerTest::testDedicatedThreadNeedsAPeerAndAConfiguration() {
  const bool usedOnEntry = tarch::mpi::AbstractCommunicationManager::usesDedicatedThread();
  const bool hasPeers    = tarch::mpi::Rank::getInstance().getNumberOfRanks() > 1;

  tarch::mpi::AbstractCommunicationManager::configure(false);
  validate(not tarch::mpi::AbstractCommunicationManager::usesDedicatedThread());

  tarch::mpi::AbstractCommunicationManager::configure(true);
  validateEquals(tarch::mpi::AbstractCommunicationManager::usesDedicatedThread(), hasPeers);

  // The rank count is unchanged. The value on entry restores the configuration.
  tarch::mpi::AbstractCommunicationManager::configure(usedOnEntry);
  validateEquals(tarch::mpi::AbstractCommunicationManager::usesDedicatedThread(), usedOnEntry);
}

void
tarch::mpi::tests::CommunicationManagerTest::testSerialProgressRunsEveryPollerOnce() {
  tarch::mpi::SerialCommunicationManager manager;
  int                                    firstPollerInvocations{0};
  int                                    secondPollerInvocations{0};
  manager.registerPoller([&firstPollerInvocations]() -> bool {
    firstPollerInvocations++;
    return true;
  });
  manager.registerPoller([&secondPollerInvocations]() -> bool {
    secondPollerInvocations++;
    return true;
  });

  manager.progress();
  manager.progress();

  validateEquals(firstPollerInvocations, 2);
  validateEquals(secondPollerInvocations, 2);
}

void
tarch::mpi::tests::CommunicationManagerTest::testSerialFinishPendingWorkRunsUntilIdle() {
  constexpr int                          PassesUntilIdle = 4;
  tarch::mpi::SerialCommunicationManager manager;
  int                                    pollerInvocations{0};
  manager.registerPoller([&pollerInvocations]() -> bool {
    pollerInvocations++;
    return pollerInvocations >= PassesUntilIdle;
  });

  manager.finishPendingWork();

  validateEqualsWithParams1(pollerInvocations, PassesUntilIdle, pollerInvocations);
}

void
tarch::mpi::tests::CommunicationManagerTest::testThreadedManagerPollsWithoutProgressCalls() {
  tarch::mpi::ThreadedCommunicationManager manager;
  std::atomic<int>                         pollerInvocations{0};
  manager.registerPoller([&pollerInvocations]() -> bool {
    pollerInvocations.fetch_add(1, std::memory_order_relaxed);
    return true;
  });

  manager.start();
  while (pollerInvocations.load(std::memory_order_relaxed) < 3) {
  }
  manager.stop();

  validateWithParams1(pollerInvocations.load() >= 3, pollerInvocations.load());
}

void
tarch::mpi::tests::CommunicationManagerTest::testThreadedFinishPendingWorkWaitsForAnIdlePass() {
  constexpr int                            PassesUntilIdle = 3;
  tarch::mpi::ThreadedCommunicationManager manager;
  std::atomic<int>                         passes{0};
  manager.registerPoller([&passes]() -> bool {
    return passes.fetch_add(1, std::memory_order_relaxed) + 1 >= PassesUntilIdle;
  });

  manager.start();
  manager.finishPendingWork();
  const int passesAfterWait = passes.load(std::memory_order_relaxed);
  manager.stop();

  validateWithParams1(passesAfterWait >= PassesUntilIdle, passesAfterWait);
}

void
tarch::mpi::tests::CommunicationManagerTest::testThreadedFinishPendingWorkAfterStopRunsInline() {
  constexpr int                            PassesUntilIdle = 2;
  tarch::mpi::ThreadedCommunicationManager manager;
  std::atomic<int>                         pollerInvocations{0};
  manager.registerPoller([&pollerInvocations]() -> bool {
    return pollerInvocations.fetch_add(1, std::memory_order_relaxed) + 1 >= PassesUntilIdle;
  });

  manager.finishPendingWork();

  validateEqualsWithParams1(pollerInvocations.load(), PassesUntilIdle, pollerInvocations.load());
}
