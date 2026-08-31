// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

/**
 * @page tarch_NonCriticalAssertions Non-critical assertions
 *
 * @brief A non-critical assertion records a programming error and lets the run continue. Every
 * rank can read and adopt the rank that raised the first one.
 */
namespace tarch {
  void triggerNonCriticalAssertion(
    const std::string& file,
    int                line,
    const std::string& expression,
    const std::string& parameterValuePairs
  );
  bool hasNonCriticalAssertionBeenViolated();

  int getNonCriticalAssertionRank();

  void adoptNonCriticalAssertionRank(int rank);
} // namespace tarch
