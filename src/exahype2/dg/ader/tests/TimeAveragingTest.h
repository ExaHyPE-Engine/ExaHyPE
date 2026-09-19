// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/tests/TestCase.h"

namespace exahype2::dg::ader::tests {
  class TimeAveragingTest;
} // namespace exahype2::dg::ader::tests

/**
 * Checks the temporal quadrature that turns the space-time predictor into the time averages of a cell.
 */
class exahype2::dg::ader::tests::TimeAveragingTest final : public tarch::tests::TestCase {
public:
  TimeAveragingTest();
  TimeAveragingTest(const TimeAveragingTest&)            = delete;
  TimeAveragingTest(TimeAveragingTest&&)                 = delete;
  TimeAveragingTest& operator=(const TimeAveragingTest&) = delete;
  TimeAveragingTest& operator=(TimeAveragingTest&&)      = delete;
  ~TimeAveragingTest() override                          = default;

  void run() override;

private:
  /**
   * A predictor that holds the same value at every time node averages to that value.
   */
  void testTheAverageOfAConstantIsThatConstant();

  /**
   * The average overwrites the state, the flux and the source of the target and adds nothing to them.
   */
  void testTheTargetIsOverwrittenNotAccumulated();

  /**
   * The averaged flux holds one block per axis, and every block runs in cell node order.
   */
  void testEveryAxisBlockKeepsCellNodeOrder();

  /**
   * A switched-off flux and a switched-off source leave their target buffers untouched.
   */
  void testSwitchedOffTermsAreLeftAlone();

  /**
   * A predictor stored in a narrower precision than the target averages into the wider one.
   */
  void testAPicardPrecisionBelowTheComputePrecisionIsAveraged();
};
