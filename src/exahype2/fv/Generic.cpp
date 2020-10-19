#include "Generic.h"

#include "tarch/logging/Log.h"
#include "tarch/multicore/multicore.h"

#include "../PatchUtils.h"

#include "peano4/utils/Loop.h"


std::string exahype2::fv::plotVolume(
  double Q[],
  int    unknowns
) {
  std::string result = "(" + std::to_string(Q[0]);
  for (int i=1; i<unknowns; i++) result += "," + std::to_string(Q[i]);
  result += ")";
  return result;
};


void exahype2::fv::copyPatch(
  double* __restrict__ QinWithHalo,
  double* __restrict__ QOutWithoutHalo,
  int    unknowns,
  int    auxiliaryVariables,
  int    numberOfVolumesPerAxisInPatch,
  int    haloSize
) {
  dfor(k,numberOfVolumesPerAxisInPatch) {
    tarch::la::Vector<Dimensions,int>   source = k + tarch::la::Vector<Dimensions,int>(haloSize);
    int sourceSerialised      = peano4::utils::dLinearised(source,numberOfVolumesPerAxisInPatch+haloSize*2);
    int destinationSerialised = peano4::utils::dLinearised(k,numberOfVolumesPerAxisInPatch);
    for (int i=0; i<unknowns+auxiliaryVariables; i++) {
      QOutWithoutHalo[destinationSerialised*(unknowns+auxiliaryVariables)+i] = QinWithHalo[sourceSerialised*(unknowns+auxiliaryVariables)+i];
    }
  }
}


void exahype2::fv::gpu::copyPatch(
  double* __restrict__ QinWithHalo,
  double* __restrict__ QOutWithoutHalo,
  int    unknowns,
  int    auxiliaryVariables,
  int    numberOfVolumesPerAxisInPatch,
  int    haloSize
) {
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
}


