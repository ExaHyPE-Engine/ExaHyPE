#include "MultigridDefaultMulticoreOrchestration.h"

#include "tarch/Assertions.h"
#include "tarch/accelerator/Device.h"
#include "tarch/multicore/Task.h"

#include "peano4/parallel/parallel.h"

#include <limits>

mghype::MultigridDefaultMulticoreOrchestration::MultigridDefaultMulticoreOrchestration():
  _isInBspSection(false) {}

void mghype::MultigridDefaultMulticoreOrchestration::startBSPSection(int nestedParallelismLevel) {
  if (nestedParallelismLevel <= 1) {
    _isInBspSection = true;
  }
}

void mghype::MultigridDefaultMulticoreOrchestration::endBSPSection(int nestedParallelismLevel) {
  if (nestedParallelismLevel <= 1) {
    _isInBspSection = false;
  }
}

tarch::multicore::orchestration::Strategy::FuseInstruction mghype::MultigridDefaultMulticoreOrchestration::fuse(int taskType) {
  if (
    not _residualComputationTasks.contains(taskType)
    and
    not _operatorInversionTasks.contains(taskType)
  ) {
    if ( peano4::parallel::getClassNameOfType(taskType).find("UpdateResidualTask")!=std::string::npos ) {
      _residualComputationTasks.insert( taskType );
    }
    if ( peano4::parallel::getClassNameOfType(taskType).find("InvertTask")!=std::string::npos ) {
      _operatorInversionTasks.insert( taskType );
    }
  }

  assertion1(
    _residualComputationTasks.contains(taskType)
    or
    _operatorInversionTasks.contains(taskType),
    taskType
  );

  constexpr int fusedResidualTasks = 65536;

  if (_residualComputationTasks.contains(taskType)) {
//    std::cout << "a";
    return FuseInstruction(tarch::multicore::Task::Host, fusedResidualTasks, 2*fusedResidualTasks, std::chrono::microseconds(1000));
  }
  if (_operatorInversionTasks.contains(taskType)) {
//    std::cout << "b";
    return FuseInstruction(tarch::multicore::Task::Host, 0, 1);
  }

  assertion(false);

  return FuseInstruction(tarch::multicore::Task::Host, 0, 1);
}


tarch::multicore::orchestration::Strategy::ExecutionPolicy mghype::MultigridDefaultMulticoreOrchestration::paralleliseForkJoinSection(
  int nestedParallelismLevel,
  int /*numberOfTasks*/,
  int /*codeLocationIdentifier*/
) {
  if (nestedParallelismLevel <= 1) {
    return tarch::multicore::orchestration::Strategy::ExecutionPolicy::RunParallel;
  } else {
    return tarch::multicore::orchestration::Strategy::ExecutionPolicy::RunSerially;
  }
}


void mghype::MultigridDefaultMulticoreOrchestration::updateFuseStatistics(
  int    taskType,
  int    queueSize,
  double averageNumberOfPendingReadyTasksPerProcessTaskExecution
) {
  // empty on purpose
}
