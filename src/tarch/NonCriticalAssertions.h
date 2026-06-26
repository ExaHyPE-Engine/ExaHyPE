// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include <string>

/**
 * @page tarch_NoncriticalAssertions Noncritical assertions
 *
 * @brief A noncritical assertion is an assertion which indicates that
 * something went horribly wrong due to a programming error, but the code
 * should still continue with this mesh traversal, then dump the data, and
 * eventually terminate.
 */
namespace tarch {
  /**
   * Register the MPI service that propagates a non-critical assertion across
   * ranks. Call once from tarch::init after the service repository is up. No-op
   * without MPI. From then on a non-critical assertion raised on any rank
   * reaches every other rank over the regular dangling-message channel, so no
   * code on the hot path has to run a collective to discover it.
   */
  void initNonCriticalAssertionEnvironment();

  void enableNonCriticalAssertions(bool value);

  void triggerNonCriticalAssertion(
    const std::string& file,
    int                line,
    const std::string& expression,
    const std::string& parameterValuePairs
  );
  bool hasNonCriticalAssertionBeenViolated();
} // namespace tarch
