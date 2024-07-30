// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "exahype2/CellData.h"
#include "exahype2/VolumeIndex.h"
#include "Functors.h"
#include "LoopBodies.h"
#include "peano4/utils/Loop.h"

namespace exahype2::fv::riemann {
  template <
    int  NumberOfVolumesPerAxisInPatch,
    int  HaloSize,
    int  NumberOfUnknowns,
    int  NumberOfAuxiliaryVariables,
    bool EvaluateFlux,
    bool EvaluateNonconservativeProduct,
    bool EvaluateSource,
    bool EvaluateRiemann,
    bool EvaluateMaximumEigenvalueAfterTimeStep,
    class TempDataEnumeratorType>
  KeywordToAvoidDuplicateSymbolsForInlinedFunctions void
    timeStepWithRiemannPatchwiseHeapFunctors(
      CellData&                            patchData,
      const FluxFunctor&                   fluxFunctor,
      const NonconservativeProductFunctor& nonconservativeProductFunctor,
      const SourceFunctor&                 sourceFunctor,
      const EigenvaluesFunctor&            eigenvaluesFunctor,
      const RiemannFunctor&                riemannFunctor,
      peano4::utils::LoopPlacement         loopPlacement = peano4::utils::
        LoopPlacement::Serial
    ) InlineMethod;


  template <
    class SolverType,
    int  NumberOfVolumesPerAxisInPatch,
    int  HaloSize,
    int  NumberOfUnknowns,
    int  NumberOfAuxiliaryVariables,
    bool EvaluateFlux,
    bool EvaluateNonconservativeProduct,
    bool EvaluateSource,
    bool EvaluateRiemann,
    bool EvaluateMaximumEigenvalueAfterTimeStep,
    class TempDataEnumeratorType>
  KeywordToAvoidDuplicateSymbolsForInlinedFunctions void
    timeStepWithRiemannPatchwiseHeapStateless(
      CellData&                    patchData,
      peano4::utils::LoopPlacement loopPlacement = peano4::utils::
        LoopPlacement::Serial
    ) InlineMethod;


  namespace internal {
    tarch::la::Vector<Dimensions + 1, int> rangeOverVolumesTimesUnknowns(
      int numberOfVolumesPerAxisInPatch,
      int unknowns
    );
    tarch::la::Vector<Dimensions + 1, int>
      rangeOverVolumesTimesUnknownsPlusAuxiliaryVariables(
        int numberOfVolumesPerAxisInPatch,
        int unknowns,
        int auxiliaryVariables
      );

    tarch::la::Vector<Dimensions + 1, int> rangeOverVolumesTimesPatches(
      int numberOfVolumesPerAxisInPatch,
      int patches
    );
    tarch::la::Vector<Dimensions + 2, int> rangeOverVolumesTimesUnknownsTimesPatches(
      int numberOfVolumesPerAxisInPatch,
      int unknowns,
      int patches
    );
    tarch::la::Vector<Dimensions + 2, int>
      rangeOverVolumesTimesUnknownsPlusAuxiliaryVariablesTimesPatches(
        int numberOfVolumesPerAxisInPatch,
        int unknowns,
        int auxiliaryVariables,
        int patches
      );

    /**
     * Construct iteration range
     *
     * If you have a 6x6x6 range and a halo of 3, then you get
     *
     * - (6+2*3)x6x6 if extendInBothDirections is true;
     * - (6+3)x6x6 if extendInBothDirections is false.
     */
    tarch::la::Vector<Dimensions, int> rangeOverVolumesPlusHaloInXDirection(
      int  numberOfVolumesPerAxisInPatch,
      int  haloSize,
      bool extendInBothDirections
    );
    tarch::la::Vector<Dimensions, int> rangeOverVolumesPlusHaloInYDirection(
      int  numberOfVolumesPerAxisInPatch,
      int  haloSize,
      bool extendInBothDirections
    );
    tarch::la::Vector<3, int> rangeOverVolumesPlusHaloInZDirection(
      int  numberOfVolumesPerAxisInPatch,
      int  haloSize,
      bool extendInBothDirections
    );

    tarch::la::Vector<Dimensions + 1, int>
      rangeOverVolumesTimesPatchesPlusHaloInXDirection(
        int numberOfVolumesPerAxisInPatch,
        int haloSize,
        int patches
      );
    tarch::la::Vector<Dimensions + 1, int>
      rangeOverVolumesTimesPatchesPlusHaloInYDirection(
        int numberOfVolumesPerAxisInPatch,
        int haloSize,
        int patches
      );
    tarch::la::Vector<3 + 1, int> rangeOverVolumesTimesPatchesPlusHaloInZDirection(
      int numberOfVolumesPerAxisInPatch,
      int haloSize,
      int patches
    );
  } // namespace internal
} // namespace exahype2::fv::riemann

#include "PatchwiseFunctors.cpph"
#include "PatchwiseStateless.cpph"
