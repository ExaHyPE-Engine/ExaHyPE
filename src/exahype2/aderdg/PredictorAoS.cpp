#include "Predictor.h"
#include "KernelUtils.h"

GPUCallableMethod void exahype2::aderdg::clearAll_body_AoS(
  double *   __restrict__ data,
  const int               stride,
  const int               linearisedIndex) {
  for (int elem = 0; elem < stride; elem++) {
    data[ linearisedIndex*stride + elem ] = 0.0;
  }
}

GPUCallableMethod void exahype2::aderdg::clearRange_body_AoS(
  double *   __restrict__ data,
  const int               range,
  const int               stride,
  const int               linearisedIndex) {
  for (int elem = 0; elem < range; elem++) {
    data[ linearisedIndex*stride + elem ] = 0.0;
  }
}

// @todo put utility functions at top to right location

GPUCallableMethod void exahype2::aderdg::spaceTimePredictor_initialGuess_body_AoS(
  double*       __restrict__ Qout,
  const double* __restrict__ UIn,
  const int                  nodesPerAxis,
  const int                  strideQ,
  const int                  linearisedIndex) {
  for (int var = 0; var < strideQ; var++) {
    Qout[ linearisedIndex*strideQ + var ] = UIn[ ( linearisedIndex / nodesPerAxis )*strideQ + var ];
  }
}

GPUCallableMethod void exahype2::aderdg::spaceTimePredictor_PicardLoop_initialiseRhs_AoS(
  double* __restrict__       rhsOut,
  const double* __restrict__ UIn,
  const double* __restrict__ FLCoeff,
  const int                  nodesPerAxis,
  const int                  strideQ,
  const int                  linearisedIndex) {
  const int it = delineariseIndex(linearisedIndex,getStrides(nodesPerAxis))[0];
  const double coeff = FLCoeff[ it ];
  
  for (int var = 0; var < unknowns+auxiliaryVariables; var++) {
    rhsOut[ linearisedIndex*strideRhs + var ] = coeff * UIn[ ( linearisedIndex / nodesPerAxis ) * strideQ + var ];
  }
}

GPUCallableMethod void exahype2::aderdg::spaceTimePredictor_PicardLoop_addFluxContributionsToRhs_body_AoS (
    std::function< void(
      double * __restrict__                       Q,
      const tarch::la::Vector<Dimensions,double>& x,
      double                                      t,
      int                                         normal,
      double * __restrict__                       F
    ) >   flux,
    double* __restrict__                          rhsOut, 
    double* __restrict__                          FAux, 
    const double* __restrict__                    QIn 
    const double* __restrict__                    nodes,
    const double* __restrict__                    weights,
    const double* __restrict__                    Kxi,
    const double                                  t,
    const double                                  dt,
    const tarch::la::Vector<Dimensions+1,double>& centre,
    const double                                  invDx,
    const int                                     nodesPerAxis,
    const int                                     unknowns,
    const int                                     strideQ,
    const int                                     strideRhs,
    const int                                     linearisedIndex) {
  tarch::la::Vector<Dimensions+1,int> strides   = getStrides(nodesPerAxis);
  tarch::la::Vector<Dimensions+1,int> index     = delineariseIndex(linearisedIndex,strides);
  tarch::la::Vector<Dimensions+1,double> coords = getCoordinates(index,centre,dx,t,dt,nodes);
  tarch::la::Vector<Dimensions,double> x(&coords[1]);
  const double t = coords[0];
  
  for ( int d = 1; d < Dimensions+1; d++) {
    const double coeff0 = dt * invDx/*[d-1]*/ * weights[index[0]] / weights[index[d]];
      
    for ( int a = 0; a < nodesPerAxis; a++ ) { // further collapsing causes data races, synchronization or GPU shared mem usage required
      const double coeff = coeff0 * Kxi[a][index[d]];
      const double * Q = QIn[ ( linearisedIndex + (a - index[d])*strides[d] )*strideQ ];
      flux(Q, x, t, d-1, FAux);
      for (int var=0; var < unknowns; var++) {
        rhsOut[ linearisedIndex*strideRhs+var ] -= coeff * FAux[ var ];
      }
    }
  }
}

