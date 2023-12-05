#include "LoopBody.h"
#include "exahype2/fd/FD_Helper.cpph"
#include "peano4/utils/Loop.h"

void exahype2::fd::reduceMaxEigenvalue_patchwise_functors(
  ::exahype2::CellData&   patchData,
  int                     numberOfGridCellsPerPatchPerAxis,
  int                     overlap,
  int                     unknowns,
  int                     auxiliaryVariables,
  MaxEigenvalue           maxEigenvalue
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
          patchData.t[patchIndex],
          patchData.dt[patchIndex]
        )
      );
    }
    patchData.maxEigenvalue[patchIndex] = newMaxEigenvalue;
  }
}

template <>
double exahype2::fd::internal::reduceMaxEigenvalue_LoopBody(
    const double* __restrict__                   QOut,
    const exahype2::enumerator::AoSLexicographicEnumerator& QOutEnumerator,
    exahype2::fd::MaxEigenvalue                  maxEigenvalue,
    const tarch::la::Vector<Dimensions,double>&  patchCentre,
    const tarch::la::Vector<Dimensions,double>&  patchSize,
    int                                          patchIndex,
    const tarch::la::Vector<Dimensions,int>&     volumeIndex,
    double                                       t,
    double                                       dt
) {
    double result = 0.0;
    for (int normal=0; normal<Dimensions; normal++) {
      result = std::max(
        result,
        maxEigenvalue(
          & (QOut[ QOutEnumerator(patchIndex,volumeIndex,0) ]),
          ::exahype2::fd::getGridCellCentre( patchCentre, patchSize, QOutEnumerator._numberOfDoFsPerAxisInCell, volumeIndex ),
          ::exahype2::fd::getGridCellSize( patchSize, QOutEnumerator._numberOfDoFsPerAxisInCell ),
          t,
          dt,
          normal
        )
      );
    }

    return result;
}
