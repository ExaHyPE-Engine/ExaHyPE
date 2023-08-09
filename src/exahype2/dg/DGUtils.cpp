#include "exahype2/dg/DGUtils.h"


void exahype2::dg::copyOneSideOfFaceProjection(
  int    unknownsPlusAuxiliaryVariables,
  int    order,
  int    numberOfProjectedQuantities,
  int    normal,
  int    isRightFaceHalf,
  const double* __restrict__   srcQ,
  double* __restrict__         destQ
) {
  ::exahype2::fv::copyHalfOfHalo(
    unknownsPlusAuxiliaryVariables,
    order+1,
    numberOfProjectedQuantities,
    normal,
    isRightFaceHalf,
    srcQ,
    destQ
  );
}


double  exahype2::dg::getQuadratureWeight(
  const tarch::la::Vector<3,double>&  cellSize,
  const tarch::la::Vector<3,int>&     index,
  const double* __restrict__          quadratureWeights
) {
  double result = 1.0;
  for (int d=0; d<Dimensions; d++) {
    result *= cellSize(d) * quadratureWeights[ index(d) ];
  }
  return result;
}


tarch::la::Vector<2,double>  exahype2::dg::getQuadraturePoint(
  const tarch::la::Vector<2,double>&  cellCentre,
  const tarch::la::Vector<2,double>&  cellSize,
  const tarch::la::Vector<2,int>&     index,
  int                                 polynomialOrder,
  const double* __restrict__          quadraturePoints
) {
  tarch::la::Vector<2,double> result;

  result(0) = quadraturePoints[index(0)];
  result(1) = quadraturePoints[index(1)];

  return tarch::la::multiplyComponents(result,cellSize) + cellCentre - 0.5*cellSize;
}


tarch::la::Vector<3,double>  exahype2::dg::getQuadraturePoint(
  const tarch::la::Vector<3,double>&  cellCentre,
  const tarch::la::Vector<3,double>&  cellSize,
  const tarch::la::Vector<3,int>&     index,
  int                                 polynomialOrder,
  const double* __restrict__          quadraturePoints
) {
  tarch::la::Vector<3,double> result;

  result(0) = quadraturePoints[index(0)];
  result(1) = quadraturePoints[index(1)];
  result(2) = quadraturePoints[index(2)];

  return tarch::la::multiplyComponents(result,cellSize) + cellCentre - 0.5*cellSize;
}


int exahype2::dg::getNodesPerCell(int nodesPerAxis){
  int nodesPerCell=nodesPerAxis;
  for(int d=1; d<Dimensions; d++) {
    nodesPerCell *= nodesPerAxis;
  }
  return nodesPerCell;
}

tarch::la::Vector<Dimensions,int> exahype2::dg::getStrides(int nodesPerAxis){
  tarch::la::Vector<Dimensions,int> strides(1);
  for(int i=1; i<Dimensions; i++){
    strides[i] = strides[i-1]*nodesPerAxis;
  }
  return strides;
}

tarch::la::Vector<Dimensions,int> exahype2::dg::getIndex(
    int                                   node,
    tarch::la::Vector<Dimensions,int>  strides
){

  tarch::la::Vector<Dimensions,int> index(0);
  int tmp = 0;
  for(int d=Dimensions-1; d>=0; d--){
    index[d] = (node - tmp) / strides[d];
    tmp += strides[d]*index[d];
  }
  return index;

}


int exahype2::dg::cellIndexToHullIndex(
    const tarch::la::Vector<Dimensions,int>&  indexCell,
    const int                                 direction,
    const int                                 orientation,
    const int                                 nodesPerAxis
    ){

  /*
   * Imagine a 3d cell of order 2, so there are 9 nodes
   * for a node [i,j,k], we want face element [j,k] if in
   * direction i, [i,k] in direction j and [i,j] if in
   * direction k.
   * Element [a,b] of a face is element a*strides[0]+b
   * (concept is same in other dimensions but with
   * additional terms and greater strides)
   *
   * However since the Hull is one array, we need to skip
   * the previous faces, so ignore nodesPerFace*dimensions
   * indexes (nodesPerFace*dimensions+1) if right.
  */

  int HullIndex = 0;
  int stride = 1;

  for(int dim=0; dim<Dimensions; dim++){
    if(dim!=direction){
      HullIndex += indexCell[dim]*stride;
      stride *= nodesPerAxis;
    }
  }

  /* if left face need to skip left and right faces for each previous dimension
   * if right face, need to skip those plus the left face
   */
  return HullIndex + stride*(2*direction+orientation); //stride is nodesPerAxis^(d)

}


void exahype2::dg::computeGradient(
        const double* __restrict__ const QCell,
        const double* __restrict__ const derivativeOperator,
        const double                     invDx,
        const int                        nodesPerAxis,
        const int                        strideQ,
        const int                        scalarIndex,
        double* __restrict__             gradQ
        ){

  tarch::la::Vector<Dimensions,int> strides = getStrides(nodesPerAxis);
  tarch::la::Vector<Dimensions,int> index   = getIndex(scalarIndex, strides);

  for(int dim=0; dim<Dimensions; dim++){

    for(int var=0; var<strideQ; var++){
      gradQ[dim*strideQ+var] = 0.0;
    }//var

    for(int node=0; node<nodesPerAxis; node++){
//      const double coeff = invDx * derivativeOperator[node+nodesPerAxis*index[dim]];
      const double coeff = invDx * derivativeOperator[node+nodesPerAxis*index[dim]];
      for(int var=0; var<strideQ; var++){
          gradQ[dim*strideQ+var] += coeff*QCell[(scalarIndex + (node-index[dim])*strides[dim])*strideQ + var];
      }//var
    }//node
  }//dim

}


void exahype2::dg::subtractCell(
    double* __restrict__        QOut,
    const double* __restrict__  Qsubstract,
    const int                   order,
    const int                   unknowns,
    const int                   auxiliaryVariables
) {
  const int nodesPerAxis = order+1;
  const int nodesPerCell = getNodesPerCell(nodesPerAxis);
  const int strideQ = unknowns+auxiliaryVariables;

  for(int node=0; node<nodesPerCell; node++){
    for(int var=0; var<strideQ; var++){
      QOut[node*strideQ+var] -= Qsubstract[node*strideQ+var];
    }
  }
}


std::string exahype2::dg::plotCell(
  const double* __restrict__ Q,
  const int order,
  const int unknowns,
  const int auxiliaryVariables
) {
  return ::exahype2::fv::plotPatch(
    Q,
    order+1,
    unknowns,
    auxiliaryVariables,
    Dimensions==2
  );
}


std::string exahype2::dg::plotFace(
  const double* __restrict__  Q,
  const int                   order,
  const int                   unknowns,
  const int                   auxiliaryVariables,
  int                         normal,
  int                         numberOfQuantitiesProjectedOntoFace
) {
  return ::exahype2::fv::plotPatchOverlap(
    Q,
    unknowns,
    auxiliaryVariables,
    order+1,
    numberOfQuantitiesProjectedOntoFace,
    normal,
    Dimensions==2
  );
}
