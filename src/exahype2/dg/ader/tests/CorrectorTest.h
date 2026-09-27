// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/tests/TestCase.h"

namespace exahype2::dg::ader::tests {
  class CorrectorTest;
} // namespace exahype2::dg::ader::tests

/** Checks the corrector phase of one ADER-DG cell. */
class exahype2::dg::ader::tests::CorrectorTest final : public tarch::tests::TestCase {
public:
  CorrectorTest();
  CorrectorTest(const CorrectorTest&)            = delete;
  CorrectorTest(CorrectorTest&&)                 = delete;
  CorrectorTest& operator=(const CorrectorTest&) = delete;
  CorrectorTest& operator=(CorrectorTest&&)      = delete;
  ~CorrectorTest() override                      = default;

  void run() override;

private:
  void testTheFaceIntegralsReachTheCellState();
  void testZeroFluxDifferencesLeaveTheCellStateBitwise();
  void testTheCorrectionReportsTheWaveSpeedsOfTheUpdatedState();
};
