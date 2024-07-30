// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org

namespace exahype2::fv::rusanov::internal {
  template <
    int  NumberOfVolumesPerAxisInPatch,
    int  HaloSize,
    int  NumberOfUnknowns,
    int  NumberOfAuxiliaryVariables,
    bool EvaluateFlux,
    bool EvaluateNonconservativeProduct,
    bool EvaluateSource,
    bool EvaluateMaximumEigenvalueAfterTimeStep,
    class TempDataEnumeratorType>
  KeywordToAvoidDuplicateSymbolsForInlinedFunctions void timeStepWithRusanovPatchwiseFunctors(
    CellData&                                      patchData,
    const FluxFunctor&                             fluxFunctor,
    const NonconservativeProductFunctor&           nonconservativeProductFunctor,
    const SourceFunctor&                           sourceFunctor,
    const MaxEigenvalueFunctor&                    maxEigenvalueFunctor,
    peano4::utils::LoopPlacement                 loopParallelism,
    const enumerator::AoSLexicographicEnumerator& QInEnumerator,
    const enumerator::AoSLexicographicEnumerator& QOutEnumerator,
    const TempDataEnumeratorType&                  fluxEnumerator,
    const TempDataEnumeratorType&                  ncpEnumerator,
    const TempDataEnumeratorType&                  eigenvalueEnumerator,
    double*                                        tempFluxX,
    double*                                        tempFluxY,
    double*                                        tempFluxZ,
    double*                                        tempNonconservativeProductX,
    double*                                        tempNonconservativeProductY,
    double*                                        tempNonconservativeProductZ,
    double*                                        tempEigenvalueX,
    double*                                        tempEigenvalueY,
    double*                                        tempEigenvalueZ
  ) InlineMethod {
    static_assert(HaloSize == 1);

    static tarch::logging::Log _log("exahype2::fv::rusanov");
    logTraceIn("timeStepWithRusanovPatchwiseFunctors()");

    parallelForWithSchedulerInstructions(patchIndex, patchData.numberOfCells, loopParallelism) {
      // ====================================================
      // Copy solution over and evaluate source (if required)
      // ====================================================
      if constexpr (EvaluateSource) {
        parallelDforWithSchedulerInstructions(volume, NumberOfVolumesPerAxisInPatch, loopParallelism) {
          loopbodies::copySolutionAndAddSourceTerm<NumberOfUnknowns, NumberOfAuxiliaryVariables>(
            patchData.QIn[patchIndex],
            QInEnumerator,
            sourceFunctor,
            patchData.cellCentre[patchIndex],
            patchData.cellSize[patchIndex],
            patchIndex,
            volume,
            patchData.t[patchIndex],
            patchData.dt[patchIndex],
            patchData.QOut[patchIndex],
            QOutEnumerator
          );
        }
        endParallelDfor
      } else {
        parallelDPlusOneForWithSchedulerInstructions(
          volume, internal::rangeOverVolumesTimesUnknownsPlusAuxiliaryVariables(NumberOfVolumesPerAxisInPatch, NumberOfUnknowns, NumberOfAuxiliaryVariables), loopParallelism
        ) {
          loopbodies::copySolution(
            patchData.QIn[patchIndex],
            QInEnumerator,
            patchIndex,
            tarch::la::slice<Dimensions>(volume, 0), // first Dimensions entries
            volume(Dimensions),                      // unknown
            patchData.QOut[patchIndex],
            QOutEnumerator
          );
        }
        endParallelDfor
      }

      // ====================================================
      // Compute damping due to max eigenvalue
      // ====================================================
      parallelDforWithSchedulerInstructions(volume, internal::rangeOverVolumesPlusHaloInXDirection(NumberOfVolumesPerAxisInPatch, HaloSize, true), loopParallelism) {
        volume(0) -= HaloSize;
        loopbodies::computeMaxEigenvalue<NumberOfUnknowns, NumberOfAuxiliaryVariables>(
          patchData.QIn[patchIndex],
          QInEnumerator,
          maxEigenvalueFunctor,
          patchData.cellCentre[patchIndex],
          patchData.cellSize[patchIndex],
          patchIndex,
          volume,
          patchData.t[patchIndex],
          patchData.dt[patchIndex],
          0,
          tempEigenvalueX,
          eigenvalueEnumerator
        );
      }
      endParallelDfor

      parallelDforWithSchedulerInstructions(volume, internal::rangeOverVolumesPlusHaloInYDirection(NumberOfVolumesPerAxisInPatch, HaloSize, true), loopParallelism) {
        volume(1) -= HaloSize;
        loopbodies::computeMaxEigenvalue<NumberOfUnknowns, NumberOfAuxiliaryVariables>(
          patchData.QIn[patchIndex],
          QInEnumerator,
          maxEigenvalueFunctor,
          patchData.cellCentre[patchIndex],
          patchData.cellSize[patchIndex],
          patchIndex,
          volume,
          patchData.t[patchIndex],
          patchData.dt[patchIndex],
          1,
          tempEigenvalueY,
          eigenvalueEnumerator
        );
      }
      endParallelDfor

#if Dimensions == 3
      parallelDforWithSchedulerInstructions(volume, internal::rangeOverVolumesPlusHaloInZDirection(NumberOfVolumesPerAxisInPatch, HaloSize, true), loopParallelism) {
        volume(2) -= HaloSize;
        loopbodies::computeMaxEigenvalue<NumberOfUnknowns, NumberOfAuxiliaryVariables>(
          patchData.QIn[patchIndex],
          QInEnumerator,
          maxEigenvalueFunctor,
          patchData.cellCentre[patchIndex],
          patchData.cellSize[patchIndex],
          patchIndex,
          volume,
          patchData.t[patchIndex],
          patchData.dt[patchIndex],
          2,
          tempEigenvalueZ,
          eigenvalueEnumerator
        );
      }
      endParallelDfor
#endif

      parallelDPlusOneForWithSchedulerInstructions(volume, internal::rangeOverVolumesTimesUnknowns(NumberOfVolumesPerAxisInPatch, NumberOfUnknowns), loopParallelism) {
        loopbodies::updateSolutionWithEigenvalueDamping(
          patchData.QIn[patchIndex],
          QInEnumerator,
          tempEigenvalueX,
          tempEigenvalueY,
          tempEigenvalueZ,
          eigenvalueEnumerator,
          patchData.cellCentre[patchIndex],
          patchData.cellSize[patchIndex],
          patchIndex,
          tarch::la::slice<Dimensions>(volume, 0),
          volume(Dimensions), // unknown
          patchData.dt[patchIndex],
          patchData.QOut[patchIndex],
          QOutEnumerator
        );
      }
      endParallelDfor

        // ====================================================
        // Normal (conservative) flux
        // ====================================================
        if constexpr (EvaluateFlux) {
        parallelDforWithSchedulerInstructions(volume, internal::rangeOverVolumesPlusHaloInXDirection(NumberOfVolumesPerAxisInPatch, HaloSize, true), loopParallelism) {
          volume(0) -= HaloSize;
          loopbodies::computeFlux<NumberOfUnknowns, NumberOfAuxiliaryVariables>(
            patchData.QIn[patchIndex],
            QInEnumerator,
            fluxFunctor,
            patchData.cellCentre[patchIndex],
            patchData.cellSize[patchIndex],
            patchIndex,
            volume,
            patchData.t[patchIndex],
            patchData.dt[patchIndex],
            0, // normal
            tempFluxX,
            fluxEnumerator
          );
        }
        endParallelDfor

          parallelDforWithSchedulerInstructions(volume, internal::rangeOverVolumesPlusHaloInYDirection(NumberOfVolumesPerAxisInPatch, HaloSize, true), loopParallelism) {
          volume(1) -= HaloSize;
          loopbodies::computeFlux<NumberOfUnknowns, NumberOfAuxiliaryVariables>(
            patchData.QIn[patchIndex],
            QInEnumerator,
            fluxFunctor,
            patchData.cellCentre[patchIndex],
            patchData.cellSize[patchIndex],
            patchIndex,
            volume,
            patchData.t[patchIndex],
            patchData.dt[patchIndex],
            1, // normal
            tempFluxY,
            fluxEnumerator
          );
        }
        endParallelDfor

#if Dimensions == 3
        parallelDforWithSchedulerInstructions(volume, internal::rangeOverVolumesPlusHaloInZDirection(NumberOfVolumesPerAxisInPatch, HaloSize, true), loopParallelism) {
          volume(2) -= HaloSize;
          loopbodies::computeFlux<NumberOfUnknowns, NumberOfAuxiliaryVariables>(
            patchData.QIn[patchIndex],
            QInEnumerator,
            fluxFunctor,
            patchData.cellCentre[patchIndex],
            patchData.cellSize[patchIndex],
            patchIndex,
            volume,
            patchData.t[patchIndex],
            patchData.dt[patchIndex],
            2, // normal
            tempFluxZ,
            fluxEnumerator
          );
        }
        endParallelDfor
#endif

        parallelDPlusOneForWithSchedulerInstructions(volume, internal::rangeOverVolumesTimesUnknowns(NumberOfVolumesPerAxisInPatch, NumberOfUnknowns), loopParallelism) {
          loopbodies::updateSolutionWithFlux(
            tempFluxX,
            tempFluxY,
            tempFluxZ,
            fluxEnumerator,
            patchData.cellCentre[patchIndex],
            patchData.cellSize[patchIndex],
            patchIndex,
            tarch::la::slice<Dimensions>(volume, 0),
            volume(Dimensions),
            patchData.dt[patchIndex],
            patchData.QOut[patchIndex],
            QOutEnumerator
          );
        }
        endParallelDfor
      }

      if constexpr (EvaluateNonconservativeProduct) {
        parallelDforWithSchedulerInstructions(volume, internal::rangeOverVolumesPlusHaloInXDirection(NumberOfVolumesPerAxisInPatch, HaloSize, false), loopParallelism) {
          volume(0) -= HaloSize;
          loopbodies::computeNonconservativeFlux<NumberOfUnknowns, NumberOfAuxiliaryVariables>(
            patchData.QIn[patchIndex],
            QInEnumerator,
            nonconservativeProductFunctor,
            patchData.cellCentre[patchIndex],
            patchData.cellSize[patchIndex],
            patchIndex,
            volume,
            patchData.t[patchIndex],
            patchData.dt[patchIndex],
            0, // normal
            tempNonconservativeProductX,
            ncpEnumerator
          );
        }
        endParallelDfor

        parallelDforWithSchedulerInstructions(volume, internal::rangeOverVolumesPlusHaloInYDirection(NumberOfVolumesPerAxisInPatch, HaloSize, false), loopParallelism) {
          volume(1) -= HaloSize;
          loopbodies::computeNonconservativeFlux<NumberOfUnknowns, NumberOfAuxiliaryVariables>(
            patchData.QIn[patchIndex],
            QInEnumerator,
            nonconservativeProductFunctor,
            patchData.cellCentre[patchIndex],
            patchData.cellSize[patchIndex],
            patchIndex,
            volume,
            patchData.t[patchIndex],
            patchData.dt[patchIndex],
            1, // normal
            tempNonconservativeProductY,
            ncpEnumerator
          );
        }
        endParallelDfor

#if Dimensions == 3
        parallelDforWithSchedulerInstructions(volume, internal::rangeOverVolumesPlusHaloInZDirection(NumberOfVolumesPerAxisInPatch, HaloSize, false), loopParallelism) {
          volume(2) -= HaloSize;
          loopbodies::computeNonconservativeFlux<NumberOfUnknowns, NumberOfAuxiliaryVariables>(
            patchData.QIn[patchIndex],
            QInEnumerator,
            nonconservativeProductFunctor,
            patchData.cellCentre[patchIndex],
            patchData.cellSize[patchIndex],
            patchIndex,
            volume,
            patchData.t[patchIndex],
            patchData.dt[patchIndex],
            2, // normal
            tempNonconservativeProductZ,
            ncpEnumerator
          );
        }
        endParallelDfor
#endif

        parallelDPlusOneForWithSchedulerInstructions(volume, internal::rangeOverVolumesTimesUnknowns(NumberOfVolumesPerAxisInPatch, NumberOfUnknowns), loopParallelism) {
          loopbodies::updateSolutionWithNonconservativeFlux(
            tempNonconservativeProductX,
            tempNonconservativeProductY,
            tempNonconservativeProductZ,
            ncpEnumerator,
            patchData.cellCentre[patchIndex],
            patchData.cellSize[patchIndex],
            patchIndex,
            tarch::la::slice<Dimensions>(volume, 0),
            volume(Dimensions),
            patchData.dt[patchIndex],
            patchData.QOut[patchIndex],
            QOutEnumerator
          );
        }
        endParallelDfor
      }

      if constexpr (EvaluateMaximumEigenvalueAfterTimeStep) {
        double newMaxEigenvalue = 0.0;
        dfor(volume, NumberOfVolumesPerAxisInPatch) {
          newMaxEigenvalue = std::max(
            newMaxEigenvalue,
            loopbodies::reduceMaxEigenvalue<NumberOfUnknowns, NumberOfAuxiliaryVariables>(
              patchData.QOut[patchIndex],
              QOutEnumerator,
              maxEigenvalueFunctor,
              patchData.cellCentre[patchIndex],
              patchData.cellSize[patchIndex],
              patchIndex,
              volume,
              patchData.t[patchIndex],
              patchData.dt[patchIndex]
            )
          );
        }
        patchData.maxEigenvalue[patchIndex] = newMaxEigenvalue;
      }
    }
    endParallelFor

      logTraceOut("timeStepWithRusanovPatchwiseFunctors()");
  }
} // namespace exahype2::fv::rusanov::internal


