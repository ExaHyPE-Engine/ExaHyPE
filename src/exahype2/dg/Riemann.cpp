#include "Riemann.h"

void exahype2::dg::getMaxEigenvaluePerFace(
    MaxEigenvalue                                 maxEigenvalue,
    double* __restrict__                          QHull[Dimensions*2],
    const int                                     order,
    const int                                     unknowns,
    const int                                     auxiliaryVariables,
    const tarch::la::Vector<Dimensions,double>&   x,
    const double                                  t,
    const double                                  dt,
    double                                        eigenvalues[Dimensions*2]
    ){

  const int nodesPerAxis = order+1;
  const int nodesPerFace = getNodesPerCell(nodesPerAxis)/nodesPerAxis;

  const int strideQ = unknowns+auxiliaryVariables;

  for(int dim=0; dim<Dimensions; dim++){

    for(int lr=0; lr<2; lr++){

      eigenvalues[2*dim+lr] = std::numeric_limits<double>::min();

      for(int node=0; node<2*nodesPerFace; node++){
        double result = maxEigenvalue(
          &QHull[2*dim+lr][node*strideQ], //if left face, then inner side is the right one and reversely
          x,
          t,
          dt,
          dim
        );
        eigenvalues[2*dim+lr] = std::max(eigenvalues[2*dim+lr], std::abs(result));
      }

    }
  }

}

void exahype2::dg::computeRusanovOnHull(
    Flux                                          flux,
    NonConservativeProduct                        nonConservativeProduct,
    double*                                       QHullIn[Dimensions*2],
    double* __restrict__                          QHullOut,
    const int                                     order,
    const int                                     unknowns,
    const int                                     auxiliaryVariables,
    const tarch::la::Vector<Dimensions,double>&   x,
    const double                                  dx,
    const double                                  t,
    const double                                  dt,
    const double                                  maxEigenvalueAlongFace[Dimensions*2],
    const bool                                    useFlux,
    const bool                                    useNCP
    ){

  const int nodesPerAxis = order+1;
  const int nodesPerFace = getNodesPerCell(nodesPerAxis)/nodesPerAxis;

  const int strideQ = unknowns+auxiliaryVariables;

  double* fluxValuesLeft      = nullptr;
  double* fluxValuesRight     = nullptr;
  double* ncpValues           = nullptr;
  double* QValues             = nullptr;
  double* gradQValues         = nullptr;

  if(useFlux){
    fluxValuesLeft  = new double[unknowns]{0.0};
    fluxValuesRight = new double[unknowns]{0.0};
  }
  if(useNCP){
    ncpValues       = new double[strideQ]{0.0};
    QValues         = new double[strideQ]{0.0};
    gradQValues     = new double[strideQ]{0.0};
  }

  for(int dim=0; dim<Dimensions; dim++){

    for(int lr=0; lr<2; lr++){

      const double coeff = 0.5*maxEigenvalueAlongFace[2*dim+lr];
      const double* ql = nullptr;
      const double* qr = nullptr;

      for(int node=0; node<nodesPerFace; node++){

        /*for dimension i you need to skip 2*(i-1) faces
        **if right face you additionally need to skip left face
        **each skipped face having 2*nodesPerFace elements since it contains elements
        **from its left and right
        **Then when looking at node n you still need to skip all elements before it, so
        **a total of 2*n elements by the same logic
        */

        ql = &QHullIn[2*dim+lr][2*node*strideQ];
        qr = &QHullIn[2*dim+lr][(2*node+1)*strideQ];

        for(int var=0; var<unknowns; var++){
          QHullOut[((2*dim+lr)*nodesPerFace+node)*strideQ+var] = 0.0;
        }

        if(useFlux){
            flux( ql,
                  x, //TODO: add positioning
                  t,
                  dt,
                  dim, //normal
                  fluxValuesLeft);
            flux( qr,
                  x, //TODO: add positioning
                  t,
                  dt,
                  dim, //normal
                  fluxValuesRight);

            for(int var=0; var<unknowns; var++){
              //Rusanov flux
              QHullOut[((2*dim+lr)*nodesPerFace+node)*strideQ+var] += 0.5*(fluxValuesLeft[var]+fluxValuesRight[var])
                  - coeff*(qr[var]-ql[var]);
            } //var

        } //if useFlux

        if(useNCP){
          for(int var=0; var<strideQ; var++){
            gradQValues[var] = qr[var] - ql[var];
          }
          for(int var=0; var<strideQ; var++){
            QValues[var] = 0.5* (qr[var] + ql[var]);
          }

          nonConservativeProduct(
              QValues,
              gradQValues,
              x, //TODO: add positioning
              t,
              dt,
              dim,
              ncpValues);

          // this is 0.5 for the left face and -0.5 for the right face, this is just the term that takes into account the direction multiplied with 0.5
          const double coeff = (-0.5 + 1.0*lr);

          for(int var=0; var<unknowns; var++){
            QHullOut[((2*dim+lr)*nodesPerFace+node)*strideQ+var] += coeff*ncpValues[var];
          }

        }

      } //node
    } //lr
  } //dim

  if(useFlux){
    delete [] fluxValuesLeft;
    delete [] fluxValuesRight;
  }
  if(useNCP){
    delete [] ncpValues;
    delete [] QValues;
    delete [] gradQValues;
  }

}




