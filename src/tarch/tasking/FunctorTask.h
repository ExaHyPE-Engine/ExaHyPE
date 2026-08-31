// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/tasking/Task.h"

namespace tarch::tasking {
  class FunctorTask;
} // namespace tarch::tasking

/**
 * Task around a functor.
 *
 * The functor implements tarch::tasking::Task::run() and takes no arguments.
 * The class stores it by value, keeping captured state valid after the spawning
 * routine returns.
 */
class tarch::tasking::FunctorTask final : public tarch::tasking::Task {
public:
  FunctorTask()                              = delete;
  FunctorTask(const FunctorTask&)            = delete;
  FunctorTask(FunctorTask&&)                 = delete;
  FunctorTask& operator=(const FunctorTask&) = delete;
  FunctorTask& operator=(FunctorTask&&)      = delete;
  ~FunctorTask() override                    = default;

  explicit FunctorTask(std::function<void()> taskFunctor);

  void run() override;

private:
  std::function<void()> _taskFunctor;
};
