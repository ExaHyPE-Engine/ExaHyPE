#include "CorrectorAoS.h"
//#include <functional>

#include "tarch/la/Vector.h"

#include "KernelUtils.h"

namespace exahype2 { 
  namespace aderdg {

    GPUCallableMethod void corrector_addFluxContributions_body_AoS(
        std::function< void(
          const double * __restrict__                 Q,
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
        const double               cellCentre,
        const double               dx,
        const double               t,
        const double               dt,
        const int                  nodesPerAxis,
        const int                  unknowns,
        const int                  strideQ,
        const int                  linearisedIndex) {
      tarch::la::Vector<Dimensions+1,int> strides = getStrides(nodesPerAxis,false);
      tarch::la::Vector<Dimensions+1,int> index   = delineariseIndex(linearisedIndex,strides);
      const tarch::la::Vector<Dimensions+1, double> coords = getCoordinates(index,cellCentre,dx,t,dt,nodes);
      const tarch::la::Vector<Dimensions, double>   x( ( coords.data() + 1 ) );
    
      const double invDx = 1.0/dx;
    
      for ( int d = 1; d < Dimensions+1; d++) {
        const double coeff0 = dt * invDx/*[d-1]*/ / weights[index[d]];
          
        for ( int id = 0; id < nodesPerAxis; id++ ) { // loop over spatial neighbours
          const double coeff1 = coeff0 * Kxi[ index[d]*nodesPerAxis + id ]; // note: transposed vs. predictor flux computation
          for ( int it = 0; it < nodesPerAxis; it++ ) { // time loop
            const int linearisedIndexQ = ( linearisedIndex + (id - index[d])*strides[d] )*nodesPerAxis + it;
            const double* Q = &QIn[ linearisedIndexQ*strideQ ];
            
            const double time = t + nodes[it] * dt; 
            const double coeff = coeff1 * weights[it]; // note: transposed vs. predictor flux computation
    
            flux( Q, x, time, d-1, FAux ); 
            for (int var=0; var < unknowns; var++) {
              UOut[ linearisedIndex*strideQ+var ] += coeff * FAux[ var ]; // note: different vs predictor flux computation
            }
          }
        }
      }
    }
    
    GPUCallableMethod void corrector_addSourceContribution_body_AoS(
        std::function< void(
          const double * __restrict__                 Q,
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
        const int                                   linearisedIndex) {
      const tarch::la::Vector<Dimensions+1,int>     index  = delineariseIndex(linearisedIndex,getStrides(nodesPerAxis,false));
      const tarch::la::Vector<Dimensions+1, double> coords = getCoordinates(index,cellCentre,dx,t,dt,nodes);
      const tarch::la::Vector<Dimensions, double>   x( ( coords.data() + 1 ) );
     
      for ( int it=0; it < nodesPerAxis; it++ ) { // time-integral 
        const int linearisedIndexQ = linearisedIndex*nodesPerAxis + it;
        const double* Q            = &QIn[ linearisedIndexQ*strideQ ];
        
        const double time = t + nodes[it] * dt; 
        algebraicSource(Q, x, time, SAux);
        
        const double coeff = dt * weights[it];
        for (int var = 0; var < unknowns; var++) {
          UOut[ linearisedIndex*strideQ ] += coeff * SAux[var];
        }
      }
    }
    
    GPUCallableMethod void corrector_addNcpContribution_body_AoS(
        std::function< void(
          const double * __restrict__                 Q,
          double                                      gradQ[][Dimensions],
          const tarch::la::Vector<Dimensions,double>& x,
          double                                      t,
          double * __restrict__                       BgradQ
        ) >                                         nonconservativeProduct,
        double* __restrict__                        UOut,
        double* __restrict__                        gradQAux,
        double* __restrict__                        SAux,
        const double* __restrict__                  QIn,
        const double* __restrict__                  nodes,
        const double* __restrict__                  weights,
        const double* __restrict__                  dudx,
        const tarch::la::Vector<Dimensions,double>& cellCentre,
        const double                                dx,
        const double                                t,
        const double                                dt,
        const int                                   nodesPerAxis,
        const int                                   unknowns,
        const int                                   strideQ,
        const int                                   linearisedIndex) {
      const tarch::la::Vector<Dimensions+1,int>     index  = delineariseIndex(linearisedIndex,getStrides(nodesPerAxis,false));
      const tarch::la::Vector<Dimensions+1, double> coords = getCoordinates(index,cellCentre,dx,t,dt,nodes);
      const tarch::la::Vector<Dimensions, double>   x( ( coords.data() + 1 ) );
   
      const double invDx = 1.0/dx;
 
      for ( int it=0; it < nodesPerAxis; it++ ) { // time-integral 
        const int linearisedIndexQ = linearisedIndex*nodesPerAxis + it; 
        
        gradient_AoS( QIn, dudx, invDx, nodesPerAxis, strideQ, linearisedIndexQ, gradQAux );
        
        const double time = t + nodes[it] * dt; 
        const double* Q   = &QIn[ linearisedIndexQ*strideQ ];
        nonconservativeProduct( Q, (double (*)[Dimensions]) gradQAux, x, time, SAux );
        
         const double coeff = dt * weights[it];
        for(int var=0; var<unknowns; var++) {
          UOut[ linearisedIndex*strideQ + var ] += coeff * SAux[var];
        }
      }
    }
    
    GPUCallableMethod void corrector_addRiemannContributions_body_AoS(
      double * __restrict__       UOut,
      const double * __restrict__ riemannResultIn,
      const double * __restrict__ weights,
      const double * __restrict__ FCoeff[2],
      const double                invDx,
      const double                dt,
      const int                   nodesPerAxis,
      const int                   unknowns,
      const int                   strideQ,
      const int                   linearisedIndex) {
      const tarch::la::Vector<Dimensions+1,int> index = delineariseIndex(linearisedIndex,getStrides(nodesPerAxis,false));
      
      for (int d=0; d < Dimensions; d++) {
        for (int lr=0; lr<2; lr++) {
          const int linearisedIndexFace = mapCellIndexToLinearisedHullIndex(index,d,lr,nodesPerAxis);
    
          for (int id=0; id < nodesPerAxis; id++) { 
            const double coeff = dt * FCoeff[lr][index[d+1]] * invDx/*[d]*/;
            for (int var=0; var < unknowns; var++) {
              UOut[ linearisedIndex*strideQ + var ] += coeff * riemannResultIn[ (linearisedIndexFace*nodesPerAxis + id)*strideQ + var ]; 
            }
          }
        }
      } 
    }

  }
}
