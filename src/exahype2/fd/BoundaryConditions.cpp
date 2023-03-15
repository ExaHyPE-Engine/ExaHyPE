#include "BoundaryConditions.h"

#include "peano4/utils/Globals.h"
#include "peano4/utils/Loop.h"

#include "tarch/logging/Log.h"
#include "tarch/accelerator/accelerator.h"

#include "exahype2/fd/PatchUtils.h"
#include "exahype2/enumerator/FaceAoSLexicographicEnumerator.h"


void exahype2::fd::applyBoundaryConditions(
  std::function< void(
    const double* __restrict__                   Qinside,
    double * __restrict__                        Qoutside,
    const tarch::la::Vector<Dimensions,double>&  faceCentre,
    const tarch::la::Vector<Dimensions,double>&  volumeH,
    double                                       t,
    double                                       dt,
    int                                          normal
  ) >   boundaryCondition,
  const tarch::la::Vector<Dimensions,double>&  faceCentre,
  const tarch::la::Vector<Dimensions,double>&  patchSize,
  double                                       t,
  double                                       dt,
  int                                          numberOfGridCellsPerPatchPerAxis,
  int                                          overlap,
  int                                          unknownsPlusAuxiliaryVariables,
  int                                          faceNumber,
  double* __restrict__                         Q
) {
  static tarch::logging::Log _log( "exahype2::fd" );
  logTraceInWith4Arguments( "applyBoundaryConditions(...)", faceCentre, patchSize, numberOfGridCellsPerPatchPerAxis, faceNumber);

  exahype2::enumerator::FaceAoSLexicographicEnumerator enumerator(
    faceNumber,
    numberOfGridCellsPerPatchPerAxis,
    overlap,
    unknownsPlusAuxiliaryVariables,
    0
  );

  tarch::la::Vector<Dimensions,double> volumeH    = exahype2::fd::getGridCellSize(patchSize, numberOfGridCellsPerPatchPerAxis);
  tarch::la::Vector<Dimensions,double> faceOffset = faceCentre - 0.5 * patchSize;
  faceOffset(faceNumber%Dimensions) += 0.5 * patchSize(faceNumber%Dimensions);

  dfore(volume,numberOfGridCellsPerPatchPerAxis,faceNumber % Dimensions,0) {
    tarch::la::Vector<Dimensions,int> insideGridCell  = volume;
    tarch::la::Vector<Dimensions,int> outsideGridCell = volume;
    tarch::la::Vector<Dimensions,double> x          = faceOffset + tarch::la::multiplyComponents( tarch::la::convertScalar<double>(volume)+tarch::la::Vector<Dimensions,double>(0.5), volumeH);

    x(faceNumber%Dimensions) -= 0.5 * volumeH(faceNumber%Dimensions);

    for (int layer=0; layer<overlap; layer++) {
      if (faceNumber<Dimensions) {
        insideGridCell(faceNumber % Dimensions)  = overlap-layer;
        outsideGridCell(faceNumber % Dimensions) = overlap-1-layer;
      }
      else {
        insideGridCell(faceNumber % Dimensions)  = layer+overlap-1;
        outsideGridCell(faceNumber % Dimensions) = layer+overlap;
      }

      logDebug(
        "applyBoundaryConditions(...)",
        faceCentre << " x " << faceNumber << ": " <<
        insideGridCell << "->" << outsideGridCell << " (" << enumerator(insideGridCell,0) << "->" << enumerator(outsideGridCell,0) << "): " <<
        *(Q + enumerator(insideGridCell,0))
      );

      boundaryCondition(
        Q + enumerator(insideGridCell,0),
        Q + enumerator(outsideGridCell,0),
        x, volumeH, t, dt, faceNumber
      );
    }
  }

  logTraceOut( "applyBoundaryConditions(...)" );
}


