// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include <cstdlib>

#include "exahype2/CellData.h"
#include "exahype2/fv/rusanov/LoopBodies.h"
#include "GPUCellData.h"
#include "tarch/accelerator/accelerator.h"
#include "tarch/compiler/CompilerSpecificSettings.h"
#include "tarch/timing/Measurement.h"
#include "tarch/timing/Watch.h"

namespace exahype2::fv::rusanov::sycl {
  /**
   * All the memory management is discussed in the documentation of
   * GPUCellData.
   *
   * Task-graph implementation with copying
   *
   * Task-graph realisation of the kernel where the input data and result
   * are explicitly copied forth and back between GPU and CPU. This
   * routine employs GPUCopyCellData as a wrapper around the host data.
   *
   * After that, the call is forwarded to the corresponding internal
   * implementation.
   *
   * @see exahype2::fv::rusanov::sycl::internal
   */
  template <
    class SolverType,
    std::size_t NumberOfVolumesPerAxisInPatch,
    std::size_t HaloSize,
    std::size_t NumberOfUnknowns,
    std::size_t NumberOfAuxiliaryVariables,
    bool        EvaluateFlux,
    bool        EvaluateNonconservativeProduct,
    bool        EvaluateSource,
    bool        EvaluateMaximumEigenvalueAfterTimeStep,
    class TempDataEnumeratorType>
  KeywordToAvoidDuplicateSymbolsForInlinedFunctions void timeStepWithRusanovTaskgraphCopyStateless(int targetDevice, CellData& patchData) InlineMethod;


  /*
   * Batched realisation of the kernel
   *
   * Batched realisation of the kernel where the input data and result
   * are explicitly copied forth and back between GPU and CPU. This
   * routine employs GPUCopyCellData as a wrapper around the host data.
   *
   * After that, the call is forwarded to the corresponding internal
   * implementation.
   *
   * @see exahype2::fv::rusanov::sycl::internal
   *
   */
  template <
    class SolverType,
    std::size_t NumberOfVolumesPerAxisInPatch,
    std::size_t HaloSize,
    std::size_t NumberOfUnknowns,
    std::size_t NumberOfAuxiliaryVariables,
    bool        EvaluateFlux,
    bool        EvaluateNonconservativeProduct,
    bool        EvaluateSource,
    bool        EvaluateMaximumEigenvalueAfterTimeStep,
    class TempDataEnumeratorType>
  KeywordToAvoidDuplicateSymbolsForInlinedFunctions void timeStepWithRusanovBatchedHeapStateless(int targetDevice, CellData& patchData) InlineMethod;


  /*
   * Patch-wise realisation of the kernel
   *
   * Patch-wise realisation of the kernel where the input data and result
   * are explicitly copied forth and back between GPU and CPU. This
   * routine employs GPUCopyCellData as a wrapper around the host data.
   *
   * After that, the call is forwarded to the corresponding internal
   * implementation.
   *
   * @see exahype2::fv::rusanov::sycl::internal
   *
   */
  template <
    class SolverType,
    std::size_t NumberOfVolumesPerAxisInPatch,
    std::size_t HaloSize,
    std::size_t NumberOfUnknowns,
    std::size_t NumberOfAuxiliaryVariables,
    bool        EvaluateFlux,
    bool        EvaluateNonconservativeProduct,
    bool        EvaluateSource,
    bool        EvaluateMaximumEigenvalueAfterTimeStep,
    class TempDataEnumeratorType>
  KeywordToAvoidDuplicateSymbolsForInlinedFunctions void timeStepWithRusanovPatchwiseHeapStateless(int targetDevice, CellData& patchData) InlineMethod;


  /*
   * This version allocates the temporary data on the GPU via a device
   * malloc, handing in the standard memory, and finally frees the memory
   * again. That is, the version relies on SYCL's USM to move data
   * automatically forth and back.
   *
   */
  template <
    class SolverType,
    std::size_t NumberOfVolumesPerAxisInPatch,
    std::size_t HaloSize,
    std::size_t NumberOfUnknowns,
    std::size_t NumberOfAuxiliaryVariables,
    bool        EvaluateFlux,
    bool        EvaluateNonconservativeProduct,
    bool        EvaluateSource,
    bool        EvaluateMaximumEigenvalueAfterTimeStep,
    class TempDataEnumeratorType>
  KeywordToAvoidDuplicateSymbolsForInlinedFunctions void timeStepWithRusanovTaskgraphUSMStateless(int targetDevice, CellData& patchData) InlineMethod;


