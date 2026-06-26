// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "runner/RuntimeParameters.h"
#include "runner/Statistics.h" // RepositoryCallbacks, MainMeasurements, StatisticsLog
#include "tarch/la/Vector.h"

#include <bitset>
#include <functional>
#include <string>
#include <vector>

namespace runner {
  /** A registerable unit of work: a named program step whose opaque body runs one traversal. */
  struct Step {
    int                   programStep;
    std::string           name;
    std::function<void()> body;
  };

  /** Everything the orchestrator and statistics need at run time, handed over by the Runner. */
  struct RunContext {
    const RepositoryCallbacks&            repositories;
    const RuntimeParameters&              parameters;
    tarch::la::Vector<DIMENSIONS, double> domainSize;
    MainMeasurements&                     measurements;
    StatisticsLog&                        statisticsLog;
  };

  /**
   * Drives the SPMD run: on the global master it lets the Orchestrator pick and run the
   * steps; on every other rank it executes whatever step the master broadcasts until it is
   * told to terminate. runStep() is the one primitive the orchestrator uses - broadcast the
   * step, execute it, refresh the load balancer, and return the step's wall time.
   */
  class Runner {
  public:
    Runner(
      tarch::la::Vector<DIMENSIONS, double> domainOffset,
      tarch::la::Vector<DIMENSIONS, double> domainSize,
      std::bitset<DIMENSIONS>               periodicBC,
      RepositoryCallbacks                   callbacks,
      std::vector<Step>                     steps
    );

    int run();

    /** Master only: broadcast + execute one step, update the load balancer, return its wall time.
     */
    double runStep(int programStep);

    const Step& findStep(int programStep) const;

  private:
    tarch::la::Vector<DIMENSIONS, double> _domainOffset;
    tarch::la::Vector<DIMENSIONS, double> _domainSize;
    std::bitset<DIMENSIONS>               _periodicBC;
    RepositoryCallbacks                   _callbacks;
    std::vector<Step>                     _steps;
    MainMeasurements                      _measurements;
    StatisticsLog                         _statisticsLog;

    void executeStepBody(int programStep) const;
    void runWorker() const;
    void assertConsistentStepRegistryAcrossRanks() const;
  };

  /**
   * The whole run lifecycle in one call: parse the merged RuntimeParameters, bring up tarch
   * and peano4, build the data repository, run the schedule through the Runner, tear it all
   * down. A generated main collapses to a single call into this.
   */
  int run(
    const tarch::la::Vector<DIMENSIONS, double>& domainOffset,
    const tarch::la::Vector<DIMENSIONS, double>& domainSize,
    const std::bitset<DIMENSIONS>&               periodicBC,
    RepositoryCallbacks                          callbacks,
    std::vector<Step>                            steps,
    int&                                         argc,
    char**                                       argv
  );
} // namespace runner
