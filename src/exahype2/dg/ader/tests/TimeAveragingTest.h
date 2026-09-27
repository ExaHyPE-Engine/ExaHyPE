// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/tests/TestCase.h"

namespace exahype2::dg::ader::tests {
  class TimeAveragingTest;
} // namespace exahype2::dg::ader::tests

/**
 * Checks the temporal quadrature that turns the space-time predictor into the time-averaged state of a cell.
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
   * Every time slice enters the average with the weight of its time node.
   */
  void testEveryTimeSliceEntersWithItsWeight();

  /**
   * The average overwrites the first time slice and leaves the later ones untouched.
   */
  void testTheLaterTimeSlicesKeepTheirValues();
};