GPUCallableMethod void exahype2::aderdg::spaceTimePredictor_PicardLoop_addSourceContributionToRhs_body_AoS(
    std::function< void(
      double * __restrict__                       Q,
      const tarch::la::Vector<Dimensions,double>& x,
      double                                      t,
      double * __restrict__                       S
    ) >   algebraicSource,
    double* __restrict__                       rhsOut,
    double* __restrict__                       SAux,
    const double* __restrict__                 QIn,
    const double* __restrict__                 nodes,
    const double* __restrict__                 weights,
    const tarch::la::Vector<Dimensions,double> cellCentre,
    const double                               dx,
    const double                               t,
    const double                               dt,
    const int                                  nodesPerAxis,
    const int                                  unknowns,
    const int                                  strideQ,
    const int                                  strideRhs,
    const int                                  linearisedIndex) {
  const tarch::la::Vector<Dimensions+1,int>     index  = delineariseIndex(linearisedIndex, getStrides(nodesPerAxis));
  const tarch::la::Vector<Dimensions+1, double> coords = getCoordinates(index,cellCentre,dx,t,dt,nodes);
  const tarch::la::Vector<Dimensions, double> x( ( coords.data() + 1 ) );
  const double int t = coords[0];
  
  const double coeff = dt * weights[index[0]];
  
  const double* Q    = QIn[ linearisedIndex*strideQ ];
  double*       SAux = S  [ linearisedIndex*strideQ ];

  algebraicSource(Q, x, t, SAux);
  for (int var = 0; var < unknowns; var++) {
    rhsOut[ linearisedIndex*strideRhs ] += coeff * SAux[var];
  }
}

GPUCallableMethod void exahype2::aderdg::gradient_AoS(
  const double* __restrict__ QIn,
  const double* __restrict__ dudx,
  const double               invDx,
  const int                  nodesPerAxis,
  const int                  unknowns,
  const int                  strideQ,
  const int                  strideGradQ,
  const int                  linearisedIndex,
  double* __restrict__       gradQAux) {
  const tarch::la::Vector<Dimensions+1,int> strides = getStrides(nodesPerAxis);
  const tarch::la::Vector<Dimensions+1,int> index   = delineariseIndex(linearisedIndex, strides);
  
  for ( int d = 0; d < Dimensions; d++ ) { // x -> y -> z
    for (int a=0; a < nodesPerAxis; a++) { 
      const coeff = invDx/*[d]*/ * dudx[ a ][ index[d+1] ];
      for (int var=0; var < unknowns; var++) {
        //gradQ[ d*Dimensions+var ] += coeff * QIn[ ( linearisedIndex + (a-index[d+1])*strides[d+1] )*stridesQ + var ]; 
        gradQ[ d + var*Dimensions ] += coeff * QIn[ ( linearisedIndex + (a-index[d+1])*strides[d+1] )*stridesQ + var ]; 
      }
    }
  }
}

GPUCallableMethod void exahype2::aderdg::spaceTimePredictor_PicardLoop_addNcpContributionToRhs_body_AoS(
  std::function< void(
    double * __restrict__                       Q,
    double                                      gradQ[][Dimensions],
    const tarch::la::Vector<Dimensions,double>& x,
    double                                      t,
    double * __restrict__                       BgradQ
  ) >                                         nonconservativeProduct,
  double* __restrict__                        rhsOut,
  double* __restrict__                        gradQAux,
  double* __restrict__                        SAux,
  const double* __restrict__                  QIn,
  const double* __restrict__                  nodes,
  const double* __restrict__                  weights,
  const tarch::la::Vector<Dimensions,double>& cellCentre,
  const double                                dx,
  const double                                t,
  const double                                dt,
  const int                                   nodesPerAxis,
  const int                                   unknowns,
  const int                                   strideRhs,
  const int                                   strideQ,
  const int                                   linearisedIndex) {
  tarch::la::Vector<Dimensions+1,int> strides = getStrides(nodesPerAxis);
  tarch::la::Vector<Dimensions+1,int> index   = delineariseIndex(linearisedIndex,strides);
  const tarch::la::Vector<Dimensions+1, double> coords = getCoordinates(index,cellCentre,dx,t,dt,nodes);
  const tarch::la::Vector<Dimensions, double> x( ( coords.data() + 1 ) );
  const double int t = coords[0];
  
  const double coeff = dt * weights[index[0]];
  
  gradient_AoS(QIn,dudx,invDx,nodesPerAxis,unknowns,strideQ,strideGradQ,linearisedIndex,gradQAux);
 
  double* Q    = QIn [ linearisedIndex*strideQ ];
  nonConservativeProduct(Q, (double (*)[Dimensions]) gradQAux, x, t, SAux );
  for(int var=0; var < unknowns; var++) {
    rhsOut[ linearisedIndex*strideRhs + var ] += coeff * SAux[var];
  }
}

