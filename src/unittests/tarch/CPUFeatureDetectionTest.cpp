// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "tarch/CPUFeatureDetection.h"

#include "tarch/testing/UnitTest.h"

UNIT_TEST("The detected instruction sets belong to the architecture the test runs on") {
  GIVEN("The instruction sets of this processor") {
    const tarch::CPUFeatures features = tarch::CPUFeatures::detect();

    THEN("AVX2 and AVX512 come with AVX and the instruction sets of another architecture are absent") {
      VALIDATE(features.avx or not features.avx2);
      VALIDATE(features.avx or not features.avx512);
#if defined(__x86_64__) or defined(_M_X64)
      VALIDATE(features.sse2);
      VALIDATE(not features.neon);
      VALIDATE(not features.sve);
#elif defined(__aarch64__) or defined(_M_ARM64)
      VALIDATE(features.neon);
      VALIDATE(not features.sse2);
      VALIDATE(not features.avx);
#endif
    }
  }
}

UNIT_TEST("The report names every detected instruction set once, in a fixed order and separated by commas") {
  GIVEN("The instruction sets of this processor and their names in the order of the report") {
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

    WHEN("detectCPUFeatures() writes the report") {
      const std::string report = tarch::detectCPUFeatures();

      THEN("The report lists the names or None for a processor without one of them") {
        VALIDATE_EQUALS(report, expectedReport);
      }
    }
  }
}
