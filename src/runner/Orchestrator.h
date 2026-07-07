// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "runner/Engine.h" // Step

#include <vector>

namespace runner {
  struct PhaseContext;

  /** Runs the ordered list of phases into the shared box. Add a phase by registering it in run(). */
  class Orchestrator final {
  public:
    explicit Orchestrator(const std::vector<Step>& steps);

    /** True if a noncritical assertion ended the run early. */
    bool run(PhaseContext& context);

  private:
    int _createGridStep    = -1;
    int _loadBalancingStep = -1;
    int _initGridStep      = -1;
    int _plotStep          = -1;
    int _timeStepStep      = -1;
  };
} // namespace runner
