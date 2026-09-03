// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "runner/ReportBox.h"
#include "runner/RepositoryCallbacks.h"
#include "tarch/la/Vector.h"

namespace runner {
  struct Step;
  struct StepName;
  class Engine;
} // namespace runner

/** One program step of a run; its body runs one traversal. */
struct runner::Step {
  int                   programStep{-1};
  std::string           name{};
  std::function<void()> body{};
};

struct runner::StepName {
  static constexpr const char* CreateGrid         = "CreateGrid";
  static constexpr const char* InitGrid           = "InitGrid";
  static constexpr const char* PlotSolution       = "PlotSolution";
  static constexpr const char* CheckpointSolution = "CheckpointSolution";
  static constexpr const char* TimeStep           = "TimeStep";
  static constexpr const char* DynamicAMR         = "DynamicAMR";
};

/**
 * The single-program multiple-data execution engine. On the global master it lets the Orchestrator
 * pick and run the steps; on every other rank it executes the step the master broadcasts until the
 * master broadcasts the termination step.
 */
class runner::Engine final {
public:
  Engine(
    tarch::la::Vector<DIMENSIONS, double> domainOffset,
    tarch::la::Vector<DIMENSIONS, double> domainSize,
    std::bitset<DIMENSIONS>               periodicBC,
    runner::RepositoryCallbacks           callbacks,
    std::vector<runner::Step>             steps
  );

  int run();

  [[nodiscard]] double runStep(int programStep);

  const runner::Step& findStep(int programStep) const;

private:
  tarch::la::Vector<DIMENSIONS, double> _domainOffset;
  tarch::la::Vector<DIMENSIONS, double> _domainSize;
  std::bitset<DIMENSIONS>               _periodicBC;
  runner::RepositoryCallbacks           _callbacks;
  std::vector<runner::Step>             _steps;
  runner::MainMeasurements              _measurements;
  runner::ReportBox                     _box;

  void executeStepBody(int programStep) const;
  void runWorker() const;
  void assertConsistentStepRegistryAcrossRanks() const;
};
