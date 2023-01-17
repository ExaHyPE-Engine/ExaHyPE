#include "LoopBody.h"
#include "exahype2/fd/FD_Helper.cpph"

void exahype2::fd::reduceMaxEigenvalue_patchwise_functors(
  ::exahype2::CellData&   patchData,
  int                     numberOfGridCellsPerPatchPerAxis,
  int                     unknowns,
  int                     auxiliaryVariables,
  MaxEigenvalue           maxEigenvalue
) {
  exahype2::enumerator::AoSLexicographicEnumerator QOutEnumerator(1, numberOfGridCellsPerPatchPerAxis, 0, unknowns, auxiliaryVariables);

  for (int patchIndex=0; patchIndex<patchData.numberOfCells; patchIndex++) {
    double newMaxEigenvalue = 0.0;
    otterLoopBegin(); // TODO: how do I clarify that this is a reduction? Do we need this info here?
    for (int x = 0; x < numberOfGridCellsPerPatchPerAxis; x++)
    for (int y = 0; y < numberOfGridCellsPerPatchPerAxis; y++)
    for (int z = 0; z < numberOfGridCellsPerPatchPerAxis; z++) {
      otterLoopIterationBegin();
      newMaxEigenvalue = std::max(
        newMaxEigenvalue,
        ::exahype2::fd::internal::reduceMaxEigenvalue_LoopBody(
          *(patchData.QOut + patchIndex),
          QOutEnumerator,
          maxEigenvalue,
          patchData.cellCentre[patchIndex],
          patchData.cellSize[patchIndex],
          patchIndex,
          gridCellIndex3d(x,y,z),
          patchData.t[patchIndex],
          patchData.dt[patchIndex]
        )
      );
      otterLoopIterationEnd();
    }
    otterLoopEnd();
    patchData.maxEigenvalue[patchIndex] = newMaxEigenvalue;
  }
}
