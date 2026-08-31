// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "peano4/Globals.h"
#include "tarch/accelerator/accelerator.h"
#include "tarch/la/Vector.h"

namespace exahype2 {
  /**
   * Holds a set of cells with the data a kernel requires to process the stored
   * patches or polynomial shape functions. The constructor allocates the
   * internal tables, and the destructor frees them.
   */
  template <typename inType, typename outType, int NumFaceTypes = 1>
  struct CellData {
    inType** QIn;
    inType*  QInFlattened;

    inType* (*faces)[NumFaceTypes][TWO_TIMES_D];
    inType*   facesFlattened;
    uint64_t* faceIndices;
    uint64_t* patchIndicesOfBoundaryFaces;
    uint64_t* boundaryFaceIndices;
    uint8_t*  boundaryFaceNumbers;

    tarch::la::Vector<DIMENSIONS, double>* cellCentre;
    tarch::la::Vector<DIMENSIONS, double>* cellSize;

    outType** QOut;
    outType*  QOutFlattened;
    outType*  maxEigenvalue;

    double t;
    double dt;

    int                         numberOfBoundaryFaces;
    const int                   numberOfCells;
    const tarch::MemoryLocation memoryLocation;
    const int                   targetDevice;

    CellData(
      inType* const                                QIn_,
      const tarch::la::Vector<DIMENSIONS, double>& cellCentre_,
      const tarch::la::Vector<DIMENSIONS, double>& cellSize_,
      double                                       t_,
      double                                       dt_,
      outType* const                               QOut_,
      tarch::MemoryLocation                        memoryLocation_ = tarch::MemoryLocation::Heap,
      int                                          targetDevice_   = tarch::accelerator::Device::DefaultDevice
    );

    CellData(
      inType* const                                QIn_,
      inType* const                                faces_[NumFaceTypes][TWO_TIMES_D],
      const tarch::la::Vector<DIMENSIONS, double>& cellCentre_,
      const tarch::la::Vector<DIMENSIONS, double>& cellSize_,
      double                                       t_,
      double                                       dt_,
      outType* const                               QOut_,
      tarch::MemoryLocation                        memoryLocation_ = tarch::MemoryLocation::Heap,
      int                                          targetDevice_   = tarch::accelerator::Device::DefaultDevice
    );

    CellData(
      int                   numberOfCells_,
      tarch::MemoryLocation memoryLocation_ = tarch::MemoryLocation::Heap,
      int                   targetDevice_   = tarch::accelerator::Device::DefaultDevice
    );

    CellData(const CellData& copy)            = delete;
    CellData& operator=(const CellData& copy) = delete;

    ~CellData();

    std::string toString() const;
  };
} // namespace exahype2

#include "exahype2/CellData.cpph"