void exahype2::fv::applySplit1DRiemannToPatch_Overlap1AoS2d(
      std::function< void(
        double * __restrict__ QL,
        double * __restrict__ QR,
        const tarch::la::Vector<Dimensions,double>&  faceCentre,
        double                                       volumeH,
        double                                       t,
        double                                       dt,
        int                                          normal,
        double * __restrict__ FL,
        double * __restrict__ FR
      ) >   splitRiemannSolve1d,
      const tarch::la::Vector<Dimensions,double>&  patchCentre,
      const tarch::la::Vector<Dimensions,double>&  patchSize,
      double                                       t,
      double                                       dt,
      int                                          numberOfVolumesPerAxisInPatch,
      int                                          unknowns,
      int                                          auxiliaryVariables,
      double * __restrict__ Qin,
      double * __restrict__ Qout
) {
  static tarch::logging::Log _log( "exahype2::fv" );
  logTraceInWith6Arguments( "applySplit1DRiemannToPatch_Overlap1AoS2d(...)", patchCentre, patchSize, t, dt, numberOfVolumesPerAxisInPatch, unknowns );

  assertion( dt>=tarch::la::NUMERICAL_ZERO_DIFFERENCE );

  tarch::la::Vector<Dimensions,double> volumeH = exahype2::getVolumeSize(patchSize, numberOfVolumesPerAxisInPatch);

  double numericalFluxL[unknowns]; // helper out variable
  double numericalFluxR[unknowns]; // helper out variable

  for (int y=0; y<numberOfVolumesPerAxisInPatch; y++)
  for (int x=0; x<=numberOfVolumesPerAxisInPatch; x++) {
    const int leftVoxelInPreimage  =  x
                                   + (y+1) * (2 + numberOfVolumesPerAxisInPatch);
    const int rightVoxelInPreimage =  x+1
                                   + (y+1) * (2 + numberOfVolumesPerAxisInPatch);

    const int leftVoxelInImage     = x-1
                                   + y * numberOfVolumesPerAxisInPatch;
    const int rightVoxelInImage    = x
                                   + y * numberOfVolumesPerAxisInPatch;

    tarch::la::Vector<Dimensions,double> volumeX = patchCentre - 0.5 * patchSize;
    volumeX(0) += x * volumeH(0);
    volumeX(1) += (y+0.5) * volumeH(1);

    splitRiemannSolve1d(
      Qin + leftVoxelInPreimage*(unknowns+auxiliaryVariables),
      Qin + rightVoxelInPreimage*(unknowns+auxiliaryVariables),
      volumeX,volumeH(0), t, dt, 0, //  last argument = normal
      numericalFluxL, numericalFluxR
    );

    for (int unknown=0; unknown<unknowns; unknown++) {
      if (x>0) {
        Qout[ leftVoxelInImage*(unknowns+auxiliaryVariables)+unknown ] -= dt / volumeH(0) * numericalFluxL[unknown];
      }
      if (x<numberOfVolumesPerAxisInPatch) {
        Qout[ rightVoxelInImage*(unknowns+auxiliaryVariables)+unknown ] += dt / volumeH(0) * numericalFluxR[unknown];
      }
    }
  }

  for (int y=0; y<=numberOfVolumesPerAxisInPatch; y++)
  for (int x=0; x<numberOfVolumesPerAxisInPatch; x++) {
    const int lowerVoxelInPreimage =  x+1
                                   +  y * (2 + numberOfVolumesPerAxisInPatch);
    const int upperVoxelInPreimage =  x+1
                                   + (y+1) * (2 + numberOfVolumesPerAxisInPatch);

    const int lowerVoxelInImage    = x
                                   + (y-1) * numberOfVolumesPerAxisInPatch;
    const int upperVoxelInImage    = x
                                   + y * numberOfVolumesPerAxisInPatch;

    tarch::la::Vector<Dimensions,double> volumeX = patchCentre - 0.5 * patchSize;
    volumeX(0) += (x+0.5) * volumeH(0);
    volumeX(1) += y * volumeH(1);

    splitRiemannSolve1d(
      Qin + lowerVoxelInPreimage*(unknowns+auxiliaryVariables),
      Qin + upperVoxelInPreimage*(unknowns+auxiliaryVariables),
      volumeX, volumeH(0), t, dt, 1, //  last argument = normal
      numericalFluxL, numericalFluxR
    );

    for (int unknown=0; unknown<unknowns; unknown++) {
      if (y>0) {
        Qout[ lowerVoxelInImage*(unknowns+auxiliaryVariables)+unknown ] -= dt / volumeH(0) * numericalFluxL[unknown];
      }
      if (y<numberOfVolumesPerAxisInPatch) {
        Qout[ upperVoxelInImage*(unknowns+auxiliaryVariables)+unknown ] += dt / volumeH(0) * numericalFluxR[unknown];
      }
    }
  }

  logTraceOut( "applySplit1DRiemannToPatch_Overlap1AoS2d(...)" );
}




