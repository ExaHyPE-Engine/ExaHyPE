// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "PostprocessingKernels.h"
#include "peano4/Loop.h"


void exahype2::fv::mapInnerNeighbourVoxelAlongBoundayOntoAuxiliaryVariable(
  double* NOALIAS Q,
  int    unknowns,
  int    auxiliaryVariables,
  int    numberOfVolumesPerAxisInPatch,
  int    fromIndex,
  int    toIndex
) {
  const int stride = unknowns + auxiliaryVariables;

  for (int d=0; d<DIMENSIONS; d++) {
    dfore(k,numberOfVolumesPerAxisInPatch,d,0) {
      tarch::la::Vector<DIMENSIONS,int> src  = k;
      tarch::la::Vector<DIMENSIONS,int> dest = k;

      dest(d) = 0;
      for (int dd=0; dd<DIMENSIONS; dd++) {
        if (dest(dd)==0)                               src(dd)=1;
        if (dest(dd)==numberOfVolumesPerAxisInPatch-1) src(dd)=numberOfVolumesPerAxisInPatch-2;
      }
      Q[ peano4::dLinearised(dest, numberOfVolumesPerAxisInPatch) * stride + toIndex ]
        = Q[ peano4::dLinearised(src, numberOfVolumesPerAxisInPatch) * stride + fromIndex ];

      dest(d) = numberOfVolumesPerAxisInPatch-1;
      for (int dd=0; dd<DIMENSIONS; dd++) {
        if (dest(dd)==0)                               src(dd)=1;
        if (dest(dd)==numberOfVolumesPerAxisInPatch-1) src(dd)=numberOfVolumesPerAxisInPatch-2;
      }
      Q[ peano4::dLinearised(dest, numberOfVolumesPerAxisInPatch) * stride + toIndex ]
        = Q[ peano4::dLinearised(src, numberOfVolumesPerAxisInPatch) * stride + fromIndex ];
    }
  }

}
