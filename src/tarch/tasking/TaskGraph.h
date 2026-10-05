// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/CompilerSpecificSettings.h"

namespace tarch::tasking {
  class Task;

  enum class RunSerial { No, Yes };

  void spawnTask(
    tarch::tasking::Task*     task,
    std::span<void* const>    inDependencies  = {},
    std::span<void* const>    outDependencies = {},
    tarch::tasking::RunSerial runSerial       = tarch::tasking::RunSerial::No
  );

  INLINE_METHOD void spawnTask(
    tarch::tasking::Task*        task,
    std::initializer_list<void*> inDependencies,
    std::initializer_list<void*> outDependencies,
    tarch::tasking::RunSerial    runSerial = tarch::tasking::RunSerial::No
  );

  void waitForDependency(void* dependency);

  void waitForDependencies(std::span<void* const> dependencies);
} // namespace tarch::tasking

#include "tarch/tasking/TaskGraph.cpph"