template <
  int  NumberOfVolumesPerAxisInPatch,
  int  HaloSize,
  int  NumberOfUnknowns,
  int  NumberOfAuxiliaryVariables,
  bool EvaluateFlux,
  bool EvaluateNonconservativeProduct,
  bool EvaluateSource,
  bool EvaluateMaximumEigenvalueAfterTimeStep,
  class TempDataEnumeratorType>
void exahype2::fv::rusanov::timeStepWithRusanovPatchwiseHeapFunctors(
  CellData&                            patchData,
  const FluxFunctor&                   fluxFunctor,
  const NonconservativeProductFunctor& nonconservativeProductFunctor,
  const SourceFunctor&                 sourceFunctor,
  const MaxEigenvalueFunctor&          maxEigenvalueFunctor,
  tarch::timing::Measurement&          measurement,
  peano4::utils::LoopPlacement       loopParallelism
) {
  static tarch::logging::Log _log("exahype2::fv::rusanov");
  logTraceIn("timeStepWithRusanovPatchwiseHeapFunctors()");

  const enumerator::AoSLexicographicEnumerator QInEnumerator(1, NumberOfVolumesPerAxisInPatch, HaloSize, NumberOfUnknowns, NumberOfAuxiliaryVariables);
  const enumerator::AoSLexicographicEnumerator QOutEnumerator(1, NumberOfVolumesPerAxisInPatch, 0, NumberOfUnknowns, NumberOfAuxiliaryVariables);
  const TempDataEnumeratorType                  fluxEnumerator(patchData.numberOfCells, NumberOfVolumesPerAxisInPatch, HaloSize, NumberOfUnknowns, 0);
  const TempDataEnumeratorType                  ncpEnumerator(patchData.numberOfCells, NumberOfVolumesPerAxisInPatch + 1, HaloSize, NumberOfUnknowns, 0);
  const TempDataEnumeratorType                  eigenvalueEnumerator(patchData.numberOfCells, NumberOfVolumesPerAxisInPatch, HaloSize, 1, 0);

  double* tempFluxX                   = EvaluateFlux ? tarch::allocateMemory<double>(fluxEnumerator.size(), tarch::MemoryLocation::Heap) : nullptr;
  double* tempFluxY                   = EvaluateFlux ? tarch::allocateMemory<double>(fluxEnumerator.size(), tarch::MemoryLocation::Heap) : nullptr;
  double* tempFluxZ                   = (EvaluateFlux and Dimensions == 3) ? tarch::allocateMemory<double>(fluxEnumerator.size(), tarch::MemoryLocation::Heap) : nullptr;
  double* tempNonconservativeProductX = EvaluateNonconservativeProduct ? tarch::allocateMemory<double>(ncpEnumerator.size(), tarch::MemoryLocation::Heap) : nullptr;
  double* tempNonconservativeProductY = EvaluateNonconservativeProduct ? tarch::allocateMemory<double>(ncpEnumerator.size(), tarch::MemoryLocation::Heap) : nullptr;
  double* tempNonconservativeProductZ = (EvaluateNonconservativeProduct and Dimensions == 3)
                                          ? tarch::allocateMemory<double>(ncpEnumerator.size(), tarch::MemoryLocation::Heap)
                                          : nullptr;
  double* tempEigenvalueX             = tarch::allocateMemory<double>(eigenvalueEnumerator.size(), tarch::MemoryLocation::Heap);
  double* tempEigenvalueY             = tarch::allocateMemory<double>(eigenvalueEnumerator.size(), tarch::MemoryLocation::Heap);
  double* tempEigenvalueZ             = (Dimensions == 3) ? tarch::allocateMemory<double>(eigenvalueEnumerator.size(), tarch::MemoryLocation::Heap) : nullptr;

  tarch::timing::Watch watch("exahype2::fv::rusanov", "timeStepWithRusanovPatchwiseHeapFunctors", false, true);
  exahype2::fv::rusanov::internal::timeStepWithRusanovPatchwiseFunctors<
    NumberOfVolumesPerAxisInPatch,
    HaloSize,
    NumberOfUnknowns,
    NumberOfAuxiliaryVariables,
    EvaluateFlux,
    EvaluateNonconservativeProduct,
    EvaluateSource,
    EvaluateMaximumEigenvalueAfterTimeStep,
    TempDataEnumeratorType>(
    patchData,
    fluxFunctor,
    nonconservativeProductFunctor,
    sourceFunctor,
    maxEigenvalueFunctor,
    loopParallelism,
    QInEnumerator,
    QOutEnumerator,
    fluxEnumerator,
    ncpEnumerator,
    eigenvalueEnumerator,
    tempFluxX,
    tempFluxY,
    tempFluxZ,
    tempNonconservativeProductX,
    tempNonconservativeProductY,
    tempNonconservativeProductZ,
    tempEigenvalueX,
    tempEigenvalueY,
    tempEigenvalueZ
  );
  watch.stop();
  measurement.setValue(watch.getCalendarTime());

  if (tempFluxX != nullptr) {
    tarch::freeMemory(tempFluxX, tarch::MemoryLocation::Heap);
  }
  if (tempFluxY != nullptr) {
    tarch::freeMemory(tempFluxY, tarch::MemoryLocation::Heap);
  }
  if (tempFluxZ != nullptr) {
    tarch::freeMemory(tempFluxZ, tarch::MemoryLocation::Heap);
  }
  if (tempNonconservativeProductX != nullptr) {
    tarch::freeMemory(tempNonconservativeProductX, tarch::MemoryLocation::Heap);
  }
  if (tempNonconservativeProductY != nullptr) {
    tarch::freeMemory(tempNonconservativeProductY, tarch::MemoryLocation::Heap);
  }
  if (tempNonconservativeProductZ != nullptr) {
    tarch::freeMemory(tempNonconservativeProductZ, tarch::MemoryLocation::Heap);
  }
  if (tempEigenvalueX != nullptr) {
    tarch::freeMemory(tempEigenvalueX, tarch::MemoryLocation::Heap);
  }
  if (tempEigenvalueY != nullptr) {
    tarch::freeMemory(tempEigenvalueY, tarch::MemoryLocation::Heap);
  }
  if (tempEigenvalueZ != nullptr) {
    tarch::freeMemory(tempEigenvalueZ, tarch::MemoryLocation::Heap);
  }

  logTraceOut("timeStepWithRusanovPatchwiseHeapFunctors()");
}


