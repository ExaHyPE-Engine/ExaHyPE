// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "peano4/utils/Globals.h"
#include "tarch/accelerator/accelerator.h"
#include "tarch/accelerator/MemoryManager.h"
#include "tarch/la/Vector.h"

#include <functional>
#include <string>

namespace exahype2 {
  /**
   * Represents the faces of one cell, with a total of 2*Dim faces per cell
   * For ADER QIn will contain the values, QOut would contain the fluxes
   * The faces can still each be made to contain either only the values for the
   * given cell, or those and the neighbours' values
   */
  template <typename inType = double, typename outType = double> struct CellFaceData {
    /**
     * QIn may not be const, as some kernels delete it straightaway once
     * the input data has been handled.
     */
    inType* (*QIn)[2 * Dimensions];
    tarch::la::Vector<Dimensions, double>* cellCentre;
    tarch::la::Vector<Dimensions, double>* cellSize;

    double* t;
    double* dt;

    /**
     * Id of underlying task. Required when we fuse many enclave tasks
     * and load them off to the GPU, as we have to know afterwards which
     * outcome corresponds to which task.
     */
    int* id;

    /**
     * As we store data as SoA, we have to know how big the actual
     * arrays are.
     */
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

    /**
     * Out values.
     */
    outType* (*QOut)[2 * Dimensions];

    /**
     * Construct patch data object for one single cell.
     *
     * Usually, I do so only to be able to use the same kernels everywhere:
     * Kernels accept CellFaceData, i.e. multiple patches. Even if we have only
     * one cell, we thus wrap this cell's data into an instance of CellFaceData
     * and pass it in. The id can be set to any dummy in this case, as we
     * know which task has wrapped this single cell, i.e. we usually do not
     * read it later.
     */
    CellFaceData(
      inType*                                      QIn_[2 * Dimensions],
      const tarch::la::Vector<Dimensions, double>& cellCentre_,
      const tarch::la::Vector<Dimensions, double>& cellSize_,
      double                                       t_,
      double                                       dt_,
      outType*                                     QOut_[2 * Dimensions],
      tarch::MemoryLocation                        memoryLocation_ = tarch::MemoryLocation::Heap,
      int                                          targetDevice_   = tarch::accelerator::Device::DefaultDevice
    );

    CellFaceData(
      int                   numberOfCells_,
      tarch::MemoryLocation memoryLocation_ = tarch::MemoryLocation::Heap,
      int                   targetDevice_   = tarch::accelerator::Device::DefaultDevice
    );

    CellFaceData(const CellFaceData& copy)       = delete;
    CellFaceData& operator=(const CellFaceData&) = delete;

    ~CellFaceData();

    std::string toString() const;
  };
} // namespace exahype2

template <typename inType, typename outType>
exahype2::CellFaceData<inType, outType>::CellFaceData(
  inType*                                      QIn_[2 * Dimensions],
  const tarch::la::Vector<Dimensions, double>& cellCentre_,
  const tarch::la::Vector<Dimensions, double>& cellSize_,
  double                                       t_,
  double                                       dt_,
  outType*                                     QOut_[2 * Dimensions],
  tarch::MemoryLocation                        memoryLocation_,
  int                                          targetDevice_
):
  CellFaceData(1, memoryLocation_, targetDevice_) {
  QIn[0]        = QIn_;
  cellCentre[0] = cellCentre_;
  cellSize[0]   = cellSize_;
  t[0]          = t_;
  dt[0]         = dt_;
  QOut[0]       = QOut_;
  id[0]         = -1;
}

template <typename inType, typename outType>
exahype2::CellFaceData<inType, outType>::
  CellFaceData(int numberOfCells_, tarch::MemoryLocation memoryLocation_, int targetDevice_):
  numberOfCells(numberOfCells_),
  memoryLocation(memoryLocation_),
  targetDevice(targetDevice_) {

  QIn = reinterpret_cast<inType* (*)[2 * Dimensions]>(tarch::accelerator::MemoryManager::getInstance().allocate<inType*>(
    2 * Dimensions * numberOfCells_,
    memoryLocation_,
    targetDevice_
  ));
  cellCentre = tarch::accelerator::MemoryManager::getInstance().allocate<tarch::la::Vector<Dimensions, double>>(
    numberOfCells_,
    memoryLocation_,
    targetDevice_
  );
  cellSize = tarch::accelerator::MemoryManager::getInstance().allocate<tarch::la::Vector<Dimensions, double>>(
    numberOfCells_,
    memoryLocation_,
    targetDevice_
  );
  t = tarch::accelerator::MemoryManager::getInstance().allocate<double>(numberOfCells_, memoryLocation_, targetDevice_);
  dt = tarch::accelerator::MemoryManager::getInstance().allocate<double>(
    numberOfCells_,
    memoryLocation_,
    targetDevice_
  );
  id   = tarch::accelerator::MemoryManager::getInstance().allocate<int>(numberOfCells_, memoryLocation_, targetDevice_);
  QOut = reinterpret_cast<inType* (*)[2 * Dimensions]>(tarch::accelerator::MemoryManager::getInstance().allocate<outType*>(
    2 * Dimensions * numberOfCells_,
    memoryLocation_,
    targetDevice_
  ));
}

template <typename inType, typename outType> exahype2::CellFaceData<inType, outType>::~CellFaceData() {
  // tarch::freeMemory(QIn, memoryLocation, targetDevice);
  // tarch::freeMemory(cellCentre, memoryLocation, targetDevice);
  // tarch::freeMemory(cellSize, memoryLocation, targetDevice);
  // tarch::freeMemory(t, memoryLocation, targetDevice);
  // tarch::freeMemory(dt, memoryLocation, targetDevice);
  // tarch::freeMemory(QOut, memoryLocation, targetDevice);
  // tarch::freeMemory(id, memoryLocation, targetDevice);

  tarch::accelerator::MemoryManager::getInstance().free(QIn, targetDevice);
  tarch::accelerator::MemoryManager::getInstance().free(cellCentre, targetDevice);
  tarch::accelerator::MemoryManager::getInstance().free(cellSize, targetDevice);
  tarch::accelerator::MemoryManager::getInstance().free(t, targetDevice);
  tarch::accelerator::MemoryManager::getInstance().free(dt, targetDevice);
  tarch::accelerator::MemoryManager::getInstance().free(QOut, targetDevice);
  tarch::accelerator::MemoryManager::getInstance().free(id, targetDevice);
}

template <typename inType, typename outType> std::string exahype2::CellFaceData<inType, outType>::toString() const {
  std::ostringstream msg;
  msg << "[";
  for (int i = 0; i < numberOfCells; i++) {
    msg << "(x=" << cellCentre[i] << ",h=" << cellSize[i] << ",t=" << t[i] << ",dt=" << dt[i] << ",id=" << id[i] << ")";
  }
  msg << "]";
  return msg.str();
}
