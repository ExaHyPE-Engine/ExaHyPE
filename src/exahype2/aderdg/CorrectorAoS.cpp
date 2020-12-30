#include "CorrectorAoS.h"
#include "tarch/la/Vector.h"

GPUCallableMethod void exahype2::aderdg::corrector_addFluxContributions_body_AoS(
    std::function< void(
      double * __restrict__                       Q,
      const tarch::la::Vector<Dimensions,double>& x,
      double                                      t,
      int                                         normal,
      double * __restrict__                       F
    ) >                        flux,
    double* __restrict__       UOut, 
    const double* __restrict__ QIn,
    double* __restrict__       FAux, // must be allocated per thread as size is runtime parameter
    const double* __restrict__ nodes,
    const double* __restrict__ weights,
    const double* __restrict__ Kxi,
    const double               centre,
    const double               dx,
    const double               t,
    const double               dt,
    const int                  nodesPerAxis,
    const int                  unknowns,
    const int                  strideQ,
    const int                  strideRhs,
    const int                  linearisedIndex) {
  tarch::la::Vector<Dimensions+1,int> strides = getStrides(nodesPerAxis,false);
  tarch::la::Vector<Dimensions+1,int> index   = delineariseIndex(linearisedIndex,strides);
  const tarch::la::Vector<Dimensions+1, double> coords = getCoordinates(index,cellCentre,dx,t,dt,nodes);
  const tarch::la::Vector<Dimensions, double>   x( ( coords.data() + 1 ) );

  const double invDx = 1.0/dx;

  for ( int d = 1; d < Dimensions+1; d++) {
    const double coeff0 = dt * invDx/*[d-1]*/ / weights[index[d]];
      
    for ( int id = 0; id < nodesPerAxis; id++ ) { // loop over spatial neighbours
      const double coeff1 = coeff0 * Kxi[index[d]][id]; // note: transposed vs. predictor flux computation
      for ( int it = 0; it < nodesPerAxis; it++ ) { // time loop
        const int linearisedIndexQ = ( linearisedIndex + (id - index[d])*strides[d] )*nodesPerAxis + it;
        const double* Q = QIn[ linearisedIndexQ*strideQ ];
        
        const double time = t + nodes[it] * dt; 
        const double coeff = coeff1 * weights[it]; // note: transposed vs. predictor flux computation

        flux( Q, x, time, d-1, FAux); 
        for (int var=0; var < unknowns; var++) {
          UOut[ linearisedIndex*strideRhs+var ] += coeff * FAux[ var ]; // note: different vs predictor flux computation
        }
      }
    }
  }
}

GPUCallableMethod void exahype2::aderdg::corrector_addSourceContribution_body_AoS(
    std::function< void(
      double * __restrict__                       Q,
      const tarch::la::Vector<Dimensions,double>& x,
      double                                      t,
      double * __restrict__                       S
    ) >                                         algebraicSource,
    double* __restrict__                        UOut,
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
    const int                                   strideQ,
    const int                                   strideRhs,
    const int                                   linearisedIndex) {
  const tarch::la::Vector<Dimensions+1,int>     index  = delineariseIndex(linearisedIndex,getStrides(nodesPerAxis,false));
  const tarch::la::Vector<Dimensions+1, double> coords = getCoordinates(index,cellCentre,dx,t,dt,nodes);
  const tarch::la::Vector<Dimensions, double>   x( ( coords.data() + 1 ) );
 
  for ( int it=0; it < nodesPerAxis; it++ ) { // time-integral 
    const int linearisedIndexQ = linearisedIndex*nodesPerAxis + it;
    const double* Q            = QIn[ linearisedIndexQ*strideQ ];
    
    const double time = t + nodes[it] * dt; 
    algebraicSource(Q, x, time, Q, SAux);
    
    const double coeff = dt * weights[it];
    for (int var = 0; var < unknowns; var++) {
      UOut[ linearisedIndex*strideRhs ] += coeff * SAux[var];
    }
  }
}

GPUCallableMethod void exahype2::aderdg::corrector_addNcpContribution_body_AoS(
    std::function< void(
      double * __restrict__                       Q,
      double                                      gradQ[][Dimensions],
      const tarch::la::Vector<Dimensions,double>& x,
      double                                      t,
      double * __restrict__                       BgradQ
    ) >                                         nonconservativeProduct,
    double* __restrict__                        UOut,
    const double* __restrict__                  QIn,
    const double* __restrict__                  gradQAux,
    const double* __restrict__                  SAux,
    const double* __restrict__                  nodes,
    const double* __restrict__                  weights,
    const double* __restrict__                  dudx,
    const tarch::la::Vector<Dimensions,double>& cellCentre,
    const double                                dx,
    const double                                t,
    const double                                dt,
    const int                                   nodesPerAxis,
    const int                                   unknowns,
    const int                                   strideRhs,
    const int                                   linearisedIndex) {
  const tarch::la::Vector<Dimensions+1,int>     index  = delineariseIndex(linearisedIndex,getStrides(nodesPerAxis,false));
  const tarch::la::Vector<Dimensions+1, double> coords = getCoordinates(index,cellCentre,dx,t,dt,nodes);
  const tarch::la::Vector<Dimensions, double>   x( ( coords.data() + 1 ) );

  for ( int it=0; it < nodesPerAxis; it++ ) { // time-integral 
    const int linearisedIndexQ = linearisedIndex*nodesPerAxis + it; 
    
    gradient_AoS( QIn, dudx, invDx, nodesPerAxis, unknowns, strideQ, strideGradQ, linearisedIndexQ, gradQAux );
    
    const double time = t + nodes[it] * dt; 
    double* Q         = QIn[ linearisedIndexQ*strideQ ];
    nonConservativeProduct( Q, gradQAux, x, time, SAux );
    
     const double coeff = dt * weights[it];
    for(int var=0; var<unknowns; var++) {
      UOut[ linearisedIndex*strideRhs + var ] += coeff * SAux[var];
    }
  }
}