  /**
   * This version allocates the temporary data on the GPU via a device
   * malloc, handing in the standard memory, and finally frees the memory
   * again. That is, the version relies on SYCL's USM to move data
   * automatically forth and back.
   */
  template <
    class SolverType,
    std::size_t NumberOfVolumesPerAxisInPatch,
    std::size_t HaloSize,
    std::size_t NumberOfUnknowns,
    std::size_t NumberOfAuxiliaryVariables,
    bool        EvaluateFlux,
    bool        EvaluateNonconservativeProduct,
    bool        EvaluateSource,
    bool        EvaluateMaximumEigenvalueAfterTimeStep,
    class TempDataEnumeratorType>
  KeywordToAvoidDuplicateSymbolsForInlinedFunctions void timeStepWithRusanovBatchedUSMStateless(int targetDevice, CellData& patchData) InlineMethod;


  /**
   * 1:1 translation of the numerical scheme.
   *
   * This version allocates the temporary data on the GPU via a device
   * malloc, handing in the standard memory, and finally frees the memory
   * again. That is, the version relies on SYCL's USM to move data
   * automatically forth and back.
   */
  template <
    class SolverType,
    std::size_t NumberOfVolumesPerAxisInPatch,
    std::size_t HaloSize,
    std::size_t NumberOfUnknowns,
    std::size_t NumberOfAuxiliaryVariables,
    bool        EvaluateFlux,
    bool        EvaluateNonconservativeProduct,
    bool        EvaluateSource,
    bool        EvaluateMaximumEigenvalueAfterTimeStep,
    class TempDataEnumeratorType>
  KeywordToAvoidDuplicateSymbolsForInlinedFunctions void timeStepWithRusanovPatchwiseUSMStateless(int targetDevice, CellData& patchData) InlineMethod;


  /**
   *
   */
  template <
    class SolverType,
    std::size_t NumberOfVolumesPerAxisInPatch,
    std::size_t HaloSize,
    std::size_t NumberOfUnknowns,
    std::size_t NumberOfAuxiliaryVariables,
    bool        EvaluateFlux,
    bool        EvaluateNonconservativeProduct,
    bool        EvaluateSource,
    bool        EvaluateMaximumEigenvalueAfterTimeStep,
    class TempDataEnumeratorType>
  KeywordToAvoidDuplicateSymbolsForInlinedFunctions void timeStepWithRusanovTaskgraphManagedStateless(int targetDevice, CellData& patchData) InlineMethod;


  /**
   *
   */
  template <
    class SolverType,
    std::size_t NumberOfVolumesPerAxisInPatch,
    std::size_t HaloSize,
    std::size_t NumberOfUnknowns,
    std::size_t NumberOfAuxiliaryVariables,
    bool        EvaluateFlux,
    bool        EvaluateNonconservativeProduct,
    bool        EvaluateSource,
    bool        EvaluateMaximumEigenvalueAfterTimeStep,
    class TempDataEnumeratorType>
  KeywordToAvoidDuplicateSymbolsForInlinedFunctions void timeStepWithRusanovBatchedManagedStateless(int targetDevice, CellData& patchData) InlineMethod;


  /**
   *
   */
  template <
    class SolverType,
    std::size_t NumberOfVolumesPerAxisInPatch,
    std::size_t HaloSize,
    std::size_t NumberOfUnknowns,
    std::size_t NumberOfAuxiliaryVariables,
    bool        EvaluateFlux,
    bool        EvaluateNonconservativeProduct,
    bool        EvaluateSource,
    bool        EvaluateMaximumEigenvalueAfterTimeStep,
    class TempDataEnumeratorType>
  KeywordToAvoidDuplicateSymbolsForInlinedFunctions void timeStepWithRusanovPatchwiseManagedStateless(int targetDevice, CellData& patchData) InlineMethod;


  /**
   * Variant of kernel invocation which times core compute time
   */
  template <
    class SolverType,
    std::size_t NumberOfVolumesPerAxisInPatch,
    std::size_t HaloSize,
    std::size_t NumberOfUnknowns,
    std::size_t NumberOfAuxiliaryVariables,
    bool        EvaluateFlux,
    bool        EvaluateNonconservativeProduct,
    bool        EvaluateSource,
    bool        EvaluateMaximumEigenvalueAfterTimeStep,
    class TempDataEnumeratorType>
  KeywordToAvoidDuplicateSymbolsForInlinedFunctions void timeStepWithRusanovTaskgraphCopyStateless(int targetDevice, CellData& patchData, ::tarch::timing::Measurement& measurement)
    InlineMethod;


