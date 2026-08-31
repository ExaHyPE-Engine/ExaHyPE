// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/tests/TestCase.h"

namespace tarch::mpi::tests {
  class DiagnosticsTest;
} // namespace tarch::mpi::tests

class tarch::mpi::tests::DiagnosticsTest final : public tarch::tests::TestCase {
public:
  DiagnosticsTest();
  DiagnosticsTest(const DiagnosticsTest&)            = delete;
  DiagnosticsTest(DiagnosticsTest&&)                 = delete;
  DiagnosticsTest& operator=(const DiagnosticsTest&) = delete;
  DiagnosticsTest& operator=(DiagnosticsTest&&)      = delete;
  ~DiagnosticsTest() override                        = default;

  void run() override;

private:
  void testRegisterReportClear();
  void testTableSurvivesConcurrentChurn();
};
