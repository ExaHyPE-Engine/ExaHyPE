#include "Generic.h"

#include "tarch/logging/Log.h"
#include "tarch/multicore/multicore.h"

#include "../PatchUtils.h"

#include "peano4/utils/Loop.h"

#include "exahype2/NonCriticalAssertions.h"


std::string exahype2::aderdg::plotVolume(
  double Q[],
  int    unknowns
) {
//  std::string result = "(" + std::to_string(Q[0]);
//  for (int i=1; i<unknowns; i++) result += "," + std::to_string(Q[i]);
//  result += ")";
//  return result;
};


void exahype2::aderdg::validatePatch(
  double* __restrict__ Q,
  int    unknowns,
  int    auxiliaryVariables,
  int    numberOfVolumesPerAxisInPatch,
  int    haloSize,
  const std::string& location
) {
//  const int PatchSize = numberOfVolumesPerAxisInPatch+2*haloSize;
//  dfor (k,PatchSize) {
//    int index = peano4::utils::dScalar(k,PatchSize) * (unknowns+auxiliaryVariables);
//	for (int i=0; i<unknowns+auxiliaryVariables; i++) {
//      nonCriticalAssertion7( std::isfinite(Q[index+i]), unknowns, auxiliaryVariables, numberOfVolumesPerAxisInPatch, haloSize, k, i, location );
//	}
//  }
}



#if defined(GPUOffloading)
#pragma omp declare target
#endif
void exahype2::aderdg::copyPatch(
  double* __restrict__ QinWithHalo,
  double* __restrict__ QOutWithoutHalo,
  int    unknowns,
  int    auxiliaryVariables,
  int    numberOfVolumesPerAxisInPatch,
  int    haloSize
) {
//  dfor(k,numberOfVolumesPerAxisInPatch) {
//    tarch::la::Vector<Dimensions,int>   source = k + tarch::la::Vector<Dimensions,int>(haloSize);
//    int sourceSerialised      = peano4::utils::dScalar(source,numberOfVolumesPerAxisInPatch+haloSize*2);
//    int destinationSerialised = peano4::utils::dScalar(k,numberOfVolumesPerAxisInPatch);
//    for (int i=0; i<unknowns+auxiliaryVariables; i++) {
//      QOutWithoutHalo[destinationSerialised*(unknowns+auxiliaryVariables)+i] = QinWithHalo[sourceSerialised*(unknowns+auxiliaryVariables)+i];
//    }
//  }

/*
  #if Dimensions==2
  int sourceSerialised      = numberOfVolumesPerAxisInPatch+haloSize*2+haloSize;
  int destinationSerialised = 0;
  for (int y=0; y<numberOfVolumesPerAxisInPatch; y++) {
    for (int x=0; x<numberOfVolumesPerAxisInPatch; x++) {
      for (int i=0; i<unknowns+auxiliaryVariables; i++) {
        QOutWithoutHalo[destinationSerialised*(unknowns+auxiliaryVariables)+i] = QinWithHalo[sourceSerialised*(unknowns+auxiliaryVariables)+i];
      }
      sourceSerialised++;
      destinationSerialised++;
    }
    sourceSerialised += 2*haloSize;
  }
  #else
  int sourceSerialised      = (numberOfVolumesPerAxisInPatch+haloSize*2)*(numberOfVolumesPerAxisInPatch+haloSize*2) + numberOfVolumesPerAxisInPatch+haloSize*2+haloSize;
  int destinationSerialised = 0;
  for (int z=0; z<numberOfVolumesPerAxisInPatch; z++) {
    for (int y=0; y<numberOfVolumesPerAxisInPatch; y++) {
      for (int x=0; x<numberOfVolumesPerAxisInPatch; x++) {
        for (int i=0; i<unknowns+auxiliaryVariables; i++) {
          QOutWithoutHalo[destinationSerialised*(unknowns+auxiliaryVariables)+i] = QinWithHalo[sourceSerialised*(unknowns+auxiliaryVariables)+i];
        }
        sourceSerialised++;
        destinationSerialised++;
      }
      sourceSerialised += 2*haloSize;
    }
    sourceSerialised += 2*haloSize+2*(numberOfVolumesPerAxisInPatch+haloSize*2);
  }
  #endif
  */
}
#if defined(GPUOffloading)
#pragma omp end declare target
#endif
