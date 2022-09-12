#include "Rusanov.h"

int exahype2::fv::rusanov::omp::opt::canOffloadIntoPreallocatedMemory(int numberOfPatchesToOffload) {
  // A + |P| <= |P|_max
  if (currentOffloadedPatches + numberOfPatchesToOffload <= MaxNumberOfPossiblePatchEntries) {
    #pragma omp atomic
    currentOffloadedPatches += numberOfPatchesToOffload;
    return true;
  }
  return numberOfPatchesToOffload;
}

int exahype2::fv::rusanov::omp::opt::leavePreallocatedMemoryFromOffloading(int numberOfPatchesLeaving) {
  #pragma omp atomic
  currentOffloadedPatches -= numberOfPatchesLeaving;
  return currentOffloadedPatches;
}