  template <
    class SolverType,
    std::size_t NumberOfVolumesPerAxisInPatch,
    std::size_t HaloSize,
    std::size_t NumberOfUnknowns,
    std::size_t NumberOfAuxiliaryVariables,
    bool        EvaluateFlux,
    bool        EvaluateNonconservativeProduct,
    bool        EvaluateSource,
    bool        EvaluateMaximumEigenvalueAfterTimeStep,
    class TempDataEnumeratorType>
  KeywordToAvoidDuplicateSymbolsForInlinedFunctions void timeStepWithRusanovBatchedHeapStateless(int targetDevice, CellData& patchData, ::tarch::timing::Measurement& measurement)
    InlineMethod;


  template <
    class SolverType,
    std::size_t NumberOfVolumesPerAxisInPatch,
    std::size_t HaloSize,
    std::size_t NumberOfUnknowns,
    std::size_t NumberOfAuxiliaryVariables,
    bool        EvaluateFlux,
    bool        EvaluateNonconservativeProduct,
    bool        EvaluateSource,
    bool        EvaluateMaximumEigenvalueAfterTimeStep,
    class TempDataEnumeratorType>
  KeywordToAvoidDuplicateSymbolsForInlinedFunctions void timeStepWithRusanovPatchwiseHeapStateless(int targetDevice, CellData& patchData, ::tarch::timing::Measurement& measurement)
    InlineMethod;


  template <
    class SolverType,
    std::size_t NumberOfVolumesPerAxisInPatch,
    std::size_t HaloSize,
    std::size_t NumberOfUnknowns,
    std::size_t NumberOfAuxiliaryVariables,
    bool        EvaluateFlux,
    bool        EvaluateNonconservativeProduct,
    bool        EvaluateSource,
    bool        EvaluateMaximumEigenvalueAfterTimeStep,
    class TempDataEnumeratorType>
  KeywordToAvoidDuplicateSymbolsForInlinedFunctions void timeStepWithRusanovTaskgraphUSMStateless(int targetDevice, CellData& patchData, ::tarch::timing::Measurement& measurement)
    InlineMethod;


  template <
    class SolverType,
    std::size_t NumberOfVolumesPerAxisInPatch,
    std::size_t HaloSize,
    std::size_t NumberOfUnknowns,
    std::size_t NumberOfAuxiliaryVariables,
    bool        EvaluateFlux,
    bool        EvaluateNonconservativeProduct,
    bool        EvaluateSource,
    bool        EvaluateMaximumEigenvalueAfterTimeStep,
    class TempDataEnumeratorType>
  KeywordToAvoidDuplicateSymbolsForInlinedFunctions void timeStepWithRusanovBatchedUSMStateless(int targetDevice, CellData& patchData, ::tarch::timing::Measurement& measurement)
    InlineMethod;


  template <
    class SolverType,
    std::size_t NumberOfVolumesPerAxisInPatch,
    std::size_t HaloSize,
    std::size_t NumberOfUnknowns,
    std::size_t NumberOfAuxiliaryVariables,
    bool        EvaluateFlux,
    bool        EvaluateNonconservativeProduct,
    bool        EvaluateSource,
    bool        EvaluateMaximumEigenvalueAfterTimeStep,
    class TempDataEnumeratorType>
  KeywordToAvoidDuplicateSymbolsForInlinedFunctions void timeStepWithRusanovPatchwiseUSMStateless(int targetDevice, CellData& patchData, ::tarch::timing::Measurement& measurement)
    InlineMethod;


  /**
   *
   */
  template <
    class SolverType,
    std::size_t NumberOfVolumesPerAxisInPatch,
    std::size_t HaloSize,
    std::size_t NumberOfUnknowns,
    std::size_t NumberOfAuxiliaryVariables,
    bool        EvaluateFlux,
    bool        EvaluateNonconservativeProduct,
    bool        EvaluateSource,
    bool        EvaluateMaximumEigenvalueAfterTimeStep,
    class TempDataEnumeratorType>
  KeywordToAvoidDuplicateSymbolsForInlinedFunctions void timeStepWithRusanovTaskgraphManagedStateless(
    int targetDevice, CellData& patchData, ::tarch::timing::Measurement& measurement
  ) InlineMethod;


