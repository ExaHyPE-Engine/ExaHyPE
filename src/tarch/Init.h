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
    double mpiTimeoutWarning{0.0};
    double mpiDeadlockTimeout{0.0};
    bool   mpiRequestTrackingEnabled{false};
    bool   communicationThreadEnabled{false};
  };

  void init(int* argc, char*** argv, const InitConfiguration& configuration);
  void shutdown();
} // namespace tarch
