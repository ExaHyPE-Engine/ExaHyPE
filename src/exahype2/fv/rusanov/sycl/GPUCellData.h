// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "config.h"

#if defined(GPUOffloadingSYCL)

#include "exahype2/enumerator/AoSLexicographicEnumerator.h"
#include "peano4/utils/Globals.h"
#include "tarch/accelerator/accelerator.h"
#include "tarch/la/Vector.h"

namespace exahype2 {
  /**
   * Forward declaration
   */
  struct CellData;

  namespace fv::rusanov::sycl {
    /**
     * Wrapper around CellData
     *
     * Within ExaHyPE2, data that has to be processed by the GPU is
     * initially on the heap allocated via ::tarch::accelerator::ManagedSharedAcceleratorDeviceMemory.
     * This holds for the core user data. It does not hold for any meta data.
     *
     * Therefore, we cannot transfer the CellData objects directly to the
     * GPU in SYCL, even though the core user data (patch data) already
     * resides in shared memory. We have to kind of wrap it. We provide
     * two different wrappers:
     *
     * - The simpler wrapper exploits the USM property, i.e., copies all
     *   non-patch data into shared memory and otherwise hands out the
     *   shared pointers.
     * - The copy version manually copies all the data to the device and
     *   works with the device data.
     *
     * Both versions' signature (public pointers)
     * equals exactly ::exahype2::CellData.
     *
     * ## Usage
     *
     * Create an instance of this class on the host using the constructor that
     * accepts the CellData. This instance wraps around the real cell data, i.e.,
     * creates copies copy of all relevant data in the shared memory.
     *
     * Copy this wrapper object over to the accelerator. This is a byte-wise
     * copy. We can do this, as we have made all pointer of this wrapper object
     * point to shared memory.
     *
     * Destroy the object on the accelerator. No data is actually freed.
     *
     * Call destroy() on the host once the kernels have finished. This actually
     * frees the copies in the shared memory.
     *
     *
     * ## Implementation
     *
     * It is worth comparing this version with GPUCopyCellData. The big
     * difference here is that we leave the actual patch data where it is,
     * i.e., we exploit the fact that these data are already in the shared
     * memory space.
     *
     * For all the other data, which we have to copy explicitly, we have
     * to decide if we want to allocate these data explicitly on the
     * device or make it shared, too. To be "clean", I put everything into
     * the shared memory, so I can use simple memcopies on the host to
     * get everything set up properly.
     */
    struct GPUUSMCellData {
      double**                                 QIn;
      ::tarch::la::Vector<Dimensions, double>* cellCentre;
      ::tarch::la::Vector<Dimensions, double>* cellSize;
      double*                                  t;
      double*                                  dt;
      int                                      numberOfCells;
      double**                                 QOut;
      double*                                  maxEigenvalue;

      /**
       * We never create a GPU wrapper object directly
       *
       * CPUCellData is a wrapper and either wraps around an
       * existing ::exahype::CellData object, or it is instantiated
       * as copy of an existing object on the accelerator.
       */
      GPUUSMCellData() = delete;

      /**
       * This operation is required to support the migration
       * to the GPU. As we label it as default, it is a plain
       * pointer copy. This means: Once we have a valid instance
       * of this object on the host which points to shared memory
       * regions (shared between GPU and CPU), we can simply copy
       * the object over and use it within a queue.
       */
      GPUUSMCellData(const GPUUSMCellData& copy) = default;

      /**
       * Do nothing.
       *
       * The object is either a GPU copy or a wrapper around an
       * existing object on the CPU. If it is the GPU representative,
       * we can always destroy it if we want. The object has no
       * data ownership, so we don't have to free data. If the
       * object is hosted on the CPU, it wraps around an existing
       * ::exahype::CellData object, and we expect the user code
       * to call destroy() explicitly.
       */
      ~GPUUSMCellData() = default;

      /**
       * Create a wrapper around a cell data object which can then be
       * copied to the CPU.
       *
       * The user has to call destroy() eventually to free all internal
       * data structures.
       *
       * This routine is very similar to the constructor of
       * GPUCopyCellData. The big difference to this one is that we know
       * that the actual QIn do already reside in shared memory. That is,
       * we don't have to copy those guys.
       *
       * @see destroy()
       */
      GPUUSMCellData(
        const CellData& hostObject, const enumerator::AoSLexicographicEnumerator& inEnumerator, const enumerator::AoSLexicographicEnumerator& outEnumerator, ::sycl::queue& queue
      );

      void destroy(CellData& hostObject, const enumerator::AoSLexicographicEnumerator& outEnumerator, bool copyEigenvaluesBack, ::sycl::queue& queue);
    };

    /**
     * Wrapper around CellData
     *
     * Please consult GPUUSMCellData for a description.
     *
     * ## Implementation
     *
     * The QIn and QOut in the original patch data is an array over pointers,
     * where each pointer points to the QIn/QOut data of one patch. When we
     * copy the data over to the GPU, it makes no sense to work with a
     * fragmented memory. Instead, we allocate the data for all N patches
     * in one big go. The corresponding entry is stored in QInCopyInOneHugeBuffer or
     * QOutCopyInOneHugeBuffer, respectively. The QIn and QOut then are index arrays
     * over this big copy.
     *
     * ## Usage
     *
     * Please consult GPUUSMCellData for a description. There is one
     * difference behind the scenes, which you might want to take into
     * account or you might not want to: The whole patch data are copied
     * over into one huge chunk.
     */
    struct GPUCopyCellData {
      double**                                 QIn;
      ::tarch::la::Vector<Dimensions, double>* cellCentre;
      ::tarch::la::Vector<Dimensions, double>* cellSize;
      double*                                  t;
      double*                                  dt;
      int                                      numberOfCells;
      double**                                 QOut;
      double*                                  maxEigenvalue;