template <
  int  NumberOfVolumesPerAxisInPatch,
  int  HaloSize,
  int  NumberOfUnknowns,
  int  NumberOfAuxiliaryVariables,
  bool EvaluateFlux,
  bool EvaluateNonconservativeProduct,
  bool EvaluateSource,
  bool EvaluateMaximumEigenvalueAfterTimeStep,
  class TempDataEnumeratorType>
void exahype2::fv::rusanov::timeStepWithRusanovPatchwiseHeapFunctors(
  CellData&                            patchData,
  const FluxFunctor&                   fluxFunctor,
  const NonconservativeProductFunctor& nonconservativeProductFunctor,
  const SourceFunctor&                 sourceFunctor,
  const MaxEigenvalueFunctor&          maxEigenvalueFunctor,
  peano4::utils::LoopPlacement       loopParallelism
) {
  static tarch::logging::Log _log("exahype2::fv::rusanov");
  logTraceIn("timeStepWithRusanovPatchwiseHeapFunctors()");

  const enumerator::AoSLexicographicEnumerator QInEnumerator(1, NumberOfVolumesPerAxisInPatch, HaloSize, NumberOfUnknowns, NumberOfAuxiliaryVariables);
  const enumerator::AoSLexicographicEnumerator QOutEnumerator(1, NumberOfVolumesPerAxisInPatch, 0, NumberOfUnknowns, NumberOfAuxiliaryVariables);
  const TempDataEnumeratorType                  fluxEnumerator(patchData.numberOfCells, NumberOfVolumesPerAxisInPatch, HaloSize, NumberOfUnknowns, 0);
  const TempDataEnumeratorType                  ncpEnumerator(patchData.numberOfCells, NumberOfVolumesPerAxisInPatch + 1, HaloSize, NumberOfUnknowns, 0);
  const TempDataEnumeratorType                  eigenvalueEnumerator(patchData.numberOfCells, NumberOfVolumesPerAxisInPatch, HaloSize, 1, 0);

  double* tempFluxX                   = EvaluateFlux ? tarch::allocateMemory<double>(fluxEnumerator.size(), tarch::MemoryLocation::Heap) : nullptr;
  double* tempFluxY                   = EvaluateFlux ? tarch::allocateMemory<double>(fluxEnumerator.size(), tarch::MemoryLocation::Heap) : nullptr;
  double* tempFluxZ                   = (EvaluateFlux and Dimensions == 3) ? tarch::allocateMemory<double>(fluxEnumerator.size(), tarch::MemoryLocation::Heap) : nullptr;
  double* tempNonconservativeProductX = EvaluateNonconservativeProduct ? tarch::allocateMemory<double>(ncpEnumerator.size(), tarch::MemoryLocation::Heap) : nullptr;
  double* tempNonconservativeProductY = EvaluateNonconservativeProduct ? tarch::allocateMemory<double>(ncpEnumerator.size(), tarch::MemoryLocation::Heap) : nullptr;
  double* tempNonconservativeProductZ = (EvaluateNonconservativeProduct and Dimensions == 3)
                                          ? tarch::allocateMemory<double>(ncpEnumerator.size(), tarch::MemoryLocation::Heap)
                                          : nullptr;
  double* tempEigenvalueX             = tarch::allocateMemory<double>(eigenvalueEnumerator.size(), tarch::MemoryLocation::Heap);
  double* tempEigenvalueY             = tarch::allocateMemory<double>(eigenvalueEnumerator.size(), tarch::MemoryLocation::Heap);
  double* tempEigenvalueZ             = (Dimensions == 3) ? tarch::allocateMemory<double>(eigenvalueEnumerator.size(), tarch::MemoryLocation::Heap) : nullptr;

  exahype2::fv::rusanov::internal::timeStepWithRusanovPatchwiseFunctors<
    NumberOfVolumesPerAxisInPatch,
    HaloSize,
    NumberOfUnknowns,
    NumberOfAuxiliaryVariables,
    EvaluateFlux,
    EvaluateNonconservativeProduct,
    EvaluateSource,
    EvaluateMaximumEigenvalueAfterTimeStep,
    TempDataEnumeratorType>(
    patchData,
    fluxFunctor,
    nonconservativeProductFunctor,
    sourceFunctor,
    maxEigenvalueFunctor,
    loopParallelism,
    QInEnumerator,
    QOutEnumerator,
    fluxEnumerator,
    ncpEnumerator,
    eigenvalueEnumerator,
    tempFluxX,
    tempFluxY,
    tempFluxZ,
    tempNonconservativeProductX,
    tempNonconservativeProductY,
    tempNonconservativeProductZ,
    tempEigenvalueX,
    tempEigenvalueY,
    tempEigenvalueZ
  );

  if (tempFluxX != nullptr) {
    tarch::freeMemory(tempFluxX, tarch::MemoryLocation::Heap);
  }
  if (tempFluxY != nullptr) {
    tarch::freeMemory(tempFluxY, tarch::MemoryLocation::Heap);
  }
  if (tempFluxZ != nullptr) {
    tarch::freeMemory(tempFluxZ, tarch::MemoryLocation::Heap);
  }
  if (tempNonconservativeProductX != nullptr) {
    tarch::freeMemory(tempNonconservativeProductX, tarch::MemoryLocation::Heap);
  }
  if (tempNonconservativeProductY != nullptr) {
    tarch::freeMemory(tempNonconservativeProductY, tarch::MemoryLocation::Heap);
  }
  if (tempNonconservativeProductZ != nullptr) {
    tarch::freeMemory(tempNonconservativeProductZ, tarch::MemoryLocation::Heap);
  }
  if (tempEigenvalueX != nullptr) {
    tarch::freeMemory(tempEigenvalueX, tarch::MemoryLocation::Heap);
  }
  if (tempEigenvalueY != nullptr) {
    tarch::freeMemory(tempEigenvalueY, tarch::MemoryLocation::Heap);
  }
  if (tempEigenvalueZ != nullptr) {
    tarch::freeMemory(tempEigenvalueZ, tarch::MemoryLocation::Heap);
  }

  logTraceOut("timeStepWithRusanovPatchwiseHeapFunctors()");
}
