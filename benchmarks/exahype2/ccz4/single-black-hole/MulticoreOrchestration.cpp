#include "MulticoreOrchestration.h"

#include <limits>

#include "tarch/multicore/Task.h"
#include "tarch/accelerator/Device.h"

#if !defined(PureFV)
#include "tasks/CCZ4SBH_FD4EnclaveTask.h"
#endif

#if !defined(PureFD4)
#include "tasks/CCZ4SBH_FVEnclaveTask.h"
#endif


benchmarks::exahype2::ccz4::MulticoreOrchestration::MulticoreOrchestration(
  const tarch::multicore::orchestration::FusionImplementation&  fusionImplementation
):
  _nestedBSPLevels(0),
  _maxFiniteVolumeTasks(0),
  _finiteVolumeTasksInThisBSPSection(0),
  _fusionImplementation(fusionImplementation)
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


benchmarks::exahype2::ccz4::MulticoreOrchestration::FuseInstruction benchmarks::exahype2::ccz4::MulticoreOrchestration::fuse(
  int taskType
) {
  int targetDevice = tarch::multicore::Task::Host;

  if (tarch::accelerator::Device::getInstance().getNumberOfDevices() > 0) {
    targetDevice = 1 + rand() % (tarch::accelerator::Device::getInstance().getNumberOfDevices());
  }

  auto FVFuseInstruction = benchmarks::exahype2::ccz4::MulticoreOrchestration::FuseInstruction(
    targetDevice, 1, 1
  );

  auto FDFuseInstruction = benchmarks::exahype2::ccz4::MulticoreOrchestration::FuseInstruction(
    targetDevice, 16, std::numeric_limits<int>::max()
  );

  #if defined(PureFD4) or defined(PureFV)
  return FDFuseInstruction;
  #else
  return taskType == tasks::CCZ4SBH_FVEnclaveTask::getEnclaveTaskTypeId()
                   ? FVFuseInstruction
                   : FDFuseInstruction;
  #endif
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


void benchmarks::exahype2::ccz4::MulticoreOrchestration::updateFuseStatistics(
  int    taskType,
  int    queueSize,
  double averageNumberOfPendingReadyTasksPerProcessTaskExecution,
  double probabilityThatTaskIsCompleted
) {}


tarch::multicore::orchestration::FusionImplementation benchmarks::exahype2::ccz4::MulticoreOrchestration::getFusionImplementation() const {
  return _fusionImplementation;
}
