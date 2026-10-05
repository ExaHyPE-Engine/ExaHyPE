// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/tests/TestCase.h"

namespace tarch::accelerator::tests {
  class BufferTest;
} // namespace tarch::accelerator::tests

/**
 * Checks that a buffer owns its array in the requested memory location and copies values to and
 * from the host.
 */
class tarch::accelerator::tests::BufferTest final : public tarch::tests::TestCase {
public:
  BufferTest();
  BufferTest(const BufferTest&)            = delete;
  BufferTest(BufferTest&&)                 = delete;
  BufferTest& operator=(const BufferTest&) = delete;
  BufferTest& operator=(BufferTest&&)      = delete;
  ~BufferTest() override                   = default;

  void run() override;

private:
  void testEmptyBuffer();
  void testHeapBufferIsAddressable();
  void testDeviceRoundTrip();
  void testAllocateReplacesTheArray();
  void testMoveTransfersTheArray();
  void testPinnedBuffer();
  void testPinnedCopiesEndBeforeTheyReturn();
  void testAllocatedBytesFollowTheLocation();
};