void exahype2::fd::applySommerfeldConditions(
  std::function< double(
    const double* __restrict__                   Q,
    const tarch::la::Vector<Dimensions,double>&  faceCentre,
    const tarch::la::Vector<Dimensions,double>&  gridCellH,
    double                                       t,
    double                                       dt,
    int                                          normal
  ) >   maxEigenvalue,
  std::function< void(
    double* __restrict__                         Q,
    const tarch::la::Vector<Dimensions,double>&  faceCentre,
    const tarch::la::Vector<Dimensions,double>&  gridCellH
  ) >   farFieldSolution,
  const tarch::la::Vector<Dimensions,double>&  faceCentre,
  const tarch::la::Vector<Dimensions,double>&  patchSize,
  double                                       t,
  double                                       dt,
  int                                          numberOfGridCellsPerPatchPerAxis,
  int                                          overlap,
  int                                          numberOfUnknowns,
  int                                          numberOfAuxiliaryVariables,
  int                                          faceNumber,
  const tarch::la::Vector<Dimensions,double>&  systemOrigin,
  double* __restrict__                         Qold,
  double* __restrict__                         Qnew
) {
  static tarch::logging::Log _log( "exahype2::fd" );
  logTraceInWith4Arguments( "applySommerfeldConditions(...)", faceCentre, patchSize, numberOfGridCellsPerPatchPerAxis, faceNumber);

  exahype2::enumerator::FaceAoSLexicographicEnumerator enumerator(
    faceNumber,
    numberOfGridCellsPerPatchPerAxis,
    overlap,
    numberOfUnknowns+numberOfAuxiliaryVariables,
    0
  );

  double* QInf = tarch::allocateMemory<double>( numberOfUnknowns+numberOfAuxiliaryVariables, tarch::MemoryLocation::Heap );

  tarch::la::Vector<Dimensions,double> volumeH    = exahype2::fd::getGridCellSize(patchSize, numberOfGridCellsPerPatchPerAxis);
  tarch::la::Vector<Dimensions,double> faceOffset = faceCentre - 0.5 * patchSize;
  faceOffset(faceNumber%Dimensions) += 0.5 * patchSize(faceNumber%Dimensions);

  dfore(volume,numberOfGridCellsPerPatchPerAxis,faceNumber % Dimensions,0) {
    tarch::la::Vector<Dimensions,int> insideGridCell  = volume;
    tarch::la::Vector<Dimensions,int> outsideGridCell = volume;
    tarch::la::Vector<Dimensions,double> x          = faceOffset + tarch::la::multiplyComponents( tarch::la::convertScalar<double>(volume)+tarch::la::Vector<Dimensions,double>(0.5), volumeH);

    tarch::la::Vector<Dimensions,double> xOnLayer = x;

    for (int layer=0; layer<overlap; layer++) {
      if (faceNumber<Dimensions) {
        insideGridCell(faceNumber % Dimensions)  = overlap-layer;
        outsideGridCell(faceNumber % Dimensions) = overlap-1-layer;
        xOnLayer(faceNumber%Dimensions) = x(faceNumber%Dimensions) - layer * volumeH(faceNumber%Dimensions); //for each layer the x is different
      }
      else {
        insideGridCell(faceNumber % Dimensions)  = layer+overlap-1;
        outsideGridCell(faceNumber % Dimensions) = layer+overlap;
        xOnLayer(faceNumber%Dimensions) = x(faceNumber%Dimensions) + layer * volumeH(faceNumber%Dimensions); //for each layer the x is different
      }

      if (tarch::la::equals(t,0.0)){ //for the initial timestep, we need to fill the outside volume for old data as well.
        for(int i=0; i<numberOfUnknowns+numberOfAuxiliaryVariables; i++) {
          (Qold + enumerator(outsideGridCell,0))[i]=(Qold+enumerator(insideGridCell,0))[i];
        }
      }
      
      const double rOnLayer=tarch::la::norm2(xOnLayer-systemOrigin);
      const double waveSpeed=maxEigenvalue( Qold + enumerator(outsideGridCell,0), faceCentre, volumeH, t, dt, faceNumber % Dimensions); 

      logDebug(
        "applySommerfeldConditions(...)",
        faceCentre << " x " << faceNumber << ": " <<
        insideGridCell << "->" << outsideGridCell << " (" << enumerator(insideGridCell,0) << "->" << enumerator(outsideGridCell,0) << "): " <<
        *(Qnew + enumerator(insideGridCell,0))
      );

      farFieldSolution(
        QInf,
        xOnLayer,
        volumeH
      );

      //do something here, we have Qold, Qnew, waveSpeed, xOnLayer, rOnLayer, dt, volumeH, Qinf
      //we try the first approach first to see if it is already sufficient.
      double dtInverse=1/dt;
      double spaceFactor=waveSpeed*rOnLayer/volumeH(faceNumber%Dimensions)/xOnLayer(faceNumber%Dimensions);
      //logInfo("text","factor "<< spaceFactor <<" waveSpeed "<<waveSpeed<<" rOnLayer "<<rOnLayer<<" volumeH "<<volumeH(faceNumber%Dimensions)
      //  <<" xOnLayer "<<xOnLayer(faceNumber%Dimensions));

      for(int i=0; i<numberOfUnknowns; i++) {
        double numerator=-(waveSpeed/rOnLayer)*((Qold + enumerator(outsideGridCell,0))[i]+(Qold+enumerator(insideGridCell,0))[i]-2*QInf[i])
                          +(+dtInverse-spaceFactor)*(Qold + enumerator(outsideGridCell,0))[i]
                          +(-dtInverse+spaceFactor)*(Qnew + enumerator( insideGridCell,0))[i]
                          +(+dtInverse+spaceFactor)*(Qold + enumerator( insideGridCell,0))[i];
        //logInfo("numerator","numerator "<<numerator );
        (Qnew + enumerator(outsideGridCell,0))[i]=numerator/(dtInverse+spaceFactor);
      }
      for(int i=numberOfUnknowns; i<numberOfUnknowns+numberOfAuxiliaryVariables; i++) {
          (Qnew + enumerator(outsideGridCell,0))[i]=(Qold+enumerator(insideGridCell,0))[i];
      } //in principle we do not need to assign auxiliary variables, but I do this here for safety

    }
  }

  tarch::freeMemory( QInf, tarch::MemoryLocation::Heap );

  logTraceOut( "applySommerfeldConditions(...)" );
}


