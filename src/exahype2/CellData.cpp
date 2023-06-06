#include "CellData.h"
#include "tarch/accelerator/accelerator.h"

exahype2::CellData::CellData(
  double*   QIn_,
  const tarch::la::Vector<Dimensions,double>&   cellCentre_,
  const tarch::la::Vector<Dimensions,double>&   cellSize_,
  double    t_,
  double    dt_,
  double*   QOut_
):
  CellData(1) {
  QIn[0]         = QIn_;
  cellCentre[0]  = cellCentre_;
  cellSize[0]    = cellSize_;
  t[0]           = t_;
  dt[0]          = dt_;
  QOut[0]        = QOut_;
  id[0]          = -1;
}

exahype2::CellData::CellData( int numberOfCells_ ):
  numberOfCells(numberOfCells_) {
  QIn           = tarch::allocateMemory<double*>(numberOfCells_, tarch::MemoryLocation::Heap);
  cellCentre    = new (std::align_val_t(AlignmentOnHeap)) tarch::la::Vector<Dimensions,double>[numberOfCells];
  cellSize      = new (std::align_val_t(AlignmentOnHeap)) tarch::la::Vector<Dimensions,double>[numberOfCells];
  t             = new (std::align_val_t(AlignmentOnHeap)) double[numberOfCells];
  dt            = new (std::align_val_t(AlignmentOnHeap)) double[numberOfCells];
  id            = new (std::align_val_t(AlignmentOnHeap)) int[numberOfCells];
  QOut          = new (std::align_val_t(AlignmentOnHeap)) double*[numberOfCells];
  maxEigenvalue = new (std::align_val_t(AlignmentOnHeap)) double[numberOfCells];
}

exahype2::CellData::~CellData() {
  tarch::freeMemory( QIn, tarch::MemoryLocation::Heap );
  delete[] cellCentre;
  delete[] cellSize;
  delete[] t;
  delete[] dt;
  delete[] QOut;
  delete[] id;
  delete[] maxEigenvalue;
}

std::string exahype2::CellData::toString() const {
  std::ostringstream msg;
  msg << "[";
  for (int i=0; i<numberOfCells; i++) {
    msg << "(x=" << cellCentre[i]
        << ",h=" << cellSize[i]
        << ",t=" << t[i]
        << ",dt=" << dt[i]
        << ",id=" << id[i]
        << ",lambda=" << maxEigenvalue[i]
        << ")";
  }
  msg << "]";
  return msg.str();
}

