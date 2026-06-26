// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "runner/Runner.h" // Step, RunContext, Runner

#include <string>
#include <vector>

namespace runner {
  class Orchestrator {
  public:
    explicit Orchestrator(const std::vector<Step>& steps);

    /** Drives the whole schedule. Returns true if the run ended early because a noncritical
     *  assertion was triggered (one final plot was written), false on a normal completion. */
    bool run(Runner& runner, RunContext& context);

  private:
    struct Role {
      int         programStep = -1;
      std::string name;
      bool        exists() const { return programStep >= 0; }
    };
    Role _createGrid;
    Role _loadBalancing;
    Role _initGrid;
    Role _plot;
    Role _timeStep;

    void runGridConstruction(Runner& runner, RunContext& context);
    void runGridInitialisation(Runner& runner, RunContext& context);
    bool runTimeLoop(Runner& runner, RunContext& context);
  };
} // namespace runner
