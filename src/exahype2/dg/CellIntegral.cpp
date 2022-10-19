#include "CellIntegral.h"

#include "peano4/utils/Loop.h"

#include "exahype2/enumerator/AoSLexicographicEnumerator.h"

void exahype2::dg::cellIntegral_patchwise_in_situ_GaussLegendre_functors(
  ::exahype2::CellData&                          cellData,
  const int                                      order,
  const int                                      unknowns,
  const int                                      auxiliaryVariables,
  Flux                                           flux,
  NonConservativeProduct                         nonconservativeProduct,
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
  exahype2::enumerator::AoSLexicographicEnumerator enumeratorIn(
    1,           // number of cells in one memory chunk
    order+1,     // numberOfDoFsPerAxisInCell
    0,           // no halo
    unknowns,
    auxiliaryVariables
  );

  exahype2::enumerator::AoSLexicographicEnumerator enumeratorOut(
    1,           // number of cells in one memory chunk
    order+1,     // numberOfDoFsPerAxisInCell
    0,           // no halo
    unknowns,
    0            // no auxiliary variables
  );

  double* fluxValues      = evaluateFlux                   ? new double[unknowns]                      : nullptr;
  double* sourceValues    = evaluateSource                 ? new double[unknowns]                      : nullptr;
  double* gradientValues  = evaluateNonconservativeProduct ? new double[(unknowns+auxiliaryVariables)] : nullptr;
  double* ncpValues       = evaluateNonconservativeProduct ? new double[unknowns]                      : nullptr;

  for (int currentCell=0; currentCell<cellData.numberOfCells; currentCell++) {
    const double* __restrict__                  cellQin  = cellData.QIn[currentCell];
    double* __restrict__                        cellQout = cellData.QOut[currentCell];
    const tarch::la::Vector<Dimensions,double>  x        = cellData.cellCentre[currentCell];
    const tarch::la::Vector<Dimensions,double>  h        = cellData.cellSize[currentCell];
    const double                                t        = cellData.t[currentCell];
    const double                                dt       = cellData.dt[currentCell];

    const int nodesPerAxis = order+1;
    const int strideQ=unknowns+auxiliaryVariables;
    const int nodesPerCell = getNodesPerCell(nodesPerAxis);

    dfor( dof, order+1 ) {
      for(int var=0; var<unknowns; var++){
        cellQout[enumeratorOut(currentCell,dof,var)] = 0.0;
      }

      if(evaluateFlux){
        // direction in which flux contributions are being computed
        for(int dim=0; dim<Dimensions; dim++)
        for(int i=0; i<nodesPerAxis; i++){
          // compute contributing node real position
          tarch::la::Vector<Dimensions,int> contributingNodeIndex = dof;
          contributingNodeIndex[dim] = i;
          tarch::la::Vector<Dimensions,double> position = getQuadraturePoint(
              x,
              h,
              contributingNodeIndex,
              order,
              quadratureNodes);

          flux( &cellQin[enumeratorIn(currentCell, contributingNodeIndex, 0)],
                position,
                t,
                dt,
                dim, //normal
                fluxValues);

          // Coefficient within matrix is tensor product over dimensions, too.
          // So we take the stiffness matrix in the direction that we are
          // evaluating and multiply it for the d-1 remaining directions with
          // the mass matrix. We integrate over the whole cell, but the h-terms
          // are eliminated along the derivative direction.
          double coeff = stiffnessMatrix[i+dof(dim)*nodesPerAxis];
          for (int dd=0; dd<Dimensions; dd++) {
            coeff *= (dd==dim) ? 1.0 : massMatrixDiagonal[ dof(dd) ] * h(dd);
          }

          for(int var=0; var<unknowns; var++){
            cellQout[enumeratorOut(currentCell, dof, var)] += coeff * fluxValues[var];
          } //var
        } // dim/i combination
      }//if useFlux

      if(evaluateSource){
        //compute real node position
        tarch::la::Vector<Dimensions,double> position = getQuadraturePoint(
          x,
          h,
          dof,
          order,
          quadratureNodes
        );

        //compute source term contributions
        source(
          &cellQin[enumeratorIn(currentCell, dof, 0)],
          position,
          t,
          dt,
          sourceValues
        );

        #if Dimensions==2
        double coeff = massMatrixDiagonal[ dof(0) ] * massMatrixDiagonal[ dof(1) ] * tarch::la::volume(h);
        #else
        double coeff = massMatrixDiagonal[ dof(0) ] * massMatrixDiagonal[ dof(1) ] * massMatrixDiagonal[ dof(2) ] * tarch::la::volume(h);
        #endif

        for(int var=0; var<unknowns; var++){
          cellQout[enumeratorOut(currentCell, dof, var)] += coeff * sourceValues[var];
        } //var
      } //if useSource

      // @todo Point Source missing

      if (evaluateNonconservativeProduct) {
        //compute real node position
        tarch::la::Vector<Dimensions,double> position = getQuadraturePoint(
          x,
          h,
          dof,
          order,
          quadratureNodes
        );

        // Mass matrix.
        double coeff = 1.0;
        for (int dd=0; dd<Dimensions; dd++) {
          coeff *= massMatrixDiagonal[ dof(dd) ] * h(dd);
        }

        for(int dim=0; dim<Dimensions; dim++) {
          //computes gradient values in all directions for the given node
          std::fill(gradientValues, &gradientValues[strideQ-1], 0.0);

          const int invDx = 1 / h(dim);

          // computing gradients in direction dim
          for(int node=0; node<nodesPerAxis; node++){
            tarch::la::Vector<Dimensions,int> contributingNodeIndex = dof;
            contributingNodeIndex[dim] = node;
            const double coeffDerivative = invDx * derivativeOperator[node + nodesPerAxis*dof(dim)];

            for(int var=0; var<strideQ; var++){
              gradientValues[var] += coeffDerivative * cellQin[enumeratorIn(currentCell, contributingNodeIndex, var)];
              assertion(gradientValues[var]==gradientValues[var]);
            } // var
          } //node

          nonconservativeProduct(
            &cellQin[enumeratorIn(currentCell, dof, 0)],
            gradientValues,
            position,
            t,
            dt,
            dim,
            ncpValues);

          for(int var=0; var<unknowns; var++){
            assertion(ncpValues[var]==ncpValues[var]);
            cellQout[enumeratorOut(currentCell, dof, var)] -= coeff * ncpValues[var];
          } //var
        } //dim
      } //if useNCP
    } // dof
  } //currentCell

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


void exahype2::dg::reduceMaxEigenvalue_patchwise_functors(
  ::exahype2::CellData&   cellData,
  const int               order,
  const int               unknowns,
  const int               auxiliaryVariables,
  MaxEigenvalue           maxEigenvalue,
  const double* __restrict__   QuadratureNodes1d
) {
  ::exahype2::enumerator::AoSLexicographicEnumerator  enumerator(
    1, // number of patches stored per continuous memory segment
    order+1,
    0, // halo
    unknowns,
    auxiliaryVariables
  );

  for (int cellIndex=0; cellIndex<cellData.numberOfCells; cellIndex++) {
    double eigenvalue = std::numeric_limits<double>::min();

    dfor(k,order+1) {
      for(int direction=0; direction<Dimensions; direction++) {
        double nodeEigenvalue = maxEigenvalue(
          cellData.QOut[cellIndex] + enumerator(0,k,0),
          getQuadraturePoint(
            cellData.cellCentre[cellIndex],
            cellData.cellSize[cellIndex],
            k,
            order,
            QuadratureNodes1d
          ),
          cellData.t[cellIndex],
          cellData.dt[cellIndex],
          direction
        );

        assertion( nodeEigenvalue>=0.0 );

        eigenvalue = std::max(eigenvalue, nodeEigenvalue);
      }
    }

    cellData.maxEigenvalue[cellIndex] = eigenvalue;
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


void exahype2::dg::multiplyWithInvertedMassMatrix_GaussLegendre(
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
      double diagonal = massMatrixDiagonal1d[ dof(0) ] * massMatrixDiagonal1d[ dof(1) ] * (cellData.cellSize[currentCell])(0) * (cellData.cellSize[currentCell])(1);
      #else
      double diagonal = massMatrixDiagonal1d[ dof(0) ]
                      * massMatrixDiagonal1d[ dof(1) ]
                      * massMatrixDiagonal1d[ dof(2) ]
                      * (cellData.cellSize[currentCell])(0)
                      * (cellData.cellSize[currentCell])(1)
                      * (cellData.cellSize[currentCell])(2);
      #endif

      assertion( diagonal>0.0 );

      for(int var=0; var<unknowns; var++) {
        cellQInOut[ enumerator(currentCell,dof,var) ] = 1.0 / diagonal * cellQInOut[enumerator(currentCell,dof,var)];
      }
    }
  }
}
