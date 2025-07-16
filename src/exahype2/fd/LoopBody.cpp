#include "LoopBody.h"
#include "exahype2/fd/FD_Helper.cpph"
#include "peano4/utils/Loop.h"

void exahype2::fd::reduceMaxEigenvalue_patchwise_functors(
  ::exahype2::CellData<double, double>& patchData,
  int                                   numberOfGridCellsPerPatchPerAxis,
  int                                   overlap,
  int                                   unknowns,
  int                                   auxiliaryVariables,
  MaxEigenvalue                         maxEigenvalue
) {
  exahype2::enumerator::AoSLexicographicEnumerator QOutEnumerator(1, numberOfGridCellsPerPatchPerAxis, overlap, unknowns, auxiliaryVariables);

  for (int patchIndex=0; patchIndex<patchData.numberOfCells; patchIndex++) {
    double newMaxEigenvalue = 0.0;
    dfor(cell,numberOfGridCellsPerPatchPerAxis) {
      newMaxEigenvalue = std::max(
        newMaxEigenvalue,
        ::exahype2::fd::internal::reduceMaxEigenvalue_LoopBody(
          *(patchData.QOut + patchIndex),
          QOutEnumerator,
          maxEigenvalue,
          patchData.cellCentre[patchIndex],
          patchData.cellSize[patchIndex],
          patchIndex,
          cell,
          patchData.t,
          patchData.dt
        )
      );
    }
    patchData.maxEigenvalue[patchIndex] = newMaxEigenvalue;
  }
}
