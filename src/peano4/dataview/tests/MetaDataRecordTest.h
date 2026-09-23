// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/tests/TestCase.h"

namespace peano4::dataview::tests {
  class MetaDataRecordTest;
} // namespace peano4::dataview::tests

class peano4::dataview::tests::MetaDataRecordTest final : public tarch::tests::TestCase {
public:
  MetaDataRecordTest();

  void run() override;

private:
  void testSendFollowsTheRecordsCondition();
  void testReceiveAndMergeFollowsTheRecordsCondition();
  void testLoadAlwaysHolds();
};
