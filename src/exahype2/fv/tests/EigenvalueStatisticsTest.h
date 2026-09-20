// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/tests/TestCase.h"

namespace exahype2::fv::tests {
  class EigenvalueStatisticsTest;
} // namespace exahype2::fv::tests

class exahype2::fv::tests::EigenvalueStatisticsTest final : public tarch::tests::TestCase {
public:
  EigenvalueStatisticsTest();
  EigenvalueStatisticsTest(const EigenvalueStatisticsTest&)            = delete;
  EigenvalueStatisticsTest(EigenvalueStatisticsTest&&)                 = delete;
  EigenvalueStatisticsTest& operator=(const EigenvalueStatisticsTest&) = delete;
  EigenvalueStatisticsTest& operator=(EigenvalueStatisticsTest&&)      = delete;
  ~EigenvalueStatisticsTest() override                                 = default;

  void run() override;

private:
  void testAccumulation();
  void testInvalidEigenvalueAccumulation();
};
