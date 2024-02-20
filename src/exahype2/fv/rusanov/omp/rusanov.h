// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include <cstring>

#include "tarch/accelerator/omp/GPUMemoryManager.h"

namespace exahype2::fv::rusanov::omp {
  /**
   * The name patchwise is a little bit irritating in a GPU context.
   * What it means is that the code runs logically through the patches
   * one by one and issues all kernel on one patch before it continues
   * with the next one. Obviously, the patches are all independent of
   * each other. Therefore, we can deploy this sequence of patch
   * updates to a distributed for over the team.
   *
   * ## Memory management
   *
   * If you run this routine without GPU offloading, we place all
   * temporary data on the heap. If GPUs are to be used, then we create
   * pointers to the heap, but we map the data explicitly to GPU memory.
   * As we use explicit allocs and frees, the map clauses basically
   * replicate the heap behaviour on the accelerator.
   *
   * We assign the host pointers null, as the actual temporary data
   * is to be created on the GPU. Such a 'create a pointer which is a
   * nullptr' ensures that the code still compiles on the host even though
   * it is semantically broken as the temporary data is not allocated.
   *
   *
   * ## Patch data transfer
   *
   * All the data within the CellData object is trivially copyable besides the
   * actual patch sizes and positions. Trivially copyable means that they are
   * plain double arrays. That is, we can map them onto the device. The actual
   * input and output data is stored as pointer to pointers. Here, we have to
   * loop over the entries and copy each individual pointer over separately.
   *
   * The positions and sizes of the patches are in principle trivially copyable
   * as well, as they simply host arrays of known sizes. However, it is quite
   * cumbersome to tell OpenMP that this stuff can just be copied. It also might
   * become annoying the compiler decided to add padding bytes. So we copy those
   * guys manually into one large buffer of double values, and then we reconstruct
   * the objects on the device. In this particular context, the resulting overhead
   * is acceptable, as this meta data is not extraordinary large.
   *
   * If you copy the pointers to pointers from the host to the GPU, you have to
   * be careful. It makes no sense to copy the pointers within the pointer array
   * to the GPU, as these are host pointers. So first deploy the actual data to
   * the GPU per patch. Then we grab the GPU pointer address to which the memory
   * region has been mapped to. We store this data in mappedPointersToQIn. We
   * transfer mappedPointersToQIn to the GPU and then reconstruct there the
   * pointer-to-pointer map that we need within the kernels.
   *
   */
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
  KeywordToAvoidDuplicateSymbolsForInlinedFunctions void timeStepWithRusanovPatchwiseHeapStateless(int targetDevice, CellData& patchData, tarch::timing::Measurement& measurement)
    InlineMethod;


  /**
   * @see timeStepWithRusanovPatchwiseHeapStateless()
   */
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
  KeywordToAvoidDuplicateSymbolsForInlinedFunctions void timeStepWithRusanovPatchwiseHeapStateless(int targetDevice, CellData& patchData) InlineMethod;


  /**
   * @see timeStepWithRusanovPatchwiseHeapStateless()
   */
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
  KeywordToAvoidDuplicateSymbolsForInlinedFunctions void timeStepWithRusanovBatchedHeapStateless(int targetDevice, CellData& patchData, tarch::timing::Measurement& measurement)
    InlineMethod;


  /**
   * @see timeStepWithRusanovPatchwiseHeapStateless()
   */
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
  KeywordToAvoidDuplicateSymbolsForInlinedFunctions void timeStepWithRusanovBatchedHeapStateless(int targetDevice, CellData& patchData) InlineMethod;


  /**
   * @see timeStepWithRusanovPatchwiseHeapStateless()
   */
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
  KeywordToAvoidDuplicateSymbolsForInlinedFunctions void timeStepWithRusanovPatchwiseUSMStateless(int targetDevice, CellData& patchData, tarch::timing::Measurement& measurement)
    InlineMethod;


  /**
   * @see timeStepWithRusanovPatchwiseHeapStateless()
   */
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
  KeywordToAvoidDuplicateSymbolsForInlinedFunctions void timeStepWithRusanovPatchwiseUSMStateless(int targetDevice, CellData& patchData) InlineMethod;


  /**
   * @see timeStepWithRusanovPatchwiseHeapStateless()
   */
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
  KeywordToAvoidDuplicateSymbolsForInlinedFunctions void timeStepWithRusanovBatchedUSMStateless(int targetDevice, CellData& patchData, tarch::timing::Measurement& measurement)
    InlineMethod;


  /**
   * @see timeStepWithRusanovPatchwiseHeapStateless()
   */
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
  KeywordToAvoidDuplicateSymbolsForInlinedFunctions void timeStepWithRusanovBatchedUSMStateless(int targetDevice, CellData& patchData) InlineMethod;


  /**
   * @see timeStepWithRusanovPatchwiseHeapStateless()
   */
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
  KeywordToAvoidDuplicateSymbolsForInlinedFunctions void timeStepWithRusanovVolumewiseStateless(int targetDevice, CellData& patchData, tarch::timing::Measurement& measurement)
    InlineMethod;


  /**
   * @see timeStepWithRusanovPatchwiseHeapStateless()
   */
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
  KeywordToAvoidDuplicateSymbolsForInlinedFunctions void timeStepWithRusanovVolumewiseStateless(int targetDevice, CellData& patchData) InlineMethod;
} // namespace exahype2::fv::rusanov::omp

#include "BatchedStateless.cpph"
#include "PatchwiseStateless.cpph"
#include "VolumewiseStateless.cpph"
