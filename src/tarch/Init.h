// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

namespace tarch {
  struct InitConfiguration {
    double mpiTimeOutWarning;
    double mpiDeadlockTimeOut;
    bool   mpiDeadlockDetectionEnabled;
  };

  void init(int* argc, char*** argv, const InitConfiguration& configuration);
  void shutdown();
} // namespace tarch