void exahype2::fv::applySplit1DRiemannToPatch_Overlap1AoS3d(
      std::function< void(
        double * __restrict__ QL,
        double * __restrict__ QR,
        const tarch::la::Vector<Dimensions,double>&  faceCentre,
        double                                       volumeH,
        double                                       t,
        double                                       dt,
        int                                          normal,
        double * __restrict__ FL,
        double * __restrict__ FR
      ) >   splitRiemannSolve1d,
      const tarch::la::Vector<Dimensions,double>&  patchCentre,
      const tarch::la::Vector<Dimensions,double>&  patchSize,
      double                                       t,
      double                                       dt,
      int                                          numberOfVolumesPerAxisInPatch,
      int                                          unknowns,
      int                                          auxiliaryVariables,
      double * __restrict__ Qin,
      double * __restrict__ Qout
) {
  static tarch::logging::Log _log( "exahype2::fv" );
  logTraceInWith6Arguments( "applySplit1DRiemannToPatch_Overlap1AoS3d(...)", patchCentre, patchSize, t, dt, numberOfVolumesPerAxisInPatch, unknowns );

  assertion( dt>=tarch::la::NUMERICAL_ZERO_DIFFERENCE );

  tarch::la::Vector<Dimensions,double> volumeH = exahype2::getVolumeSize(patchSize, numberOfVolumesPerAxisInPatch);

  double numericalFluxL[unknowns]; // helper out variable
  double numericalFluxR[unknowns]; // helper out variable

  for (int z=0; z<numberOfVolumesPerAxisInPatch; z++)
  for (int y=0; y<numberOfVolumesPerAxisInPatch; y++)
  for (int x=0; x<=numberOfVolumesPerAxisInPatch; x++) {
    const int leftVoxelInPreimage  =  x
                                   + (y+1) * (2 + numberOfVolumesPerAxisInPatch)
                                   + (z+1) * (2 + numberOfVolumesPerAxisInPatch) * (2 + numberOfVolumesPerAxisInPatch);
    const int rightVoxelInPreimage =  x+1
                                   + (y+1) * (2 + numberOfVolumesPerAxisInPatch)
                                   + (z+1) * (2 + numberOfVolumesPerAxisInPatch) * (2 + numberOfVolumesPerAxisInPatch);

    const int leftVoxelInImage     = x-1
                                   + y * numberOfVolumesPerAxisInPatch
                                   + z * numberOfVolumesPerAxisInPatch * numberOfVolumesPerAxisInPatch;
    const int rightVoxelInImage    = x
                                   + y * numberOfVolumesPerAxisInPatch
                                   + z * numberOfVolumesPerAxisInPatch * numberOfVolumesPerAxisInPatch;

    tarch::la::Vector<Dimensions,double> volumeX = patchCentre - 0.5 * patchSize;
    volumeX(0) += x * volumeH(0);
    volumeX(1) += (y+0.5) * volumeH(1);
    volumeX(2) += (z+0.5) * volumeH(2);

    splitRiemannSolve1d(
      Qin + leftVoxelInPreimage*(unknowns+auxiliaryVariables),
      Qin + rightVoxelInPreimage*(unknowns+auxiliaryVariables),
      volumeX, volumeH(0), t, dt, 0, //  last argument = normal
      numericalFluxL, numericalFluxR
    );

    for (int unknown=0; unknown<unknowns; unknown++) {
      if (x>0) {
        Qout[ leftVoxelInImage*(unknowns+auxiliaryVariables)+unknown ] -= dt / volumeH(0) * numericalFluxL[unknown];
      }
      if (x<numberOfVolumesPerAxisInPatch) {
        Qout[ rightVoxelInImage*(unknowns+auxiliaryVariables)+unknown ] += dt / volumeH(0) * numericalFluxR[unknown];
      }
    }
  }

  for (int z=0; z<numberOfVolumesPerAxisInPatch; z++)
  for (int y=0; y<=numberOfVolumesPerAxisInPatch; y++)
  for (int x=0; x<numberOfVolumesPerAxisInPatch; x++) {
    const int lowerVoxelInPreimage =  x+1
                                   +  y * (2 + numberOfVolumesPerAxisInPatch)
                                   + (z+1) * (2 + numberOfVolumesPerAxisInPatch) * (2 + numberOfVolumesPerAxisInPatch);
    const int upperVoxelInPreimage =  x+1
                                   + (y+1) * (2 + numberOfVolumesPerAxisInPatch)
                                   + (z+1) * (2 + numberOfVolumesPerAxisInPatch) * (2 + numberOfVolumesPerAxisInPatch);

    const int lowerVoxelInImage    = x
                                   + (y-1) * numberOfVolumesPerAxisInPatch
                                   + z * numberOfVolumesPerAxisInPatch * numberOfVolumesPerAxisInPatch;
    const int upperVoxelInImage    = x
                                   + y * numberOfVolumesPerAxisInPatch
                                   + z * numberOfVolumesPerAxisInPatch * numberOfVolumesPerAxisInPatch;


    tarch::la::Vector<Dimensions,double> volumeX = patchCentre - 0.5 * patchSize;
    volumeX(0) += (x+0.5) * volumeH(0);
    volumeX(1) += y * volumeH(1);
    volumeX(2) += (z+0.5) * volumeH(2);

    splitRiemannSolve1d(
      Qin + lowerVoxelInPreimage*(unknowns+auxiliaryVariables),
      Qin + upperVoxelInPreimage*(unknowns+auxiliaryVariables),
      volumeX, volumeH(0), t, dt, 1, //  last argument = normal
      numericalFluxL, numericalFluxR
    );

    for (int unknown=0; unknown<unknowns; unknown++) {
      if (y>0) {
        Qout[ lowerVoxelInImage*(unknowns+auxiliaryVariables)+unknown ] -= dt / volumeH(0) * numericalFluxL[unknown];
      }
      if (y<numberOfVolumesPerAxisInPatch) {
        Qout[ upperVoxelInImage*(unknowns+auxiliaryVariables)+unknown ] += dt / volumeH(0) * numericalFluxR[unknown];
      }
    }
  }


  for (int z=0; z<=numberOfVolumesPerAxisInPatch; z++)
  for (int y=0; y<numberOfVolumesPerAxisInPatch; y++)
  for (int x=0; x<numberOfVolumesPerAxisInPatch; x++) {
    const int lowerVoxelInPreimage =  x+1
                                   + (y+1) * (2 + numberOfVolumesPerAxisInPatch)
                                   +  z * (2 + numberOfVolumesPerAxisInPatch) * (2 + numberOfVolumesPerAxisInPatch);
    const int upperVoxelInPreimage =  x+1
                                   + (y+1) * (2 + numberOfVolumesPerAxisInPatch)
                                   + (z+1) * (2 + numberOfVolumesPerAxisInPatch) * (2 + numberOfVolumesPerAxisInPatch);

    const int lowerVoxelInImage    = x
                                   + y * numberOfVolumesPerAxisInPatch
                                   + (z-1) * numberOfVolumesPerAxisInPatch * numberOfVolumesPerAxisInPatch;
    const int upperVoxelInImage    = x
                                   + y * numberOfVolumesPerAxisInPatch
                                   + z * numberOfVolumesPerAxisInPatch * numberOfVolumesPerAxisInPatch;

    tarch::la::Vector<Dimensions,double> volumeX = patchCentre - 0.5 * patchSize;
    volumeX(0) += (x+0.5) * volumeH(0);
    volumeX(1) += (y+0.5) * volumeH(1);
    volumeX(2) += z * volumeH(2);

    splitRiemannSolve1d(
      Qin + lowerVoxelInPreimage*(unknowns+auxiliaryVariables),
      Qin + upperVoxelInPreimage*(unknowns+auxiliaryVariables),
      volumeX, volumeH(0), t, dt, 2, //  last argument = normal
      numericalFluxL, numericalFluxR
    );

    for (int unknown=0; unknown<unknowns; unknown++) {
      if (z>0) {
        Qout[ lowerVoxelInImage*(unknowns+auxiliaryVariables)+unknown ] -= dt / volumeH(0) * numericalFluxL[unknown];
      }
      if (z<numberOfVolumesPerAxisInPatch) {
        Qout[ upperVoxelInImage*(unknowns+auxiliaryVariables)+unknown ] += dt / volumeH(0) * numericalFluxR[unknown];
      }
    }
  }

  logTraceOut( "applySplit1DRiemannToPatch_Overlap1AoS3d(...)" );
}




