#include "CellIntegral.h"

#include "peano4/utils/Loop.h"


void exahype2::dg::cellIntegral_patchwise_computeAndUpdate_functors(
    ::exahype2::CellData&                          cellData,
    const int                                      order,
    const int                                      unknowns,
    const int                                      auxiliaryVariables,
    Flux                                           flux,
    NonConservativeProduct                         nonConservativeProduct,
    Source                                         source,
    PointSources                                   pointSources,
    const double* __restrict__	                   quadratureNodes,
    const double* __restrict__                     quadWeights,
    const double* __restrict__                     MassMatrix,
    const double* __restrict__                     StiffnessMatrix,
    const double* __restrict__                     DerivativeOperator,
    const bool                                     useFlux,
    const bool                                     useNCP,
    const bool                                     useSource,
    const bool                                     usePointSources
) {
  double* fluxValues      = useFlux   ? new double[unknowns] : nullptr;
  double* sourceValues    = useSource ? new double[unknowns] : nullptr;
  double* gradientValues  = useNCP    ? new double[unknowns+auxiliaryVariables] : nullptr;
  double* ncpValues       = useNCP    ? new double[unknowns] : nullptr;

  for (int currentCell=0; currentCell<cellData.numberOfCells; currentCell++) {
    const double* __restrict__                  cellQin  = cellData.QIn[currentCell];
    double* __restrict__                        cellQout = cellData.QOut[currentCell];
    const tarch::la::Vector<Dimensions,double>  x        = cellData.cellCentre[currentCell];
    const double                                dx       = (cellData.cellSize[currentCell])(0);
    const double                                t        = cellData.t[currentCell];
    const double                                dt       = cellData.dt[currentCell];

    const int nodesPerAxis = order+1;
  const int strideQ=unknowns+auxiliaryVariables;
  const int nodesPerCell = getNodesPerCell(nodesPerAxis);
  tarch::la::Vector<Dimensions,int> strides = getStrides(nodesPerAxis);

  const double invDx = 1.0/dx;
  const double coeff0 = dt*invDx; //We assume that the cells are cubes, e.g. A/V = 1/dx

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
              quadratureNodes);

          flux( &cellQin[neighbourNode*strideQ],
                x,
                t,
                dt,
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
        quadratureNodes);

    if(useSource){
      //compute source term contributions
      source( &cellQin[node*strideQ],
              position,
              t,
              dt,
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
          dt,
          dim,
          ncpValues);

        for(int var=0; var<unknowns; var++){
          cellQout[node*strideQ+var] += dt * ncpValues[var];
        } //var

      } //dim
    } //if useNCP

  } //node
  } // loop over individual cells

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


double exahype2::dg::getMaxEigenvalue(
  MaxEigenvalue                                  maxEigenvalue,
  const double* __restrict__                     Q,
  const int                                      order,
  const int                                      strideQ,
  const tarch::la::Vector<Dimensions,double>&    x,
  const double                                   t,
  const double                                   dt
  ){

  const int nodesPerAxis = order+1;
  const int nodesPerCell = getNodesPerCell(nodesPerAxis);
  double maxEigenvalueCell = std::numeric_limits<double>::min();

  //for each node in the cell
  for(int node=0; node<nodesPerCell; node++){
    //for each direction
    for(int direction=0; direction<Dimensions; direction++){
      double maxEigenvalueNode = maxEigenvalue(
        &Q[node*strideQ],
        x,
        t,
        dt,
        direction
      );

      maxEigenvalueCell = std::max(maxEigenvalueCell, std::abs(maxEigenvalueNode));
    }
  }

  return maxEigenvalueCell;
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

