#include "Riemann.h"


void exahype2::dg::projectVolumetricDataOntoFaces(
  const double* __restrict__          cellQ,
  int                                 order,
  int                                 unknowns,
  const double* const __restrict__    BasisFunctionValuesLeft,
  double* const __restrict__          faceQLeft,
  double* const __restrict__          faceQRight,
  double* const __restrict__          faceQBottom,
  double* const __restrict__          faceQUp
) {
  // project left/right
  for (int y=0; y<order+1; y++)
  for (int unknown=0; unknown<unknowns; unknown++) {
    faceQLeft [ (2*y+1)*unknowns+unknown ] = 0.0;
    faceQRight[ (2*y+0)*unknowns+unknown ] = 0.0;
  }

  for (int y=0; y<order+1; y++)
  for (int x=0; x<order+1; x++)
  for (int unknown=0; unknown<unknowns; unknown++) {
    faceQLeft [ (2*y+1)*unknowns+unknown ] += BasisFunctionValuesLeft[x]  * cellQ[ (y*(order+1)+x)*unknowns + unknown ];
    faceQRight[ (2*y+0)*unknowns+unknown ] += BasisFunctionValuesLeft[order-x] * cellQ[ (y*(order+1)+x)*unknowns + unknown ];
  }

  // project up/down
  for (int x=0; x<order+1; x++)
  for (int unknown=0; unknown<unknowns; unknown++) {
    faceQBottom [ (x+order+1)*unknowns+unknown ] = 0.0;
    faceQUp     [ (x        )*unknowns+unknown ] = 0.0;
  }

  for (int y=0; y<order+1; y++)
  for (int x=0; x<order+1; x++)
  for (int unknown=0; unknown<unknowns; unknown++) {
    faceQBottom [ (x+order+1)*unknowns+unknown ] += BasisFunctionValuesLeft[y]  * cellQ[ (y*(order+1)+x)*unknowns + unknown ];
    faceQUp     [ (x        )*unknowns+unknown ] += BasisFunctionValuesLeft[order-y] * cellQ[ (y*(order+1)+x)*unknowns + unknown ];
  }

}


void exahype2::dg::projectVolumetricDataOntoFaces(
  const double* __restrict__          cellQ,
  int                                 order,
  int                                 unknowns,
  const double* const __restrict__    BasisFunctionValuesLeft,
  double* const __restrict__          faceQLeft,
  double* const __restrict__          faceQRight,
  double* const __restrict__          faceQFront,
  double* const __restrict__          faceQBack,
  double* const __restrict__          faceQBottom,
  double* const __restrict__          faceQUp
) {

  int nodesPerAxis = order+1;
  int nodesPerFace = nodesPerAxis*nodesPerAxis;

  // project left/right
  for (int y=0; y<order+1; y++)
  for (int z=0; z<order+1; z++)
  for (int unknown=0; unknown<unknowns; unknown++) {
    faceQLeft   [ (2*(y+z*nodesPerAxis)+1)*unknowns+unknown ] = 0.0;
    faceQRight  [ (2*(y+z*nodesPerAxis)+0)*unknowns+unknown ] = 0.0;
  }

  for (int x=0; x<order+1; x++)
  for (int y=0; y<order+1; y++)
  for (int z=0; z<order+1; z++)
  for (int unknown=0; unknown<unknowns; unknown++) {
    faceQLeft   [ (2*(y+z*nodesPerAxis)+1)*unknowns+unknown ] += BasisFunctionValuesLeft[x] * cellQ[ (z*nodesPerFace+y*nodesPerAxis+x)*unknowns + unknown ];
    faceQRight  [ (2*(y+z*nodesPerAxis)+0)*unknowns+unknown ] += BasisFunctionValuesLeft[order-x] * cellQ[ (z*nodesPerFace+y*nodesPerAxis+x)*unknowns + unknown ];
  }

  // project front/back
  for (int x=0; x<order+1; x++)
  for (int z=0; z<order+1; z++)
  for (int unknown=0; unknown<unknowns; unknown++) {
    faceQFront [ (x+0*nodesPerAxis+z*2*nodesPerAxis)*unknowns+unknown ] = 0.0;
    faceQBack  [ (x+1*nodesPerAxis+z*2*nodesPerAxis)*unknowns+unknown ] = 0.0;
  }
  for (int x=0; x<order+1; x++)
  for (int y=0; y<order+1; y++)
  for (int z=0; z<order+1; z++)
  for (int unknown=0; unknown<unknowns; unknown++) {
    faceQFront [ (x+0*nodesPerAxis+z*2*nodesPerAxis)*unknowns+unknown ] += BasisFunctionValuesLeft[y] * cellQ[ (z*nodesPerFace+y*nodesPerAxis+x)*unknowns + unknown ];
    faceQBack  [ (x+1*nodesPerAxis+z*2*nodesPerAxis)*unknowns+unknown ] += BasisFunctionValuesLeft[order-y] * cellQ[ (z*nodesPerFace+y*nodesPerAxis+x)*unknowns + unknown ];
  }

  // project down/up
  for (int x=0; x<order+1; x++)
  for (int y=0; y<order+1; y++)
  for (int unknown=0; unknown<unknowns; unknown++) {
    faceQBottom [ (x+y*nodesPerAxis+0*nodesPerFace)*unknowns+unknown ] = 0.0;
    faceQUp     [ (x+y*nodesPerAxis+1*nodesPerFace)*unknowns+unknown ] = 0.0;
  }

  for (int x=0; x<order+1; x++)
  for (int y=0; y<order+1; y++)
  for (int z=0; z<order+1; z++)
  for (int unknown=0; unknown<unknowns; unknown++) {
    faceQBottom [ (x+y*nodesPerAxis+0*nodesPerFace)*unknowns+unknown ] += BasisFunctionValuesLeft[z] * cellQ[ (z*nodesPerFace+y*nodesPerAxis+x)*unknowns + unknown ];
    faceQUp     [ (x+y*nodesPerAxis+1*nodesPerFace)*unknowns+unknown ] += BasisFunctionValuesLeft[order-z] * cellQ[ (z*nodesPerFace+y*nodesPerAxis+x)*unknowns + unknown ];
  }

}