void exahype2::fv::applySplit1DRiemannToPatch_Overlap1AoS2d_SplitLoop(
      std::function< void(
        double * __restrict__ QL,
        double * __restrict__ QR,
        const tarch::la::Vector<Dimensions,double>&  faceCentre,
        double                                       volumeH,
        double                                       t,
        double                                       dt,
        int                                          normal,
        double * __restrict__ FL,
        double * __restrict__ FR
      ) >   splitRiemannSolve1d,
      const tarch::la::Vector<Dimensions,double>&  patchCentre,
      const tarch::la::Vector<Dimensions,double>&  patchSize,
      double                                       t,
      double                                       dt,
      int                                          numberOfVolumesPerAxisInPatch,
      int                                          unknowns,
      int                                          auxiliaryVariables,
      double * __restrict__ Qin,
      double * __restrict__ Qout
) {
  static tarch::logging::Log _log( "exahype2::fv" );
  logTraceInWith6Arguments( "applySplit1DRiemannToPatch_Overlap1AoS2d(...)", patchCentre, patchSize, t, dt, numberOfVolumesPerAxisInPatch, unknowns );

  assertion( dt>=tarch::la::NUMERICAL_ZERO_DIFFERENCE );

  tarch::la::Vector<Dimensions,double> volumeH = exahype2::getVolumeSize(patchSize, numberOfVolumesPerAxisInPatch);

  double numericalFluxL[unknowns]; // helper out variable
  double numericalFluxR[unknowns]; // helper out variable

  for (int shift=0; shift<2; shift++) {
    #ifdef SharedOMP
    #pragma omp parallel for simd collapse(2)
    #endif
    for (int x=shift; x<=numberOfVolumesPerAxisInPatch; x+=2)
    for (int y=0; y<numberOfVolumesPerAxisInPatch; y++)  {
      const int leftVoxelInPreimage  =  x
                                     + (y+1) * (2 + numberOfVolumesPerAxisInPatch);
      const int rightVoxelInPreimage =  x+1
                                     + (y+1) * (2 + numberOfVolumesPerAxisInPatch);

      const int leftVoxelInImage     = x-1
                                     + y * numberOfVolumesPerAxisInPatch;
      const int rightVoxelInImage    = x
                                     + y * numberOfVolumesPerAxisInPatch;

      tarch::la::Vector<Dimensions,double> volumeX = patchCentre - 0.5 * patchSize;
      volumeX(0) += x * volumeH(0);
      volumeX(1) += (y+0.5) * volumeH(1);

      splitRiemannSolve1d(
        Qin + leftVoxelInPreimage*(unknowns+auxiliaryVariables),
        Qin + rightVoxelInPreimage*(unknowns+auxiliaryVariables),
        volumeX,volumeH(0), t, dt, 0, //  last argument = normal
        numericalFluxL, numericalFluxR
      );

      for (int unknown=0; unknown<unknowns; unknown++) {
        if (x>0) {
          Qout[ leftVoxelInImage*(unknowns+auxiliaryVariables)+unknown ] -= dt / volumeH(0) * numericalFluxL[unknown];
        }
        if (x<numberOfVolumesPerAxisInPatch) {
          Qout[ rightVoxelInImage*(unknowns+auxiliaryVariables)+unknown ] += dt / volumeH(0) * numericalFluxR[unknown];
        }
      }
    }
  }

  for (int shift=0; shift<2; shift++) {
    #ifdef SharedOMP
    #pragma omp parallel for simd collapse(2)
    #endif
    for (int y=shift; y<=numberOfVolumesPerAxisInPatch; y+=2)
    for (int x=0; x<numberOfVolumesPerAxisInPatch; x++) {
      const int lowerVoxelInPreimage =  x+1
                                     +  y * (2 + numberOfVolumesPerAxisInPatch);
      const int upperVoxelInPreimage =  x+1
                                     + (y+1) * (2 + numberOfVolumesPerAxisInPatch);

      const int lowerVoxelInImage    = x
                                     + (y-1) * numberOfVolumesPerAxisInPatch;
      const int upperVoxelInImage    = x
                                     + y * numberOfVolumesPerAxisInPatch;

      tarch::la::Vector<Dimensions,double> volumeX = patchCentre - 0.5 * patchSize;
      volumeX(0) += (x+0.5) * volumeH(0);
      volumeX(1) += y * volumeH(1);

      splitRiemannSolve1d(
        Qin + lowerVoxelInPreimage*(unknowns+auxiliaryVariables),
        Qin + upperVoxelInPreimage*(unknowns+auxiliaryVariables),
        volumeX, volumeH(0), t, dt, 1, //  last argument = normal
        numericalFluxL, numericalFluxR
      );

      for (int unknown=0; unknown<unknowns; unknown++) {
        if (y>0) {
          Qout[ lowerVoxelInImage*(unknowns+auxiliaryVariables)+unknown ] -= dt / volumeH(0) * numericalFluxL[unknown];
        }
        if (y<numberOfVolumesPerAxisInPatch) {
          Qout[ upperVoxelInImage*(unknowns+auxiliaryVariables)+unknown ] += dt / volumeH(0) * numericalFluxR[unknown];
        }
      }
    }
  }

  logTraceOut( "applySplit1DRiemannToPatch_Overlap1AoS2d(...)" );
}




