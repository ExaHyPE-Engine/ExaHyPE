#include "../dg/DGUtils.h"

#include "tarch/Assertions.h"
#include "tarch/la/la.h"


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


void exahype2::dg::projectVolumetricDataOntoFaces(
  const double* __restrict__          cellQ,
  int                                 order,
  int                                 unknowns,
  const double* const __restrict__    BasisFunctionValuesLeft,
  const double* const __restrict__    BasisFunctionValuesRight,
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
    faceQRight[ (2*y+0)*unknowns+unknown ] += BasisFunctionValuesRight[x] * cellQ[ (y*(order+1)+x)*unknowns + unknown ];
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
    faceQUp     [ (x        )*unknowns+unknown ] += BasisFunctionValuesRight[y] * cellQ[ (y*(order+1)+x)*unknowns + unknown ];
  }

  return;

  int nodesPerAxis = order+1;

  for(int row=0; row<nodesPerAxis; row++){
    //unrolled from rest of loop to initialize elements of faces

    for(int var=0; var<unknowns; var++){

      faceQLeft[(2*row+1)*unknowns+var]   = BasisFunctionValuesLeft[0] * cellQ[(row*nodesPerAxis+0)*unknowns + var];
      faceQRight[2*row*unknowns+var]      = BasisFunctionValuesRight[0] * cellQ[(row*nodesPerAxis+0)*unknowns + var];

      faceQBottom[(2*row+1)*unknowns+var] = BasisFunctionValuesLeft[0] * cellQ[(0*nodesPerAxis+row)*unknowns + var];
      faceQUp[2*row*unknowns+var]         = BasisFunctionValuesRight[0] * cellQ[(0*nodesPerAxis+row)*unknowns + var];
      } // var

    for(int node=1; node<nodesPerAxis; node++){
      for(int var=0; var<unknowns; var++){
        /*iterate over nodes from left to right; remember that in face elements are in order left1, then right1,
        then left 2, then right 2 etc, and left face here is the face this cell is to the right of*/
        faceQLeft[(2*row+1)*unknowns+var]   += BasisFunctionValuesLeft[node] * cellQ[(row*nodesPerAxis+node)*unknowns + var];
        faceQRight[2*row*unknowns+var]      += BasisFunctionValuesRight[node] * cellQ[(row*nodesPerAxis+node)*unknowns + var];

        //iterate over nodes from lowest to highest
        faceQBottom[(2*row+1)*unknowns+var] += BasisFunctionValuesLeft[node] * cellQ[(node*nodesPerAxis+row)*unknowns + var];
        faceQUp[2*row*unknowns+var]         += BasisFunctionValuesRight[node] * cellQ[(node*nodesPerAxis+row)*unknowns + var];
      } // var

    } // node

  } // row


}