void exahype2::dg::integrateOverRiemannSolutionsAndAddToVolume_GaussLegendre(
  const double* const __restrict__    faceQLeft,
  const double* const __restrict__    faceQRight,
  const double* const __restrict__    faceQBottom,
  const double* const __restrict__    faceQUp,
  int                                 order,
  int                                 unknowns,
  const int                           auxiliaryVariables,
  const tarch::la::Vector<2,double>&  cellSize,
  const double* const __restrict__    BasisFunctionValuesLeft,
  const double* __restrict__          MassMatrixDiagonal1d,
  double* __restrict__                cellQ
){
  const int strideQ = unknowns+auxiliaryVariables;

  for( int x=0; x<order+1; x++ )
  for( int y=0; y<order+1; y++ ) {
    const double coeffLeft    = MassMatrixDiagonal1d[y]*BasisFunctionValuesLeft[x];
    const double coeffRight   = MassMatrixDiagonal1d[y]*BasisFunctionValuesLeft[order-x];

    const double coeffBottom  = MassMatrixDiagonal1d[x]*BasisFunctionValuesLeft[y];
    const double coeffUp      = MassMatrixDiagonal1d[x]*BasisFunctionValuesLeft[order-y];

    for (int var=0; var<strideQ; var++) {
      cellQ[ ( x + y*(order+1))*strideQ + var ] +=
            coeffLeft   * faceQLeft   [ y*strideQ + var ]
          - coeffRight  * faceQRight  [ y*strideQ + var ]
          + coeffBottom * faceQBottom [ x*strideQ + var ]
          - coeffUp     * faceQUp     [ x*strideQ + var ];
    }//var
  }//x,y
}



