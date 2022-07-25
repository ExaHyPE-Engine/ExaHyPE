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
  int    haloSize,
  bool   prettyPrint
) {
  const int PatchSize = numberOfVolumesPerAxisInPatch+2*haloSize;

  std::ostringstream result;

  if (prettyPrint) result << std::endl;

  #if Dimensions==2
  for (int y=0; y<PatchSize; y++) {
    for (int x=0; x<PatchSize; x++) {
      int index = (y*PatchSize+x) * (unknowns+auxiliaryVariables);

      // It is a diagonal entry if this counter is bigger than 1. If it equals
      // 0, it is interior. If it equals 1, then this is a face-connected halo
      // entry.
      bool isDiagonal = (x<haloSize and y<haloSize)
                     or (x>=numberOfVolumesPerAxisInPatch+haloSize and y<haloSize)
                     or (x<haloSize and y>=numberOfVolumesPerAxisInPatch+haloSize)
                     or (x>=numberOfVolumesPerAxisInPatch+haloSize and y>=numberOfVolumesPerAxisInPatch+haloSize);

      result << "(";
      for (int i=0; i<unknowns+auxiliaryVariables; i++) {
        const int entry = index+i;

        if (i!=0)
          result << ",";

        if (not isDiagonal)
          result << Q[entry];
        else
          result << "x";
      }
      result << ")";
    }
    result << std::endl;
  }
  #else
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
  #endif

  if (prettyPrint) result << std::endl;

  return result.str();
}



std::string exahype2::fv::plotPatchOverlap(
  const double* __restrict__ Q,
  int    unknowns,
  int    auxiliaryVariables,
  int    numberOfVolumesPerAxisInPatch,
  int    haloSize,
  int    normal,
  bool   prettyPrint
) {
  assertion(normal>=0);
  assertion(normal<Dimensions);

  std::ostringstream result;

  if (prettyPrint) result << std::endl;

  #if Dimensions==2
  if (normal==0) {
    for (int y=0; y<numberOfVolumesPerAxisInPatch; y++) {
      for (int x=0; x<2*haloSize; x++) {
        int index = (y*numberOfVolumesPerAxisInPatch+x) * (unknowns+auxiliaryVariables);
        result << "(";
        for (int i=0; i<unknowns+auxiliaryVariables; i++) {
          const int entry = index+i;

          if (i!=0)
            result << ",";

          result << Q[entry];
        }
        result << ")";
        if (x==haloSize-1)
          result << " | ";
      }
      result << std::endl;
    }
  }
  else {
    result << "<not implemented yet>";
  }
  #else
  result << "<not implemented yet>";
  #endif

  if (prettyPrint) result << std::endl;

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
  #if PeanoDebug>1 && !defined(GPUOffloadingOMP)
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