void exahype2::dg::projectVolumetricDataOntoFaces(
  const double* __restrict__          cellQ,
  int                                 order,
  int                                 unknowns,
  const double* const __restrict__    BasisFunctionValuesLeft,
  const double* const __restrict__    BasisFunctionValuesRight,
  double* const __restrict__          faceQLeft,
  double* const __restrict__          faceQRight,
  double* const __restrict__          faceQBottom,
  double* const __restrict__          faceQUp,
  double* const __restrict__          faceQFront,
  double* const __restrict__          faceQBack
) {
  int nodesPerAxis = order+1;
  int nodesPerFace = nodesPerAxis*nodesPerAxis;

  for(int face=0; face<nodesPerAxis; face++){
    for(int row=0; row<nodesPerAxis; row++){

      //unrolled from rest of loop to initialize elements of faces
      for(int var=0; var<unknowns; var++){
        faceQLeft[(2*(face*nodesPerAxis+row)+1)*unknowns+var]   = BasisFunctionValuesLeft[0] * cellQ[(face*nodesPerFace+row*nodesPerAxis+0)*unknowns + var];
        faceQRight[2*(face*nodesPerAxis+row)*unknowns+var]        = BasisFunctionValuesRight[0] * cellQ[(face*nodesPerFace+row*nodesPerAxis+0)*unknowns + var];

        faceQBottom[(2*(face*nodesPerAxis+row)+1)*unknowns+var] = BasisFunctionValuesLeft[0] * cellQ[(face*nodesPerFace+row+0*nodesPerAxis)*unknowns + var];
        faceQUp[2*(face*nodesPerAxis+row)*unknowns+var]          = BasisFunctionValuesRight[0] * cellQ[(face*nodesPerFace+row+0*nodesPerAxis)*unknowns + var];

        faceQFront[(2*(face*nodesPerAxis+row)+1)*unknowns+var]  = BasisFunctionValuesLeft[0] * cellQ[(face*nodesPerAxis+row+0*nodesPerFace)*unknowns + var];
        faceQBack[2*(face*nodesPerAxis+row)*unknowns+var]         = BasisFunctionValuesRight[0] * cellQ[(face*nodesPerAxis+row+0*nodesPerFace)*unknowns + var];
      } // var

      for(int node=1; node<nodesPerAxis; node++){
        for(int var=0; var<unknowns; var++){

          /*iterate over nodes from left to right; remember that in face elements are in order left1, then right1,
           then left 2, then right 2 etc, and left face here is the face this cell is to the right of*/
          faceQLeft[(2*(face*nodesPerAxis+row)+1)*unknowns+var]   += BasisFunctionValuesLeft[node] * cellQ[(face*nodesPerFace+row*nodesPerAxis+node)*unknowns + var];
          faceQRight[2*(face*nodesPerAxis+row)*unknowns+var]        += BasisFunctionValuesRight[node] * cellQ[(face*nodesPerFace+row*nodesPerAxis+node)*unknowns + var];

          //iterate over nodes from lowest to highest
          faceQBottom[(2*(face*nodesPerAxis+row)+1)*unknowns+var] += BasisFunctionValuesLeft[node] * cellQ[(face*nodesPerFace+row+node*nodesPerAxis)*unknowns + var];
          faceQUp[2*(face*nodesPerAxis+row)*unknowns+var]          += BasisFunctionValuesRight[node] * cellQ[(face*nodesPerFace+row+node*nodesPerAxis)*unknowns + var];

          //iterate over nodes from front to back
          faceQFront[(2*(face*nodesPerAxis+row)+1)*unknowns+var]  += BasisFunctionValuesLeft[node] * cellQ[(face*nodesPerAxis+row+node*nodesPerFace)*unknowns + var];
          faceQBack[2*(face*nodesPerAxis+row)*unknowns+var]         += BasisFunctionValuesRight[node] * cellQ[(face*nodesPerAxis+row+node*nodesPerFace)*unknowns + var];

        } // var

      } // node
    } // row
  } // face

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
    ){

  const int nodesPerAxis = order+1;
  const int nodesPerCell = getNodesPerCell(nodesPerAxis);
  const int strideQ = unknowns+auxiliaryVariables;

  for(int node=0; node<nodesPerCell; node++){
    for(int var=0; var<strideQ; var++){
      QOut[node*strideQ+var] -= Qsubstract[node*strideQ+var];
    }
  }

}


void exahype2::dg::applyBoundaryConditions(
    BoundaryConditions                          boundaryConditions,
    double*                                     QFace,
    const int                                   order,
    const int                                   strideQ,
    const tarch::la::Vector<Dimensions,double>& x,
    const double                                t,
    const int                                   faceNormal,
    const int                                   lr
    ){

  const int nodesPerAxis = order+1;
  const int nodesPerFace = getNodesPerCell(nodesPerAxis)/nodesPerAxis;

  /*
   * This sets the inside and outside faces to the first left and right
   * faces as required.
   * If we're looking at the left face of the cell, the outside of the cell
   * is on it's left and inversely, which is why we use (1-lr).
  */

  /*
   * For reference of where first left and right faces would be
  const double* qlStart = &Face[0];
  const double* qrStart = &Face[strideQ];
  */
  const double* qIn = &QFace[(1-lr)*strideQ];
  double* qOut      = &QFace[(lr)*strideQ];

  //iterate over all nodes of the face using the values on the inside to
  // set the values on the outside using the boundaryConditons() function
  for(int node=0; node<nodesPerFace; node++){

    // set boundary Conditions for given node
    boundaryConditions(
          qIn,
          qOut,
          x, //TODO: add positioning
          t,
          faceNormal
          );

    /*
     * advancing the pointers to the next node, since this skips over
     * a left and a right face, it skips over 2*strideQ elements
     */
    qIn   += 2*strideQ;
    qOut  += 2*strideQ;

  }// for node

}


