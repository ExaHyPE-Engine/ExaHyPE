// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/tests/TestCase.h"

namespace tarch::plotter::tests {
  class TimeSeriesWriterTest;
} // namespace tarch::plotter::tests

class tarch::plotter::tests::TimeSeriesWriterTest final : public tarch::tests::TestCase {
public:
  TimeSeriesWriterTest();
  TimeSeriesWriterTest(const TimeSeriesWriterTest&)            = delete;
  TimeSeriesWriterTest(TimeSeriesWriterTest&&)                 = delete;
  TimeSeriesWriterTest& operator=(const TimeSeriesWriterTest&) = delete;
  TimeSeriesWriterTest& operator=(TimeSeriesWriterTest&&)      = delete;
  ~TimeSeriesWriterTest() override                             = default;

  void run() override;

private:
  void testTheTimeStampReadsBackExactly();
  void testThePVDFileListsEverySnapshotTime();
};
