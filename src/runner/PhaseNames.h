// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

namespace runner {
  struct PhaseName;
} // namespace runner

struct runner::PhaseName {
  static constexpr const char* GridConstruction   = "Grid Construction";
  static constexpr const char* AllocateMemory     = "Allocate Memory";
  static constexpr const char* GridInitialisation = "Grid Initialisation";
  static constexpr const char* TimeLoop           = "Time Loop";
  static constexpr const char* TimeStep           = "Time Step";
  static constexpr const char* DynamicAMR         = "Dynamic AMR";
  static constexpr const char* Plot               = "Plot";
};