void exahype2::fd::applySommerfeldConditions(
  std::function< double(
    const double* __restrict__                   Q,
    const tarch::la::Vector<Dimensions,double>&  faceCentre,
    const tarch::la::Vector<Dimensions,double>&  gridCellH,
    double                                       t,
    double                                       dt,
    int                                          normal
  ) >   maxEigenvalue,
  const tarch::la::Vector<Dimensions,double>&  faceCentre,
  const tarch::la::Vector<Dimensions,double>&  patchSize,
  double                                       t,
  double                                       dt,
  int                                          numberOfGridCellsPerPatchPerAxis,
  int                                          overlap,
  int                                          numberOfUnknowns,
  int                                          numberOfAuxiliaryVariables,
  int                                          faceNumber,
  const tarch::la::Vector<Dimensions,double>&  systemOrigin,
  double* __restrict__                         Qold,
  double* __restrict__                         Qnew
) {
  applySommerfeldConditions(
    maxEigenvalue,
    [&] (
      double* __restrict__                         Q,
      const tarch::la::Vector<Dimensions,double>&  faceCentre,
      const tarch::la::Vector<Dimensions,double>&  gridCellH
    ) -> void {
      for (int i=0; i<numberOfUnknowns+numberOfAuxiliaryVariables; i++) {
        Q[i] = 0.0;
      }
    },
    faceCentre,
    patchSize,
    t,
    dt,
    numberOfGridCellsPerPatchPerAxis,
    overlap,
    numberOfUnknowns,
    numberOfAuxiliaryVariables,
    faceNumber,
    systemOrigin,
    Qold,
    Qnew
  );
}