void exahype2::fv::applySplit1DRiemannToPatch_Overlap1AoS3d_SplitLoop(
      std::function< void(
        double * __restrict__ QL,
        double * __restrict__ QR,
        const tarch::la::Vector<Dimensions,double>&  faceCentre,
        double                                       volumeH,
        double                                       t,
        double                                       dt,
        int                                          normal,
        double * __restrict__ FL,
        double * __restrict__ FR
      ) >   splitRiemannSolve1d,
      const tarch::la::Vector<Dimensions,double>&  patchCentre,
      const tarch::la::Vector<Dimensions,double>&  patchSize,
      double                                       t,
      double                                       dt,
      int                                          numberOfVolumesPerAxisInPatch,
      int                                          unknowns,
      int                                          auxiliaryVariables,
      double * __restrict__ Qin,
      double * __restrict__ Qout
) {
  static tarch::logging::Log _log( "exahype2::fv" );
  logTraceInWith6Arguments( "applySplit1DRiemannToPatch_Overlap1AoS3d(...)", patchCentre, patchSize, t, dt, numberOfVolumesPerAxisInPatch, unknowns );

  assertion( dt>=tarch::la::NUMERICAL_ZERO_DIFFERENCE );

  tarch::la::Vector<Dimensions,double> volumeH = exahype2::getVolumeSize(patchSize, numberOfVolumesPerAxisInPatch);

  double numericalFluxL[unknowns]; // helper out variable
  double numericalFluxR[unknowns]; // helper out variable

  for (int shift=0; shift<2; shift++) {
    #ifdef SharedOMP
    #pragma omp parallel for simd collapse(3)
    #endif
    for (int x=shift; x<=numberOfVolumesPerAxisInPatch; x+=2)
    for (int z=0; z<numberOfVolumesPerAxisInPatch; z++)
    for (int y=0; y<numberOfVolumesPerAxisInPatch; y++) {
    const int leftVoxelInPreimage  =  x
                                   + (y+1) * (2 + numberOfVolumesPerAxisInPatch)
                                   + (z+1) * (2 + numberOfVolumesPerAxisInPatch) * (2 + numberOfVolumesPerAxisInPatch);
    const int rightVoxelInPreimage =  x+1
                                   + (y+1) * (2 + numberOfVolumesPerAxisInPatch)
                                   + (z+1) * (2 + numberOfVolumesPerAxisInPatch) * (2 + numberOfVolumesPerAxisInPatch);

    const int leftVoxelInImage     = x-1
                                   + y * numberOfVolumesPerAxisInPatch
                                   + z * numberOfVolumesPerAxisInPatch * numberOfVolumesPerAxisInPatch;
    const int rightVoxelInImage    = x
                                   + y * numberOfVolumesPerAxisInPatch
                                   + z * numberOfVolumesPerAxisInPatch * numberOfVolumesPerAxisInPatch;

    tarch::la::Vector<Dimensions,double> volumeX = patchCentre - 0.5 * patchSize;
    volumeX(0) += x * volumeH(0);
    volumeX(1) += (y+0.5) * volumeH(1);
    volumeX(2) += (z+0.5) * volumeH(2);

    splitRiemannSolve1d(
      Qin + leftVoxelInPreimage*(unknowns+auxiliaryVariables),
      Qin + rightVoxelInPreimage*(unknowns+auxiliaryVariables),
      volumeX, volumeH(0), t, dt, 0, //  last argument = normal
      numericalFluxL, numericalFluxR
    );

    for (int unknown=0; unknown<unknowns; unknown++) {
      if (x>0) {
        Qout[ leftVoxelInImage*(unknowns+auxiliaryVariables)+unknown ] -= dt / volumeH(0) * numericalFluxL[unknown];
      }
      if (x<numberOfVolumesPerAxisInPatch) {
        Qout[ rightVoxelInImage*(unknowns+auxiliaryVariables)+unknown ] += dt / volumeH(0) * numericalFluxR[unknown];
      }
    }
  }
  }

  for (int shift=0; shift<2; shift++) {
    #ifdef SharedOMP
    #pragma omp parallel for simd collapse(3)
    #endif
    for (int y=shift; y<=numberOfVolumesPerAxisInPatch; y+=2)
    for (int z=0; z<numberOfVolumesPerAxisInPatch; z++)
    for (int x=0; x<numberOfVolumesPerAxisInPatch; x++) {
      const int lowerVoxelInPreimage =  x+1
                                     +  y * (2 + numberOfVolumesPerAxisInPatch)
                                     + (z+1) * (2 + numberOfVolumesPerAxisInPatch) * (2 + numberOfVolumesPerAxisInPatch);
      const int upperVoxelInPreimage =  x+1
                                     + (y+1) * (2 + numberOfVolumesPerAxisInPatch)
                                     + (z+1) * (2 + numberOfVolumesPerAxisInPatch) * (2 + numberOfVolumesPerAxisInPatch);

      const int lowerVoxelInImage    = x
                                     + (y-1) * numberOfVolumesPerAxisInPatch
                                     + z * numberOfVolumesPerAxisInPatch * numberOfVolumesPerAxisInPatch;
      const int upperVoxelInImage    = x
                                     + y * numberOfVolumesPerAxisInPatch
                                     + z * numberOfVolumesPerAxisInPatch * numberOfVolumesPerAxisInPatch;


      tarch::la::Vector<Dimensions,double> volumeX = patchCentre - 0.5 * patchSize;
      volumeX(0) += (x+0.5) * volumeH(0);
      volumeX(1) += y * volumeH(1);
      volumeX(2) += (z+0.5) * volumeH(2);

      splitRiemannSolve1d(
        Qin + lowerVoxelInPreimage*(unknowns+auxiliaryVariables),
        Qin + upperVoxelInPreimage*(unknowns+auxiliaryVariables),
        volumeX, volumeH(0), t, dt, 1, //  last argument = normal
        numericalFluxL, numericalFluxR
      );

      for (int unknown=0; unknown<unknowns; unknown++) {
        if (y>0) {
          Qout[ lowerVoxelInImage*(unknowns+auxiliaryVariables)+unknown ] -= dt / volumeH(0) * numericalFluxL[unknown];
        }
        if (y<numberOfVolumesPerAxisInPatch) {
          Qout[ upperVoxelInImage*(unknowns+auxiliaryVariables)+unknown ] += dt / volumeH(0) * numericalFluxR[unknown];
        }
      }
    }
  }


  for (int shift=0; shift<2; shift++) {
    #ifdef SharedOMP
    #pragma omp parallel for simd collapse(3)
    #endif
    for (int z=shift; z<=numberOfVolumesPerAxisInPatch; z+=2)
    for (int y=0; y<numberOfVolumesPerAxisInPatch; y++)
    for (int x=0; x<numberOfVolumesPerAxisInPatch; x++) {
      const int lowerVoxelInPreimage =  x+1
                                     + (y+1) * (2 + numberOfVolumesPerAxisInPatch)
                                     +  z * (2 + numberOfVolumesPerAxisInPatch) * (2 + numberOfVolumesPerAxisInPatch);
      const int upperVoxelInPreimage =  x+1
                                     + (y+1) * (2 + numberOfVolumesPerAxisInPatch)
                                     + (z+1) * (2 + numberOfVolumesPerAxisInPatch) * (2 + numberOfVolumesPerAxisInPatch);

      const int lowerVoxelInImage    = x
                                     + y * numberOfVolumesPerAxisInPatch
                                     + (z-1) * numberOfVolumesPerAxisInPatch * numberOfVolumesPerAxisInPatch;
      const int upperVoxelInImage    = x
                                     + y * numberOfVolumesPerAxisInPatch
                                     + z * numberOfVolumesPerAxisInPatch * numberOfVolumesPerAxisInPatch;

      tarch::la::Vector<Dimensions,double> volumeX = patchCentre - 0.5 * patchSize;
      volumeX(0) += (x+0.5) * volumeH(0);
      volumeX(1) += (y+0.5) * volumeH(1);
      volumeX(2) += z * volumeH(2);

      splitRiemannSolve1d(
        Qin + lowerVoxelInPreimage*(unknowns+auxiliaryVariables),
        Qin + upperVoxelInPreimage*(unknowns+auxiliaryVariables),
        volumeX, volumeH(0), t, dt, 2, //  last argument = normal
        numericalFluxL, numericalFluxR
      );

      for (int unknown=0; unknown<unknowns; unknown++) {
        if (z>0) {
          Qout[ lowerVoxelInImage*(unknowns+auxiliaryVariables)+unknown ] -= dt / volumeH(0) * numericalFluxL[unknown];
        }
        if (z<numberOfVolumesPerAxisInPatch) {
          Qout[ upperVoxelInImage*(unknowns+auxiliaryVariables)+unknown ] += dt / volumeH(0) * numericalFluxR[unknown];
        }
      }
    }
  }

  logTraceOut( "applySplit1DRiemannToPatch_Overlap1AoS3d(...)" );
}