  /**
   *
   */
  template <
    class SolverType,
    std::size_t NumberOfVolumesPerAxisInPatch,
    std::size_t HaloSize,
    std::size_t NumberOfUnknowns,
    std::size_t NumberOfAuxiliaryVariables,
    bool        EvaluateFlux,
    bool        EvaluateNonconservativeProduct,
    bool        EvaluateSource,
    bool        EvaluateMaximumEigenvalueAfterTimeStep,
    class TempDataEnumeratorType>
  KeywordToAvoidDuplicateSymbolsForInlinedFunctions void timeStepWithRusanovBatchedManagedStateless(
    int targetDevice, CellData& patchData, ::tarch::timing::Measurement& measurement
  ) InlineMethod;


  /**
   *
   */
  template <
    class SolverType,
    std::size_t NumberOfVolumesPerAxisInPatch,
    std::size_t HaloSize,
    std::size_t NumberOfUnknowns,
    std::size_t NumberOfAuxiliaryVariables,
    bool        EvaluateFlux,
    bool        EvaluateNonconservativeProduct,
    bool        EvaluateSource,
    bool        EvaluateMaximumEigenvalueAfterTimeStep,
    class TempDataEnumeratorType>
  KeywordToAvoidDuplicateSymbolsForInlinedFunctions void timeStepWithRusanovPatchwiseManagedStateless(
    int targetDevice, CellData& patchData, ::tarch::timing::Measurement& measurement
  ) InlineMethod;


  namespace internal {
    /**
     * Read the docu page "ExaHyPE's finite volumes Rusanov kernel implementation"
     * (from rusanov.h) before you continue.
     * We do not build up a task graph for the individual logical steps,
     * but we build up a task graph per patch.
     *
     *
     * ## Comparison to OpenMP
     *
     * In the OpenMP world, we use nested parallelism to realise this
     * patch-wise processing. While we tried this out with SYCL (and wasn't
     * successful as we did hit some compiler issues), it turns out that the
     * SYCL approach is way more elegant with its DAG on the device.
     *
     *
     * ## Implementation details
     *
     * We hold a set of vectors over events which represent the key stages
     * of the algorithm per patch. We wait for these guys at the end of the
     * algorithm. An exception is the copySolutionEvents. We don't have to
     * wait for these, as the kernels behind updateWithEigenvalueEvent already
     * depend on them.
     *
     * So our realisation is, in principle, very simple: Each large logical
     * step (cmp. the batched variant) is formalised by a vector of events.
     * As any compute kernels depends only on events defined over its batch,
     * the kernels are, in principle, totally independent of each other and we
     * thus may speak of a patch-wise processing.
     *
     * Within each logical step, we have additional substeps. The update due
     * to the flux for example first requires us to compute the fluxes in x,
     * y and z direction. Here, we have to be very careful: Again, we model
     * the steps as events. Even though they are internal events, we also have
     * to store them in a global vector, as they have to remain persistent,
     * aka in-memory, until the final update kernel has terminated. We originally
     * modelled them as local variables, but then they are destroyed at the
     * end of the scope block, and we get a memory error.
     *
     * An alternative to the ``store them in a vector'' approach would be to
     * use smart pointers.
     *
     */
    template <
      class SolverType,
      std::size_t NumberOfVolumesPerAxisInPatch,
      std::size_t HaloSize,
      std::size_t NumberOfUnknowns,
      std::size_t NumberOfAuxiliaryVariables,
      bool        EvaluateFlux,
      bool        EvaluateNonconservativeProduct,
      bool        EvaluateSource,
      bool        EvaluateMaximumEigenvalueAfterTimeStep,
      class TempDataEnumeratorType>
    KeywordToAvoidDuplicateSymbolsForInlinedFunctions void timeStepWithRusanovTaskgraphStateless(int targetDevice, auto& patchData) InlineMethod;


