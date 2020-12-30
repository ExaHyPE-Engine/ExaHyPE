#include "Generic.h"
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
    const int                  linearisedIndex
) {
  tarch::la::Vector<Dimensions+1,int> strides = getStrides(nodesPerAxis,false);
  tarch::la::Vector<Dimensions+1,int> index   = delineariseIndex(linearisedIndex,strides);

  const double invDx = 1.0/dx;

  for ( int d = 1; d < Dimensions+1; d++) {
    const double coeff0 = dt * invDx/*[d-1]*/ / weights[index[d]];
      
    for ( int id = 0; id < nodesPerAxis; id++ ) { // loop over spatial neighbours
      const double coeff1 = coeff0 * Kxi[index[d]][id]; // note: transposed vs. predictor flux computation
      for ( int it = 0; it < nodesPerAxis; it++ ) { // time loop
        const int linearisedIndexQ = ( linearisedIndex + (id - index[d])*strides[d] )*nodesPerAxis + it;
        const double* Q = QIn[ linearisedIndexQ*strideQ ];
        
        tarch::la::Vector<Dimensions+1,double> coords = getCoordinates(
          delineariseIndex(linearisedIndexQ,getStrides(nodesPerAxis)),
          centre,dx,t,dt,nodes);
        tarch::la::Vector<Dimensions,double> x(&coords[1]);
        const double t = coords[0];

        flux( Q, x, t, d-1, FAux); 
        const double coeff = coeff1 * weights[it]; // note: transposed vs. predictor flux computation
        for (int var=0; var < unknowns; var++) {
          UOut[ linearisedIndex*strideRhs+var ] += coeff * FAux[ var ]; // note: different vs predictor flux computation
        }
      }
    }
  }
}
