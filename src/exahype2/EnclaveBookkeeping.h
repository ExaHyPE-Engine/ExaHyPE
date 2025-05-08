// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/Assertions.h"
#include "tarch/logging/Log.h"
#include "tarch/logging/Statistics.h"
#include "tarch/multicore/BooleanSemaphore.h"
#include "tarch/multicore/Lock.h"
#include "tarch/multicore/multicore.h"
#include "tarch/multicore/Task.h"

#include <list>
#include <map>

namespace exahype2 {
  class EnclaveBookkeeping;
} // namespace exahype2

class exahype2::EnclaveBookkeeping {
private:
  static tarch::logging::Log _log;

  std::map<int, std::list<tarch::multicore::Task*>> pendingTasks;
  tarch::multicore::BooleanSemaphore                pendingTasksSemaphore;

  EnclaveBookkeeping() = default;

public:
  EnclaveBookkeeping(const EnclaveBookkeeping&) = delete;
  EnclaveBookkeeping& operator=(const EnclaveBookkeeping&) = delete;
  EnclaveBookkeeping(EnclaveBookkeeping&&) = delete;
  EnclaveBookkeeping& operator=(EnclaveBookkeeping&&) = delete;

  ~EnclaveBookkeeping();

  static EnclaveBookkeeping& getInstance() {
    static EnclaveBookkeeping instance;
    return instance;
  }

  void bookmarkTask(tarch::multicore::Task* task);
  void clear();

  void fusePendingTasks();

  std::map<int, std::size_t> getNumberOfTasksPerType() const;
  std::size_t getTotalNumberOfTasks() const;
};
