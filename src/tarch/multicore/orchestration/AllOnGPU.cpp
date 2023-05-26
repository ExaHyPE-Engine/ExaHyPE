#include <limits>
#include "AllOnGPU.h"
#include <limits>


tarch::multicore::orchestration::AllOnGPU::AllOnGPU(int device):
  _device(device),
  _isInBspSection(false) {
}


void tarch::multicore::orchestration::AllOnGPU::startBSPSection() {
  _isInBspSection = true;
}


void tarch::multicore::orchestration::AllOnGPU::endBSPSection() {
  _isInBspSection = false;
}


int tarch::multicore::orchestration::AllOnGPU::getNumberOfTasksToHoldBack() {
  return _isInBspSection ? std::numeric_limits<int>::max() : 0;
}


tarch::multicore::orchestration::AllOnGPU::FuseInstruction tarch::multicore::orchestration::AllOnGPU::getNumberOfTasksToFuseAndTargetDevice() {
  return _isInBspSection ? FuseInstruction(-1,std::numeric_limits<int>::max(),0) : FuseInstruction(_device,1,std::numeric_limits<int>::max());
}


bool tarch::multicore::orchestration::AllOnGPU::fuseTasksImmediatelyWhenSpawned() {
  return false;
}


tarch::multicore::orchestration::Strategy::ExecutionPolicy tarch::multicore::orchestration::AllOnGPU::getExecutionPolicyForForkJoinSection(int nestedParallelismLevel, int numberOfTasks, int codeLocationIdentifier) {
  return nestedParallelismLevel>1 ? tarch::multicore::orchestration::Strategy::ExecutionPolicy::EmbedWithoutSubTasks : tarch::multicore::orchestration::Strategy::ExecutionPolicy::MaximiseThroughput;
}
