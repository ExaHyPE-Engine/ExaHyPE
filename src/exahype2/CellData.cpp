#include "CellData.h"

#include "tarch/accelerator/accelerator.h"

exahype2::CellData::CellData(
  double*                                      QIn_,
  const tarch::la::Vector<Dimensions, double>& cellCentre_,
  const tarch::la::Vector<Dimensions, double>& cellSize_,
  double                                       t_,
  double                                       dt_,
  double*                                      QOut_
):
  CellData(1) {
  QIn[0]        = QIn_;
  cellCentre[0] = cellCentre_;
  cellSize[0]   = cellSize_;
  t[0]          = t_;
  dt[0]         = dt_;
  QOut[0]       = QOut_;
  id[0]         = -1;
}

exahype2::CellData::CellData(int numberOfCells_):
  numberOfCells(numberOfCells_) {
  QIn           = tarch::allocateMemory<double*>(numberOfCells_, tarch::MemoryLocation::ManagedSharedAcceleratorDeviceMemory);
  cellCentre    = tarch::allocateMemory< tarch::la::Vector<Dimensions, double> >(numberOfCells_, tarch::MemoryLocation::ManagedSharedAcceleratorDeviceMemory);
  cellSize      = tarch::allocateMemory< tarch::la::Vector<Dimensions, double> >(numberOfCells_, tarch::MemoryLocation::ManagedSharedAcceleratorDeviceMemory);
  t             = tarch::allocateMemory< double >(numberOfCells_, tarch::MemoryLocation::ManagedSharedAcceleratorDeviceMemory);
  dt            = tarch::allocateMemory< double >(numberOfCells_, tarch::MemoryLocation::ManagedSharedAcceleratorDeviceMemory);
  id            = tarch::allocateMemory< int >(numberOfCells_, tarch::MemoryLocation::ManagedSharedAcceleratorDeviceMemory);
  QOut          = tarch::allocateMemory<double*>(numberOfCells_, tarch::MemoryLocation::ManagedSharedAcceleratorDeviceMemory);
  maxEigenvalue = tarch::allocateMemory< double >(numberOfCells_, tarch::MemoryLocation::ManagedSharedAcceleratorDeviceMemory);
}

exahype2::CellData::~CellData() {
  tarch::freeMemory(QIn, tarch::MemoryLocation::ManagedSharedAcceleratorDeviceMemory);
  tarch::freeMemory(cellCentre, tarch::MemoryLocation::ManagedSharedAcceleratorDeviceMemory);
  tarch::freeMemory(cellSize, tarch::MemoryLocation::ManagedSharedAcceleratorDeviceMemory);
  tarch::freeMemory(t, tarch::MemoryLocation::ManagedSharedAcceleratorDeviceMemory);
  tarch::freeMemory(dt, tarch::MemoryLocation::ManagedSharedAcceleratorDeviceMemory);
  tarch::freeMemory(QOut, tarch::MemoryLocation::ManagedSharedAcceleratorDeviceMemory);
  tarch::freeMemory(id, tarch::MemoryLocation::ManagedSharedAcceleratorDeviceMemory);
  tarch::freeMemory(maxEigenvalue, tarch::MemoryLocation::ManagedSharedAcceleratorDeviceMemory);
}

std::string exahype2::CellData::toString() const {
  std::ostringstream msg;
  msg << "[";
  for (int i = 0; i < numberOfCells; i++) {
    msg << "(x=" << cellCentre[i] << ",h=" << cellSize[i] << ",t=" << t[i] << ",dt=" << dt[i] << ",id=" << id[i]
        << ",lambda=" << maxEigenvalue[i] << ")";
  }
  msg << "]";
  return msg.str();
}
