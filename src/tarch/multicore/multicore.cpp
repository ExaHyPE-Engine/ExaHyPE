// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "multicore.h"

#include "otter.h"
#include "BooleanSemaphore.h"
#include "Core.h"
#include "Lock.h"
#include "Task.h"
#include "taskfusion/taskfusion.h"

#include "tarch/Assertions.h"
#include "tarch/mpi/Rank.h"
#include "tarch/compiler/CompilerSpecificSettings.h"

#include "tarch/logging/Statistics.h"

#ifdef UseSmartMPI
#include "smartmpi.h"
#include "topology/topologies.h"
#endif

namespace {
  tarch::multicore::orchestration::Strategy* orchestrationStrategy = tarch::multicore::orchestration::createDefaultStrategy();

  tarch::logging::Log _log("tarch::multicore");

  const std::string BSPConcurrencyLevelStatisticsIdentifier("tarch::multicore::bsp-concurrency-level");
  const std::string SpawnedTasksStatisticsIdentifier("tarch::multicore::spawned-tasks");
}


void tarch::multicore::initSmartMPI() {
  #ifdef UseSmartMPI
  using namespace smartmpi::topology;
  typedef UseSmartMPI MyTopology;
  smartmpi::topology::Topology* smartMPITopology = new MyTopology(
    tarch::mpi::Rank::getInstance().getCommunicator()
  );
  smartmpi::init( smartMPITopology );

  smartmpi::appendToSchedulerChain( "HardcodedMigrationScheduler" );

  tarch::mpi::Rank::getInstance().setCommunicator( smartMPITopology->computeNodeOrSmartServerCommunicator );
  #endif
}


void tarch::multicore::shutdownSmartMPI() {
  #ifdef UseSmartMPI
  smartmpi::shutdown();
  #endif
}


void tarch::multicore::setOrchestration(tarch::multicore::orchestration::Strategy* realisation) {
  assertion(orchestrationStrategy != nullptr);
  assertion(realisation != nullptr);

  delete orchestrationStrategy;
  orchestrationStrategy = realisation;
}


tarch::multicore::orchestration::Strategy* tarch::multicore::swapOrchestration(tarch::multicore::orchestration::Strategy* realisation) {
  assertion(orchestrationStrategy != nullptr);
  assertion(realisation != nullptr);

  tarch::multicore::orchestration::Strategy* result = orchestrationStrategy;
  orchestrationStrategy                             = realisation;
  return result;
}


tarch::multicore::orchestration::Strategy& tarch::multicore::getOrchestration() {
  return *orchestrationStrategy;
}


void tarch::multicore::waitForTasks(const std::set<TaskNumber>& inDependencies) {
  native::waitForTasks(inDependencies);
}


void tarch::multicore::waitForTask(const TaskNumber& taskNumber) {
  native::waitForTask(taskNumber);
}


void tarch::multicore::waitForTask(const double* inData) {
  native::waitForTask(inData);
}


void tarch::multicore::spawnAndWait(const std::vector<Task*>& tasks) {
  static tarch::logging::Log _log("tarch::multicore");

  if (not tasks.empty()) {
    static int                                nestedSpawnAndWaits = 0;
    static tarch::multicore::BooleanSemaphore nestingSemaphore;

    tarch::multicore::Lock nestingLock(nestingSemaphore, false);
    nestingLock.lock();
    nestedSpawnAndWaits++;
    const int localNestedSpawnAndWaits = nestedSpawnAndWaits;
    nestingLock.free();

    orchestrationStrategy->startBSPSection(localNestedSpawnAndWaits);

    switch (orchestrationStrategy->paralleliseForkJoinSection(localNestedSpawnAndWaits, tasks.size(), tasks[0]->getTaskType())) {
      case tarch::multicore::orchestration::Strategy::ExecutionPolicy::RunSerially:
        for (auto& p : tasks) {
          while (p->run()) {
            Core::getInstance().yield();
          }
          delete p;
        }
        break;
      case tarch::multicore::orchestration::Strategy::ExecutionPolicy::RunParallel:
        tarch::logging::Statistics::getInstance().inc(BSPConcurrencyLevelStatisticsIdentifier, static_cast<int>(tasks.size()), true);
        native::spawnAndWaitAsTaskLoop(tasks);
        tarch::logging::Statistics::getInstance().inc(BSPConcurrencyLevelStatisticsIdentifier, -static_cast<int>(tasks.size()), true);
        break;
    }

    orchestrationStrategy->endBSPSection(localNestedSpawnAndWaits);

    nestingLock.lock();
    nestedSpawnAndWaits--;
    nestingLock.free();
  }
}


void tarch::multicore::waitForAllTasks() {
  native::waitForAllTasks();
}


void tarch::multicore::spawnTask(
  Task*                        task,
  const std::set<TaskNumber>&  inDependencies,
  const TaskNumber&            taskNumber
) {
  assertion(task != nullptr);

  logging::Statistics::getInstance().inc(SpawnedTasksStatisticsIdentifier, 1, false, true);

  if (task->canFuse()) {
    taskfusion::translateFusableTaskIntoTaskSequence(task /*, inDependencies, taskNumber*/);
  } else {
    if (inDependencies.empty() and taskNumber == NoOutDependencies) native::spawnTask(task);
    else if (inDependencies.empty()) native::spawnTask(task, taskNumber);
    else native::spawnTask(task, inDependencies, taskNumber);
  }
}


void tarch::multicore::spawnTask(
  Task*                        task,
  const double*                outData
) {
  assertion(task != nullptr);
  assertion(outData);

  logging::Statistics::getInstance().inc(SpawnedTasksStatisticsIdentifier, 1, false, true);

  if (task->canFuse()) {
    taskfusion::translateFusableTaskIntoTaskSequence(task);
  } else {
    native::spawnTask(task, outData);
  }
}


#ifndef SharedMemoryParallelisation

void tarch::multicore::native::spawnAndWaitAsTaskLoop(const std::vector<Task*>& tasks) {
  for (auto& p : tasks) {
    while (p->run()) {
    }
    delete p;
  }
}


void tarch::multicore::native::spawnTask(
  Task* job,
  const std::set<TaskNumber>& /*inDependencies*/,
  const TaskNumber& /*taskNumber*/
) {
  while (job->run()) {
  }
  delete job;
}


void tarch::multicore::native::spawnTask(
  Task* job,
  const TaskNumber& /*taskNumber*/
) {
  while (job->run()) {
  }
  delete job;
}


void tarch::multicore::native::spawnTask(
  Task* job,
  const double* /*outData*/
) {
  while (job->run()) {
  }
  delete job;
}


void tarch::multicore::native::spawnTask(
  Task* job
) {
  while (job->run()) {
  }
  delete job;
}


void tarch::multicore::native::waitForTasks(const std::set<TaskNumber>& /*inDependencies*/) {}


void tarch::multicore::native::waitForTask(const TaskNumber& /*taskNumber*/) {}


void tarch::multicore::native::waitForTask(const double* /*inData*/) {}


void tarch::multicore::native::waitForAllTasks() {}

#endif // SharedMemoryParallelisation