std::string exahype2::dg::plotPatch(
        const double* __restrict__ Q,
        const int order,
        const int unknowns,
        const int auxiliaryVariables
        ){

  const int nodesPerCell = getNodesPerCell(order+1);
  const int strideQ = unknowns+auxiliaryVariables;

  std::ostringstream result;

  result << "(" << std::endl;
  for(int node=0; node<nodesPerCell; node++){
    result << "node " << std::to_string(node) << "[";
    for(int var=0; var<strideQ-1; var++){
        result << std::to_string(Q[node*strideQ+var]) << ", ";
    }
    result << std::to_string(Q[node*strideQ+strideQ-1]) << "]" << std::endl;
  }
  result << ")" << std::endl;

  return result.str();
}

std::string exahype2::dg::plotFace(
        const double* __restrict__ Q,
        const int order,
        const int unknowns,
        const int auxiliaryVariables
        ){

  const int nodesPerFace = getNodesPerCell(order+1)/(order+1);
  const int strideQ = unknowns+auxiliaryVariables;

  std::ostringstream result;

  result << "(" << std::endl;
  for(int node=0; node<2*nodesPerFace; node++){
    result << "node " << std::to_string(node) << "[";
    for(int var=0; var<strideQ-1; var++){
        result << std::to_string(Q[node*strideQ+var]) << ", ";
    }
    result << std::to_string(Q[node*strideQ+strideQ-1]) << "]" << std::endl;
  }
  result << ")" << std::endl;

  return result.str();
}


void exahype2::dg::addFaceSolutionToPatch(
  const double* const __restrict__          faceQLeft,
  const double* const __restrict__          faceQRight,
  const double* const __restrict__          faceQBottom,
  const double* const __restrict__          faceQUp,
  int                                 order,
  int                                 unknowns,
  const int                           auxiliaryVariables,
  const tarch::la::Vector<2,double>&  cellSize,
  const double                dt,
  const double* const __restrict__    BasisFunctionValuesLeft,
  const double* const __restrict__    BasisFunctionValuesRight,
  const double* __restrict__          quadratureWeights,
  double* __restrict__                Q
) {
  const int strideQ      = unknowns+auxiliaryVariables;
  const int nodesPerAxis = order+1;
  const int nodesPerFace = getNodesPerCell(nodesPerAxis)/nodesPerAxis;
  const int nodesPerCell = getNodesPerCell(nodesPerAxis);
  tarch::la::Vector<Dimensions,int> strides = getStrides(nodesPerAxis);

  for(int node=0; node<nodesPerCell; node++){

    tarch::la::Vector<Dimensions,int> index = getIndex(node, strides);

    // grab from left/right
    {
      int dim = 0;
      const double coeff0 = dt/cellSize(dim);
      const double coeff1 = coeff0/quadratureWeights[index[dim]];
      const double coeffLeft  = coeff1*BasisFunctionValuesLeft[index[dim]];
      const double coeffRight = coeff1*BasisFunctionValuesRight[index[dim]];
    //const int leftFaceNode  = cellIndexToHullIndex(index, dim, 0, nodesPerAxis);
    //const int rightFaceNode = cellIndexToHullIndex(index, dim, 1, nodesPerAxis);
      const int leftFaceNode  = cellIndexToHullIndex(index, dim, 0, nodesPerAxis);
      const int rightFaceNode = leftFaceNode+1;
      for(int var=0; var<unknowns; var++){
        Q[node*strideQ+var] += coeffLeft * faceQLeft [leftFaceNode*strideQ+var];
        Q[node*strideQ+var] -= coeffLeft * faceQRight[rightFaceNode*strideQ+var];
      }
    }

    // grab from top/down
    {
      int dim = 1;
      const double coeff0 = dt/cellSize(dim);
      const double coeff1 = coeff0/quadratureWeights[index[dim]];
      const double coeffLeft  = coeff1*BasisFunctionValuesLeft[index[dim]];
      const double coeffRight = coeff1*BasisFunctionValuesRight[index[dim]];
    //const int leftFaceNode  = cellIndexToHullIndex(index, dim, 0, nodesPerAxis);
    //const int rightFaceNode = cellIndexToHullIndex(index, dim, 1, nodesPerAxis);
      const int leftFaceNode  = cellIndexToHullIndex(index, dim, 0, nodesPerAxis);
      const int rightFaceNode = leftFaceNode+order+1;
      for(int var=0; var<unknowns; var++){
        Q[node*strideQ+var] += coeffLeft * faceQBottom[leftFaceNode*strideQ+var];
        Q[node*strideQ+var] -= coeffLeft * faceQUp    [rightFaceNode*strideQ+var];
      }
    }
  } //node
}

