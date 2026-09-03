// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "runner/Engine.h" // Step

namespace runner {
  struct PhaseContext;
  class Orchestrator;
} // namespace runner

/** Runs the ordered phases and writes their output into the context's box. */
class runner::Orchestrator final {
public:
  explicit Orchestrator(const std::vector<runner::Step>& steps);

  bool run(runner::PhaseContext& context);

private:
  int _createGridStep{-1};
  int _initGridStep{-1};
  int _plotStep{-1};
  int _checkpointStep{-1};
  int _timeStepStep{-1};
  int _dynamicAMRStep{-1};
};
