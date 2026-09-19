// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/tests/TestCase.h"

namespace exahype2::dg::limiting::tests {
  class AdmissibilityTest;
} // namespace exahype2::dg::limiting::tests

/**
 * Checks the relaxation term and the sampled discrete maximum principle of the limiter.
 */
class exahype2::dg::limiting::tests::AdmissibilityTest final : public tarch::tests::TestCase {
public:
  AdmissibilityTest();
  AdmissibilityTest(const AdmissibilityTest&)            = delete;
  AdmissibilityTest(AdmissibilityTest&&)                 = delete;
  AdmissibilityTest& operator=(const AdmissibilityTest&) = delete;
  AdmissibilityTest& operator=(AdmissibilityTest&&)      = delete;
  ~AdmissibilityTest() override                          = default;

  void run() override;

private:
  void testRelaxationTermReplacesTheSpecifiedParameter();
  void testSamplingExposesExtremaBetweenTheNodes();
  void testCandidateInsideTheReferenceRangeIsAccepted();
  void testCandidateOutsideTheReferenceRangeIsRejected();
  void testRelaxationWidensTheAcceptedRange();
};
