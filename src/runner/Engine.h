// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "runner/ReportBox.h"
#include "runner/RepositoryCallbacks.h"
#include "tarch/la/Vector.h"

namespace runner {
  /** A registerable unit of work: a named program step whose opaque body runs one traversal. */
  struct Step {
    int                   programStep;
    std::string           name;
    std::function<void()> body;
  };

  struct StepName {
    static constexpr const char* CreateGrid    = "CreateGrid";
    static constexpr const char* LoadBalancing = "LoadBalancing";
    static constexpr const char* InitGrid      = "InitGrid";
    static constexpr const char* PlotSolution  = "PlotSolution";
    static constexpr const char* TimeStep      = "TimeStep";
    static constexpr const char* DynamicAMR    = "DynamicAMR";
  };

  /**
   * The SPMD execution engine. On the global master it lets the Orchestrator pick and run the steps;
   * on every other rank it executes whatever step the master broadcasts until told to terminate.
   * runStep() is the one primitive the phases use - broadcast the step, execute it, refresh the load
   * balancer, and return the step's wall time.
   */
  class Engine final {
  public:
    Engine(
      tarch::la::Vector<DIMENSIONS, double> domainOffset,
      tarch::la::Vector<DIMENSIONS, double> domainSize,
      std::bitset<DIMENSIONS>               periodicBC,
      RepositoryCallbacks                   callbacks,
      std::vector<Step>                     steps
    );

    int run();

    /**
     * Master only: broadcast + execute one step, update the load balancer, return its wall time. The
     * time is also recorded internally for the final summary.
     */
    [[nodiscard]] double runStep(int programStep);

    const Step& findStep(int programStep) const;

  private:
    tarch::la::Vector<DIMENSIONS, double> _domainOffset;
    tarch::la::Vector<DIMENSIONS, double> _domainSize;
    std::bitset<DIMENSIONS>               _periodicBC;
    RepositoryCallbacks                   _callbacks;
    std::vector<Step>                     _steps;
    MainMeasurements                      _measurements;
    ReportBox                             _box;

    void executeStepBody(int programStep) const;
    void runWorker() const;
    void assertConsistentStepRegistryAcrossRanks() const;
  };
} // namespace runner