void exahype2::dg::computeRiemannIntegralAndAddToCell(
  const double* const __restrict__    faceQLeft,
  const double* const __restrict__    faceQRight,
  const double* const __restrict__    faceQBottom,
  const double* const __restrict__    faceQUp,
  int                                 order,
  int                                 unknowns,
  const int                           auxiliaryVariables,
  const tarch::la::Vector<2,double>&  cellSize,
  const double                        dt,
  const double* const __restrict__    BasisFunctionValuesLeft,
  const double* __restrict__          quadratureWeights,
  double* __restrict__                cellQ
  ){

  const int strideQ      = unknowns+auxiliaryVariables;
  const int nodesPerAxis = order+1;
  const int nodesPerFace = getNodesPerCell(nodesPerAxis)/nodesPerAxis;
  const int nodesPerCell = getNodesPerCell(nodesPerAxis);
  tarch::la::Vector<Dimensions,int> strides = getStrides(nodesPerAxis);

  //generic coefficients by dimension
  const double coeff0 = dt/cellSize[0];
  const double coeff1 = dt/cellSize[1];

  for(int node=0; node<nodesPerCell; node++){

    tarch::la::Vector<Dimensions,int> index = getIndex(node, strides);

    const double coeffNode0 = coeff0/quadratureWeights[index[0]];
    const double coeffLeft    = coeffNode0*BasisFunctionValuesLeft[index[0]];
    const double coeffRight   = coeffNode0*BasisFunctionValuesLeft[order-index[0]];

    const double coeffNode1 = coeff1/quadratureWeights[index[1]];
    const double coeffBottom  = coeffNode1*BasisFunctionValuesLeft[index[1]];
    const double coeffUp      = coeffNode1*BasisFunctionValuesLeft[order-index[1]];

    const double* leftFaceNode    = &faceQLeft[index[0]*strideQ];
    const double* rightFaceNode   = &faceQRight[index[0]*strideQ];
    const double* bottomFaceNode  = &faceQBottom[index[1]*strideQ];
    const double* upperFaceNode   = &faceQUp[index[1]*strideQ];


    for(int var=0; var<unknowns; var++){
      cellQ[node*strideQ+var] += coeffLeft*leftFaceNode[var] - coeffRight*rightFaceNode[var]
                            + coeffBottom*bottomFaceNode[var] - coeffUp*upperFaceNode[var];
    }

  } //node

}


void exahype2::dg::computeRiemannIntegralAndAddToCell(
  const double* const __restrict__    faceQLeft,
  const double* const __restrict__    faceQRight,
  const double* const __restrict__    faceQBottom,
  const double* const __restrict__    faceQUp,
  const double* const __restrict__    faceQFront,
  const double* const __restrict__    faceQBack,
  int                                 order,
  int                                 unknowns,
  const int                           auxiliaryVariables,
  const tarch::la::Vector<2,double>&  cellSize,
  const double                        dt,
  const double* const __restrict__    BasisFunctionValuesLeft,
  const double* __restrict__          quadratureWeights,
  double* __restrict__                cellQ
  ){

  const int strideQ      = unknowns+auxiliaryVariables;
  const int nodesPerAxis = order+1;
  const int nodesPerFace = getNodesPerCell(nodesPerAxis)/nodesPerAxis;
  const int nodesPerCell = getNodesPerCell(nodesPerAxis);
  tarch::la::Vector<Dimensions,int> strides = getStrides(nodesPerAxis);

  //generic coefficients by dimension
  const double coeff0 = dt/cellSize[0];
  const double coeff1 = dt/cellSize[1];
  const double coeff2 = dt/cellSize[2];

  for(int node=0; node<nodesPerCell; node++){

    tarch::la::Vector<Dimensions,int> index = getIndex(node, strides);

    const double coeffNode0 = coeff0/quadratureWeights[index[0]];
    const double coeffLeft    = coeffNode0*BasisFunctionValuesLeft[index[0]];
    const double coeffRight   = coeffNode0*BasisFunctionValuesLeft[order-index[0]];

    const double coeffNode1 = coeff1/quadratureWeights[index[1]];
    const double coeffBottom  = coeffNode1*BasisFunctionValuesLeft[index[1]];
    const double coeffUp      = coeffNode1*BasisFunctionValuesLeft[order-index[1]];

    const double coeffNode2 = coeff2/quadratureWeights[index[2]];
    const double coeffFront   = coeffNode2*BasisFunctionValuesLeft[index[2]];
    const double coeffBack    = coeffNode2*BasisFunctionValuesLeft[order-index[2]];

    const double* leftFaceNode    = &faceQLeft[index[0]*strideQ];
    const double* rightFaceNode   = &faceQRight[index[0]*strideQ];
    const double* bottomFaceNode  = &faceQBottom[index[1]*strideQ];
    const double* upperFaceNode   = &faceQUp[index[1]*strideQ];
    const double* frontFaceNode   = &faceQFront[index[2]*strideQ];
    const double* backFaceNode    = &faceQBack[index[2]*strideQ];

    for(int var=0; var<unknowns; var++){
      cellQ[node*strideQ+var] += coeffLeft*leftFaceNode[var] - coeffRight*rightFaceNode[var]
                            + coeffBottom*bottomFaceNode[var] - upperFaceNode[var]
                            + coeffFront*frontFaceNode[var] - coeffBack*backFaceNode[var];
    }

  } //node

}


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