GPUCallableMethod void exahype2::aderdg::spaceTimePredictor_PicardLoop_invert_body_AoS(
  double * __restrict__       QOut,
  double&                     squaredResiduumOut,
  const double * __restrict__ rhsIn,
  const double * __restrict__ iK1,
  const int                   nodesPerAxis,
  const int                   unknowns,
  const int                   strideQ) {
  const int it = delineariseIndex(index,getStrides(nodesPerAxis))[0];
   
  squaredResiduumOut = 0.0; 
  for (int var = 0; var < unknowns; var++) {
    double Q_new = 0;
    //const double coeff = iweight * rhsIn[ linearisedIndex*strideRhs + var ]; 
    const double coeff = rhsIn[ linearisedIndex*strideRhs + var ]; 
    // @todo: check if operator is correct
    for (int a = 0; a < nodesPerAxis; a++) { // matmul time
      Q_new += coeff * iK1[it][a]; // note: iK1 is already the transposed inverse of K1
    }
    const double difference = Q_new - QOut[ linearisedIndex*strideQ + var ]; 
    squaredResiduumOut += difference * difference;
    QOut[ linearisedIndex*strideQ + var ] = Q_new;
    
    //@todo: enable in CPU version
    //assertion3( !std::isnan( Q[ linearisedIndex ] ), linearisedIndex, dt, invDx );
    //assertion3( !std::isnan(Q_new), linearisedIndex, dt, invDx );
  }

  // @todo: Get feedback from M.D.
  // Qt is fundamental for debugging, do not remove this.
  /*
  double lQt[nodesPerAxis * unknowns];
  idx2 idx_lQt(nodesPerAxis, unknowns);
  for (int j = 0; j < nodesPerAxis; j++) {
    for (int k = 0; k < nodesPerAxis; k++) {
      const double weight = weights[j] *
                            weights[k];
      const double iweight = 1.0 / weight;

      std::memset(lQt, 0, nodesPerAxis * unknowns * sizeof(double));
      for (int l = 0; l < nodesPerAxis; l++) {
        for (int m = 0; m < unknowns; m++) {
          for (int n = 0; n < nodesPerAxis; n++) { // t == n
            lQt[idx_lQt(l, m)] += 1./dt * QOut[idx_lQi(j, k, n, m)] *
                                        dudx[l][n];
          }
          printf("Qt[%d,%d] = %f\n", l, m, lQt[idx_lQt(l,m)]);
        }
      }
    }
  }
  */
}

GPUCallableMethod void exahype2::aderdg::spaceTimePredictor_extrapolate_body_AoS(
    double * __restrict__       QHullOut,
    const double * __restrict__ QIn,
    const double * __restrict__ FCoeff[2],
    const double                invDx,
    const int                   nodesPerAxis,
    const int                   unknowns,
    const int                   auxiliaryVariables,
    const int                   strideQ,
    const int                   linearisedIndexHull) {
  const tarch::la::Vector<Dimensions+1,int> strides    = getStrides(nodesPerAxis);
  const tarch::la::Vector<Dimensions+1,int> indexQHull = delineariseIndex(linearisedIndexHull,strides); // (t,y,z,face) , (t,x,z,face), (t,x,y,face)
 
  const int d  = indexQHull[Dimensions] / 2;
  const int lr = indexQHull[Dimensions] - 2*d;

  // clear
  for (int var=0; var < unknowns; var++) {
    QHullOut[ linearisedIndexHull * strideQ + var ] = 0.0; 
  }
  for (int id=0; id<nodesPerAxis; id++) {
    const int linearisedIndexQ = mapSpaceTimeFaceIndexToLinearisedCellIndex(indexQHull,d,id);

    const double coeff = FCoeff[lr][id];
    for (int var=0; var < unknowns; var++) {
      QHullOut[ linearisedIndexHull * strideQ + var ] += coeff * QIn[ linearisedIndexQ*strideQ + var ]; 
    }
  }
}

#ifdef __HIP__

// kernels

// launcher

#endif
