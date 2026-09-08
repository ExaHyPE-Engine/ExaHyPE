// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

/**
 * @namespace tarch
 *
 * tarch denotes the technical architecture. It calls no layer above it.
 */
namespace tarch {
  struct InitConfiguration {
    /** Seconds to wait for MPI progress before warning. Zero disables the warning. */
    double mpiTimeoutWarning{0.0};

    /** Seconds to wait for MPI progress before terminating. Zero disables the abort. */
    double mpiDeadlockTimeout{0.0};

    /**
     * Records the origin, partner and tag of every non-blocking request, so a timeout
     * warning names the request that hangs.
     */
    bool mpiRequestTrackingEnabled{false};

    /**
     * Drives a rank's outstanding communication on a thread of its own. That thread reserves one
     * core of the task runtime's team.
     */
    bool communicationThreadEnabled{false};
  };

  void init(int* argc, char*** argv, const InitConfiguration& configuration);
  void shutdown();
} // namespace tarch
