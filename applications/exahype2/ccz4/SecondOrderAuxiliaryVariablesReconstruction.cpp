#include "SecondOrderAuxiliaryVariablesReconstruction.h"
#include "exahype2/fd/FD_Helper.cpph"


void applications::exahype2::ccz4::recomputeAuxiliaryVariablesFD4_4thOrder(
  ::exahype2::CellData&   patchData,
  int                     numberOfGridCellsPerPatchPerAxis,
  int                     haloSize,
  int                     unknowns,
  int                     auxiliaryVariables
) {
  assertion( haloSize >= 3 );
  ::exahype2::enumerator::AoSLexicographicEnumerator QInEnumerator (1,numberOfGridCellsPerPatchPerAxis,haloSize,unknowns,auxiliaryVariables);

  for (int patchIndex=0; patchIndex<patchData.numberOfCells; patchIndex++) {
    if (Dimensions==2) {
        for (int x = 0; x < numberOfGridCellsPerPatchPerAxis; x++)
        for (int y = 0; y < numberOfGridCellsPerPatchPerAxis; y++) {
          internal::recomputeAuxiliaryVariablesFD4_4thOrder_LoopBody(
            patchData.QIn[patchIndex],
            QInEnumerator,
            patchData.cellCentre[patchIndex],
            patchData.cellSize[patchIndex],
            patchIndex,
            gridCellIndex2d(x,y),
            0  // normal
          );
        }

        for (int x = 0; x < numberOfGridCellsPerPatchPerAxis; x++)
        for (int y = 0; y < numberOfGridCellsPerPatchPerAxis; y++) {
          internal::recomputeAuxiliaryVariablesFD4_4thOrder_LoopBody(
            patchData.QIn[patchIndex],
            QInEnumerator,
            patchData.cellCentre[patchIndex],
            patchData.cellSize[patchIndex],
            patchIndex,
            gridCellIndex2d(x,y),
            1 // normal
          );
        }
    }
    else {
        for (int x = 0; x < numberOfGridCellsPerPatchPerAxis; x++)
        for (int y = 0; y < numberOfGridCellsPerPatchPerAxis; y++)
        for (int z = 0; z < numberOfGridCellsPerPatchPerAxis; z++) {
          internal::recomputeAuxiliaryVariablesFD4_4thOrder_LoopBody(
            patchData.QIn[patchIndex],
            QInEnumerator,
            patchData.cellCentre[patchIndex],
            patchData.cellSize[patchIndex],
            patchIndex,
            gridCellIndex3d(x,y,z),
            0 // normal
          );
        }

        for (int x = 0; x < numberOfGridCellsPerPatchPerAxis; x++)
        for (int y = 0; y < numberOfGridCellsPerPatchPerAxis; y++)
        for (int z = 0; z < numberOfGridCellsPerPatchPerAxis; z++) {
          internal::recomputeAuxiliaryVariablesFD4_4thOrder_LoopBody(
            patchData.QIn[patchIndex],
            QInEnumerator,
            patchData.cellCentre[patchIndex],
            patchData.cellSize[patchIndex],
            patchIndex,
            gridCellIndex3d(x,y,z),
            1 // normal
          );
        }

        for (int x = 0; x < numberOfGridCellsPerPatchPerAxis; x++)
        for (int y = 0; y < numberOfGridCellsPerPatchPerAxis; y++)
        for (int z = 0; z < numberOfGridCellsPerPatchPerAxis; z++) {
          internal::recomputeAuxiliaryVariablesFD4_4thOrder_LoopBody(
            patchData.QIn[patchIndex],
            QInEnumerator,
            patchData.cellCentre[patchIndex],
            patchData.cellSize[patchIndex],
            patchIndex,
            gridCellIndex3d(x,y,z),
            2 // normal
          );
        }
    }
  }
}


