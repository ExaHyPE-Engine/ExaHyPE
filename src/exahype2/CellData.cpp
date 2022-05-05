#include "CellData.h"


exahype2::CellData::CellData(
  double*   QIn_,
  const tarch::la::Vector<Dimensions,double>&   patchCentre_,
  const tarch::la::Vector<Dimensions,double>&   patchSize_,
  double    t_,
  double    dt_,
  double*   QOut_
):
  CellData(1) {
  QIn[0]         = QIn_;
  patchCentre[0] = patchCentre_;
  patchSize[0]   = patchSize_;
  t[0]           = t_;
  dt[0]          = dt_;
  QOut[0]        = QOut_;
  id[0]          = -1;
}


exahype2::CellData::CellData( int numberOfCells_ ):
  numberOfCells(numberOfCells_) {
  QIn           = new (std::align_val_t(Alignment)) double*[numberOfCells];
  patchCentre   = new (std::align_val_t(Alignment)) tarch::la::Vector<Dimensions,double>[numberOfCells];
  patchSize     = new (std::align_val_t(Alignment)) tarch::la::Vector<Dimensions,double>[numberOfCells];
  t             = new (std::align_val_t(Alignment)) double[numberOfCells];
  dt            = new (std::align_val_t(Alignment)) double[numberOfCells];
  id            = new (std::align_val_t(Alignment)) int[numberOfCells];
  QOut          = new (std::align_val_t(Alignment)) double*[numberOfCells];
  maxEigenvalue = new (std::align_val_t(Alignment)) double[numberOfCells];
}


exahype2::CellData::~CellData() {
  delete QIn;
  delete patchCentre;
  delete patchSize;
  delete t;
  delete dt;
  delete QOut;
  delete maxEigenvalue;
}


std::string exahype2::CellData::toString() const {
  std::ostringstream msg;
  msg << "[";
  for (int i=0; i<numberOfCells; i++) {
    msg << "(x=" << patchCentre[i]
        << ",h=" << patchSize[i]
        << ",t=" << t[i]
        << ",dt=" << dt[i]
        << ",id=" << id[i]
        << ",lambda=" << maxEigenvalue[i]
        << ")";
  }
  msg << "]";
  return msg.str();
}

