#include "Riemann.h"

void exahype2::dg::getMaxEigenvaluePerFace(
    std::function< double(
        const double * __restrict__                 Q,
        const tarch::la::Vector<Dimensions,double>& x,
        double                                      t,
        int                                          normal
        ) >   maxEigenvalue,
    double* __restrict__                     QHull[Dimensions*2],
    const int                                     order,
    const int                                     unknowns,
    const int                                     auxiliaryVariables,
    const tarch::la::Vector<Dimensions,double>&   x,
    const double                                  t,
    double                                        eigenvalues[Dimensions*2]
    ){

  const int nodesPerAxis = order+1;
  const int nodesPerFace = 2*getNodesPerCell(nodesPerAxis)/nodesPerAxis;

  const int strideQ = unknowns+auxiliaryVariables;

  for(int dim=0; dim<Dimensions; dim++){
    for(int lr=0; lr<2; lr++){

      double maxEigenvalueFace = 0.0;
      for(int node=0; node<nodesPerFace; node++){

        double result = maxEigenvalue(
//            &QHull[((2*dim+lr)*nodesPerFace+node)*strideQ],
            &QHull[2*dim+lr][node*strideQ],
            x,
            t,
            node%2);

        maxEigenvalueFace = std::max(eigenvalues[2*dim+lr], std::abs(result));

      } //node

      eigenvalues[2*dim+lr] = maxEigenvalueFace;

    }
  }




}

void exahype2::dg::computeRusanovOnHull(
    std::function< void(
      const double * __restrict__                  Q,
      const tarch::la::Vector<Dimensions,double>&  x,
      double                                       t,
      int                                          normal,
      double * __restrict__                        F
    ) >   flux,
    std::function< void(
      const double * __restrict__                  Qinside,
      double * __restrict__                         Qoutside,
      const tarch::la::Vector<Dimensions,double>&  x,
      double                                       t,
      int                                          normal
    ) >   boundaryConditions,
    double*                                         QHullIn[Dimensions*2],
    double* __restrict__                           QHullOut,
    const int                                     order,
    const int                                     unknowns,
    const int                                     auxiliaryVariables,
    const tarch::la::Vector<Dimensions,double>&   x,
    const double                                  dx,
    const double                                  t,
    const double                                  dt,
    const double                                   maxEigenvalueAlongFace[Dimensions*2],
    const bool                                      atBoundary[Dimensions*2]
    ){

  const int nodesPerAxis = order+1;
  const int nodesPerFace = getNodesPerCell(nodesPerAxis)/nodesPerAxis;

  const int strideQ = unknowns+auxiliaryVariables;

  double* boundaryValuesLeft  = new double[strideQ]{0.0};
  double* boundaryValuesRight  = new double[strideQ]{0.0};
  double* fluxValuesLeft       = new double[unknowns]{0.0};
  double* fluxValuesRight     = new double[unknowns]{0.0};

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
        if(atBoundary[2*dim+lr]&&lr==0){ //on left boundary
          //qr = &QHullIn[(2*((2*Dimensions+lr)*nodesPerFace+node)+1)*strideQ];
          qr = &QHullIn[2*Dimensions+lr][(2*node+1)*strideQ];
          boundaryConditions(  qr,
                boundaryValuesLeft,
                x, //TODO: add positioning
                t,
                dim //normal
                );
          ql = boundaryValuesLeft;
        }
        else if(atBoundary[2*dim+lr]&&lr==1){ //on right boundary
          //ql = &QHullIn[2*((2*Dimensions+lr)*nodesPerFace+node)*strideQ];
          ql = &QHullIn[2*dim+lr][2*node*strideQ];
          boundaryConditions(  ql,
                boundaryValuesRight,
                x, //TODO: add positioning
                t,
                dim //normal
                );
          qr = boundaryValuesRight;
        }
        else{
          //ql = &QHullIn[2*((2*Dimensions+lr)*nodesPerFace+node)*strideQ];
          //qr = &QHullIn[(2*((2*Dimensions+lr)*nodesPerFace+node)+1)*strideQ];
          ql = &QHullIn[2*dim+lr][2*node*strideQ];
          qr = &QHullIn[2*dim+lr][2*(node+1)*strideQ];
        }


        flux(  ql,
              x, //TODO: add positioning
              t,
              dim, //normal
              fluxValuesLeft);
        flux(  qr,
              x, //TODO: add positioning
              t,
              dim, //normal
              fluxValuesRight);

        for(int var=0; var<unknowns; var++){
          //Rusanov flux
          QHullOut[((2*Dimensions+lr)*nodesPerFace+node)*strideQ] = 0.5*(fluxValuesLeft[var]+fluxValuesRight[var])
              - coeff*(qr[var]-ql[var]);
        } //var

      } //node

    } //lr
  } //dim

  delete [] fluxValuesLeft;
  delete [] fluxValuesRight;

}


void exahype2::dg::applyRiemannToPatch(
    double* __restrict__         Q,
    const double* __restrict__   QHullIn,
    const int                   order,
    const int                   unknowns,
    const int                   auxiliaryVariables,
    const double                 dx,
    const double                 dt,
    const double* __restrict__  BasisFunctionValuesLeft,
    const double* __restrict__  BasisFunctionValuesRight,
    const double* __restrict__  quadratureWeights
    ){

  const int strideQ=order+unknowns;
  const int nodesPerAxis = order+1;
  const int nodesPerFace = getNodesPerCell(nodesPerAxis)/nodesPerAxis;
  const int nodesPerCell = getNodesPerCell(nodesPerAxis);
  tarch::la::Vector<Dimensions,int> strides = getStrides(nodesPerAxis);

  const double coeff0 = dt/dx;

  for(int node=0; node<nodesPerCell; node++){

    tarch::la::Vector<Dimensions,int> index = getIndex(node, strides);

    // direction in which riemann contributions are being computed
    for(int dim=0; dim<Dimensions; dim++){

      const double coeff1     = coeff0/quadratureWeights[index[dim]];
      const double coeffLeft   = coeff1*BasisFunctionValuesLeft[index[dim]];
      const double coeffRight = coeff1*BasisFunctionValuesRight[index[dim]];
      const int leftFaceNode  = cellIndexToHullIndex(index, dim, 0, nodesPerAxis);
      const int rightFaceNode = cellIndexToHullIndex(index, dim, 1, nodesPerAxis);

      for(int var=0; var<unknowns; var++){
        Q[node*strideQ+var] += coeffLeft * QHullIn[leftFaceNode+var] - coeffRight * QHullIn[rightFaceNode+var];
      }

    } //dim

  } //node


}


