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


