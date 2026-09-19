// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "MemoryUsage.h"

#include "tarch/mpi/Collectives.h"
#include "tarch/mpi/Rank.h"

void peano4::data::MemoryUsage::update(
  const std::uint64_t cellBytes,
  const std::uint64_t faceBytes,
  const std::uint64_t vertexBytes
) {
  if (tarch::mpi::Rank::getInstance().getNumberOfRanks() > 1) {
    if (_hasReducedDataMemory) {
      _globalCellDataMemoryUsageInBytes   = _reducedDataMemory[0];
      _globalFaceDataMemoryUsageInBytes   = _reducedDataMemory[1];
      _globalVertexDataMemoryUsageInBytes = _reducedDataMemory[2];
    } else {
      _globalCellDataMemoryUsageInBytes   = cellBytes;
      _globalFaceDataMemoryUsageInBytes   = faceBytes;
      _globalVertexDataMemoryUsageInBytes = vertexBytes;
    }

    // Every rank reaches this once per sweep, right after its traversal has joined.
    _reducedDataMemory = {cellBytes, faceBytes, vertexBytes};
    tarch::mpi::allreduce(
      MPI_IN_PLACE,
      _reducedDataMemory.data(),
      NumberOfDataMemoryWords,
      MPI_UINT64_T,
      MPI_SUM,
      tarch::mpi::Rank::getInstance().getCommunicator()
    );
    _hasReducedDataMemory = true;
  } else {
    _globalCellDataMemoryUsageInBytes   = cellBytes;
    _globalFaceDataMemoryUsageInBytes   = faceBytes;
    _globalVertexDataMemoryUsageInBytes = vertexBytes;
  }
}

std::size_t peano4::data::MemoryUsage::getGlobalCellDataMemoryUsageInBytes() {
  return _globalCellDataMemoryUsageInBytes;
}

std::size_t peano4::data::MemoryUsage::getGlobalFaceDataMemoryUsageInBytes() {
  return _globalFaceDataMemoryUsageInBytes;
}

std::size_t peano4::data::MemoryUsage::getGlobalVertexDataMemoryUsageInBytes() {
  return _globalVertexDataMemoryUsageInBytes;
}
