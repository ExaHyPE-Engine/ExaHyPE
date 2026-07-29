// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

/**
 * @page tarch_NoncriticalAssertions Non-critical assertions
 *
 * @brief A non-critical assertion indicates that something went horribly
 * wrong due to a programming error, without stopping the run immediately.
 * We record which rank first raised it and let every rank query and adopt
 * that rank, so a caller can decide when and how to wind down.
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
