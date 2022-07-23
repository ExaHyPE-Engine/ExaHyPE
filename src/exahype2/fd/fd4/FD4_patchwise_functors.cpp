#include "FD4.h"
#include "tarch/multicore/otter.h"
#include "exahype2/fd/LoopBody.h"
#include "exahype2/fd/fd4/LoopBody.h"
#include "exahype2/fd/FD_Helper.cpph"


void exahype2::fd::fd4::timeStep_patchwise_heap_functors(
  ::exahype2::CellData&   patchData,
  int                     numberOfGridCellsPerPatchPerAxis,
  int                     haloSize,
  int                     unknowns,
  int                     auxiliaryVariables,
  double                  KOSigma,
  bool                    evaluateFlux,
  bool                    evaluateDifferentialSource, //for ncp
  bool                    evaluateAlgebraicSource, //real source
  bool                    copyOldTimeStepAndScaleWithTimeStepSize,
  Flux                    flux,
  NonconservativeProduct  DifferentialSource,
  Source                  AlgebraicSource
) {
  static tarch::logging::Log _log( "exahype2::fd" );

  assertionEquals(haloSize,3);

  bool evaluateKODissipation = KOSigma>0.0;

  exahype2::enumerator::AoSLexicographicEnumerator QInEnumerator (1,numberOfGridCellsPerPatchPerAxis,haloSize,unknowns,auxiliaryVariables);
  exahype2::enumerator::AoSLexicographicEnumerator QOutEnumerator(1,numberOfGridCellsPerPatchPerAxis,0,       unknowns,auxiliaryVariables);

  double* tempFluxX =  evaluateFlux                                                ? tarch::allocateMemory( QOutEnumerator.size(),tarch::MemoryLocation::Heap ) : nullptr;
  double* tempFluxY =  evaluateFlux                                                ? tarch::allocateMemory( QOutEnumerator.size(),tarch::MemoryLocation::Heap ) : nullptr;
  double* tempFluxZ = (evaluateFlux and Dimensions==3)                             ? tarch::allocateMemory( QOutEnumerator.size(),tarch::MemoryLocation::Heap ) : nullptr;
  double* tempDifferentialSourceX =  evaluateDifferentialSource                    ? tarch::allocateMemory( QOutEnumerator.size(),tarch::MemoryLocation::Heap ) : nullptr;
  double* tempDifferentialSourceY =  evaluateDifferentialSource                    ? tarch::allocateMemory( QOutEnumerator.size(),tarch::MemoryLocation::Heap ) : nullptr;
  double* tempDifferentialSourceZ = (evaluateDifferentialSource and Dimensions==3) ? tarch::allocateMemory( QOutEnumerator.size(),tarch::MemoryLocation::Heap ) : nullptr;
  double* tempKODissipationX      =  evaluateKODissipation                         ? tarch::allocateMemory( QOutEnumerator.size(),tarch::MemoryLocation::Heap ) : nullptr;
  double* tempKODissipationY      =  evaluateKODissipation                         ? tarch::allocateMemory( QOutEnumerator.size(),tarch::MemoryLocation::Heap ) : nullptr;
  double* tempKODissipationZ      = (evaluateKODissipation      and Dimensions==3) ? tarch::allocateMemory( QOutEnumerator.size(),tarch::MemoryLocation::Heap ) : nullptr;


  // STEP 1: copy solution from old array to new array, and/or add the change due to algebraic source terms
  for (int patchIndex=0; patchIndex<patchData.numberOfCells; patchIndex++) {
    const double timeStepSize = copyOldTimeStepAndScaleWithTimeStepSize ? patchData.dt[patchIndex] : 1.0;

    logDebug( "timeStepWithFD4_sequential(...)", "evaluate algebraic source terms and/or copy solution over" );
    if (Dimensions==2 and evaluateAlgebraicSource and copyOldTimeStepAndScaleWithTimeStepSize) {
      otterLoopBegin();
      for (int x = 0; x < numberOfGridCellsPerPatchPerAxis; x++)
      for (int y = 0; y < numberOfGridCellsPerPatchPerAxis; y++) {
        otterLoopIterationBegin();
        ::exahype2::fd::internal::copySolutionAndAddAlgebraicSourceTerm_LoopBody(
          patchData.QIn[patchIndex],
          QInEnumerator,
          AlgebraicSource,
          patchData.cellCentre[patchIndex],
          patchData.cellSize[patchIndex],
          patchIndex,
          gridCellIndex2d(x,y),
          patchData.t[patchIndex],
          timeStepSize,
          patchData.QOut[patchIndex],
          QOutEnumerator
        );
        otterLoopIterationEnd();
      }
      otterLoopEnd();
    }
    else if (Dimensions==3 and evaluateAlgebraicSource and copyOldTimeStepAndScaleWithTimeStepSize) {
      otterLoopBegin();
      for (int x = 0; x < numberOfGridCellsPerPatchPerAxis; x++)
      for (int y = 0; y < numberOfGridCellsPerPatchPerAxis; y++)
      for (int z = 0; z < numberOfGridCellsPerPatchPerAxis; z++) {
        otterLoopIterationBegin();
        ::exahype2::fd::internal::copySolutionAndAddAlgebraicSourceTerm_LoopBody(
          patchData.QIn[patchIndex],
          QInEnumerator,
          AlgebraicSource,
          patchData.cellCentre[patchIndex],
          patchData.cellSize[patchIndex],
          patchIndex,
          gridCellIndex3d(x,y,z),
          patchData.t[patchIndex],
          timeStepSize,
          patchData.QOut[patchIndex],
          QOutEnumerator
        );
        otterLoopIterationEnd();
      }
      otterLoopEnd();
    }
    else if (Dimensions==2 and copyOldTimeStepAndScaleWithTimeStepSize) {
      otterLoopBegin();
      for (int x = 0; x < numberOfGridCellsPerPatchPerAxis; x++)
      for (int y = 0; y < numberOfGridCellsPerPatchPerAxis; y++)
      for (int unknown=0; unknown<unknowns+auxiliaryVariables; unknown++) {
        otterLoopIterationBegin();
        ::exahype2::fd::internal::copySolution_LoopBody(
          patchData.QIn[patchIndex],
          QInEnumerator,
          patchIndex,
          gridCellIndex2d(x,y),
          unknown,
          patchData.QOut[patchIndex],
          QOutEnumerator
        );
        otterLoopIterationEnd();
      }
      otterLoopEnd();
    }
    else if ( copyOldTimeStepAndScaleWithTimeStepSize ) {
      otterLoopBegin();
      for (int x = 0; x < numberOfGridCellsPerPatchPerAxis; x++)
      for (int y = 0; y < numberOfGridCellsPerPatchPerAxis; y++)
      for (int z = 0; z < numberOfGridCellsPerPatchPerAxis; z++)
      for (int unknown=0; unknown<unknowns+auxiliaryVariables; unknown++) {
        otterLoopIterationBegin();
        ::exahype2::fd::internal::copySolution_LoopBody(
          patchData.QIn[patchIndex],
          QInEnumerator,
          patchIndex,
          gridCellIndex3d(x,y,z),
          unknown,
          patchData.QOut[patchIndex],
          QOutEnumerator
        );
        otterLoopIterationEnd();
      }
      otterLoopEnd();
    }
    else if (Dimensions==3){
      otterLoopBegin();
      for (int x = 0; x < numberOfGridCellsPerPatchPerAxis; x++)
      for (int y = 0; y < numberOfGridCellsPerPatchPerAxis; y++)
      for (int z = 0; z < numberOfGridCellsPerPatchPerAxis; z++)
      for (int unknown=0; unknown<unknowns+auxiliaryVariables; unknown++) {
        otterLoopIterationBegin();
        ::exahype2::fd::internal::clearSolution_LoopBody(
          patchIndex,
          gridCellIndex3d(x,y,z),
          unknown,
          patchData.QOut[patchIndex],
          QOutEnumerator
        );
        otterLoopIterationEnd();
      }
      otterLoopEnd();
    }
    else {
      otterLoopBegin();
      for (int x = 0; x < numberOfGridCellsPerPatchPerAxis; x++)
      for (int y = 0; y < numberOfGridCellsPerPatchPerAxis; y++)
      for (int unknown=0; unknown<unknowns+auxiliaryVariables; unknown++) {
        otterLoopIterationBegin();
        ::exahype2::fd::internal::clearSolution_LoopBody(
          patchIndex,
          gridCellIndex2d(x,y),
          unknown,
          patchData.QOut[patchIndex],
          QOutEnumerator
        );
        otterLoopIterationEnd();
      }
      otterLoopEnd();
    }


    // STEP 2: Evaluate flux and update solution accordingly 
    if (evaluateFlux and Dimensions==2) {
      otterLoopBegin();
      for (int x = 0; x < numberOfGridCellsPerPatchPerAxis; x++)
      for (int y = 0; y < numberOfGridCellsPerPatchPerAxis; y++) {
        otterLoopIterationBegin();
        internal::computeFlux_LoopBody(
          patchData.QIn[patchIndex],
          QInEnumerator,
          flux,
          patchData.cellCentre[patchIndex],
          patchData.cellSize[patchIndex],
          patchIndex,
          gridCellIndex2d(x,y),
          patchData.t[patchIndex],
          timeStepSize,
          0, // normal
          tempFluxX,
          QOutEnumerator
        );
        otterLoopIterationEnd();
      }
      otterLoopEnd();

      otterLoopBegin();
      for (int x = 0; x < numberOfGridCellsPerPatchPerAxis; x++)
      for (int y = 0; y < numberOfGridCellsPerPatchPerAxis; y++) {
        otterLoopIterationBegin();
        internal::computeFlux_LoopBody(
          patchData.QIn[patchIndex],
          QInEnumerator,
          flux,
          patchData.cellCentre[patchIndex],
          patchData.cellSize[patchIndex],
          patchIndex,
          gridCellIndex2d(x,y),
          patchData.t[patchIndex],
          timeStepSize,
          1, // normal
          tempFluxY,
          QOutEnumerator
        );
        otterLoopIterationEnd();
      }
      otterLoopEnd();

      otterLoopBegin();
      for (int x = 0; x < numberOfGridCellsPerPatchPerAxis; x++)
      for (int y = 0; y < numberOfGridCellsPerPatchPerAxis; y++)
      for (int unknown=0; unknown<unknowns; unknown++) {
        otterLoopIterationBegin();
        internal::updateSolutionWithFlux_LoopBody(
          tempFluxX, tempFluxY, tempFluxZ,
          QOutEnumerator,
          patchData.cellCentre[patchIndex],
          patchData.cellSize[patchIndex],
          patchIndex,
          gridCellIndex2d(x,y),
          unknown,
          timeStepSize,
          *(patchData.QOut + patchIndex),
          QOutEnumerator
        );
        otterLoopIterationEnd();
      }
      otterLoopEnd();
    }
    else if (evaluateFlux and Dimensions==3) {
      otterLoopBegin();
      for (int x = 0; x < numberOfGridCellsPerPatchPerAxis; x++)
      for (int y = 0; y < numberOfGridCellsPerPatchPerAxis; y++)
      for (int z = 0; z < numberOfGridCellsPerPatchPerAxis; z++) {
        otterLoopIterationBegin();
        internal::computeFlux_LoopBody(
          patchData.QIn[patchIndex],
          QInEnumerator,
          flux,
          patchData.cellCentre[patchIndex],
          patchData.cellSize[patchIndex],
          patchIndex,
          gridCellIndex3d(x,y,z),
          patchData.t[patchIndex],
          timeStepSize,
          0, // normal
          tempFluxX,
          QOutEnumerator
        );
        otterLoopIterationEnd();
      }
      otterLoopEnd();

      otterLoopBegin();
      for (int x = 0; x < numberOfGridCellsPerPatchPerAxis; x++)
      for (int y = 0; y < numberOfGridCellsPerPatchPerAxis; y++)
      for (int z = 0; z < numberOfGridCellsPerPatchPerAxis; z++) {
        otterLoopIterationBegin();
        internal::computeFlux_LoopBody(
          patchData.QIn[patchIndex],
          QInEnumerator,
          flux,
          patchData.cellCentre[patchIndex],
          patchData.cellSize[patchIndex],
          patchIndex,
          gridCellIndex3d(x,y,z),
          patchData.t[patchIndex],
          timeStepSize,
          1, // normal
          tempFluxY,
          QOutEnumerator
        );
        otterLoopIterationEnd();
      }
      otterLoopEnd();

      otterLoopBegin();
      for (int x = 0; x < numberOfGridCellsPerPatchPerAxis; x++)
      for (int y = 0; y < numberOfGridCellsPerPatchPerAxis; y++)
      for (int z = 0; z < numberOfGridCellsPerPatchPerAxis; z++) {
        otterLoopIterationBegin();
        internal::computeFlux_LoopBody(
          patchData.QIn[patchIndex],
          QInEnumerator,
          flux,
          patchData.cellCentre[patchIndex],
          patchData.cellSize[patchIndex],
          patchIndex,
          gridCellIndex3d(x,y,z),
          patchData.t[patchIndex],
          timeStepSize,
          2, // normal
          tempFluxZ,
          QOutEnumerator
        );
        otterLoopIterationEnd();
      }
      otterLoopEnd();

      for (int x = 0; x < numberOfGridCellsPerPatchPerAxis; x++)
      for (int y = 0; y < numberOfGridCellsPerPatchPerAxis; y++)
      for (int z = 0; z < numberOfGridCellsPerPatchPerAxis; z++)
      for (int unknown=0; unknown<unknowns; unknown++) {
        internal::updateSolutionWithFlux_LoopBody(
          tempFluxX, tempFluxY, tempFluxZ,
          QOutEnumerator,
          patchData.cellCentre[patchIndex],
          patchData.cellSize[patchIndex],
          patchIndex,
          gridCellIndex2d(x,y),
          unknown,
          timeStepSize,
          *(patchData.QOut + patchIndex),
          QOutEnumerator
        );
      }
    }

    // STEP 3: Evaluate differential source and update solution accordingly 
    if (evaluateDifferentialSource and Dimensions==2) {
      otterLoopBegin();
      for (int x = 0; x < numberOfGridCellsPerPatchPerAxis; x++)
      for (int y = 0; y < numberOfGridCellsPerPatchPerAxis; y++) {
        otterLoopIterationBegin();
        internal::computeDifferentialSourceTerm_LoopBody(
          patchData.QIn[patchIndex],
          QInEnumerator,
          DifferentialSource,
          patchData.cellCentre[patchIndex],
          patchData.cellSize[patchIndex],
          patchIndex,
          gridCellIndex2d(x,y),
          patchData.t[patchIndex],
          timeStepSize,
          0, // normal
          tempDifferentialSourceX,
          QOutEnumerator
        );
        otterLoopIterationEnd();
      }
      otterLoopEnd();

      otterLoopBegin();
      for (int x = 0; x < numberOfGridCellsPerPatchPerAxis; x++)
      for (int y = 0; y < numberOfGridCellsPerPatchPerAxis; y++) {
        otterLoopIterationBegin();
        internal::computeDifferentialSourceTerm_LoopBody(
          patchData.QIn[patchIndex],
          QInEnumerator,
          DifferentialSource,
          patchData.cellCentre[patchIndex],
          patchData.cellSize[patchIndex],
          patchIndex,
          gridCellIndex2d(x,y),
          patchData.t[patchIndex],
          timeStepSize,
          1, // normal
          tempDifferentialSourceY,
          QOutEnumerator
        );
        otterLoopIterationEnd();
      }
      otterLoopEnd();

      otterLoopBegin();
      for (int x = 0; x < numberOfGridCellsPerPatchPerAxis; x++)
      for (int y = 0; y < numberOfGridCellsPerPatchPerAxis; y++)
      for (int unknown=0; unknown<unknowns; unknown++) {
        otterLoopIterationBegin();
        internal::updateSolutionWithDifferentialSourceTerm_LoopBody(
          tempDifferentialSourceX, tempDifferentialSourceY, tempDifferentialSourceZ,
          QOutEnumerator,
          patchData.cellCentre[patchIndex],
          patchData.cellSize[patchIndex],
          patchIndex,
          gridCellIndex2d(x,y),
          unknown,
          timeStepSize,
          *(patchData.QOut + patchIndex),
          QOutEnumerator
        );
        otterLoopIterationEnd();
      }
      otterLoopEnd();
    }
    else if (evaluateDifferentialSource and Dimensions==3) {
      otterLoopBegin();
      for (int x = 0; x < numberOfGridCellsPerPatchPerAxis; x++)
      for (int y = 0; y < numberOfGridCellsPerPatchPerAxis; y++)
      for (int z = 0; z < numberOfGridCellsPerPatchPerAxis; z++) {
        otterLoopIterationBegin();
        internal::computeDifferentialSourceTerm_LoopBody(
          patchData.QIn[patchIndex],
          QInEnumerator,
          DifferentialSource,
          patchData.cellCentre[patchIndex],
          patchData.cellSize[patchIndex],
          patchIndex,
          gridCellIndex3d(x,y,z),
          patchData.t[patchIndex],
          timeStepSize,
          0, // normal
          tempDifferentialSourceX,
          QOutEnumerator
        );
        otterLoopIterationEnd();
      }
      otterLoopEnd();

      otterLoopBegin();
      for (int x = 0; x < numberOfGridCellsPerPatchPerAxis; x++)
      for (int y = 0; y < numberOfGridCellsPerPatchPerAxis; y++)
      for (int z = 0; z < numberOfGridCellsPerPatchPerAxis; z++) {
        otterLoopIterationBegin();
        internal::computeDifferentialSourceTerm_LoopBody(
          patchData.QIn[patchIndex],
          QInEnumerator,
          DifferentialSource,
          patchData.cellCentre[patchIndex],
          patchData.cellSize[patchIndex],
          patchIndex,
          gridCellIndex3d(x,y,z),
          patchData.t[patchIndex],
          timeStepSize,
          1, // normal
          tempDifferentialSourceY,
          QOutEnumerator
        );
        otterLoopIterationEnd();
      }
      otterLoopEnd();

      otterLoopBegin();
      for (int x = 0; x < numberOfGridCellsPerPatchPerAxis; x++)
      for (int y = 0; y < numberOfGridCellsPerPatchPerAxis; y++)
      for (int z = 0; z < numberOfGridCellsPerPatchPerAxis; z++) {
        otterLoopIterationBegin();
        internal::computeDifferentialSourceTerm_LoopBody(
          patchData.QIn[patchIndex],
          QInEnumerator,
          DifferentialSource,
          patchData.cellCentre[patchIndex],
          patchData.cellSize[patchIndex],
          patchIndex,
          gridCellIndex3d(x,y,z),
          patchData.t[patchIndex],
          timeStepSize,
          2, // normal
          tempDifferentialSourceZ,
          QOutEnumerator
        );
        otterLoopIterationEnd();
      }
      otterLoopEnd();

      otterLoopBegin();
      for (int x = 0; x < numberOfGridCellsPerPatchPerAxis; x++)
      for (int y = 0; y < numberOfGridCellsPerPatchPerAxis; y++)
      for (int z = 0; z < numberOfGridCellsPerPatchPerAxis; z++)
      for (int unknown=0; unknown<unknowns; unknown++) {
        otterLoopIterationBegin();
        internal::updateSolutionWithDifferentialSourceTerm_LoopBody(
          tempDifferentialSourceX, tempDifferentialSourceY, tempDifferentialSourceZ,
          QOutEnumerator,
          patchData.cellCentre[patchIndex],
          patchData.cellSize[patchIndex],
          patchIndex,
          gridCellIndex3d(x,y,z),
          unknown,
          timeStepSize,
          *(patchData.QOut + patchIndex),
          QOutEnumerator
        );
        otterLoopIterationEnd();
      }
      otterLoopEnd();
    }

    // STEP 4: compute the Kreiss Oliger dissipation and add it to the solution
    if (evaluateKODissipation and Dimensions==2) {
      otterLoopBegin();
      for (int x = 0; x < numberOfGridCellsPerPatchPerAxis; x++)
      for (int y = 0; y < numberOfGridCellsPerPatchPerAxis; y++) {
        otterLoopIterationBegin();
        internal::computeKreissOligerDissipationTerm_LoopBody(
          patchData.QIn[patchIndex],
          QInEnumerator,
          patchData.cellCentre[patchIndex],
          patchData.cellSize[patchIndex],
          patchIndex,
          gridCellIndex2d(x,y),
          patchData.t[patchIndex],
          timeStepSize,
          0, // normal
          tempKODissipationX,
          QOutEnumerator
        );
        otterLoopIterationEnd();
      }
      otterLoopEnd();

      otterLoopBegin();
      for (int x = 0; x < numberOfGridCellsPerPatchPerAxis; x++)
      for (int y = 0; y < numberOfGridCellsPerPatchPerAxis; y++) {
        otterLoopIterationBegin();
        internal::computeKreissOligerDissipationTerm_LoopBody(
          patchData.QIn[patchIndex],
          QInEnumerator,
          patchData.cellCentre[patchIndex],
          patchData.cellSize[patchIndex],
          patchIndex,
          gridCellIndex2d(x,y),
          patchData.t[patchIndex],
          timeStepSize,
          1, // normal
          tempKODissipationY,
          QOutEnumerator
        );
        otterLoopIterationEnd();
      }
      otterLoopEnd();

      otterLoopBegin();
      for (int x = 0; x < numberOfGridCellsPerPatchPerAxis; x++)
      for (int y = 0; y < numberOfGridCellsPerPatchPerAxis; y++)
      for (int unknown=0; unknown<unknowns; unknown++) {
        otterLoopIterationBegin();
        internal::updateSolutionWithKODissipationTerm_LoopBody(
          KOSigma, tempKODissipationX, tempKODissipationY, tempKODissipationZ,
          QOutEnumerator,
          patchData.cellCentre[patchIndex],
          patchData.cellSize[patchIndex],
          patchIndex,
          gridCellIndex2d(x,y),
          unknown,
          timeStepSize,
          *(patchData.QOut + patchIndex),
          QOutEnumerator
        );
        otterLoopIterationEnd();
      }
      otterLoopEnd();
    }
    else if (evaluateKODissipation and Dimensions==3) {
        otterLoopBegin();
        for (int x = 0; x < numberOfGridCellsPerPatchPerAxis; x++)
        for (int y = 0; y < numberOfGridCellsPerPatchPerAxis; y++)
        for (int z = 0; z < numberOfGridCellsPerPatchPerAxis; z++) {
          otterLoopIterationBegin();
          internal::computeKreissOligerDissipationTerm_LoopBody(
            patchData.QIn[patchIndex],
            QInEnumerator,
            patchData.cellCentre[patchIndex],
            patchData.cellSize[patchIndex],
            patchIndex,
            gridCellIndex3d(x,y,z),
            patchData.t[patchIndex],
            timeStepSize,
            0, // normal
            tempKODissipationX,
            QOutEnumerator
          );
          otterLoopIterationEnd();
        }
        otterLoopEnd();

        otterLoopBegin();
        for (int x = 0; x < numberOfGridCellsPerPatchPerAxis; x++)
        for (int y = 0; y < numberOfGridCellsPerPatchPerAxis; y++)
        for (int z = 0; z < numberOfGridCellsPerPatchPerAxis; z++) {
          otterLoopIterationBegin();
          internal::computeKreissOligerDissipationTerm_LoopBody(
            patchData.QIn[patchIndex],
            QInEnumerator,
            patchData.cellCentre[patchIndex],
            patchData.cellSize[patchIndex],
            patchIndex,
            gridCellIndex3d(x,y,z),
            patchData.t[patchIndex],
            timeStepSize,
            1, // normal
            tempKODissipationY,
            QOutEnumerator
          );
          otterLoopIterationEnd();
        }
        otterLoopEnd();

        otterLoopBegin();
        for (int x = 0; x < numberOfGridCellsPerPatchPerAxis; x++)
        for (int y = 0; y < numberOfGridCellsPerPatchPerAxis; y++)
        for (int z = 0; z < numberOfGridCellsPerPatchPerAxis; z++) {
          otterLoopIterationBegin();
          internal::computeKreissOligerDissipationTerm_LoopBody(
            patchData.QIn[patchIndex],
            QInEnumerator,
            patchData.cellCentre[patchIndex],
            patchData.cellSize[patchIndex],
            patchIndex,
            gridCellIndex3d(x,y,z),
            patchData.t[patchIndex],
            timeStepSize,
            2, // normal
            tempKODissipationZ,
            QOutEnumerator
          );
          otterLoopIterationEnd();
        }
        otterLoopEnd();

        otterLoopBegin();
        for (int x = 0; x < numberOfGridCellsPerPatchPerAxis; x++)
        for (int y = 0; y < numberOfGridCellsPerPatchPerAxis; y++)
        for (int z = 0; z < numberOfGridCellsPerPatchPerAxis; z++)
        for (int unknown=0; unknown<unknowns; unknown++) {
          otterLoopIterationBegin();
          internal::updateSolutionWithKODissipationTerm_LoopBody(
            KOSigma, tempKODissipationX, tempKODissipationY, tempKODissipationZ,
            QOutEnumerator,
            patchData.cellCentre[patchIndex],
            patchData.cellSize[patchIndex],
            patchIndex,
            gridCellIndex3d(x,y,z),
            unknown,
            timeStepSize,
            *(patchData.QOut + patchIndex),
            QOutEnumerator
          );
          otterLoopIterationEnd();
        }
        otterLoopEnd();
    }
  }

  if (tempDifferentialSourceX!=nullptr) tarch::freeMemory(tempDifferentialSourceX, tarch::MemoryLocation::Heap);
  if (tempDifferentialSourceY!=nullptr) tarch::freeMemory(tempDifferentialSourceY, tarch::MemoryLocation::Heap);
  if (tempDifferentialSourceZ!=nullptr) tarch::freeMemory(tempDifferentialSourceZ, tarch::MemoryLocation::Heap);
  if (tempKODissipationX!=nullptr)      tarch::freeMemory(tempKODissipationX,      tarch::MemoryLocation::Heap);
  if (tempKODissipationY!=nullptr)      tarch::freeMemory(tempKODissipationY,      tarch::MemoryLocation::Heap);
  if (tempKODissipationZ!=nullptr)      tarch::freeMemory(tempKODissipationZ,      tarch::MemoryLocation::Heap);

}
