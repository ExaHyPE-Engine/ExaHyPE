#include "CellData.h"

#include "tarch/accelerator/accelerator.h"

exahype2::CellData::CellData(
  double*                                      QIn_,
  const tarch::la::Vector<Dimensions, double>& cellCentre_,
  const tarch::la::Vector<Dimensions, double>& cellSize_,
  double                                       t_,
  double                                       dt_,
  double*                                      QOut_,
  int                                          targetDevice_
):
  CellData(1, targetDevice_) {
  QIn[0]        = QIn_;
  cellCentre[0] = cellCentre_;
  cellSize[0]   = cellSize_;
  t[0]          = t_;
  dt[0]         = dt_;
  QOut[0]       = QOut_;
  id[0]         = -1;
}

exahype2::CellData::CellData(int numberOfCells_, int targetDevice_):
  numberOfCells(numberOfCells_),
  targetDevice(targetDevice_) {
  QIn           = tarch::allocateMemory<double*>(numberOfCells_, tarch::MemoryLocation::ManagedSharedAcceleratorDeviceMemory, targetDevice);
  cellCentre    = tarch::allocateMemory<tarch::la::Vector<Dimensions, double>>(numberOfCells_, tarch::MemoryLocation::ManagedSharedAcceleratorDeviceMemory, targetDevice);
  cellSize      = tarch::allocateMemory<tarch::la::Vector<Dimensions, double>>(numberOfCells_, tarch::MemoryLocation::ManagedSharedAcceleratorDeviceMemory, targetDevice);
  t             = tarch::allocateMemory<double>(numberOfCells_, tarch::MemoryLocation::ManagedSharedAcceleratorDeviceMemory, targetDevice);
  dt            = tarch::allocateMemory<double>(numberOfCells_, tarch::MemoryLocation::ManagedSharedAcceleratorDeviceMemory, targetDevice);
  id            = tarch::allocateMemory<int>(numberOfCells_, tarch::MemoryLocation::ManagedSharedAcceleratorDeviceMemory, targetDevice);
  QOut          = tarch::allocateMemory<double*>(numberOfCells_, tarch::MemoryLocation::ManagedSharedAcceleratorDeviceMemory, targetDevice);
  maxEigenvalue = tarch::allocateMemory<double>(numberOfCells_, tarch::MemoryLocation::ManagedSharedAcceleratorDeviceMemory, targetDevice);
}

exahype2::CellData::~CellData() {
  tarch::freeMemory(QIn, tarch::MemoryLocation::ManagedSharedAcceleratorDeviceMemory, targetDevice);
  tarch::freeMemory(cellCentre, tarch::MemoryLocation::ManagedSharedAcceleratorDeviceMemory, targetDevice);
  tarch::freeMemory(cellSize, tarch::MemoryLocation::ManagedSharedAcceleratorDeviceMemory, targetDevice);
  tarch::freeMemory(t, tarch::MemoryLocation::ManagedSharedAcceleratorDeviceMemory, targetDevice);
  tarch::freeMemory(dt, tarch::MemoryLocation::ManagedSharedAcceleratorDeviceMemory, targetDevice);
  tarch::freeMemory(QOut, tarch::MemoryLocation::ManagedSharedAcceleratorDeviceMemory, targetDevice);
  tarch::freeMemory(id, tarch::MemoryLocation::ManagedSharedAcceleratorDeviceMemory, targetDevice);
  tarch::freeMemory(maxEigenvalue, tarch::MemoryLocation::ManagedSharedAcceleratorDeviceMemory, targetDevice);
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
