#include "Euler.h"
#include "peano4/utils/Loop.h"


void exahype2::dg::explicitEulerOnCell_FluxNCPSource_EvaluateTermsRedundantly(
    const double* __restrict__                    cellQin,
    double* __restrict__                          cellQout,
    const int                                     order,
    const int                                     unknowns,
    const int                                     auxiliaryVariables,
    const tarch::la::Vector<Dimensions,double>&    x,
    const double                                   dx,
    const double                                   t,
    const double                                   dt,
    std::function< void(
      const double * __restrict__                  Q,
      const tarch::la::Vector<Dimensions,double>&  x,
      double                                       t,
      int                                          normal,
      double * __restrict__                        F
    ) >   flux,
    std::function< void(
      const double * __restrict__                  Q,
      const double * __restrict__                  dQdx,
      const tarch::la::Vector<Dimensions,double>&  x,
      double                                       t,
      int                                          normal,
      double * __restrict__                        F
    ) >   nonConservativeProduct,
    std::function< void(
      const double * __restrict__                  Q,
      const tarch::la::Vector<Dimensions,double>&  x,
      double                                       t,
      double * __restrict__                        S
    ) >   source,
    const double* __restrict__	                   quadNodes,
    const double* __restrict__                     quadWeights,
    const double* __restrict__                     MassMatrix,
    const double* __restrict__                     StiffnessMatrix,
    const double* __restrict__                     DerivativeOperator,
    const bool                                     useFlux,
    const bool                                     useNCP,
    const bool                                     useSource
) {

  const int nodesPerAxis = order+1;
  const int strideQ=unknowns+auxiliaryVariables;
  const int nodesPerCell = getNodesPerCell(nodesPerAxis);
  tarch::la::Vector<Dimensions,int> strides = getStrides(nodesPerAxis);

  const double invDx = 1.0/dx;
  const double coeff0 = dt*invDx; //We assume that the cells are cubes, e.g. A/V = 1/dx

  double* fluxValues      = nullptr;
  double* sourceValues    = nullptr;
  double* gradientValues  = nullptr;
  double* ncpValues       = nullptr;

  if(useFlux){
    fluxValues      = new double[unknowns]{0.0};
  }
  if(useSource){
    sourceValues    = new double[unknowns]{0.0};
  }
  if(useNCP){
    gradientValues  = new double[strideQ*Dimensions]{0.0};
    ncpValues       = new double[unknowns]{0.0};
  }

  //which node's value is being computed
  for(int node=0; node<nodesPerCell; node++){

    for(int var=0; var<strideQ; var++){
      cellQout[node*strideQ+var] = 0.0;
    }

    /*
    for(int var=0; var<auxiliaryVariables; var++){
      cellQout[node*strideQ+unknowns+var] = cellQin[node*strideQ+unknowns+var];
    }
    */

    tarch::la::Vector<Dimensions,int> index = getIndex(node, strides);

    if(useFlux){
      // direction in which flux contributions are being computed
      for(int dim=0; dim<Dimensions; dim++){

        double coeff1 = coeff0 / quadWeights[index[dim]];

        // contributing node along this direction
        for(int i=0; i<nodesPerAxis; i++){

          // compute contributing node real position
          const int neighbourNode = node + (i - index[dim])*strides[dim];
          tarch::la::Vector<Dimensions,int> contributingNodeIndex = index;
          contributingNodeIndex[dim] = i;
          tarch::la::Vector<Dimensions,double> position = getQuadraturePoint( x,
              dx,
              contributingNodeIndex,
              order,
              quadNodes);

          flux( &cellQin[neighbourNode*strideQ],
                x,
                t,
                dim, //normal
                fluxValues);

          double coeff = coeff1 * StiffnessMatrix[i+index[dim]*nodesPerAxis];
          for(int var=0; var<unknowns; var++){
              cellQout[node*strideQ+var] += coeff * fluxValues[var];
          } //var

        } //i
      } //dim
    }//if useFlux


    //compute real node position
    tarch::la::Vector<Dimensions,double> position = getQuadraturePoint( x,
        dx,
        index,
        order,
        quadNodes);

    if(useSource){
      //compute source term contributions
      source( &cellQin[node*strideQ],
              position,
              t,
              sourceValues);
      for(int var=0; var<unknowns; var++){
        cellQout[node*strideQ+var] += dt * sourceValues[var];
      } //var
    } //if useSource

    if(useNCP){

      computeGradient(
        cellQin,
        DerivativeOperator,
        invDx,
        nodesPerAxis,
        strideQ,
        node,
        gradientValues
        );

      for(int dim=0; dim<Dimensions; dim++){

        nonConservativeProduct(
          &cellQin[node*strideQ],
          &gradientValues[dim*strideQ],
          position,
          t,
          dim,
          ncpValues);

        for(int var=0; var<unknowns; var++){
          cellQout[node*strideQ+var] += dt * ncpValues[var];
        } //var

      } //dim
    } //if useNCP

  } //node

  if(useFlux){
    delete [] fluxValues;
  }
  if(useSource){
    delete [] sourceValues;
  }
  if(useNCP){
    delete [] gradientValues;
    delete [] ncpValues;
  }

}


void exahype2::dg::internal::copySolution(
  const double* __restrict__                    cellQin,
  const int                                     order,
  const int                                     unknowns,
  const int                                     auxiliaryVariables,
  const tarch::la::Vector<Dimensions,int>&      node,
  double* __restrict__                          cellQout
) {
  const int nodeSerialised = peano4::utils::dLinearised( node, order+1 );
  for(int var=0; var<unknowns+auxiliaryVariables; var++) {
    cellQout[nodeSerialised+var] = cellQin[nodeSerialised+var];
  }
}

