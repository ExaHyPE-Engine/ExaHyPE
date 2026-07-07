// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "runner/RepositoryCallbacks.h"
#include "tarch/la/Vector.h"

#include <bitset>
#include <string>
#include <vector>

namespace runner {
  class Engine;
  class ReportBox;
  class RuntimeParameters;

  struct PhaseContext {
    Engine&                               engine;
    ReportBox&                            box;
    const RepositoryCallbacks&            repositories;
    const RuntimeParameters&              parameters;
    MainMeasurements&                     measurements;
    tarch::la::Vector<DIMENSIONS, double> domainOffset;
    tarch::la::Vector<DIMENSIONS, double> domainSize;
    std::bitset<DIMENSIONS>               periodicBC;
  };

  class Phase {
  public:
    Phase()          = default;
    virtual ~Phase() = default;
    // Phases are held and run polymorphically, never copied or moved.
    Phase(const Phase&)            = delete;
    Phase(Phase&&)                 = delete;
    Phase& operator=(const Phase&) = delete;
    Phase& operator=(Phase&&)      = delete;

    /** Widest lines this phase can print. */
    virtual std::vector<std::string> widthSamples() const { return {}; }

    virtual void run() = 0;
  };
} // namespace runner
