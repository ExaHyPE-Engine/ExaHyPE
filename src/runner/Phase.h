// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "runner/RepositoryCallbacks.h"
#include "tarch/la/Vector.h"

namespace runner {
  class Engine;
  class Phase;
  class ReportBox;
  class RuntimeParameters;
  struct PhaseContext;
} // namespace runner

struct runner::PhaseContext {
  runner::Engine&                       engine;
  runner::ReportBox&                    box;
  const runner::RepositoryCallbacks&    repositories;
  const runner::RuntimeParameters&      parameters;
  runner::MainMeasurements&             measurements;
  tarch::la::Vector<DIMENSIONS, double> domainOffset;
  tarch::la::Vector<DIMENSIONS, double> domainSize;
  std::bitset<DIMENSIONS>               periodicBC;
};

class runner::Phase {
public:
  Phase()          = default;
  virtual ~Phase() = default;
  // Phases are held and run polymorphically, never copied or moved.
  Phase(const runner::Phase& other)                    = delete;
  Phase(runner::Phase&& other)                         = delete;
  runner::Phase& operator=(const runner::Phase& other) = delete;
  runner::Phase& operator=(runner::Phase&& other)      = delete;

  virtual void run() = 0;
};
