#include "Rusanov.h"
#include "otter/otter.h"


void exahype2::fv::rusanov::timeStepWithRusanov_overlap1_sequential_AoS(
  PatchData&              patchData,
  int                     numberOfVolumesPerAxisInPatch,
  int                     unknowns,
  int                     auxiliaryVariables,
  bool                    evaluateFlux,
  bool                    evaluateNonconservativeProduct,
  bool                    evaluateSource,
  bool                    evaluateMaximumEigenvalueAfterTimeStep,
  Flux                    flux,
  NonconservativeProduct  nonconservativeProduct,
  Source                  source,
  MaxEigenvalue           maxEigenvalue
) {
  static tarch::logging::Log _log( "exahype2::fv::rusanov" );
  constexpr int haloSize        = 1;

  const int faceDoFs        = exahype2::fv::getTotalNumberOfFaceDoFs(        patchData.numberOfPatches, numberOfVolumesPerAxisInPatch, haloSize );
  const int volumeDoFs      = exahype2::fv::getTotalNumberOfVolumeDoFs(      patchData.numberOfPatches, numberOfVolumesPerAxisInPatch, haloSize );
  const int innerVolumeDoFs = exahype2::fv::getTotalNumberOfInnerVolumeDoFs( patchData.numberOfPatches, numberOfVolumesPerAxisInPatch, haloSize );

  double* tempFluxX                  = evaluateFlux                     ? tarch::allocateMemory( volumeDoFs * unknowns,      tarch::MemoryLocation::Heap ) : nullptr;
  double* tempFluxY                  = evaluateFlux                     ? tarch::allocateMemory( volumeDoFs * unknowns,      tarch::MemoryLocation::Heap ) : nullptr;
  double* tempFluxZ                  = (evaluateFlux and Dimensions==3) ? tarch::allocateMemory( volumeDoFs * unknowns,      tarch::MemoryLocation::Heap ) : nullptr;
  //double* tempNonconservativeProduct = evaluateNonconservativeProduct ? tarch::allocateMemory( faceDoFs,        tarch::MemoryLocation::Heap ) : nullptr;
  double* tempEigenvalueX            =                                    tarch::allocateMemory( volumeDoFs,      tarch::MemoryLocation::Heap );
  double* tempEigenvalueY            =                                    tarch::allocateMemory( volumeDoFs,      tarch::MemoryLocation::Heap );
  double* tempEigenvalueZ            = (Dimensions==3)                  ? tarch::allocateMemory( volumeDoFs,      tarch::MemoryLocation::Heap ) : nullptr;

  exahype2::fv::enumerator::AoSLexicographicVolumeEnumerator QInEnumerator       (                        1, numberOfVolumesPerAxisInPatch,haloSize,unknowns,auxiliaryVariables);
  exahype2::fv::enumerator::AoSLexicographicVolumeEnumerator QOutEnumerator      (                        1, numberOfVolumesPerAxisInPatch,       0,unknowns,auxiliaryVariables);
  exahype2::fv::enumerator::AoSLexicographicVolumeEnumerator fluxEnumerator      (patchData.numberOfPatches, numberOfVolumesPerAxisInPatch,haloSize,unknowns,0);
  exahype2::fv::enumerator::AoSLexicographicVolumeEnumerator eigenvalueEnumerator(patchData.numberOfPatches, numberOfVolumesPerAxisInPatch,haloSize,1,0);

  logDebug( "timeStepWithRusanov_overlap1_sequential_AoS(...)", "evaluate source terms and/or copy solution over" );
  if (Dimensions==2 and evaluateSource) {
    // @todo Add OpenMP pragmas
    // @todo Add Otter pragma
    for (int patchIndex=0; patchIndex<patchData.numberOfPatches; patchIndex++)
    for (int x = 0; x < numberOfVolumesPerAxisInPatch; x++)
    for (int y = 0; y < numberOfVolumesPerAxisInPatch; y++) {
      internal::copy_solution_and_add_source_term_loop_body(
        patchData.QIn[patchIndex],
        QInEnumerator,
        source,
        patchData.patchCentre[patchIndex],
        patchData.patchSize[patchIndex],
        patchIndex,
        {x,y},
        patchData.t[patchIndex],
        patchData.dt[patchIndex],
        patchData.QOut[patchIndex],
        QOutEnumerator
      );
    }
  }
  else if (Dimensions==3 and evaluateSource) {
      // @todo Add OpenMP pragmas
    // @todo Add Otter pragma
    for (int patchIndex=0; patchIndex<patchData.numberOfPatches; patchIndex++)
    for (int x = 0; x < numberOfVolumesPerAxisInPatch; x++)
    for (int y = 0; y < numberOfVolumesPerAxisInPatch; y++)
    for (int z = 0; z < numberOfVolumesPerAxisInPatch; z++)
    for (int unknown=0; unknown<unknowns+auxiliaryVariables; unknown++) {
      internal::copy_solution_and_add_source_term_loop_body(
        patchData.QIn[patchIndex],
        QInEnumerator,
        source,
        patchData.patchCentre[patchIndex],
        patchData.patchSize[patchIndex],
        patchIndex,
        {x,y,z},
        patchData.t[patchIndex],
        patchData.dt[patchIndex],
        patchData.QOut[patchIndex],
        QOutEnumerator
      );
    }
  }
  else if (Dimensions==2) {
    // @todo Add OpenMP pragmas
    // @todo Add Otter pragma
    for (int patchIndex=0; patchIndex<patchData.numberOfPatches; patchIndex++)
    for (int x = 0; x < numberOfVolumesPerAxisInPatch; x++)
    for (int y = 0; y < numberOfVolumesPerAxisInPatch; y++)
    for (int unknown=0; unknown<unknowns+auxiliaryVariables; unknown++) {
      internal::copy_solution_loop_body(
        patchData.QIn[patchIndex],
        QInEnumerator,
        patchIndex,
        {x,y},
        unknown,
        patchData.QOut[patchIndex],
        QOutEnumerator
      );
    }
  }
  else {
    // @todo Add OpenMP pragmas
    // @todo Add Otter pragma
    for (int patchIndex=0; patchIndex<patchData.numberOfPatches; patchIndex++)
    for (int x = 0; x < numberOfVolumesPerAxisInPatch; x++)
    for (int y = 0; y < numberOfVolumesPerAxisInPatch; y++)
    for (int z = 0; z < numberOfVolumesPerAxisInPatch; z++)
    for (int unknown=0; unknown<unknowns+auxiliaryVariables; unknown++) {
      internal::copy_solution_loop_body(
        patchData.QIn[patchIndex],
        QInEnumerator,
        patchIndex,
        {x,y,z},
        unknown,
        patchData.QOut[patchIndex],
        QOutEnumerator
      );
    }
  }


  logDebug( "timeStepWithRusanov_overlap1_sequential_AoS(...)", "evaluate directional eigenvalues per volume" );
  if (Dimensions==2) {
    // @todo Add OpenMP pragmas
    // @todo Add Otter pragma
    for (int patchIndex=0; patchIndex<patchData.numberOfPatches; patchIndex++)
    for (int x = -haloSize; x < numberOfVolumesPerAxisInPatch+haloSize; x++)
    for (int y = -haloSize; y < numberOfVolumesPerAxisInPatch+haloSize; y++) {
      bool isDiagonalCell = false;
      isDiagonalCell |= (x==-haloSize and y==-haloSize);
      isDiagonalCell |= (x==-haloSize and y==numberOfVolumesPerAxisInPatch+haloSize-1);
      isDiagonalCell |= (x==numberOfVolumesPerAxisInPatch+haloSize-1 and y==-haloSize);
      isDiagonalCell |= (x==numberOfVolumesPerAxisInPatch+haloSize-1 and y==numberOfVolumesPerAxisInPatch+haloSize-1);
      if ( not isDiagonalCell ) {
        internal::compute_max_eigenvalue_loop_body(
          patchData.QIn[patchIndex],
          QInEnumerator,
          maxEigenvalue,
          patchData.patchCentre[patchIndex],
          patchData.patchSize[patchIndex],
          patchIndex,
          {x,y},
          patchData.t[patchIndex], patchData.dt[patchIndex],
          0,
          tempEigenvalueX,
          eigenvalueEnumerator
        );

        internal::compute_max_eigenvalue_loop_body(
          patchData.QIn[patchIndex],
          QInEnumerator,
          maxEigenvalue,
          patchData.patchCentre[patchIndex],
          patchData.patchSize[patchIndex],
          patchIndex,
          {x,y},
          patchData.t[patchIndex], patchData.dt[patchIndex],
          1,
          tempEigenvalueY,
          eigenvalueEnumerator
        );
      }
    }
  }
  else {
    // @todo Add OpenMP pragmas
    // @todo Add Otter pragma
    for (int patchIndex=0; patchIndex<patchData.numberOfPatches; patchIndex++)
    for (int x = -haloSize; x < numberOfVolumesPerAxisInPatch+haloSize; x++)
    for (int y = -haloSize; y < numberOfVolumesPerAxisInPatch+haloSize; y++)
    for (int z = -haloSize; z < numberOfVolumesPerAxisInPatch+haloSize; z++) {
      bool isDiagonalCell = false;
        isDiagonalCell |= (x==-haloSize and y==-haloSize);
        isDiagonalCell |= (x==-haloSize and z==-haloSize);
        isDiagonalCell |= (y==-haloSize and z==-haloSize);
        isDiagonalCell |= (x==-haloSize and y==numberOfVolumesPerAxisInPatch+haloSize-1);
        isDiagonalCell |= (x==-haloSize and z==numberOfVolumesPerAxisInPatch+haloSize-1);
        isDiagonalCell |= (y==-haloSize and z==numberOfVolumesPerAxisInPatch+haloSize-1);
        isDiagonalCell |= (x==numberOfVolumesPerAxisInPatch+haloSize-1 and y==-haloSize);
        isDiagonalCell |= (x==numberOfVolumesPerAxisInPatch+haloSize-1 and z==-haloSize);
        isDiagonalCell |= (y==numberOfVolumesPerAxisInPatch+haloSize-1 and z==-haloSize);
        isDiagonalCell |= (x==numberOfVolumesPerAxisInPatch+haloSize-1 and y==numberOfVolumesPerAxisInPatch+haloSize-1);
        isDiagonalCell |= (x==numberOfVolumesPerAxisInPatch+haloSize-1 and z==numberOfVolumesPerAxisInPatch+haloSize-1);
        isDiagonalCell |= (y==numberOfVolumesPerAxisInPatch+haloSize-1 and z==numberOfVolumesPerAxisInPatch+haloSize-1);
        if ( not isDiagonalCell ) {
          internal::compute_max_eigenvalue_loop_body(
            patchData.QIn[patchIndex],
            QInEnumerator,
            maxEigenvalue,
            patchData.patchCentre[patchIndex],
            patchData.patchSize[patchIndex],
            patchIndex,
            {x,y,z},
            patchData.t[patchIndex], patchData.dt[patchIndex],
            0,
            tempEigenvalueX,
            eigenvalueEnumerator
          );

          internal::compute_max_eigenvalue_loop_body(
            patchData.QIn[patchIndex],
            QInEnumerator,
            maxEigenvalue,
            patchData.patchCentre[patchIndex],
            patchData.patchSize[patchIndex],
            patchIndex,
            {x,y,z},
            patchData.t[patchIndex], patchData.dt[patchIndex],
            1,
            tempEigenvalueY,
            eigenvalueEnumerator
          );

          internal::compute_max_eigenvalue_loop_body(
            patchData.QIn[patchIndex],
            QInEnumerator,
            maxEigenvalue,
            patchData.patchCentre[patchIndex],
            patchData.patchSize[patchIndex],
            patchIndex,
            {x,y,z},
            patchData.t[patchIndex], patchData.dt[patchIndex],
            2,
            tempEigenvalueY,
            eigenvalueEnumerator
          );
        }
      }
  }

  if (evaluateFlux and Dimensions==2) {
    logDebug( "timeStepWithRusanov_overlap1_sequential_AoS(...)", "evaluate flux in x-direction" );
        // @todo Add OpenMP pragmas
        // @todo Add Otter pragma
    for (int patchIndex=0; patchIndex<patchData.numberOfPatches; patchIndex++)
    for (int x = -haloSize; x < numberOfVolumesPerAxisInPatch+haloSize; x++)
    for (int y =         0; y < numberOfVolumesPerAxisInPatch;          y++) {
      internal::compute_flux_loop_body(
        patchData.QIn[patchIndex],
        QInEnumerator,
        flux,
        patchData.patchCentre[patchIndex],
        patchData.patchSize[patchIndex],
        patchIndex,
        {x,y},
        patchData.t[patchIndex],
        patchData.dt[patchIndex],
        0, // normal
        tempFluxX,
        fluxEnumerator
      );
    }

    // @todo Add OpenMP pragmas
    // @todo Add Otter pragma
    for (int patchIndex=0; patchIndex<patchData.numberOfPatches; patchIndex++)
    for (int x =         0; x < numberOfVolumesPerAxisInPatch;           x++)
    for (int y = -haloSize; y < numberOfVolumesPerAxisInPatch+haloSize;  y++) {
      internal::compute_flux_loop_body(
        patchData.QIn[patchIndex],
        QInEnumerator,
        flux,
        patchData.patchCentre[patchIndex],
        patchData.patchSize[patchIndex],
        patchIndex,
        {x,y},
        patchData.t[patchIndex],
        patchData.dt[patchIndex],
        1, // normal
        tempFluxY,
        fluxEnumerator
      );
    }

    // @todo Add OpenMP pragmas
    // @todo Add Otter pragma
    for (int patchIndex=0; patchIndex<patchData.numberOfPatches; patchIndex++)
    for (int x = 0; x < numberOfVolumesPerAxisInPatch; x++)
    for (int y = 0; y < numberOfVolumesPerAxisInPatch; y++)
    for (int unknown=0; unknown<unknowns+auxiliaryVariables; unknown++) {
      internal::update_solution_with_flux_loop_body(
        patchData.QIn[patchIndex],
        QInEnumerator,
        tempFluxX, tempFluxY, tempFluxZ,
        fluxEnumerator,
        tempEigenvalueX, tempEigenvalueY, tempEigenvalueZ,
        eigenvalueEnumerator,
        patchData.patchCentre[patchIndex],
        patchData.patchSize[patchIndex],
        patchIndex,
        {x,y},
        unknown,
        patchData.dt[patchIndex],
        *(patchData.QOut + patchIndex),
        QOutEnumerator
      );
    }
  }
  else if (evaluateFlux and Dimensions==3) {
    logDebug( "timeStepWithRusanov_overlap1_sequential_AoS(...)", "evaluate flux in x-direction" );
        // @todo Add OpenMP pragmas
        // @todo Add Otter pragma
    for (int patchIndex=0; patchIndex<patchData.numberOfPatches; patchIndex++)
    for (int x = -haloSize; x < numberOfVolumesPerAxisInPatch+haloSize; x++)
    for (int y =         0; y < numberOfVolumesPerAxisInPatch;          y++)
    for (int z =         0; z < numberOfVolumesPerAxisInPatch;          z++) {
      internal::compute_flux_loop_body(
        patchData.QIn[patchIndex],
        QInEnumerator,
        flux,
        patchData.patchCentre[patchIndex],
        patchData.patchSize[patchIndex],
        patchIndex,
        {x,y,z},
        patchData.t[patchIndex],
        patchData.dt[patchIndex],
        0, // normal
        tempFluxX,
        fluxEnumerator
      );
    }

    // @todo Add OpenMP pragmas
    // @todo Add Otter pragma
    for (int patchIndex=0; patchIndex<patchData.numberOfPatches; patchIndex++)
    for (int x =         0; x < numberOfVolumesPerAxisInPatch;           x++)
    for (int y = -haloSize; y < numberOfVolumesPerAxisInPatch+haloSize;  y++)
    for (int z =         0; z < numberOfVolumesPerAxisInPatch;           z++) {
      internal::compute_flux_loop_body(
        patchData.QIn[patchIndex],
        QInEnumerator,
        flux,
        patchData.patchCentre[patchIndex],
        patchData.patchSize[patchIndex],
        patchIndex,
        {x,y,z},
        patchData.t[patchIndex],
        patchData.dt[patchIndex],
        1, // normal
        tempFluxY,
        fluxEnumerator
      );
    }

    for (int patchIndex=0; patchIndex<patchData.numberOfPatches; patchIndex++)
    for (int x =         0; x < numberOfVolumesPerAxisInPatch;           x++)
    for (int y =         0; y < numberOfVolumesPerAxisInPatch;           y++)
    for (int z = -haloSize; z < numberOfVolumesPerAxisInPatch+haloSize;  z++) {
      internal::compute_flux_loop_body(
        patchData.QIn[patchIndex],
        QInEnumerator,
        flux,
        patchData.patchCentre[patchIndex],
        patchData.patchSize[patchIndex],
        patchIndex,
        {x,y,z},
        patchData.t[patchIndex],
        patchData.dt[patchIndex],
        2, // normal
        tempFluxZ,
        fluxEnumerator
      );
    }

    // @todo Add OpenMP pragmas
    // @todo Add Otter pragma
    for (int patchIndex=0; patchIndex<patchData.numberOfPatches; patchIndex++)
    for (int x = 0; x < numberOfVolumesPerAxisInPatch; x++)
    for (int y = 0; y < numberOfVolumesPerAxisInPatch; y++)
    for (int z = 0; z < numberOfVolumesPerAxisInPatch; z++)
    for (int unknown=0; unknown<unknowns+auxiliaryVariables; unknown++) {
      internal::update_solution_with_flux_loop_body(
        patchData.QIn[patchIndex],
        QInEnumerator,
        tempFluxX, tempFluxY, tempFluxZ,
        fluxEnumerator,
        tempEigenvalueX, tempEigenvalueY, tempEigenvalueZ,
        eigenvalueEnumerator,
        patchData.patchCentre[patchIndex],
        patchData.patchSize[patchIndex],
        patchIndex,
        {x,y,z},
        unknown,
        patchData.dt[patchIndex],
        *(patchData.QOut + patchIndex),
        QOutEnumerator
      );
    }
  }

  if (evaluateMaximumEigenvalueAfterTimeStep and Dimensions==2) {
    logDebug( "timeStepWithRusanov_overlap1_sequential_AoS(...)", "compute maximum eigenvalue for subsequent time step" );
    for (int patchIndex=0; patchIndex<patchData.numberOfPatches; patchIndex++) {
      patchData.maxEigenvalue[patchIndex] = 0.0;
      // @todo Otter
      for (int x = 0; x < numberOfVolumesPerAxisInPatch; x++)
      for (int y = 0; y < numberOfVolumesPerAxisInPatch; y++) {
        patchData.maxEigenvalue[patchIndex] = std::max(
          patchData.maxEigenvalue[patchIndex],
          internal::reduce_max_eigenvalue_loop_body(
            *(patchData.QOut + patchIndex),
            QOutEnumerator,
            maxEigenvalue,
            patchData.patchCentre[patchIndex],
            patchData.patchSize[patchIndex],
            patchIndex,
            {x,y},
            patchData.t[patchIndex],
            patchData.dt[patchIndex]
          )
        );
      }
    }
  }
  else if (evaluateMaximumEigenvalueAfterTimeStep) {
    logDebug( "timeStepWithRusanov_overlap1_sequential_AoS(...)", "compute maximum eigenvalue for subsequent time step" );
    for (int patchIndex=0; patchIndex<patchData.numberOfPatches; patchIndex++) {
      patchData.maxEigenvalue[patchIndex] = 0.0;
      // @todo Otter
      for (int x = 0; x < numberOfVolumesPerAxisInPatch; x++)
      for (int y = 0; y < numberOfVolumesPerAxisInPatch; y++)
      for (int z = 0; z < numberOfVolumesPerAxisInPatch; z++) {
        patchData.maxEigenvalue[patchIndex] = std::max(
          patchData.maxEigenvalue[patchIndex],
          internal::reduce_max_eigenvalue_loop_body(
            *(patchData.QOut + patchIndex),
            QOutEnumerator,
            maxEigenvalue,
            patchData.patchCentre[patchIndex],
            patchData.patchSize[patchIndex],
            patchIndex,
            {x,y,z},
            patchData.t[patchIndex],
            patchData.dt[patchIndex]
          )
        );
      }
    }
  }

  if (tempFluxX!=nullptr)               tarch::freeMemory(tempFluxX, tarch::MemoryLocation::Heap);
  if (tempFluxY!=nullptr)               tarch::freeMemory(tempFluxY, tarch::MemoryLocation::Heap);
  if (tempFluxZ!=nullptr)               tarch::freeMemory(tempFluxZ, tarch::MemoryLocation::Heap);
  //if (tempNonconservativeProduct) tarch::freeMemory(tempFlux, tarch::MemoryLocation::Heap);

  if (tempEigenvalueX!=nullptr)         tarch::freeMemory(tempEigenvalueX, tarch::MemoryLocation::Heap);
  if (tempEigenvalueY!=nullptr)         tarch::freeMemory(tempEigenvalueY, tarch::MemoryLocation::Heap);
  if (tempEigenvalueZ!=nullptr)         tarch::freeMemory(tempEigenvalueZ, tarch::MemoryLocation::Heap);
}
