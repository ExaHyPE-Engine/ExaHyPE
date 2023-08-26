#include "AllOnGPU.h"

#include <limits>

tarch::multicore::orchestration::AllOnGPU::AllOnGPU(int device):
  _device(device),
  _isInBspSection(false) {}

void tarch::multicore::orchestration::AllOnGPU::startBSPSection(int nestedParallelismLevel) {
  if (nestedParallelismLevel <= 1) {
    _isInBspSection = true;
  }
}

void tarch::multicore::orchestration::AllOnGPU::endBSPSection(int nestedParallelismLevel) {
  if (nestedParallelismLevel <= 1) {
    _isInBspSection = false;
  }
}

int tarch::multicore::orchestration::AllOnGPU::getNumberOfTasksToHoldBack(int taskType) {
  return _isInBspSection ? std::numeric_limits<int>::max() : 0;
}

tarch::multicore::orchestration::AllOnGPU::FuseInstruction tarch::multicore::orchestration::AllOnGPU::
  getNumberOfTasksToFuseAndTargetDevice(int taskType) {
  return _isInBspSection
           ? FuseInstruction(-1, std::numeric_limits<int>::max(), 0)
           : FuseInstruction(_device, 1, std::numeric_limits<int>::max());
}

bool tarch::multicore::orchestration::AllOnGPU::fuseTasksImmediatelyWhenSpawned(int taskType) { return false; }

tarch::multicore::orchestration::Strategy::ExecutionPolicy tarch::multicore::orchestration::AllOnGPU::
  paralleliseForkJoinSection(int nestedParallelismLevel, int numberOfTasks, int codeLocationIdentifier) {
  if (nestedParallelismLevel <= 1) {
    return tarch::multicore::orchestration::Strategy::ExecutionPolicy::RunParallel;
  } else {
    return tarch::multicore::orchestration::Strategy::ExecutionPolicy::RunParallelAndIgnoreWithholdSubtasks;
  }
}