      double* QInCopyInOneHugeBuffer;
      double* QOutCopyInOneHugeBuffer;

      /**
       * We never create a GPU wrapper object directly
       *
       * CPUCellData is a wrapper and either wraps around an
       * existing ::exahype::CellData object, or it is instantiated
       * as copy of an existing object on the accelerator.
       */
      GPUCopyCellData() = delete;

      /**
       * This operation is required to support the migration
       * to the GPU. As we label it as default, it is a plain
       * pointer copy. This means: Once we have a valid instance
       * of this object on the host which points to shared memory
       * regions (shared between GPU and CPU), we can simply copy
       * the object over and use it within a queue.
       */
      GPUCopyCellData(const GPUCopyCellData& copy) = default;

      /**
       * Do nothing.
       *
       * The object is either a GPU copy or a wrapper around an
       * existing object on the CPU. If it is the GPU representative,
       * we can always destroy it if we want. The object has no
       * data ownership, so we don't have to free data. If the
       * object is hosted on the CPU, it wraps around an existing
       * ::exahype::CellData object, and we expect the user code
       * to call destroy() explicitly.
       */
      ~GPUCopyCellData() = default;

      /**
       * Create a wrapper around a cell data object
       *
       * The resulting object then is copied to the CPU.
       * The user has to call destroy() eventually to free all internal
       * data structures.
       * Internally, the code first flattens the whole QIn object into
       * one big buffer, and then copies this buffer over in one rush.
       * We make this linearlisation be the last step, so the other copy
       * operations can overlap with this linearlisation overhead on the
       * host.
       *
       * The events needs to be stored in a std::vector. I could, in
       * theory, work with an array, but SYCL does not support a wait
       * over an array. After all, it has to know the size (number) of
       * the events when it waits.
       *
       * @see destroy()
       */
      GPUCopyCellData(
        const CellData& hostObject, const enumerator::AoSLexicographicEnumerator& inEnumerator, const enumerator::AoSLexicographicEnumerator& outEnumerator, ::sycl::queue& queue
      );

      void destroy(CellData& hostObject, const enumerator::AoSLexicographicEnumerator& outEnumerator, bool copyEigenvaluesBack, ::sycl::queue& queue);
    };


    /**
     * Wrapper around CellData
     *
     * Please consult GPUUSMCellData for a description.
     *
     * ## Usage
     *
     * Create an instance of this class on the host using the constructor that
     * accepts the CellData. This instance wraps around the real cell data, i.e.,
     * creates copies copy of all relevant data in the shared memory.
     *
     * Copy this wrapper object over to the accelerator. This is a byte-wise
     * copy. We can do this, as we have made all pointer of this wrapper object
     * point to shared memory.
     *
     * Destroy the object on the accelerator. No data is actually freed.
     *
     * Call destroy() on the host once the kernels have finished. This actually
     * frees the copies in the shared memory.
     *
     */
    struct GPUManagedCellData {
      double**                                 QIn;
      ::tarch::la::Vector<Dimensions, double>* cellCentre;
      ::tarch::la::Vector<Dimensions, double>* cellSize;
      double*                                  t;
      double*                                  dt;
      int                                      numberOfCells;
      double**                                 QOut;
      double*                                  maxEigenvalue;


      double* QInCopyInOneHugeBuffer;
      double* QOutCopyInOneHugeBuffer;

      /**
       * We never create a GPU wrapper object directly
       *
       * CPUCellData is a wrapper and either wraps around an
       * existing ::exahype::CellData object, or it is instantiated
       * as copy of an existing object on the accelerator.
       */
      GPUManagedCellData() = delete;

      /**
       * This operation is required to support the migration
       * to the GPU. As we label it as default, it is a plain
       * pointer copy. This means: Once we have a valid instance
       * of this object on the host which points to shared memory
       * regions (shared between GPU and CPU), we can simply copy
       * the object over and use it within a queue.
       */
      GPUManagedCellData(const GPUManagedCellData& copy) = default;

      /**
       * Do nothing.
       *
       * The object is either a GPU copy or a wrapper around an
       * existing object on the CPU. If it is the GPU representative,
       * we can always destroy it if we want. The object has no
       * data ownership, so we don't have to free data. If the
       * object is hosted on the CPU, it wraps around an existing
       * ::exahype::CellData object, and we expect the user code
       * to call destroy() explicitly.
       */
      ~GPUManagedCellData() = default;

      /**
       * Create a wrapper around a cell data object which can then be
       * copied to the CPU. The user has to call destroy() eventually to
       * free all internal data structures. This version is very similar
       * to GPUCopyCellData, but all the allocs are piped towards the
       * memory manager.
       *
       * @see destroy()
       */
      GPUManagedCellData(
        const CellData& hostObject, const enumerator::AoSLexicographicEnumerator& inEnumerator, const enumerator::AoSLexicographicEnumerator& outEnumerator, ::sycl::queue& queue
      );

      void destroy(CellData& hostObject, const enumerator::AoSLexicographicEnumerator& outEnumerator, bool copyEigenvaluesBack, ::sycl::queue& queue);
    };
  } // namespace fv::rusanov::sycl
} // namespace exahype2

#endif
