// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/tests/TestCase.h"

namespace tarch::tests {
  class SpanTest;
} // namespace tarch::tests

class tarch::tests::SpanTest final : public tarch::tests::TestCase {
public:
  SpanTest();
  SpanTest(const SpanTest& other)            = delete;
  SpanTest(SpanTest&& other)                 = delete;
  SpanTest& operator=(const SpanTest& other) = delete;
  SpanTest& operator=(SpanTest&& other)      = delete;
  ~SpanTest() override                       = default;

  void run() override;

private:
  void testTheSpanReadsTheElementsBehindItsData();
  void testTheEmptySpanHasNoElements();
};
