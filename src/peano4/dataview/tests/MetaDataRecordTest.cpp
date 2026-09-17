// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "MetaDataRecordTest.h"

#include "peano4/dataview/CellMarker.h"
#include "peano4/dataview/MetaDataRecord.h"
#include "tarch/tests/TestMacros.h"

namespace {
  /**
   * Stands in for a generated metadata record. The base contributes the
   * exchange behaviour, the record its payload and the two boundary
   * conditions.
   */
  struct TestRecord final : public peano4::dataview::MetaDataRecord<TestRecord, peano4::dataview::CellMarker> {
    bool sendAllowed{false};
    bool receiveAndMergeAllowed{false};

    INLINE_METHOD bool sendCondition([[maybe_unused]] const peano4::dataview::CellMarker& marker) const {
      return sendAllowed;
    }

    INLINE_METHOD bool receiveAndMergeCondition([[maybe_unused]] const peano4::dataview::CellMarker& marker) const {
      return receiveAndMergeAllowed;
    }
  };
} // namespace

peano4::dataview::tests::MetaDataRecordTest::MetaDataRecordTest() :
  TestCase("peano4::dataview::tests::MetaDataRecordTest") {}

void peano4::dataview::tests::MetaDataRecordTest::run() {
  testMethod(testSendFollowsTheRecordsCondition);
  testMethod(testReceiveAndMergeFollowsTheRecordsCondition);
  testMethod(testLoadAlwaysHolds);
}

void peano4::dataview::tests::MetaDataRecordTest::testSendFollowsTheRecordsCondition() {
  const peano4::dataview::CellMarker marker{};
  TestRecord                         record;

  validateFalse(record.send(marker, peano4::grid::SendReceiveContext::InterRank));
  validateFalse(record.send(marker, peano4::grid::SendReceiveContext::IntraRank));

  record.sendAllowed = true;
  validate(record.send(marker, peano4::grid::SendReceiveContext::InterRank));
  validate(record.send(marker, peano4::grid::SendReceiveContext::IntraRank));
}

void peano4::dataview::tests::MetaDataRecordTest::testReceiveAndMergeFollowsTheRecordsCondition() {
  const peano4::dataview::CellMarker marker{};
  TestRecord                         record;

  validateFalse(record.receiveAndMerge(marker, peano4::grid::SendReceiveContext::InterRank));
  validateFalse(record.receiveAndMerge(marker, peano4::grid::SendReceiveContext::IntraRank));

  record.receiveAndMergeAllowed = true;
  validate(record.receiveAndMerge(marker, peano4::grid::SendReceiveContext::InterRank));
  validate(record.receiveAndMerge(marker, peano4::grid::SendReceiveContext::IntraRank));
}

void peano4::dataview::tests::MetaDataRecordTest::testLoadAlwaysHolds() {
  const peano4::dataview::CellMarker marker{};
  validate(TestRecord::load(marker));
}
