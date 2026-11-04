// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/tests/TestCase.h"

namespace tarch::tests {
  class CPUFeatureDetectionTest;
} // namespace tarch::tests

class tarch::tests::CPUFeatureDetectionTest final : public tarch::tests::TestCase {
public:
  CPUFeatureDetectionTest();
  CPUFeatureDetectionTest(const CPUFeatureDetectionTest& other)            = delete;
  CPUFeatureDetectionTest(CPUFeatureDetectionTest&& other)                 = delete;
  CPUFeatureDetectionTest& operator=(const CPUFeatureDetectionTest& other) = delete;
  CPUFeatureDetectionTest& operator=(CPUFeatureDetectionTest&& other)      = delete;
  ~CPUFeatureDetectionTest() override                                      = default;

  void run() override;

private:
  void testTheFeaturesBelongToTheArchitecture();
  void testTheReportListsTheDetectedFeatures();
};
