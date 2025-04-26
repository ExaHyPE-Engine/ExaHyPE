// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "config.h"

#include "peano4/utils/Globals.h"

#include "tarch/la/Vector.h"
#include "tarch/accelerator/accelerator.h"
#include "tarch/accelerator/MemoryManager.h"

#include <functional>
#include <string>

namespace exahype2 {
  /**
   * Representation of a number of cells which contains all information
   * that's required to process the stored patches or polynomial shape
   * functions (given you know the semantics of the data).
   *
   * As the CellData does maintain some internal tables which are allocated
   * in the constructor and freed in the destructor, it does not provide a copy
   * constructor. This one is explicitly deleted.
   */
  template <typename inType = double, typename outType = double>
  struct CellData {
    inType**                                QIn;
    inType*                                 QIn1D;
    tarch::la::Vector<Dimensions, double>*  cellCentre;
    tarch::la::Vector<Dimensions, double>*  cellSize;
    outType**                               QOut;
    outType*                                QOut1D;
    outType*                                maxEigenvalue;

    // Assuming global time stepping
    double t;
    double dt;

    const int numberOfCells;

    /**
     * We might want to allocate data on the heap or an accelerator,
     * therefore we save the target device id.
     */
    const tarch::MemoryLocation memoryLocation;

    /**
     * We might want to allocate data on an accelerator,
     * therefore we save the target device id.
     */
    const int targetDevice;

    CellData(
      inType* const                                 QIn_,
      const tarch::la::Vector<Dimensions, double>&  cellCentre_,
      const tarch::la::Vector<Dimensions, double>&  cellSize_,
      double                                        t_,
      double                                        dt_,
      outType* const                                QOut_,
      tarch::MemoryLocation                         memoryLocation_ = tarch::MemoryLocation::Heap,
      int                                           targetDevice_   = tarch::accelerator::Device::DefaultDevice
    );

    CellData(
      int                                           numberOfCells_,
      tarch::MemoryLocation                         memoryLocation_ = tarch::MemoryLocation::Heap,
      int                                           targetDevice_   = tarch::accelerator::Device::DefaultDevice
    );

    CellData(const CellData& copy) = delete;
    CellData& operator=(const CellData&) = delete;

    ~CellData();

    std::string toString() const;
  };
} // namespace exahype2

#include "CellData.cpph"
