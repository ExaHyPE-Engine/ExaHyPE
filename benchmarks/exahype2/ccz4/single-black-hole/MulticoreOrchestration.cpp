#include "MulticoreOrchestration.h"

#include <limits>

#include "tarch/multicore/Tasks.h"
#include "tarch/accelerator/Device.h"
#ifdef PureFV
#else
#include "tasks/CCZ4SBH_FD4EnclaveTask.h"
#endif

#ifdef PureFD4
#else
#include "tasks/CCZ4SBH_FVEnclaveTask.h"
#endif

benchmarks::exahype2::ccz4::MulticoreOrchestration::MulticoreOrchestration():
  _nestedBSPLevels(0),
  _maxFiniteVolumeTasks(0),
  _finiteVolumeTasksInThisBSPSection(0)
  {}

void benchmarks::exahype2::ccz4::MulticoreOrchestration::startBSPSection(int nestedParallelismLevel) {
  if (_nestedBSPLevels==0) {
    _finiteVolumeTasksInThisBSPSection = 0;
  }
  _nestedBSPLevels++;

}

void benchmarks::exahype2::ccz4::MulticoreOrchestration::endBSPSection(int nestedParallelismLevel) {
  _nestedBSPLevels--;
  if (_nestedBSPLevels==0) {
    _maxFiniteVolumeTasks = std::max(_maxFiniteVolumeTasks, _finiteVolumeTasksInThisBSPSection);
  }
}

int benchmarks::exahype2::ccz4::MulticoreOrchestration::getNumberOfTasksToHoldBack(int taskType) {
  if ( taskType == tasks::CCZ4SBH_FVEnclaveTask::getEnclaveTaskTypeId() ) {
    _finiteVolumeTasksInThisBSPSection++;
  }

  if (
    _nestedBSPLevels>0
    and
    tarch::accelerator::Device::getInstance().getNumberOfDevices()>0
    and
    taskType == tasks::CCZ4SBH_FVEnclaveTask::getEnclaveTaskTypeId()
  ) {
    return _maxFiniteVolumeTasks;
  }
  else {
    return 0;
  }
}

benchmarks::exahype2::ccz4::MulticoreOrchestration::FuseInstruction benchmarks::exahype2::ccz4::MulticoreOrchestration::getNumberOfTasksToFuseAndTargetDevice(
  int taskType
) {
  if (
    taskType == tasks::CCZ4SBH_FVEnclaveTask::getEnclaveTaskTypeId()
  ) {
    return benchmarks::exahype2::ccz4::MulticoreOrchestration::FuseInstruction(
      0, 1, 1
    );
  }
  else {
    return benchmarks::exahype2::ccz4::MulticoreOrchestration::FuseInstruction(
      0, 16, std::numeric_limits<int>::max()
    );
  }
}

bool benchmarks::exahype2::ccz4::MulticoreOrchestration::fuseTasksImmediatelyWhenSpawned(int taskType) {
  return true;
}

tarch::multicore::orchestration::Strategy::ExecutionPolicy benchmarks::exahype2::ccz4::MulticoreOrchestration::paralleliseForkJoinSection(
  int nestedParallelismLevel, int numberOfTasks, int codeLocationIdentifier
) {
  if (nestedParallelismLevel > 1) {
    return tarch::multicore::orchestration::Strategy::ExecutionPolicy::RunSerially;
  }
  else {
    return tarch::multicore::orchestration::Strategy::ExecutionPolicy::RunParallel;
  }
}
