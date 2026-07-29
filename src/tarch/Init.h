// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

/**
 * @namespace tarch
 *
 * tarch denotes the Technical Architectures. It is used within Peano, but it
 * is designed in a way that it is more or less independent of Peano, i.e.
 * Peano uses the tarch but not the other way round.
 *
 * \section Instrumentation and diagnostics
 *
 * - PEANO_TRACE = 1: Enable timed trace instrumentation
 * - PEANO_ASSERTIONS = 1: Enable assertion and non-critical assertion macros
 * - PEANO_DEBUG = 1: Enable debug-only data, debug checks, and debug messages
 *
 * \section Default libraries
 *
 * By default, Peano builds all core libraries in four different versions:
 *
 * - A production variant with PEANO_TRACE=0, PEANO_ASSERTIONS=0, PEANO_DEBUG=0
 * - A trace variant with PEANO_TRACE=1, PEANO_ASSERTIONS=0, PEANO_DEBUG=0
 * - An asserts variant with PEANO_TRACE=0, PEANO_ASSERTIONS=1, PEANO_DEBUG=0
 * - A debug variant with PEANO_TRACE=0, PEANO_ASSERTIONS=1, PEANO_DEBUG=1
 *
 * Note: The trace variant does not contain any assertions, as assertions are
 * typically expensive and slow down the code considerably.
 *
 */
namespace tarch {
  struct InitConfiguration {
    double mpiTimeoutWarning;
    double mpiDeadlockTimeout;

    /**
     * Records the origin, partner and tag of every non-blocking request, so a timeout
     * warning names the request that hangs.
     */
    bool mpiRequestTrackingEnabled;
  };

  void init(int* argc, char*** argv, const InitConfiguration& configuration);
  void shutdown();
} // namespace tarch
