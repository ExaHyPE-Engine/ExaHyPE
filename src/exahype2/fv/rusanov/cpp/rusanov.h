// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include <algorithm>
#include <execution>
#include <iterator>
#include <numeric>
#include <vector>

#include "tarch/accelerator/cpp/cartesian_product.h"

namespace exahype2::fv::rusanov::cpp {
  template <
    class SolverType,
    int  NumberOfVolumesPerAxisInPatch,
    int  HaloSize,
    int  NumberOfUnknowns,
    int  NumberOfAuxiliaryVariables,
    bool EvaluateFlux,
    bool EvaluateNonconservativeProduct,
    bool EvaluateSource,
    bool EvaluateMaximumEigenvalueAfterTimeStep,
    class TempDataEnumeratorType>
  KeywordToAvoidDuplicateSymbolsForInlinedFunctions void timeStepWithRusanovPatchwiseStateless(int targetDevice, CellData& patchData, tarch::timing::Measurement& measurement)
    InlineMethod;


  template <
    class SolverType,
    int  NumberOfVolumesPerAxisInPatch,
    int  HaloSize,
    int  NumberOfUnknowns,
    int  NumberOfAuxiliaryVariables,
    bool EvaluateFlux,
    bool EvaluateNonconservativeProduct,
    bool EvaluateSource,
    bool EvaluateMaximumEigenvalueAfterTimeStep,
    class TempDataEnumeratorType>
  KeywordToAvoidDuplicateSymbolsForInlinedFunctions void timeStepWithRusanovPatchwiseStateless(int targetDevice, CellData& patchData) InlineMethod;


  template <
    class SolverType,
    int  NumberOfVolumesPerAxisInPatch,
    int  HaloSize,
    int  NumberOfUnknowns,
    int  NumberOfAuxiliaryVariables,
    bool EvaluateFlux,
    bool EvaluateNonconservativeProduct,
    bool EvaluateSource,
    bool EvaluateMaximumEigenvalueAfterTimeStep,
    class TempDataEnumeratorType>
  KeywordToAvoidDuplicateSymbolsForInlinedFunctions void timeStepWithRusanovBatchedStateless(int targetDevice, CellData& patchData, tarch::timing::Measurement& measurement)
    InlineMethod;


  template <
    class SolverType,
    int  NumberOfVolumesPerAxisInPatch,
    int  HaloSize,
    int  NumberOfUnknowns,
    int  NumberOfAuxiliaryVariables,
    bool EvaluateFlux,
    bool EvaluateNonconservativeProduct,
    bool EvaluateSource,
    bool EvaluateMaximumEigenvalueAfterTimeStep,
    class TempDataEnumeratorType>
  KeywordToAvoidDuplicateSymbolsForInlinedFunctions void timeStepWithRusanovBatchedStateless(int targetDevice, CellData& patchData) InlineMethod;
} // namespace exahype2::fv::rusanov::cpp

#include "BatchedStateless.cpph"
#include "PatchwiseStateless.cpph"
