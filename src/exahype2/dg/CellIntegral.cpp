#include "CellIntegral.h"

#include "peano4/utils/Loop.h"

#include "exahype2/enumerator/AoSLexicographicEnumerator.h"


void exahype2::dg::cellIntegral_patchwise_GaussLegendre_functors(
  ::exahype2::CellData&                          cellData,
  const int                                      order,
  const int                                      unknowns,
  const int                                      auxiliaryVariables,
  Flux                                           flux,
  NonConservativeProduct                         nonConservativeProduct,
  Source                                         source,
  PointSources                                   pointSources,
  const double* __restrict__                     quadratureNodes,
  const double* __restrict__                     massMatrixDiagonal,
  const double* __restrict__                     stiffnessMatrix,
  const double* __restrict__                     derivativeOperator,
  bool                                           evaluateFlux,
  bool                                           evaluateNonconservativeProduct,
  bool                                           evaluateSource,
  bool                                           evaluatePointSources
) {
  double* fluxValues      = evaluateFlux   ? new double[unknowns] : nullptr;
  double* sourceValues    = evaluateSource ? new double[unknowns] : nullptr;
  double* gradientValues  = evaluateNonconservativeProduct    ? new double[unknowns+auxiliaryVariables] : nullptr;
  double* ncpValues       = evaluateNonconservativeProduct    ? new double[unknowns] : nullptr;

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

    //which node's value is being computed
    for(int node=0; node<nodesPerCell; node++){
      for(int var=0; var<unknowns; var++){
        cellQout[node*unknowns+var] = 0.0;
      }

      tarch::la::Vector<Dimensions,int> index = getIndex(node, strides);

      if(evaluateFlux){
        // direction in which flux contributions are being computed
        for(int dim=0; dim<Dimensions; dim++)
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
                position,
                t,
                dt,
                dim, //normal
                fluxValues);

          // Coefficient within matrix is tensor product over dimensions, too
          double coeff = stiffnessMatrix[i+index[dim]*nodesPerAxis];
          for (int dd=0; dd<Dimensions; dd++) {
            coeff *= (dd==dim) ? 1.0 : massMatrixDiagonal[ index[dd] ];
          }

          for(int var=0; var<unknowns; var++){
            cellQout[node*unknowns+var] += coeff * fluxValues[var];
          } //var
        } // dim/i combination
      }//if useFlux

      if(evaluateSource){
        //compute real node position
        tarch::la::Vector<Dimensions,double> position = getQuadraturePoint( x,
          dx,
          index,
          order,
          quadratureNodes);

        //compute source term contributions
        source( &cellQin[node*strideQ],
              position,
              t,
              dt,
              sourceValues);

        #if Dimensions==2
        double coeff = massMatrixDiagonal[ index(0) ] * massMatrixDiagonal[ index(1) ];
        #else
        double coeff = massMatrixDiagonal[ index(0) ] * massMatrixDiagonal[ index(1) ] * massMatrixDiagonal[ index(2) ];
        #endif

        for(int var=0; var<unknowns; var++){
          cellQout[node*unknowns+var] += coeff * sourceValues[var];
        } //var
      } //if useSource

      // @todo Point Source missing

      if (evaluateNonconservativeProduct) {
        //compute real node position
        tarch::la::Vector<Dimensions,double> position = getQuadraturePoint( x,
          dx,
          index,
          order,
          quadratureNodes);

        computeGradient(
          cellQin,
          derivativeOperator,
          1.0 / dx,
          nodesPerAxis,
          strideQ,
          node,
          gradientValues
        );

        for(int dim=0; dim<Dimensions; dim++) {
          nonConservativeProduct(
            &cellQin[node*strideQ],
            &gradientValues[dim*strideQ],
            position,
            t,
            dt,
            dim,
            ncpValues);

          // @todo There's a matrix missing, isn't it? Or
          //  can we now really model this via the mass matrix?
          double coeff = 1.0;
          for (int dd=0; dd<Dimensions; dd++) {
            coeff *= massMatrixDiagonal[ index(dim) ];
          }

          for(int var=0; var<unknowns; var++){
            cellQout[node*unknowns+var] += coeff * ncpValues[var];
          } //var
        } //dim
      } //if useNCP
    } //node
  } // loop over individual cells

  if (evaluateFlux) {
    delete [] fluxValues;
  }
  if (evaluateSource) {
    delete [] sourceValues;
  }
  if (evaluateNonconservativeProduct) {
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


void exahype2::dg::multiplyWithInvertedMassMatrixAndClearAuxiliaryVariables_GaussLegendre(
  ::exahype2::CellData&                          cellData,
  const int                                      order,
  const int                                      unknowns,
  const int                                      auxiliaryVariables,
  const double* __restrict__                     massMatrixDiagonal1d
) {
  exahype2::enumerator::AoSLexicographicEnumerator enumerator(
    1,           // number of cells in one memory chunk
    order+1,     // numberOfDoFsPerAxisInCell
    0,           // no halo
    unknowns,
    0            // no auxiliary variables here
  );

  for (int currentCell=0; currentCell<cellData.numberOfCells; currentCell++) {
    dfor( dof, order+1 ) {
      double* __restrict__  cellQInOut = cellData.QOut[currentCell];

      #if Dimensions==2
      double diagonal = massMatrixDiagonal1d[ dof(0) ] * massMatrixDiagonal1d[ dof(1) ];
      #else
      double diagonal = massMatrixDiagonal1d[ dof(0) ] * massMatrixDiagonal1d[ dof(1) ] * massMatrixDiagonal1d[ dof(2) ];
      #endif

      assertion( diagonal>0.0 );

      for(int var=0; var<unknowns; var++) {
        cellQInOut[ enumerator(currentCell,dof,var) ] = 1.0 / diagonal * cellQInOut[enumerator(currentCell,dof,var)];
      }
    }
  }
}