void exahype2::dg::integrateOverRiemannSolutionsAndAddToVolume_GaussLegendre(
  const double* const __restrict__    faceQLeft,
  const double* const __restrict__    faceQRight,
  const double* const __restrict__    faceQFront,
  const double* const __restrict__    faceQBack,
  const double* const __restrict__    faceQBottom,
  const double* const __restrict__    faceQUp,
  int                                 order,
  int                                 unknowns,
  const int                           auxiliaryVariables,
  const tarch::la::Vector<3,double>&  cellSize,
  const double* const __restrict__    BasisFunctionValuesLeft,
  const double* __restrict__          MassMatrixDiagonal1d,
  double* __restrict__                cellQ
  ){
  const int strideQ = unknowns+auxiliaryVariables;
  const int nodesPerAxis = order+1;
  const int nodesPerFace = getNodesPerCell(nodesPerAxis)/nodesPerAxis;

  //generic coefficients by dimension
  /*
  const double coeff0 = 1.0/cellSize[0];
  const double coeff1 = 1.0/cellSize[1];
  const double coeff2 = 1.0/cellSize[2];
  */
  for (int x=0; x<order+1; x++)
  for (int y=0; y<order+1; y++)
  for (int z=0; z<order+1; z++){

        //coefficients of faces in x direction
        const double coeffLeft  = MassMatrixDiagonal1d[y]*MassMatrixDiagonal1d[z]*BasisFunctionValuesLeft[x];
        const double coeffRight = MassMatrixDiagonal1d[y]*MassMatrixDiagonal1d[z]*BasisFunctionValuesLeft[order-x];
        //coefficients of faces in y direction
        const double coeffFront = MassMatrixDiagonal1d[x]*MassMatrixDiagonal1d[z]*BasisFunctionValuesLeft[y];
        const double coeffBack  = MassMatrixDiagonal1d[x]*MassMatrixDiagonal1d[z]*BasisFunctionValuesLeft[order-y];
        //coefficients of faces in z direction
        const double coeffBottom  = MassMatrixDiagonal1d[x]*MassMatrixDiagonal1d[y]*BasisFunctionValuesLeft[z];
        const double coeffUp      = MassMatrixDiagonal1d[x]*MassMatrixDiagonal1d[y]*BasisFunctionValuesLeft[order-z];

        for (int var=0; var<strideQ; var++) {
          cellQ[ (z*nodesPerFace+y*nodesPerAxis+x)*strideQ + var ] +=
              coeffLeft   * faceQLeft   [(y+z*nodesPerAxis)*strideQ+var]
            - coeffRight  * faceQRight  [(y+z*nodesPerAxis)*strideQ+var]
            + coeffFront  * faceQFront  [(x+z*nodesPerAxis)*strideQ+var]
            - coeffBack   * faceQBack   [(x+z*nodesPerAxis)*strideQ+var]
            + coeffBottom * faceQBottom [(x+y*nodesPerAxis)*strideQ+var]
            - coeffUp     * faceQUp     [(x+y*nodesPerAxis)*strideQ+var]
            ;
        }//var

  }//x,y,z


}


/*
void exahype2::dg::applyRiemannToPatch(
    double* __restrict__        Q,
    const double* __restrict__  QHullIn,
    const int                   order,
    const int                   unknowns,
    const int                   auxiliaryVariables,
    const double                dx,
    const double                dt,
    const double* __restrict__  BasisFunctionValuesLeft,
    const double* __restrict__  BasisFunctionValuesRight,
    const double* __restrict__  quadratureWeights
    ){

  const int strideQ      = unknowns+auxiliaryVariables;
  const int nodesPerAxis = order+1;
  const int nodesPerFace = getNodesPerCell(nodesPerAxis)/nodesPerAxis;
  const int nodesPerCell = getNodesPerCell(nodesPerAxis);
  tarch::la::Vector<Dimensions,int> strides = getStrides(nodesPerAxis);

  const double coeff0 = dt/dx;

  for(int node=0; node<nodesPerCell; node++){

    tarch::la::Vector<Dimensions,int> index = getIndex(node, strides);

    // direction in which riemann contributions are being computed
    for(int dim=0; dim<Dimensions; dim++){

      const double coeff1 = coeff0/quadratureWeights[index[dim]];
      const double coeffLeft  = coeff1*BasisFunctionValuesLeft[index[dim]];
      const double coeffRight = coeff1*BasisFunctionValuesRight[index[dim]];
      const int leftFaceNode  = cellIndexToHullIndex(index, dim, 0, nodesPerAxis);
      const int rightFaceNode = cellIndexToHullIndex(index, dim, 1, nodesPerAxis);
      for(int var=0; var<unknowns; var++){
        Q[node*strideQ+var] += coeffLeft * QHullIn[leftFaceNode*strideQ+var] - coeffRight * QHullIn[rightFaceNode*strideQ+var];
      }

    } //dim

  } //node


}
*/





