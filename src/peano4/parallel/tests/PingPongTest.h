// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#ifdef PEANO_WITH_MPI

#include "tarch/tests/TestCase.h"

namespace peano4::parallel::tests {
  class PingPongTest;
} // namespace peano4::parallel::tests

/**
 * This is a ping pong test for our application-specific data types
 */
class peano4::parallel::tests::PingPongTest final : public tarch::tests::TestCase {
private:
  void testBuiltInType();

  /**
   * Exchange a message type through its own send() and through MPI_Send
   */
  void testIntegerMessageExchange();
  void testStartTraversalMessageExchange();
  void testMessageArrayExchange();

  void testTreeManagementMessageArrayExchange();

  void testMultithreadedPingPongWithBlockingReceives();
  void testMultithreadedPingPongWithBlockingSends();
  void testMultithreadedPingPongWithBlockingSendsAndReceives();

  void testMultithreadedPingPongWithNonblockingReceives();
  void testMultithreadedPingPongWithNonblockingSends();
  void testMultithreadedPingPongWithNonblockingSendsAndReceives();

public:
  PingPongTest();

  /**
   * Build up suite of test routines
   */
  void run() override;
};

#endif