    /**
     * Implementation of the Rusanov kernel which builds up a task graph which consists of the
     * logical steps of a Rusanov solver. So we have a step to copy the data into the target,
     * then one to compute the eigenvalues along the x-direction, then one along the y-direction,
     * then one to update with the eigenvalue damping (which depends on all previous steps), and
     * so forth.
     *
     * Each individual step runs over all patches handed in. So the
     * individual steps are relatively heavy, while the DAG exclusively
     * represents logical steps of the underlying numerical scheme. It is a
     * (trivial) sequence of steps. We call the approach batched, as each
     * individual logical step in the task graph runs over the batch of patches.
     *
     *
     * ## Memory management
     *
     * If you run this routine without GPU offloading, i.e., if you use it
     * non-GPU cousin, we place all temporary data on the heap. If you use
     * the SYCL variant, we explicitly allocate the temp memory on the device.
     *
     * Once the kernel has finished, we have to free all temporary data. We
     * use ::sycl::free() to delete our scratchpads.
     *
     * The QIn and QOut as wrapped by ::exahype2::CellData also have to go to the GPU.
     * There are different options for this one the table, and each option
     * is offered through one of the routines in the namespace above. The
     * realisation of these options is encapsulated within classes from
     * GPUCellData and we parameterise this routine with this type, i.e.,
     * we can alter the memory transfer model by exchanging the type in the
     * template.
     *
     *
     * ## SYCL details
     *
     * SYCL required std::size_t for all of its ranges. We store the ranges
     * implicitly in the patch description as integer. When you try to pass
     * in the integers into the SYCL routines, C++ likely will complain with
     *
     *      error: non-constant-expression cannot be narrowed from type 'int' to 'size_t' (aka 'unsigned long') in initializer list [-Wc++11-narrowing]
     *
     * If we explicitly convert the variable into a std::size_t before, then
     * everything is fine. We can do this here, as we know what we are doing.
     * This is also the reason why we have remodelled the template signature
     * which now uses std::size_t rather than int (if you compare the
     * template arguments to the non-GPU version).
     *
     * We try to stick with ranges where possible and to avoid nd_range.
     * Whenever we do not need things like grain sizes or a substructuring
     * of iteration spaces, we avoid it. We keep stuff as simple as possible.
     *
     * ## Syntax
     *
     * As we work in a subnamespace sycl of the Rusanov solvers, any
     * reference to a namespace sycl will make C++ search for something
     * within our rusanov::sycl namespace. Therefore, we have to qualify
     * the namespace absolutely with ::sycl if we want to use something
     * from the standard.
     *
     * @param patchData A GPU patch data object which encapsulates all
     *   data transfer to/from GPU.
     */
    template <
      class SolverType,
      std::size_t NumberOfVolumesPerAxisInPatch,
      std::size_t HaloSize,
      std::size_t NumberOfUnknowns,
      std::size_t NumberOfAuxiliaryVariables,
      bool        EvaluateFlux,
      bool        EvaluateNonconservativeProduct,
      bool        EvaluateSource,
      bool        EvaluateMaximumEigenvalueAfterTimeStep,
      class TempDataEnumeratorType>
    KeywordToAvoidDuplicateSymbolsForInlinedFunctions void timeStepWithRusanovBatchedStateless(int targetDevice, auto& patchData) InlineMethod;


    /**
     * 1:1 translation of the numerical scheme.
     *
     * We grab the SYCL queue that we want to use (Peano puts one SYCL
     * queue on each available device and enumerates them logically
     * starting from 0). Next, we create enumerators which are simply
     * classes that translate multidimensional indices into an integer.
     * As we use enumerators which are template arguments, we can
     * quickly exchange SoA with AoS for example. Once the enumerators
     * are in place, we allocate the memory for all temporary data on
     * the device.
     *
     * This implementation is called patch-wise. This means that we have
     * a big outer loop which runs over all the patches. If we had a
     * sequential accelerator, this would handle one patch after the
     * other, i.e., process the patches patch-wisely. As these loop steps
     * are independent of each other, we can use a parallel for.
     *
     * Per patch, we then run through the numerical steps and issue
     * (nested) parallel fors.
     *
     * ## Flaws
     *
     * - We would like to use higher-dimensional range loops.
     * - This version seems to crash from time to time. We assume it is
     *   due to the fact that the parallel fors have different range
     *   dimensions.
     *
     * @see timeStepWithRusanovBatchedStateless() for further
     *   SYCL-relevant details.
     */
    template <
      class SolverType,
      std::size_t NumberOfVolumesPerAxisInPatch,
      std::size_t HaloSize,
      std::size_t NumberOfUnknowns,
      std::size_t NumberOfAuxiliaryVariables,
      bool        EvaluateFlux,
      bool        EvaluateNonconservativeProduct,
      bool        EvaluateSource,
      bool        EvaluateMaximumEigenvalueAfterTimeStep,
      class TempDataEnumeratorType>
    KeywordToAvoidDuplicateSymbolsForInlinedFunctions void timeStepWithRusanovPatchwiseStateless(int targetDevice, auto& patchData) InlineMethod;
  } // namespace internal
} // namespace exahype2::fv::rusanov::sycl

#include "BatchedStateless.cpph"
#include "PatchwiseStateless.cpph"
#include "TaskgraphStateless.cpph"
