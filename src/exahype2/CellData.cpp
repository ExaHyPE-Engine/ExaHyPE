#include "CellData.h"

#include "tarch/accelerator/accelerator.h"

exahype2::CellData::CellData(
  double*                                      QIn_,
  const tarch::la::Vector<Dimensions, double>& cellCentre_,
  const tarch::la::Vector<Dimensions, double>& cellSize_,
  double                                       t_,
  double                                       dt_,
  double*                                      QOut_,
  tarch::MemoryLocation                        memoryLocation_,
  int                                          targetDevice_
):
  CellData(1, memoryLocation_, targetDevice_) {
  QIn[0]        = QIn_;
  cellCentre[0] = cellCentre_;
  cellSize[0]   = cellSize_;
  t[0]          = t_;
  dt[0]         = dt_;
  QOut[0]       = QOut_;
  id[0]         = -1;
}

exahype2::CellData::CellData(
  int                   numberOfCells_,
  tarch::MemoryLocation memoryLocation_,
  int                   targetDevice_
):
  numberOfCells(numberOfCells_),
  memoryLocation(memoryLocation_),
  targetDevice(targetDevice_) {
  QIn           = tarch::allocateMemory<double*>(numberOfCells_, memoryLocation_, targetDevice_);
  cellCentre    = tarch::allocateMemory<tarch::la::Vector<Dimensions, double>>(numberOfCells_, memoryLocation_, targetDevice_);
  cellSize      = tarch::allocateMemory<tarch::la::Vector<Dimensions, double>>(numberOfCells_, memoryLocation_, targetDevice_);
  t             = tarch::allocateMemory<double>(numberOfCells_, memoryLocation_, targetDevice_);
  dt            = tarch::allocateMemory<double>(numberOfCells_, memoryLocation_, targetDevice_);
  id            = tarch::allocateMemory<int>(numberOfCells_, memoryLocation_, targetDevice_);
  QOut          = tarch::allocateMemory<double*>(numberOfCells_, memoryLocation_, targetDevice_);
  maxEigenvalue = tarch::allocateMemory<double>(numberOfCells_, memoryLocation_, targetDevice_);
}

exahype2::CellData::~CellData() {
  tarch::freeMemory(QIn, memoryLocation, targetDevice);
  tarch::freeMemory(cellCentre, memoryLocation, targetDevice);
  tarch::freeMemory(cellSize, memoryLocation, targetDevice);
  tarch::freeMemory(t, memoryLocation, targetDevice);
  tarch::freeMemory(dt, memoryLocation, targetDevice);
  tarch::freeMemory(QOut, memoryLocation, targetDevice);
  tarch::freeMemory(id, memoryLocation, targetDevice);
  tarch::freeMemory(maxEigenvalue, memoryLocation, targetDevice);
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
