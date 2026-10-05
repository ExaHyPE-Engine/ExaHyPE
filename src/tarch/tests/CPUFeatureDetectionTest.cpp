// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "CPUFeatureDetectionTest.h"

#include "tarch/CPUFeatureDetection.h"

tarch::tests::CPUFeatureDetectionTest::CPUFeatureDetectionTest() :
  tarch::tests::TestCase("tarch::tests::CPUFeatureDetectionTest") {}

void
tarch::tests::CPUFeatureDetectionTest::run() {
  testMethod(testTheFeaturesBelongToTheArchitecture);
  testMethod(testTheReportListsTheDetectedFeatures);
}

/**
 * The detected instruction sets belong to the architecture the test runs on.
 */
void
tarch::tests::CPUFeatureDetectionTest::testTheFeaturesBelongToTheArchitecture() {
  const tarch::CPUFeatures features = tarch::CPUFeatures::detect();

  validate(features.avx or not features.avx2);
  validate(features.avx or not features.avx512);
#if defined(__x86_64__) or defined(_M_X64)
  validate(features.sse2);
  validate(not features.neon);
  validate(not features.sve);
#elif defined(__aarch64__) or defined(_M_ARM64)
  validate(features.neon);
  validate(not features.sse2);
  validate(not features.avx);
#endif
}

/**
 * The report names every detected instruction set once, in a fixed order and separated by commas.
 */
void
tarch::tests::CPUFeatureDetectionTest::testTheReportListsTheDetectedFeatures() {
  const tarch::CPUFeatures features = tarch::CPUFeatures::detect();

  std::vector<std::string> names;
  if (features.sse2) {
    names.emplace_back("SSE2");
  }
  if (features.avx) {
    names.emplace_back("AVX");
  }
  if (features.avx2) {
    names.emplace_back("AVX2");
  }
  if (features.avx512) {
    names.emplace_back("AVX512");
  }
  if (features.neon) {
    names.emplace_back("NEON");
  }
  if (features.sve) {
    names.emplace_back("SVE");
  }

  std::string expectedReport = names.empty() ? "None" : names.front();
  for (std::size_t position = 1; position < names.size(); position++) {
    expectedReport += ", " + names[position];
  }
  validateEquals(tarch::detectCPUFeatures(), expectedReport);
}
