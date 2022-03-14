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


exahype2::CellData::CellData( int numberOfPatches_ ):
  numberOfPatches(numberOfPatches_) {
  QIn           = new (std::align_val_t(Alignment)) double*[numberOfPatches];
  patchCentre   = new (std::align_val_t(Alignment)) tarch::la::Vector<Dimensions,double>[numberOfPatches];
  patchSize     = new (std::align_val_t(Alignment)) tarch::la::Vector<Dimensions,double>[numberOfPatches];
  t             = new (std::align_val_t(Alignment)) double[numberOfPatches];
  dt            = new (std::align_val_t(Alignment)) double[numberOfPatches];
  id            = new (std::align_val_t(Alignment)) int[numberOfPatches];
  QOut          = new (std::align_val_t(Alignment)) double*[numberOfPatches];
  maxEigenvalue = new (std::align_val_t(Alignment)) double[numberOfPatches];
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
