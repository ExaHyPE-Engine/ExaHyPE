#include "Generic.h"

#include "tarch/logging/Log.h"

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
  double QinWithHalo[],
  double QOutWithoutHalo[],
  int    unknowns,
  int    numberOfVolumesPerAxisInPatch,
  int    haloSize
) {
  dfor(k,numberOfVolumesPerAxisInPatch) {
    tarch::la::Vector<Dimensions,int>   source = k + tarch::la::Vector<Dimensions,int>(haloSize);
    int sourceSerialised      = peano4::utils::dLinearised(source,numberOfVolumesPerAxisInPatch+haloSize*2);
    int destinationSerialised = peano4::utils::dLinearised(k,numberOfVolumesPerAxisInPatch);
    for (int i=0; i<unknowns; i++) {
      QOutWithoutHalo[destinationSerialised*unknowns+i] = QinWithHalo[sourceSerialised*unknowns+i];
    }
  }
}


void exahype2::fv::gpu::copyPatch(
  double QinWithHalo[],
  double QOutWithoutHalo[],
  int    unknowns,
  int    numberOfVolumesPerAxisInPatch,
  int    haloSize
) {
  dfor(k,numberOfVolumesPerAxisInPatch) {
    tarch::la::Vector<Dimensions,int>   source = k + tarch::la::Vector<Dimensions,int>(haloSize);
    int sourceSerialised      = peano4::utils::dLinearised(source,numberOfVolumesPerAxisInPatch+haloSize*2);
    int destinationSerialised = peano4::utils::dLinearised(k,numberOfVolumesPerAxisInPatch);
    for (int i=0; i<unknowns; i++) {
      QOutWithoutHalo[destinationSerialised*unknowns+i] = QinWithHalo[sourceSerialised*unknowns+i];
    }
  }
}


void exahype2::fv::applySplit1DRiemannToPatch_Overlap1AoS2d(
      std::function< void(
        double                                       QL[],
        double                                       QR[],
        const tarch::la::Vector<Dimensions,double>&  faceCentre,
        double                                       volumeH,
        double                                       t,
        double                                       dt,
        int                                          normal,
        double                                       FL[],
        double                                       FR[]
      ) >   splitRiemannSolve1d,
      const tarch::la::Vector<Dimensions,double>&  patchCentre,
      const tarch::la::Vector<Dimensions,double>&  patchSize,
      double                                       t,
      double                                       dt,
      int                                          numberOfVolumesPerAxisInPatch,
      int                                          unknowns,
      double                                       Qin[],
      double                                       Qout[]
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
      Qin + leftVoxelInPreimage*unknowns,
      Qin + rightVoxelInPreimage*unknowns,
      volumeX,volumeH(0), t, dt, 0, //  last argument = normal
      numericalFluxL, numericalFluxR
    );

    for (int unknown=0; unknown<unknowns; unknown++) {
      if (x>0) {
        Qout[ leftVoxelInImage*unknowns+unknown ] -= dt / volumeH(0) * numericalFluxL[unknown];
      }
      if (x<numberOfVolumesPerAxisInPatch) {
        Qout[ rightVoxelInImage*unknowns+unknown ] += dt / volumeH(0) * numericalFluxR[unknown];
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
      Qin + lowerVoxelInPreimage*unknowns,
      Qin + upperVoxelInPreimage*unknowns,
      volumeX, volumeH(0), t, dt, 1, //  last argument = normal
      numericalFluxL, numericalFluxR
    );

    for (int unknown=0; unknown<unknowns; unknown++) {
      if (y>0) {
        Qout[ lowerVoxelInImage*unknowns+unknown ] -= dt / volumeH(0) * numericalFluxL[unknown];
      }
      if (y<numberOfVolumesPerAxisInPatch) {
        Qout[ upperVoxelInImage*unknowns+unknown ] += dt / volumeH(0) * numericalFluxR[unknown];
      }
    }
  }

  logTraceOut( "applySplit1DRiemannToPatch_Overlap1AoS2d(...)" );
}




void exahype2::fv::applySplit1DRiemannToPatch_Overlap1AoS3d(
      std::function< void(
        double                                       QL[],
        double                                       QR[],
        const tarch::la::Vector<Dimensions,double>&  faceCentre,
        double                                       volumeH,
        double                                       t,
        double                                       dt,
        int                                          normal,
        double                                       FL[],
        double                                       FR[]
      ) >   splitRiemannSolve1d,
      const tarch::la::Vector<Dimensions,double>&  patchCentre,
      const tarch::la::Vector<Dimensions,double>&  patchSize,
      double                                       t,
      double                                       dt,
      int                                          numberOfVolumesPerAxisInPatch,
      int                                          unknowns,
      double                                       Qin[],
      double                                       Qout[]
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
      Qin + leftVoxelInPreimage*unknowns,
      Qin + rightVoxelInPreimage*unknowns,
      volumeX, volumeH(0), t, dt, 0, //  last argument = normal
      numericalFluxL, numericalFluxR
    );

    for (int unknown=0; unknown<unknowns; unknown++) {
      if (x>0) {
        Qout[ leftVoxelInImage*unknowns+unknown ] -= dt / volumeH(0) * numericalFluxL[unknown];
      }
      if (x<numberOfVolumesPerAxisInPatch) {
        Qout[ rightVoxelInImage*unknowns+unknown ] += dt / volumeH(0) * numericalFluxR[unknown];
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
      Qin + lowerVoxelInPreimage*unknowns,
      Qin + upperVoxelInPreimage*unknowns,
      volumeX, volumeH(0), t, dt, 1, //  last argument = normal
      numericalFluxL, numericalFluxR
    );

    for (int unknown=0; unknown<unknowns; unknown++) {
      if (y>0) {
        Qout[ lowerVoxelInImage*unknowns+unknown ] -= dt / volumeH(0) * numericalFluxL[unknown];
      }
      if (y<numberOfVolumesPerAxisInPatch) {
        Qout[ upperVoxelInImage*unknowns+unknown ] += dt / volumeH(0) * numericalFluxR[unknown];
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
      Qin + lowerVoxelInPreimage*unknowns,
      Qin + upperVoxelInPreimage*unknowns,
      volumeX, volumeH(0), t, dt, 2, //  last argument = normal
      numericalFluxL, numericalFluxR
    );

    for (int unknown=0; unknown<unknowns; unknown++) {
      if (z>0) {
        Qout[ lowerVoxelInImage*unknowns+unknown ] -= dt / volumeH(0) * numericalFluxL[unknown];
      }
      if (z<numberOfVolumesPerAxisInPatch) {
        Qout[ upperVoxelInImage*unknowns+unknown ] += dt / volumeH(0) * numericalFluxR[unknown];
      }
    }
  }

  logTraceOut( "applySplit1DRiemannToPatch_Overlap1AoS3d(...)" );
}
