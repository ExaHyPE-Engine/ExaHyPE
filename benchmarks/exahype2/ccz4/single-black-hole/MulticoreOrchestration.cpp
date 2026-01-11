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


benchmarks::exahype2::ccz4::MulticoreOrchestration::FuseInstruction benchmarks::exahype2::ccz4::MulticoreOrchestration::fuse(
  int taskType
) {
  auto FVFuseInstruction = benchmarks::exahype2::ccz4::MulticoreOrchestration::FuseInstruction(
    0, 1, 1
  );

  auto FDFuseInstruction = benchmarks::exahype2::ccz4::MulticoreOrchestration::FuseInstruction(
    0, 16, std::numeric_limits<int>::max()
  );

  #if defined(PureFD4)
  return FDFuseInstruction;
  #elif defined(PureFV)
  return FVFuseInstruction;
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