void applications::exahype2::ccz4::internal::recomputeAuxiliaryVariablesFD4_4thOrder_LoopBody(
  double* __restrict__                                       QIn,
  const ::exahype2::enumerator::AoSLexicographicEnumerator&  QInEnumerator,
  const tarch::la::Vector<Dimensions,double>&  patchCentre,
  const tarch::la::Vector<Dimensions,double>&  patchSize,
  int                                          patchIndex,
  const tarch::la::Vector<Dimensions,int>&     volumeIndex,
  int                                          normal
) {
  tarch::la::Vector<Dimensions,int>       centralVolume  = volumeIndex;
  tarch::la::Vector<Dimensions,int>  leftAdjacentVolume1 = volumeIndex;
  tarch::la::Vector<Dimensions,int>  leftAdjacentVolume2 = volumeIndex;
  tarch::la::Vector<Dimensions,int> rightAdjacentVolume1 = volumeIndex;
  tarch::la::Vector<Dimensions,int> rightAdjacentVolume2 = volumeIndex;

  rightAdjacentVolume1(normal)++;
  rightAdjacentVolume2(normal)++;
  rightAdjacentVolume2(normal)++;
   leftAdjacentVolume1(normal)--;
   leftAdjacentVolume2(normal)--;
   leftAdjacentVolume2(normal)--;

  double cellsize = patchSize(normal) / QInEnumerator._numberOfDoFsPerAxisInCell;
  double gradcoef = 1.0/(12.0*cellsize);

  //std::cout<<"called here"<<std::endl;
  // A[normal] = \partial_{normal}\alpha
  // alpha is Q[16]
  //std::cout<<"before:"<<QIn[ QInEnumerator(patchIndex,centralVolume,23+normal) ]<<std::endl;
  QIn[ QInEnumerator(patchIndex,centralVolume,23+normal) ] =     gradcoef * QIn[ QInEnumerator(patchIndex, leftAdjacentVolume2,16) ] -
                                                             8.0*gradcoef * QIn[ QInEnumerator(patchIndex, leftAdjacentVolume1,16) ] +
                                                             8.0*gradcoef * QIn[ QInEnumerator(patchIndex,rightAdjacentVolume1,16) ] -
                                                                 gradcoef * QIn[ QInEnumerator(patchIndex,rightAdjacentVolume2,16) ];
  //std::cout<<"after:"<<QIn[ QInEnumerator(patchIndex,centralVolume,23+normal) ]<<std::endl;

  // P[normal] = \partial_{normal}\phi
  // phi is Q[54]
  QIn[ QInEnumerator(patchIndex,centralVolume,55+normal) ] =     gradcoef * QIn[ QInEnumerator(patchIndex, leftAdjacentVolume2,54) ] -
                                                             8.0*gradcoef * QIn[ QInEnumerator(patchIndex, leftAdjacentVolume1,54) ] +
                                                             8.0*gradcoef * QIn[ QInEnumerator(patchIndex,rightAdjacentVolume1,54) ] -
                                                                 gradcoef * QIn[ QInEnumerator(patchIndex,rightAdjacentVolume2,54) ];

  // B[normal][i] = \partial_{normal}\beta^i
  // beta^i is Q[17] - Q[19]
  for (int i=0; i<3; i++) {
    QIn[ QInEnumerator(patchIndex,centralVolume,26+3*normal+i) ] =     gradcoef * QIn[ QInEnumerator(patchIndex, leftAdjacentVolume2,17+i) ] -
                                                                   8.0*gradcoef * QIn[ QInEnumerator(patchIndex, leftAdjacentVolume1,17+i) ] +
                                                                   8.0*gradcoef * QIn[ QInEnumerator(patchIndex,rightAdjacentVolume1,17+i) ] -
                                                                       gradcoef * QIn[ QInEnumerator(patchIndex,rightAdjacentVolume2,17+i) ];
  }

  // D[normal][i][j] = 0.5*\partial_{normal}\tilde{\gamma}_{ij} -- note the 1/2 factor
  // \tilde{\gamma}_{ij} is Q[0] - Q[5]
  for (int i=0; i<3; i++)
  for (int j=i; j<3; j++) {
    int k = (i==0?j:i+j+1);
    QIn[ QInEnumerator(patchIndex,centralVolume,35+6*normal+k) ] = 0.5*gradcoef * QIn[ QInEnumerator(patchIndex, leftAdjacentVolume2,k) ] -
                                                                   4.0*gradcoef * QIn[ QInEnumerator(patchIndex, leftAdjacentVolume1,k) ] +
                                                                   4.0*gradcoef * QIn[ QInEnumerator(patchIndex,rightAdjacentVolume1,k) ] -
                                                                   0.5*gradcoef * QIn[ QInEnumerator(patchIndex,rightAdjacentVolume2,k) ];
  }
}

