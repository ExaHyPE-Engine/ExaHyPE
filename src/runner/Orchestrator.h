// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "runner/Engine.h" // Step

namespace runner {
  struct PhaseContext;

  /** Runs the ordered phases and writes their output into the context's box. Add a phase by registering it in run(). */
  class Orchestrator final {
  public:
    explicit Orchestrator(const std::vector<Step>& steps);

    bool run(PhaseContext& context);

  private:
    int _createGridStep    = -1;
    int _loadBalancingStep = -1;
    int _initGridStep      = -1;
    int _plotStep          = -1;
    int _timeStepStep      = -1;
    int _dynamicAMRStep    = -1;
  };
} // namespace runner
