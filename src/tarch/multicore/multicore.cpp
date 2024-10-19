#include "multicore.h"
#include "tarch/Assertions.h"
#include "tarch/mpi/Rank.h"
#include "tarch/compiler/CompilerSpecificSettings.h"

#ifdef UseSmartMPI
#include "smartmpi.h"
#include "topology/topologies.h"
#endif

#include "tarch/multicore/otter.h"
#include "tarch/multicore/BooleanSemaphore.h"
#include "tarch/multicore/Core.h"
#include "tarch/multicore/Lock.h"
#include "tarch/multicore/Task.h"

#include "tarch/logging/Statistics.h"


namespace {
  /**
   * Task queue of tasks which we hold back
   */
  struct NonblockingTasks {
    NonblockingTasks()                        = default;
    ~NonblockingTasks()                       = default;
    NonblockingTasks(const NonblockingTasks&) = delete;

    typedef std::list<tarch::multicore::Task*> Tasks;
    Tasks                                      tasks;
    tarch::multicore::BooleanSemaphore         semaphore;
  };

  NonblockingTasks               pendingTasksThatCanBeFused;

  tarch::multicore::orchestration::Strategy* orchestrationStrategy = tarch::multicore::orchestration::createDefaultStrategy();

  tarch::logging::Log _log("tarch::multicore");

  const std::string BSPConcurrencyLevelStatisticsIdentifier("tarch::multicore::bsp-concurrency-level");
  const std::string SpawnedTasksStatisticsIdentifier("tarch::multicore::spawned-tasks");
} // namespace


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


void tarch::multicore::waitForTasks(const std::set<TaskNumber>& inDependencies) {
  tarch::multicore::native::waitForTasks(inDependencies);
}


void tarch::multicore::waitForTask(const int taskNumber) {
  std::set<int> tmp{taskNumber};
  waitForTasks(tmp);
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
        ::tarch::logging::Statistics::getInstance().inc(BSPConcurrencyLevelStatisticsIdentifier, static_cast<int>(tasks.size()), true);
        native::spawnAndWaitAsTaskLoop(tasks);
        ::tarch::logging::Statistics::getInstance().inc(BSPConcurrencyLevelStatisticsIdentifier, -static_cast<int>(tasks.size()), true);
        break;
      case tarch::multicore::orchestration::Strategy::ExecutionPolicy::RunParallelAndFuseTasksManuallyAfterwards:
        ::tarch::logging::Statistics::getInstance().inc(BSPConcurrencyLevelStatisticsIdentifier, static_cast<int>(tasks.size()), true);
        native::spawnAndWaitAsTaskLoop(tasks);
        ::tarch::logging::Statistics::getInstance().inc(BSPConcurrencyLevelStatisticsIdentifier, -static_cast<int>(tasks.size()), true);
        internal::fusePendingTasks();
        break;
      case tarch::multicore::orchestration::Strategy::ExecutionPolicy::RunParallelAndMapAllPendingFuseTasksOntoNativeTasksAfterwards:
        ::tarch::logging::Statistics::getInstance().inc(BSPConcurrencyLevelStatisticsIdentifier, static_cast<int>(tasks.size()), true);
        native::spawnAndWaitAsTaskLoop(tasks);
        ::tarch::logging::Statistics::getInstance().inc(BSPConcurrencyLevelStatisticsIdentifier, -static_cast<int>(tasks.size()), true);
        internal::mapPendingTasksOntoNativeTasks();
        break;
    }

    orchestrationStrategy->endBSPSection(localNestedSpawnAndWaits);

    nestingLock.lock();
    nestedSpawnAndWaits--;
    nestingLock.free();
  }
}


void tarch::multicore::spawnTask(
  Task*                        task,
  const std::set<TaskNumber>&  inDependencies,
  const TaskNumber&            taskNumber
) {
  assertion(task != nullptr);

  ::tarch::logging::Statistics::getInstance().inc(SpawnedTasksStatisticsIdentifier, static_cast<int>(tasks.size()), false, true);

  if (task->canFuse()) {
  }

  tarch::multicore::native::spawnTask(task, inDependencies, taskNumber);
}
