#include "Rusanov.h"
#include "MemoryManager.h"
#include "tarch/compiler/CompilerSpecificSettings.h"
#include "exahype2/fv/rusanov/LoopBody.h"

#include <nvtx3/nvToolsExt.h>

template <
  typename Solver,
  int      numberOfVolumesPerAxisInPatch,
  int      haloSize,
  int      unknowns,
  int      auxiliaryVariables,
  typename TempDataEnumerator
>
void exahype2::fv::rusanov::omp::opt::timeStepWithRusanov_patchwise_heap_static_calls(
  int                     targetDevice,
  ::exahype2::CellData&   hostPatchData,
  bool                    evaluateFlux,
  bool                    evaluateNonconservativeProduct,
  bool                    evaluateSource,
  bool                    evaluateMaximumEigenvalueAfterTimeStep
) {
  #if defined(GPUOffloadingOMP)
  /**
   * The halo size passed in by the user might differ from the one that we have to evaluate/
   * take into account from the solver's perspective.
   */
  constexpr int solversHaloSize        = 1;

  nvtxRangePush(__FUNCTION__);
  exahype2::fv::enumerator::AoSLexicographicVolumeEnumerator QInHostEnumerator       (                        1, numberOfVolumesPerAxisInPatch, haloSize, unknowns, auxiliaryVariables);
  exahype2::fv::enumerator::AoSLexicographicVolumeEnumerator QOutHostEnumerator      (                        1, numberOfVolumesPerAxisInPatch,        0, unknowns, auxiliaryVariables);
  TempDataEnumerator fluxHostEnumerator      (hostPatchData.numberOfCells, numberOfVolumesPerAxisInPatch,   solversHaloSize, unknowns, 0);
  TempDataEnumerator ncpHostEnumerator       (hostPatchData.numberOfCells, numberOfVolumesPerAxisInPatch+1, solversHaloSize, unknowns, 0);
  TempDataEnumerator eigenvalueHostEnumerator(hostPatchData.numberOfCells, numberOfVolumesPerAxisInPatch,   solversHaloSize,        1, 0);

  static MemoryManager::ScratchpadMemoryAllocator scratchpad(targetDevice, hostPatchData.numberOfCells, fluxHostEnumerator.size(), ncpHostEnumerator.size(), eigenvalueHostEnumerator.size());
  double* tempFluxX = scratchpad.tempFluxX;
  double* tempFluxY = scratchpad.tempFluxY;
  double* tempFluxZ = scratchpad.tempFluxZ;
  double* tempNonconservativeProductX = scratchpad.tempNonconservativeProductX;
  double* tempNonconservativeProductY = scratchpad.tempNonconservativeProductY;
  double* tempNonconservativeProductZ = scratchpad.tempNonconservativeProductZ;
  double* tempEigenvalueX = scratchpad.tempEigenvalueX;
  double* tempEigenvalueY = scratchpad.tempEigenvalueY;
  double* tempEigenvalueZ = scratchpad.tempEigenvalueZ;

  double**  mappedPointersToQIn          = new double*[ hostPatchData.numberOfCells ];
  double**  mappedPointersToQOut         = new double*[ hostPatchData.numberOfCells ];

  for (int patchIndex=0; patchIndex<hostPatchData.numberOfCells; patchIndex++) {
    const double* currentQIn  = hostPatchData.QIn[patchIndex];
    double*       currentQOut = hostPatchData.QOut[patchIndex];

    #pragma omp target enter data map(to:currentQIn[0:QInHostEnumerator.size()] ) device(targetDevice)
    #pragma omp target enter data map(alloc:currentQOut[0:QOutHostEnumerator.size()] ) device(targetDevice)
    
    mappedPointersToQIn[patchIndex]  = (double*) omp_get_mapped_ptr( currentQIn,  targetDevice );
    mappedPointersToQOut[patchIndex] = (double*) omp_get_mapped_ptr( currentQOut, targetDevice );
  }

  const int numberOfCells            = hostPatchData.numberOfCells;
  double*   rawPointerToPatchCentres = hostPatchData.patchCentre[0].data();
  double*   rawPointerToPatchSizes   = hostPatchData.patchSize[0].data();
  double*   t                        = hostPatchData.t;
  double*   dt                       = hostPatchData.dt;
  double*   maxEigenvalue            = hostPatchData.maxEigenvalue;

  nvtxRangePush("Enter offloading region");
  #pragma omp target teams distribute device(targetDevice) \
    map(to:rawPointerToPatchCentres[0:hostPatchData.numberOfCells*Dimensions]) \
    map(to:rawPointerToPatchSizes  [0:hostPatchData.numberOfCells*Dimensions]) \
    map(to:mappedPointersToQIn     [0:hostPatchData.numberOfCells])            \
    map(to:mappedPointersToQOut    [0:hostPatchData.numberOfCells])            \
    map(to:t[0:hostPatchData.numberOfCells]) \
    map(to:dt[0:hostPatchData.numberOfCells]) \
    map(from:maxEigenvalue[0:hostPatchData.numberOfCells]) \
    is_device_ptr(tempFluxX, tempFluxY, tempFluxZ, \
                  tempNonconservativeProductX, tempNonconservativeProductY, tempNonconservativeProductZ, \
                  tempEigenvalueX, tempEigenvalueY, tempEigenvalueZ)
  for (int patchIndex=0; patchIndex<numberOfCells; patchIndex++) {
    // TODO: Why are these enumerators created on the device? Can't we just use the host enumerators or is there too much overhead in copying (if at all possible)?
    exahype2::fv::enumerator::AoSLexicographicVolumeEnumerator QInEnumerator       (                        1, numberOfVolumesPerAxisInPatch,haloSize,unknowns,auxiliaryVariables);
    exahype2::fv::enumerator::AoSLexicographicVolumeEnumerator QOutEnumerator      (                        1, numberOfVolumesPerAxisInPatch,       0,unknowns,auxiliaryVariables);
    TempDataEnumerator fluxEnumerator      (numberOfCells, numberOfVolumesPerAxisInPatch,haloSize,unknowns,0);
    TempDataEnumerator ncpEnumerator       (numberOfCells, numberOfVolumesPerAxisInPatch+1,haloSize,unknowns,0);
    TempDataEnumerator eigenvalueEnumerator(numberOfCells, numberOfVolumesPerAxisInPatch,haloSize,1,0);

    double* QIn = mappedPointersToQIn;
    double* QOut = mappedPointersToQOut;

    if (Dimensions==2 and evaluateSource) {
      #pragma omp parallel for simd collapse(2)
      for (int y = 0; y < numberOfVolumesPerAxisInPatch; y++)
      for (int x = 0; x < numberOfVolumesPerAxisInPatch; x++) {
        internal::copySolutionAndAddSourceTerm_LoopBody<Solver>(
          QIn[patchIndex],
          QInEnumerator,
          patchCentre[patchIndex],
          patchSize[patchIndex],
          patchIndex,
          volumeIndex2d(x,y),
          t[patchIndex],
          dt[patchIndex],
          QOut[patchIndex],
          QOutEnumerator
        );
      }
    }
    else if (Dimensions==3 and evaluateSource) {
      #pragma omp parallel for simd collapse(3)
      for (int z = 0; z < numberOfVolumesPerAxisInPatch; z++)
      for (int y = 0; y < numberOfVolumesPerAxisInPatch; y++)
      for (int x = 0; x < numberOfVolumesPerAxisInPatch; x++) {
        internal::copySolutionAndAddSourceTerm_LoopBody<Solver>(
          QIn[patchIndex],
          QInEnumerator,
          patchCentre[patchIndex],
          patchSize[patchIndex],
          patchIndex,
          volumeIndex3d(x,y,z),
          t[patchIndex],
          dt[patchIndex],
          QOut[patchIndex],
          QOutEnumerator
        );
      }
    } 
    else if (Dimensions==2) {
      for (int y = 0; y < numberOfVolumesPerAxisInPatch; y++)
      #pragma omp parallel for simd collapse(2) // TODO: Why do we only collapse(2) here?
      for (int x = 0; x < numberOfVolumesPerAxisInPatch; x++)
      for (int unknown=0; unknown<unknowns+auxiliaryVariables; unknown++) {
        internal::copySolution_LoopBody(
          QIn[patchIndex],
          QInEnumerator,
          patchIndex,
          volumeIndex2d(x,y),
          unknown,
          QOut[patchIndex],
          QOutEnumerator
        );
      }
    }
    else {
      #pragma omp parallel for simd collapse(4)
      for (int z = 0; z < numberOfVolumesPerAxisInPatch; z++)
      for (int y = 0; y < numberOfVolumesPerAxisInPatch; y++)
      for (int x = 0; x < numberOfVolumesPerAxisInPatch; x++)
      for (int unknown=0; unknown<unknowns+auxiliaryVariables; unknown++) {
        internal::copySolution_LoopBody(
          QIn[patchIndex],
          QInEnumerator,
          patchIndex,
          volumeIndex3d(x,y,z),
          unknown,
          QOut[patchIndex],
          QOutEnumerator
        );
      }
    }

    if (Dimensions==2) {
      #pragma omp parallel for collapse(2)
      for (int y = 0; y < numberOfVolumesPerAxisInPatch; y++)
      for (int x = -solversHaloSize; x < numberOfVolumesPerAxisInPatch+solversHaloSize; x++) {
        internal::computeMaxEigenvalue_LoopBody<Solver>(
            QIn[patchIndex],
            QInEnumerator,
            patchCentre[patchIndex],
            patchSize[patchIndex],
            patchIndex,
            volumeIndex2d(x,y),
            t[patchIndex], dt[patchIndex],
            0,
            tempEigenvalueX,
            eigenvalueEnumerator
        );
      }

      #pragma omp parallel for collapse(2)
      for (int y = -solversHaloSize; y < numberOfVolumesPerAxisInPatch+solversHaloSize; y++)
      for (int x = 0; x < numberOfVolumesPerAxisInPatch; x++) {
        internal::computeMaxEigenvalue_LoopBody<Solver>(
            QIn[patchIndex],
            QInEnumerator,
            patchCentre[patchIndex],
            patchSize[patchIndex],
            patchIndex,
            volumeIndex2d(x,y),
            t[patchIndex], dt[patchIndex],
            1,
            tempEigenvalueY,
            eigenvalueEnumerator
        );
      }


      #pragma omp parallel for collapse(3)
      for (int y = 0; y < numberOfVolumesPerAxisInPatch; y++)
      for (int x = 0; x < numberOfVolumesPerAxisInPatch; x++)
      for (int unknown = 0; unknown < unknowns; unknown++) {
        internal::updateSolutionWithEigenvalueDamping_LoopBody(
          QIn[patchIndex],
          QInEnumerator,
          tempEigenvalueX,
          tempEigenvalueY,
          tempEigenvalueZ,
          eigenvalueEnumerator,
          patchCentre[patchIndex],
          patchSize[patchIndex],
          patchIndex,
          volumeIndex2d(x,y),
          unknown,
          dt[patchIndex],
          QOut[patchIndex],
          QOutEnumerator
        );
      } 
    }
    else {
      #pragma omp parallel for collapse(3)
      for (int z = 0; z < numberOfVolumesPerAxisInPatch; z++)
      for (int y = 0; y < numberOfVolumesPerAxisInPatch; y++)
      for (int x = -solversHaloSize; x < numberOfVolumesPerAxisInPatch+solversHaloSize; x++) {
        internal::computeMaxEigenvalue_LoopBody<Solver>(
          QIn[patchIndex],
          QInEnumerator,
          patchCentre[patchIndex],
          patchSize[patchIndex],
          patchIndex,
          volumeIndex3d(x,y,z),
          t[patchIndex], dt[patchIndex],
          0,
          tempEigenvalueX,
          eigenvalueEnumerator
        );
      }


      #pragma omp parallel for collapse(3)
      for (int z = 0; z < numberOfVolumesPerAxisInPatch; z++)
      for (int y = -solversHaloSize; y < numberOfVolumesPerAxisInPatch+solversHaloSize; y++)
      for (int x = 0; x < numberOfVolumesPerAxisInPatch; x++) {
        internal::computeMaxEigenvalue_LoopBody<Solver>(
              QIn[patchIndex],
              QInEnumerator,
              patchCentre[patchIndex],
              patchSize[patchIndex],
              patchIndex,
              volumeIndex3d(x,y,z),
              t[patchIndex], dt[patchIndex],
              1,
              tempEigenvalueY,
              eigenvalueEnumerator
        );
      }


      #pragma omp parallel for collapse(3)
      for (int z = -solversHaloSize; z < numberOfVolumesPerAxisInPatch+solversHaloSize; z++)
      for (int y = 0; y < numberOfVolumesPerAxisInPatch; y++)
      for (int x = 0; x < numberOfVolumesPerAxisInPatch; x++) {
        internal::computeMaxEigenvalue_LoopBody<Solver>(
              QIn[patchIndex],
              QInEnumerator,
              patchCentre[patchIndex],
              patchSize[patchIndex],
              patchIndex,
              volumeIndex3d(x,y,z),
              t[patchIndex], dt[patchIndex],
              2,
              tempEigenvalueZ,
              eigenvalueEnumerator
        );
      }


      #pragma omp parallel for collapse(4)
      for (int z = 0; z < numberOfVolumesPerAxisInPatch; z++)
      for (int y = 0; y < numberOfVolumesPerAxisInPatch; y++)
      for (int x = 0; x < numberOfVolumesPerAxisInPatch; x++)
      for (int unknown = 0; unknown < unknowns; unknown++) {
        internal::updateSolutionWithEigenvalueDamping_LoopBody(
          QIn[patchIndex],
          QInEnumerator,
          tempEigenvalueX,
          tempEigenvalueY,
          tempEigenvalueZ,
          eigenvalueEnumerator,
          patchCentre[patchIndex],
          patchSize[patchIndex],
          patchIndex,
          volumeIndex3d(x,y,z),
          unknown,
          dt[patchIndex],
          QOut[patchIndex],
          QOutEnumerator
        );
      }
    }


    if (evaluateFlux and Dimensions==2) {
      #pragma omp parallel for collapse(2)
      for (int y =         0; y < numberOfVolumesPerAxisInPatch;          y++)
      for (int x = -solversHaloSize; x < numberOfVolumesPerAxisInPatch+solversHaloSize; x++) {
        internal::computeFlux_LoopBody<Solver>(
          QIn[patchIndex],
          QInEnumerator,
          patchCentre[patchIndex],
          patchSize[patchIndex],
          patchIndex,
          volumeIndex2d(x,y),
          t[patchIndex],
          dt[patchIndex],
          0, // normal
          tempFluxX,
          fluxEnumerator
        );
      }


      #pragma omp parallel for collapse(2)
      for (int y = -solversHaloSize; y < numberOfVolumesPerAxisInPatch+solversHaloSize;  y++)
      for (int x =         0; x < numberOfVolumesPerAxisInPatch;           x++) {
        internal::computeFlux_LoopBody<Solver>(
          QIn[patchIndex],
          QInEnumerator,
          patchCentre[patchIndex],
          patchSize[patchIndex],
          patchIndex,
          volumeIndex2d(x,y),
          t[patchIndex],
          dt[patchIndex],
          1, // normal
          tempFluxY,
          fluxEnumerator
        );
      }


      #pragma omp parallel for collapse(3)
      for (int y = 0; y < numberOfVolumesPerAxisInPatch; y++)
      for (int x = 0; x < numberOfVolumesPerAxisInPatch; x++)
      for (int unknown=0; unknown<unknowns; unknown++) {
        internal::updateSolutionWithFlux_LoopBody(
          tempFluxX, tempFluxY, tempFluxZ,
          fluxEnumerator,
          patchCentre[patchIndex],
          patchSize[patchIndex],
          patchIndex,
          volumeIndex2d(x,y),
          unknown,
          dt[patchIndex],
          *(QOut + patchIndex),
          QOutEnumerator
        );
      }
    }
    else if (evaluateFlux and Dimensions==3) {
      #pragma omp parallel for collapse(3)
      for (int z =         0; z < numberOfVolumesPerAxisInPatch;          z++)
      for (int y =         0; y < numberOfVolumesPerAxisInPatch;          y++)
      for (int x = -solversHaloSize; x < numberOfVolumesPerAxisInPatch+solversHaloSize; x++) {
        internal::computeFlux_LoopBody<Solver>(
          QIn[patchIndex],
          QInEnumerator,
          patchCentre[patchIndex],
          patchSize[patchIndex],
          patchIndex,
          volumeIndex3d(x,y,z),
          t[patchIndex],
          dt[patchIndex],
          0, // normal
          tempFluxX,
          fluxEnumerator
        );
      }


      #pragma omp parallel for collapse(3)
      for (int z =                0; z < numberOfVolumesPerAxisInPatch;                  z++)
      for (int y = -solversHaloSize; y < numberOfVolumesPerAxisInPatch+solversHaloSize;  y++)
      for (int x =         0; x < numberOfVolumesPerAxisInPatch;           x++) {
        internal::computeFlux_LoopBody<Solver>(
          QIn[patchIndex],
          QInEnumerator,
          patchCentre[patchIndex],
          patchSize[patchIndex],
          patchIndex,
          volumeIndex3d(x,y,z),
          t[patchIndex],
          dt[patchIndex],
          1, // normal
          tempFluxY,
          fluxEnumerator
        );
      }


      #pragma omp parallel for collapse(3)
      for (int z = -solversHaloSize; z < numberOfVolumesPerAxisInPatch+solversHaloSize; z++)
      for (int y =                0; y < numberOfVolumesPerAxisInPatch;                 y++)
      for (int x =                0; x < numberOfVolumesPerAxisInPatch;                 x++) {
        internal::computeFlux_LoopBody<Solver>(
          QIn[patchIndex],
          QInEnumerator,
          patchCentre[patchIndex],
          patchSize[patchIndex],
          patchIndex,
          volumeIndex3d(x,y,z),
          t[patchIndex],
          dt[patchIndex],
          2, // normal
          tempFluxZ,
          fluxEnumerator
        );
      }


      #pragma omp parallel for collapse(4)
      for (int z = 0; z < numberOfVolumesPerAxisInPatch; z++)
      for (int y = 0; y < numberOfVolumesPerAxisInPatch; y++)
      for (int x = 0; x < numberOfVolumesPerAxisInPatch; x++)
      for (int unknown=0; unknown<unknowns; unknown++) {
        internal::updateSolutionWithFlux_LoopBody(
          tempFluxX, tempFluxY, tempFluxZ,
          fluxEnumerator,
          patchCentre[patchIndex],
          patchSize[patchIndex],
          patchIndex,
          volumeIndex3d(x,y,z),
          unknown,
          dt[patchIndex],
          *(QOut + patchIndex),
          QOutEnumerator
        );
      }
    }


    if (evaluateNonconservativeProduct and Dimensions==2) {
      #pragma omp parallel for collapse(2)
      for (int y =                0; y < numberOfVolumesPerAxisInPatch;                   y++)
      for (int x = -solversHaloSize; x < numberOfVolumesPerAxisInPatch+solversHaloSize-1; x++) {
        internal::computeNonconservativeFlux_LoopBody<Solver,unknowns,auxiliaryVariables>(
          QIn[patchIndex],
          QInEnumerator,
          patchCentre[patchIndex],
          patchSize[patchIndex],
          patchIndex,
          volumeIndex2d(x,y),
          t[patchIndex],
          dt[patchIndex],
          0, // normal
          tempNonconservativeProductX,
          ncpEnumerator
        );
      }


      #pragma omp parallel for collapse(2)
      for (int y = -solversHaloSize; y < numberOfVolumesPerAxisInPatch+solversHaloSize-1; y++)
      for (int x =                0; x < numberOfVolumesPerAxisInPatch;                   x++) {
        internal::computeNonconservativeFlux_LoopBody<Solver,unknowns,auxiliaryVariables>(
          QIn[patchIndex],
          QInEnumerator,
          patchCentre[patchIndex],
          patchSize[patchIndex],
          patchIndex,
          volumeIndex2d(x,y),
          t[patchIndex],
          dt[patchIndex],
          1, // normal
          tempNonconservativeProductY,
          ncpEnumerator
        );
      }


      #pragma omp parallel for collapse(3)
      for (int y = 0; y < numberOfVolumesPerAxisInPatch; y++)
      for (int x = 0; x < numberOfVolumesPerAxisInPatch; x++)
      for (int unknown=0; unknown<unknowns; unknown++) {
        internal::updateSolutionWithNonconservativeFlux_LoopBody(
          tempNonconservativeProductX, tempNonconservativeProductY, tempNonconservativeProductZ,
          ncpEnumerator,
          patchCentre[patchIndex],
          patchSize[patchIndex],
          patchIndex,
          volumeIndex2d(x,y),
          unknown,
          dt[patchIndex],
          *(QOut + patchIndex),
          QOutEnumerator
        );
      }
    }
    else if (evaluateNonconservativeProduct and Dimensions==3) {
        #pragma omp parallel for collapse(3)
        for (int z =         0; z < numberOfVolumesPerAxisInPatch;            z++)
        for (int y =         0; y < numberOfVolumesPerAxisInPatch;            y++)
        for (int x = -solversHaloSize; x < numberOfVolumesPerAxisInPatch+solversHaloSize-1; x++) {
          internal::computeNonconservativeFlux_LoopBody<Solver,unknowns,auxiliaryVariables>(
            QIn[patchIndex],
            QInEnumerator,
            patchCentre[patchIndex],
            patchSize[patchIndex],
            patchIndex,
            volumeIndex3d(x,y,z),
            t[patchIndex],
            dt[patchIndex],
            0, // normal
            tempNonconservativeProductX,
            ncpEnumerator
          );
        }


        #pragma omp parallel for collapse(3)
        for (int z =         0; z < numberOfVolumesPerAxisInPatch;            z++)
        for (int y = -solversHaloSize; y < numberOfVolumesPerAxisInPatch+solversHaloSize-1; y++)
        for (int x =         0; x < numberOfVolumesPerAxisInPatch;            x++) {
          internal::computeNonconservativeFlux_LoopBody<Solver,unknowns,auxiliaryVariables>(
            QIn[patchIndex],
            QInEnumerator,
            patchCentre[patchIndex],
            patchSize[patchIndex],
            patchIndex,
            volumeIndex3d(x,y,z),
            t[patchIndex],
            dt[patchIndex],
            1, // normal
            tempNonconservativeProductY,
            ncpEnumerator
          );
        }


        #pragma omp parallel for collapse(3)
        for (int z = -solversHaloSize; z < numberOfVolumesPerAxisInPatch+solversHaloSize-1; z++)
        for (int y =         0; y < numberOfVolumesPerAxisInPatch;            y++)
        for (int x =         0; x < numberOfVolumesPerAxisInPatch;            x++) {
          internal::computeNonconservativeFlux_LoopBody<Solver,unknowns,auxiliaryVariables>(
            QIn[patchIndex],
            QInEnumerator,
            patchCentre[patchIndex],
            patchSize[patchIndex],
            patchIndex,
            volumeIndex3d(x,y,z),
            t[patchIndex],
            dt[patchIndex],
            2, // normal
            tempNonconservativeProductZ,
            ncpEnumerator
          );
        }


        #pragma omp parallel for collapse(4)
        for (int z = 0; z < numberOfVolumesPerAxisInPatch; z++)
        for (int y = 0; y < numberOfVolumesPerAxisInPatch; y++)
        for (int x = 0; x < numberOfVolumesPerAxisInPatch; x++)
        for (int unknown=0; unknown<unknowns; unknown++) {
          internal::updateSolutionWithNonconservativeFlux_LoopBody(
            tempNonconservativeProductX, tempNonconservativeProductY, tempNonconservativeProductZ,
            ncpEnumerator,
            patchCentre[patchIndex],
            patchSize[patchIndex],
            patchIndex,
            volumeIndex3d(x,y,z),
            unknown,
            dt[patchIndex],
            *(QOut + patchIndex),
            QOutEnumerator
          );
        }
    }
    if (evaluateMaximumEigenvalueAfterTimeStep and Dimensions==2) {
      double newMaxEigenvalue = 0.0;
      #pragma omp parallel for collapse(2) reduction(max:newMaxEigenvalue)
      for (int y = 0; y < numberOfVolumesPerAxisInPatch; y++)
      for (int x = 0; x < numberOfVolumesPerAxisInPatch; x++) {
        newMaxEigenvalue = std::max(
          newMaxEigenvalue,
          internal::reduceMaxEigenvalue_LoopBody<Solver>(
            *(QOut + patchIndex),
            QOutEnumerator,
            patchCentre[patchIndex],
            patchSize[patchIndex],
            patchIndex,
            volumeIndex2d(x,y),
            t[patchIndex],
            dt[patchIndex]
          )
        );
      }
      maxEigenvalue[patchIndex] = newMaxEigenvalue;
    }
    else if (evaluateMaximumEigenvalueAfterTimeStep) {
      double newMaxEigenvalue = 0.0;
      #pragma omp parallel for collapse(3) reduction(max:newMaxEigenvalue)
      for (int z = 0; z < numberOfVolumesPerAxisInPatch; z++)
      for (int y = 0; y < numberOfVolumesPerAxisInPatch; y++)
      for (int x = 0; x < numberOfVolumesPerAxisInPatch; x++) {
        newMaxEigenvalue = std::max(
          newMaxEigenvalue,
          internal::reduceMaxEigenvalue_LoopBody<Solver>(
            *(QOut + patchIndex),
            QOutEnumerator,
            patchCentre[patchIndex],
            patchSize[patchIndex],
            patchIndex,
            volumeIndex3d(x,y,z),
            t[patchIndex],
            dt[patchIndex]
          )
        );
      }
    }
  } // This is the end of the target offloading block
  nvtxRangePop();

  for (int patchIndex=0; patchIndex<hostPatchData.numberOfCells; patchIndex++) {
    const double* currentQIn  = hostPatchData.QIn[patchIndex];
    double*       currentQOut = hostPatchData.QOut[patchIndex];
    #pragma omp target exit data map(delete:currentQIn[0:QInHostEnumerator.size()] ) device(targetDevice)
    #pragma omp target exit data map(from:currentQOut[0:QOutHostEnumerator.size()] ) device(targetDevice)
  }

  delete[] mappedPointersToQIn;
  delete[] mappedPointersToQOut;

  nvtxRangePop();
  #else
  assertionMsg(false, "this routine should only be used in combination with OpenMP offloading, as it has a device parameter" );
  #endif
}

