// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

namespace tarch::tasking {
  class Task;
} // namespace tarch::tasking

/**
 * tarch::tasking::spawnTask() owns each instance and destroys it once run()
 * returns.
 */
class tarch::tasking::Task {
public:
  Task()                       = default;
  Task(const Task&)            = delete;
  Task(Task&&)                 = delete;
  Task& operator=(const Task&) = delete;
  Task& operator=(Task&&)      = delete;
  virtual ~Task()              = default;

  virtual void run() = 0;
};
