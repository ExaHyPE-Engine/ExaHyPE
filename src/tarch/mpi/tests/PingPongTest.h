// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/tests/TestCase.h"

namespace tarch::mpi::tests {
  class PingPongTest;
} // namespace tarch::mpi::tests

/**
 * Ping-pong exchange between two ranks, single-threaded and from concurrent tasks. The test covers
 * MPI_THREAD_MULTIPLE calls from independent tasks.
 */
class tarch::mpi::tests::PingPongTest final : public tarch::tests::TestCase {
public:
  PingPongTest();
  PingPongTest(const PingPongTest&)            = delete;
  PingPongTest(PingPongTest&&)                 = delete;
  PingPongTest& operator=(const PingPongTest&) = delete;
  PingPongTest& operator=(PingPongTest&&)      = delete;
  ~PingPongTest() override                     = default;

  void run() override;

private:
  void testBuiltInType();

  void testMultithreadedPingPongWithBlockingReceives();
  void testMultithreadedPingPongWithBlockingSends();
  void testMultithreadedPingPongWithBlockingSendsAndReceives();

  void testMultithreadedPingPongWithNonblockingReceives();
  void testMultithreadedPingPongWithNonblockingSends();
  void testMultithreadedPingPongWithNonblockingSendsAndReceives();
};
