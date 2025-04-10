#include "AllOnGPU.h"

#include "tarch/accelerator/Device.h"
#include "tarch/multicore/Task.h"

#include <limits>

tarch::multicore::orchestration::AllOnGPU::AllOnGPU(int device):
  _device(device),
  _deviceToUse(device==RoundRobin ? 0 : _device),
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

tarch::multicore::orchestration::AllOnGPU::FuseInstruction tarch::multicore::orchestration::AllOnGPU::fuse(int /*taskType*/) {
  if (_device==RoundRobin) toggleDevice();
  return FuseInstruction(_deviceToUse, 1, std::numeric_limits<int>::max());
}


tarch::multicore::orchestration::Strategy::ExecutionPolicy tarch::multicore::orchestration::AllOnGPU::paralleliseForkJoinSection(
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


void tarch::multicore::orchestration::AllOnGPU::toggleDevice() {
  if (tarch::accelerator::Device::getInstance().getNumberOfDevices() == 0) {
    _deviceToUse = Task::Host;
  } else {
    _deviceToUse++;
    _deviceToUse = _deviceToUse % tarch::accelerator::Device::getInstance().getNumberOfDevices();
  }
}


void tarch::multicore::orchestration::AllOnGPU::updateFuseStatistics(
  int    taskType,
  int    queueSize,
  double averageNumberOfPendingReadyTasksPerProcessTaskExecution
) {
  // empty on purpose
}
