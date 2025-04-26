// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "config.h"
#include "peano4/utils/Globals.h"
#include "tarch/accelerator/accelerator.h"
#include "tarch/accelerator/MemoryManager.h"
#include "tarch/la/Vector.h"

#include <functional>
#include <string>

namespace exahype2 {
  /**
   * Represents the sides of one face, with 2 sides (left and right) to a face
   * For ADER QIn will contain the values, QOut would contain the fluxes
   * QIn[faceId][0][...] will contain the values of the left side relative to the face
   * QIn[faceId][1][...] will contain the values of the right side relative to the face
   */
  template <typename inType = double, typename outType = double> struct FaceData {
    /**
     * QIn may not be const, as some kernels delete it straightaway once
     * the input data has been handled.
     */
    inType* (*QIn)[2];
    tarch::la::Vector<Dimensions, double>* faceCentre;
    tarch::la::Vector<Dimensions, double>* faceSize;

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
    const int numberOfFaces;

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
    outType* (*QOut)[2];

    /**
     * Construct patch data object for one single cell.
     *
     * Usually, I do so only to be able to use the same kernels everywhere:
     * Kernels accept FaceData, i.e. multiple patches. Even if we have only
     * one cell, we thus wrap this cell's data into an instance of FaceData
     * and pass it in. The id can be set to any dummy in this case, as we
     * know which task has wrapped this single cell, i.e. we usually do not
     * read it later.
     */
    FaceData(
      inType*                                      QIn_[2],
      const tarch::la::Vector<Dimensions, double>& faceCentre_,
      const tarch::la::Vector<Dimensions, double>& faceSize_,
      double                                       t_,
      double                                       dt_,
      outType*                                     QOut_[2],
      tarch::MemoryLocation                        memoryLocation_ = tarch::MemoryLocation::Heap,
      int                                          targetDevice_   = tarch::accelerator::Device::DefaultDevice
    );

    FaceData(
      int                   numberOfFaces_,
      tarch::MemoryLocation memoryLocation_ = tarch::MemoryLocation::Heap,
      int                   targetDevice_   = tarch::accelerator::Device::DefaultDevice
    );

    FaceData(const FaceData& copy)       = delete;
    FaceData& operator=(const FaceData&) = delete;

    ~FaceData();

    std::string toString() const;
  };

} // namespace exahype2

template <typename inType, typename outType>
exahype2::FaceData<inType, outType>::FaceData(
  inType*                                      QIn_[2],
  const tarch::la::Vector<Dimensions, double>& faceCentre_,
  const tarch::la::Vector<Dimensions, double>& faceSize_,
  double                                       t_,
  double                                       dt_,
  outType*                                     QOut_[2],
  tarch::MemoryLocation                        memoryLocation_,
  int                                          targetDevice_
):
  FaceData(1, memoryLocation_, targetDevice_) {
  QIn[0]        = QIn_;
  faceCentre[0] = faceCentre_;
  faceSize[0]   = faceSize_;
  t[0]          = t_;
  dt[0]         = dt_;
  QOut[0]       = QOut_;
  id[0]         = -1;
}

template <typename inType, typename outType>
exahype2::FaceData<inType, outType>::
  FaceData(int numberOfFaces_, tarch::MemoryLocation memoryLocation_, int targetDevice_):
  numberOfFaces(numberOfFaces_),
  memoryLocation(memoryLocation_),
  targetDevice(targetDevice_) {

  QIn = tarch::accelerator::MemoryManager::getInstance().allocate<inType* [2]>(
    numberOfFaces_,
    memoryLocation_,
    targetDevice_
  );
  faceCentre = tarch::accelerator::MemoryManager::getInstance().allocate<tarch::la::Vector<Dimensions, double>>(
    numberOfFaces_,
    memoryLocation_,
    targetDevice_
  );
  faceSize = tarch::accelerator::MemoryManager::getInstance().allocate<tarch::la::Vector<Dimensions, double>>(
    numberOfFaces_,
    memoryLocation_,
    targetDevice_
  );
  t = tarch::accelerator::MemoryManager::getInstance().allocate<double>(numberOfFaces_, memoryLocation_, targetDevice_);
  dt = tarch::accelerator::MemoryManager::getInstance().allocate<double>(
    numberOfFaces_,
    memoryLocation_,
    targetDevice_
  );
  id   = tarch::accelerator::MemoryManager::getInstance().allocate<int>(numberOfFaces_, memoryLocation_, targetDevice_);
  QOut = tarch::accelerator::MemoryManager::getInstance().allocate<outType* [2]>(
    numberOfFaces_,
    memoryLocation_,
    targetDevice_
  );
}

template <typename inType, typename outType> exahype2::FaceData<inType, outType>::~FaceData() {
  // tarch::freeMemory(QIn, memoryLocation, targetDevice);
  // tarch::freeMemory(faceCentre, memoryLocation, targetDevice);
  // tarch::freeMemory(faceSize, memoryLocation, targetDevice);
  // tarch::freeMemory(t, memoryLocation, targetDevice);
  // tarch::freeMemory(dt, memoryLocation, targetDevice);
  // tarch::freeMemory(QOut, memoryLocation, targetDevice);
  // tarch::freeMemory(id, memoryLocation, targetDevice);

  tarch::accelerator::MemoryManager::getInstance().free(QIn, targetDevice);
  tarch::accelerator::MemoryManager::getInstance().free(faceCentre, targetDevice);
  tarch::accelerator::MemoryManager::getInstance().free(faceSize, targetDevice);
  tarch::accelerator::MemoryManager::getInstance().free(t, targetDevice);
  tarch::accelerator::MemoryManager::getInstance().free(dt, targetDevice);
  tarch::accelerator::MemoryManager::getInstance().free(QOut, targetDevice);
  tarch::accelerator::MemoryManager::getInstance().free(id, targetDevice);
}

template <typename inType, typename outType> std::string exahype2::FaceData<inType, outType>::toString() const {
  std::ostringstream msg;
  msg << "[";
  for (int i = 0; i < numberOfFaces; i++) {
    msg << "(x=" << faceCentre[i] << ",h=" << faceSize[i] << ",t=" << t[i] << ",dt=" << dt[i] << ",id=" << id[i] << ")";
  }
  msg << "]";
  return msg.str();
}
