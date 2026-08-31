// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/CompilerSpecificSettings.h"

namespace tarch::tasking {
  class Task;

  enum class RunSerial { No, Yes };

  void spawnTask(
    tarch::tasking::Task* const     task,
    const std::span<void* const>    inDependencies  = {},
    const std::span<void* const>    outDependencies = {},
    const tarch::tasking::RunSerial runSerial       = tarch::tasking::RunSerial::No
  );

  INLINE_METHOD void spawnTask(
    tarch::tasking::Task* const        task,
    const std::initializer_list<void*> inDependencies,
    const std::initializer_list<void*> outDependencies,
    const tarch::tasking::RunSerial    runSerial = tarch::tasking::RunSerial::No
  );

  void waitForDependency(void* const dependency);

  void waitForDependencies(const std::span<void* const> dependencies);
} // namespace tarch::tasking

#include "tarch/tasking/TaskGraph.cpph"
