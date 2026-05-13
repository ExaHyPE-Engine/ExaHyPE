// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/la/Vector.h"
#include "peano4/utils/Globals.h"
#include "config.h"
#include "peano4/datamanagement/CellMarker.h"

#include <functional>
#include <string>

namespace exahype2 {
  /**
   * Representation of a number of cells which contains all information
   * that's required to process the stored patches or polynomial shape
   * functions (given you know the semantics of the data).
   *
   * This struct is a mixture of AoS and SoA. All data besides the Qin and
   * result are SoA. The reason for this "inconsistent" model is that we
   * want to transfer the patch data to accelerators quickly, while we also
   * want to avoid to copy too much stuff around. So we leave the big input
   * fields and basically manage just pointers to that. All scalar or small
   * vector data are converted in SoA which makes it fairly straightforward
   * later to move them over to an accelerator.
   *
   *
   * ## Memory responsibility
   *
   * The PatchData object is a meta object. It does not administer any major
   * heap data itself. That is, its QIn data has to be redirected to the
   * reconstructed data, i.e. the patch data including the halos, by the user.
   * This happens for example in applyKernelToCell() where we create a
   * PatchData object over both allocated input and output data.
   *
   *
   * As the PatchData does maintain some internal tables which are allocated
   * in the constructor and freed in the destructor, I do not provide a copy
   * constructor. This one is explicitly deleted. Some backends might require
   * a copy constructor one day
   *
   * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~
   * exahype2::CellData::CellData( const CellData& copy ):
   *   CellData(copy.numberOfCells) {
   *   for (int i=0; i<numberOfCells; i++) {
   *     QIn[i]           = copy.QIn[i];
   *     cellCentre[i]    = copy.cellCentre[i];
   *     cellSize[i]      = copy.cellSize[i];
   *     t[i]             = copy.t[i];
   *     dt[i]            = copy.dt[i];
   *     id[i]            = copy.id[i];
   *     QOut[i]          = copy.QOut[i];
   *     maxEigenvalue[i] = copy.maxEigenvalue[i];
   *   }
   * }
   * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~
   *
   * but for the time being, I do not offer such a variant.
   *
   *
   * ## SYCL
   *
   * If you use SYCL, you might be tempted to declare the class as copyable.
   * This is factually wrong: the class is not trivially copyable.
   *
   * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~
   * #if defined(SharedSYCL)
   * #include "tarch/accelerator/sycl/Device.h"
   * template<>
   * struct sycl::is_device_copyable<exahype2::CellData>: std::true_type {};
   * #endif
   * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~
   *
   * hence makes the file compile, but in all tests that I ran the code then
   * did crash.
   */
  template<typename inType=double, typename outType=double>
  struct CellData {
    /**
     * QIn may not be const, as some kernels delete it straightaway once
     * the input data has been handled.
     */
    inType**                                QIn;
    tarch::la::Vector<Dimensions,double>*   cellCentre;
    tarch::la::Vector<Dimensions,double>*   cellSize;

    double*    t;
    double*    dt;
    /**
     * Id of underlying task. Required when we fuse many enclave tasks
     * and load them off to the GPU, as we have to know afterwards which
     * outcome corresponds to which task.
     */
    int*       id;

    /**
     * As we store data as SoA, we have to know how big the actual
     * arrays are.
     */
    const int  numberOfCells;

    /**
     * We might want to allocate data on the heap or an accelerator,
     * therefore we save the target device id.
     */
    const tarch::MemoryLocation memoryLocation;

    /**
     * We might want to allocate data on an accelerator,
     * therefore we save the target device id.
     */
    const int  targetDevice;

    /**
     * Out values.
     */
    outType**  QOut;

    /**
     * Out values.
     */
    double*    maxEigenvalue;

    /**
     * When using an accelerator we may recieve data from both the host and device.
     * We therefore want to track the status of this data to avoid unnecessary data 
     * movements.
     */
    peano4::datamanagement::PatchWithHaloDeviceMemoryStatus* deviceMemoryStatus;

    /**
     * Construct patch data object for one single cell.
     *
     * Usually, I do so only to be able to use the same kernels everywhere:
     * Kernels accept CellData, i.e. multiple patches. Even if we have only
     * one cell, we thus wrap this cell's data into an instance of CellData
     * and pass it in. The id can be set to any dummy in this case, as we
     * know which task has wrapped this single cell, i.e. we usually do not
     * read it later.
     */
    CellData(
      inType*                                     QIn_,
      const tarch::la::Vector<Dimensions,double>& cellCentre_,
      const tarch::la::Vector<Dimensions,double>& cellSize_,
      double                t_,
      double                dt_,
      outType*              QOut_,
      tarch::MemoryLocation memoryLocation_ = tarch::MemoryLocation::Heap,
      int                   targetDevice_   = tarch::accelerator::Device::HostDevice,
      peano4::datamanagement::PatchWithHaloDeviceMemoryStatus deviceMemoryStatus_ = peano4::datamanagement::PatchWithHaloDeviceMemoryStatus::Unallocated
    );

    CellData(
      int                   numberOfCells_,
      tarch::MemoryLocation memoryLocation = tarch::MemoryLocation::Heap,
      int                   targetDevice   = tarch::accelerator::Device::HostDevice
    );

    CellData(const CellData<inType,outType>& copy) = delete;

    ~CellData();

    std::string toString() const;

    static int in(int index[Dimensions+2]);
    static int out(int index[Dimensions+2]);
  };

//  using CellData = CellData<double, double>;

  /**
   * Cousing to CellData which administers copies on the accelerator
   *
   *
   * This is a variant of CellData which also present GPU data. However, this
   * flavour copies all data manually onto the GPU, whereas its parent is a
   * proxy around data. As we copy data over, we can also rearrange all data.
   * From a user point of view, the big difference is that we have to invoke
   * copyBackAndDestroy() manually to eliminate the data on the device:
   *
   * Copies to the device occur automatically on
   * construction, but copies back must be triggered manually using
   * copyBackAndDestroy().
   * 
   * The OpenMP implementation offers two versions, one using allocations
   * on the device, and another that keeps data on the host. This second 
   * version requires explicit maps in the kernel, which do not currently
   * exist. For this reason it is currently not in use.
   *
   * @todo Shall we retire the second version then if it is not used
   *       at all?
   */
  template<typename inType=double, typename outType=double>
  struct CopyCellDataGPU{
    /* 
    See CellData for meaning of member variables. 
    The only change is changing QIn and QOut from 2D arrays to 1D.
    */

    inType*                                 QIn;
    tarch::la::Vector<Dimensions,double>*   cellCentre;
    tarch::la::Vector<Dimensions,double>*   cellSize;
    double*    t;
    double*    dt;
    int*       id;
    const int  numberOfCells;
    const tarch::MemoryLocation memoryLocation;
    const int  targetDevice;
    outType*  QOut;
    double*    maxEigenvalue;

    inType** QInHost;
    outType** QOutHost;
    double* maxEigenvalueHost;

    /**
     * Construct a copy object
     *
     * @param device Valid device by handling in value greater or equal to
     *   0 or otherwise tarch::accelerator::Device::HostDevice.
     */
    CopyCellDataGPU(
      const CellData<inType, outType>&   cellData,
      int                                inputPatchSize,
      int                                outputPatchSize,
      int                                device = tarch::accelerator::Device::HostDevice
    );
    void copyBackAndDestroy();
    
    std::string toString() const;

    int QInStride;
    int QOutStride;
  };

}

#include "CellData.cpph"
