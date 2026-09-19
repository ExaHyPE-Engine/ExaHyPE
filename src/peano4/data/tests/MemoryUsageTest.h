// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/tests/TestCase.h"

namespace peano4::data::tests {
  class MemoryUsageTest;
} // namespace peano4::data::tests

class peano4::data::tests::MemoryUsageTest final : public tarch::tests::TestCase {
public:
  MemoryUsageTest();
  MemoryUsageTest(const MemoryUsageTest&)            = delete;
  MemoryUsageTest(MemoryUsageTest&&)                 = delete;
  MemoryUsageTest& operator=(const MemoryUsageTest&) = delete;
  MemoryUsageTest& operator=(MemoryUsageTest&&)      = delete;
  ~MemoryUsageTest() override                        = default;

  void run() override;

private:
  void testPayloadBytesOfRecordsAndPools();
  void testReleasePoolStorageClearsOnlyAPool();
  void testTheCountersLagOneUpdate();
};
