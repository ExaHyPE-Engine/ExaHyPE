#include "fv/PatchUtils.h"

#include "tarch/Assertions.h"
#include "tarch/la/la.h"

#include "peano4/utils/Loop.h"

#include "exahype2/NonCriticalAssertions.h"



double  exahype2::fv::getVolumeLength(
  const tarch::la::Vector<2,double>&  h,
  int                                 numberOfVolumesPerAxisInPatch
) {
  return h(0)/numberOfVolumesPerAxisInPatch;
}


double  exahype2::fv::getVolumeLength(
  const tarch::la::Vector<3,double>&  h,
  int                                 numberOfVolumesPerAxisInPatch
) {
  assertion2( numberOfVolumesPerAxisInPatch>=1, h, numberOfVolumesPerAxisInPatch );

  return h(0)/numberOfVolumesPerAxisInPatch;
}


std::string exahype2::fv::plotVolume(
  const double* __restrict__ Q,
  int    unknowns
) {
  std::string result = "(" + std::to_string(Q[0]);
  for (int i=1; i<unknowns; i++) result += "," + std::to_string(Q[i]);
  result += ")";
  return result;
}


std::string exahype2::fv::plotPatch(
  const double* __restrict__ Q,
  int    unknowns,
  int    auxiliaryVariables,
  int    numberOfVolumesPerAxisInPatch,
  int    haloSize
) {
  const int PatchSize = numberOfVolumesPerAxisInPatch+2*haloSize;

  std::ostringstream result;

  dfor (k,PatchSize) {
    int index = peano4::utils::dLinearised(k,PatchSize) * (unknowns+auxiliaryVariables);

    // It is a diagonal entry if this counter is bigger than 1. If it equals
    // 0, it is interior. If it equals 1, then this is a face-connected halo
    // entry.
    bool isDiagonal = (tarch::la::count(k,0) + tarch::la::count(k,PatchSize-1))>1;

    result << "(";
    for (int i=0; i<unknowns+auxiliaryVariables; i++) {
      const int entry = index+i;

      if (i==0)
        result << "d[" << entry << "]=";
      else
        result << ",d[" << entry << "]=";

      if (haloSize==0 or not isDiagonal)
        result << Q[entry];
      else
        result << "x";
    }
    result << ")";
  }

  return result.str();
}



void exahype2::fv::validatePatch (
  const double *__restrict__ Q, int unknowns,
  int auxiliaryVariables,
  int numberOfVolumesPerAxisInPatch, int haloSize,
  const std::string &location,
  bool   triggerNonCriticalAssertion,
  double* minValues,
  double* maxValues
) {
  #if PeanoDebug>1 && !defined(OpenMPGPUOffloading)
  static tarch::logging::Log _log( "exahype2::fv" );
  logTraceInWith5Arguments( "validatePatch(...)", unknowns, auxiliaryVariables, numberOfVolumesPerAxisInPatch, haloSize, location );
  const int PatchSize = numberOfVolumesPerAxisInPatch+2*haloSize;
  dfor (k,PatchSize) {
    int index = peano4::utils::dLinearised(k,PatchSize) * (unknowns+auxiliaryVariables);

    // It is a diagonal entry if this counter is bigger than 1. If it equals
    // 0, it is interior. If it equals 1, then this is a face-connected halo
    // entry.
    bool isDiagonal = (tarch::la::count(k,0) + tarch::la::count(k,PatchSize-1))>1;

    if (haloSize==0 or not isDiagonal) {
      for (int i=0; i<unknowns+auxiliaryVariables; i++) {
        const int entry = index+i;
        bool dataValid = true;
        if (minValues!=nullptr and minValues[i]>Q[entry]) {
          dataValid = false;
        }
        if (maxValues!=nullptr and maxValues[i]<Q[entry]) {
          dataValid = false;
        }
        if (triggerNonCriticalAssertion) {
          nonCriticalAssertion11(
            Q[entry]==Q[entry] and std::isfinite(Q[entry]) and dataValid,
            Q[entry], unknowns,
            auxiliaryVariables,
            isDiagonal, haloSize, i,
            numberOfVolumesPerAxisInPatch, haloSize, k, i, location );
        }
        else {
          assertion12(
            Q[entry]==Q[entry] and std::isfinite(Q[entry]) and dataValid,
            Q[entry], unknowns,
            auxiliaryVariables,
            isDiagonal, haloSize, i,
            numberOfVolumesPerAxisInPatch, haloSize, k, i, location,
            plotPatch(Q,unknowns,auxiliaryVariables,numberOfVolumesPerAxisInPatch,haloSize)
          );
        }
      }
    }
  }
  logTraceOut( "validatePatch(...)" );
  #endif
}

