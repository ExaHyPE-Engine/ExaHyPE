#include "PredictorAoS.h"

#include "KernelUtils.h"

#include "tarch/la/Vector.h"

namespace exahype2 {
  namespace aderdg {

    #if defined(GPUOffloading)
    #pragma omp declare target
    #endif
    GPUCallableMethod void clearAll_body_AoS(
      double *  __restrict__ data,
      const int              stride,
      const int              scalarIndex) {
      for (int elem = 0; elem < stride; elem++) {
        data[ scalarIndex*stride + elem ] = 0.0;
      }
    }
    #if defined(GPUOffloading)
    #pragma omp end declare target
    #endif
    
    #if defined(GPUOffloading)
    #pragma omp declare target
    #endif
    GPUCallableMethod void clearRange_body_AoS(
      double *  __restrict__ data,
      const int              range,
      const int              stride,
      const int              scalarIndex) {
      for (int elem = 0; elem < range; elem++) {
        data[ scalarIndex*stride + elem ] = 0.0;
      }
    }
    #if defined(GPUOffloading)
    #pragma omp end declare target
    #endif
    
    // @todo put utility functions at top to right location
    
    #if defined(GPUOffloading)
    #pragma omp declare target
    #endif
    GPUCallableMethod void spaceTimePredictor_initialGuess_body_AoS(
      double *       __restrict__ Qout,
      const double * __restrict__ UIn,
      const int                   nodesPerAxis,
      const int                   strideQ,
      const int                   scalarIndex) {
      for (int var = 0; var < strideQ; var++) {
        Qout[ scalarIndex*strideQ + var ] = UIn[ ( scalarIndex / nodesPerAxis )*strideQ + var ];
      }
    }
    
    #if defined(GPUOffloading)
    #pragma omp end declare target
    #endif
    
    #if defined(GPUOffloading)
    #pragma omp declare target
    #endif
    GPUCallableMethod void spaceTimePredictor_PicardLoop_initialiseRhs_AoS(
      double * __restrict__       rhsOut,
      const double * __restrict__ UIn,
      const double * __restrict__ FLCoeff,
      const int                  nodesPerAxis,
      const int                  strideQ,
      const int                  strideRhs,
      const int                  scalarIndex) {
      const int it = delineariseIndex(scalarIndex,getStrides(nodesPerAxis))[0];
      const double coeff = FLCoeff[ it ];
      
      for (int var = 0; var < strideQ; var++) {
        rhsOut[ scalarIndex*strideRhs + var ] = coeff * UIn[ ( scalarIndex / nodesPerAxis ) * strideQ + var ];
      }
    }
    
    #if defined(GPUOffloading)
    #pragma omp end declare target
    #endif
    
    #if defined(GPUOffloading)
    #pragma omp declare target
    #endif
    GPUCallableMethod void spaceTimePredictor_PicardLoop_addFluxContributionsToRhs_body_AoS (
        std::function< void(
          const double * __restrict__                 Q,
          const tarch::la::Vector<Dimensions,double>& x,
          double                                      t,
          int                                         normal,
          double * __restrict__                       F
        ) >   flux,
        double* __restrict__                          rhsOut, 
        double* __restrict__                          FAux, 
        const double* __restrict__                    QIn, 
        const double* __restrict__                    nodes,
        const double* __restrict__                    weights,
        const double* __restrict__                    Kxi,
        const tarch::la::Vector<Dimensions,double>&   cellCentre,
        const double                                  dx,
        const double                                  t,
        const double                                  dt,
        const int                                     nodesPerAxis,
        const int                                     unknowns,
        const int                                     strideQ,
        const int                                     strideRhs,
        const int                                     scalarIndex) {
      const tarch::la::Vector<Dimensions+1,int> strides   = getStrides(nodesPerAxis);
      const tarch::la::Vector<Dimensions+1,int> index     = delineariseIndex(scalarIndex,strides);
      const tarch::la::Vector<Dimensions+1,double> coords = getCoordinates(index,cellCentre,dx,t,dt,nodes);
      const tarch::la::Vector<Dimensions,double> x(&coords[1]);
      const double time = coords[0];
      
      const double invDx = 1.0/dx;

      for ( int d = 1; d < Dimensions+1; d++) {
        const double coeff0 = dt * invDx/*[d-1]*/ * weights[index[0]] / weights[index[d]];
          
        for ( int a = 0; a < nodesPerAxis; a++ ) { // further collapsing causes data races, synchronization or GPU shared mem usage required
          const double coeff = coeff0 * Kxi[ a*nodesPerAxis + index[d] ];
          const double * Q = &QIn[ ( scalarIndex + (a - index[d])*strides[d] )*strideQ ];
          flux(Q, x, time, d-1, FAux);
          for (int var=0; var < unknowns; var++) {
            rhsOut[ scalarIndex*strideRhs+var ] -= coeff * FAux[ var ];
          }
        }
      }
    }
    
    #if defined(GPUOffloading)
    #pragma omp end declare target
    #endif
    
    #if defined(GPUOffloading)
    #pragma omp declare target
    #endif
    GPUCallableMethod void spaceTimePredictor_PicardLoop_addSourceContributionToRhs_body_AoS(
        std::function< void(
          const double * __restrict__                 Q,
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
        const int                                  scalarIndex) {
      const tarch::la::Vector<Dimensions+1,int>     index  = delineariseIndex(scalarIndex, getStrides(nodesPerAxis));
      const tarch::la::Vector<Dimensions+1, double> coords = getCoordinates(index,cellCentre,dx,t,dt,nodes);
      const tarch::la::Vector<Dimensions, double> x( ( coords.data() + 1 ) );
      const double time = coords[0];
      
      const double coeff = dt * weights[index[0]];
      
      const double* Q = &QIn[ scalarIndex*strideQ ];
    
      algebraicSource(Q, x, time, SAux);
      for (int var = 0; var < unknowns; var++) {
        rhsOut[ scalarIndex*strideRhs ] += coeff * SAux[var];
      }
    }
    #if defined(GPUOffloading)
    #pragma omp end declare target
    #endif
    
    #if defined(GPUOffloading)
    #pragma omp declare target
    #endif
    GPUCallableMethod void spaceTimePredictor_PicardLoop_addNcpContributionToRhs_body_AoS(
      std::function< void(
        const double * __restrict__                 Q,
        double                                      gradQ[][Dimensions],
        const tarch::la::Vector<Dimensions,double>& x,
        double                                      t,
        double * __restrict__                       BgradQ
      ) >                                         nonconservativeProduct,
      double * __restrict__                       rhsOut,
      double * __restrict__                       gradQAux,
      double * __restrict__                       SAux,
      const double * __restrict__                 QIn,
      const double * __restrict__                 nodes,
      const double * __restrict__                 weights,
      const double * __restrict__                 dudx, 
      const tarch::la::Vector<Dimensions,double>& cellCentre,
      const double                                dx,
      const double                                t,
      const double                                dt,
      const int                                   nodesPerAxis,
      const int                                   unknowns,
      const int                                   strideRhs,
      const int                                   strideQ,
      const int                                   scalarIndex) {
      tarch::la::Vector<Dimensions+1,int> strides = getStrides(nodesPerAxis);
      tarch::la::Vector<Dimensions+1,int> index   = delineariseIndex(scalarIndex,strides);
      const tarch::la::Vector<Dimensions+1, double> coords = getCoordinates(index,cellCentre,dx,t,dt,nodes);
      const tarch::la::Vector<Dimensions, double> x( ( coords.data() + 1 ) );
      const double time = coords[0];
     
      const double invDx = 1.0/dx;
 
      const double coeff = dt * weights[index[0]];
      
      gradient_AoS(QIn,dudx,invDx,nodesPerAxis,strideQ,scalarIndex,gradQAux);           
      
      const double* Q = &QIn [ scalarIndex*strideQ ];
      nonconservativeProduct(Q, (double (*)[Dimensions]) gradQAux, x, time, SAux );
      for(int var=0; var < unknowns; var++) {
        rhsOut[ scalarIndex*strideRhs + var ] += coeff * SAux[var];
      }
    }
    #if defined(GPUOffloading)
    #pragma omp end declare target
    #endif
    
    #if defined(GPUOffloading)
    #pragma omp declare target
    #endif
    GPUCallableMethod void spaceTimePredictor_PicardLoop_invert_body_AoS(
      double * __restrict__       QOut,
      double&                     squaredResiduumOut,
      const double * __restrict__ rhsIn,
      const double * __restrict__ iK1,
      const int                   nodesPerAxis,
      const int                   unknowns,
      const int                   strideQ,
      const int                   strideRhs,
      const int                   scalarIndex) {
      const int it = delineariseIndex(scalarIndex,getStrides(nodesPerAxis))[0];
       
      squaredResiduumOut = 0.0; 
      for (int var = 0; var < unknowns; var++) {
        double Q_new = 0;
        //const double coeff = iweight * rhsIn[ scalarIndex*strideRhs + var ]; 
        const double coeff = rhsIn[ scalarIndex*strideRhs + var ]; 
        // @todo: check if operator is correct
        for (int a = 0; a < nodesPerAxis; a++) { // matmul time
          Q_new += coeff * iK1[ it*nodesPerAxis + a ]; // note: iK1 is already the transposed inverse of K1
        }
        
        const double difference = Q_new - QOut[ scalarIndex*strideQ + var ]; 
        squaredResiduumOut += difference * difference;
        QOut[ scalarIndex*strideQ + var ] = Q_new;
        
        //@todo: enable in CPU version
        //assertion3( !std::isnan( Q[ scalarIndex ] ), scalarIndex, dt, invDx );
        //assertion3( !std::isnan(Q_new), scalarIndex, dt, invDx );
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
    
    #if defined(GPUOffloading)
    #pragma omp end declare target
    #endif
    
    #if defined(GPUOffloading)
    #pragma omp declare target
    #endif
    GPUCallableMethod void spaceTimePredictor_extrapolate_body_AoS(
        double * __restrict__       QHullOut,
        const double * __restrict__ QIn,
        const double * __restrict__ FCoeff[2],
        const double                invDx,
        const int                   nodesPerAxis,
        const int                   unknowns,
        const int                   auxiliaryVariables,
        const int                   strideQ,
        const int                   scalarIndexHull) {
      const tarch::la::Vector<Dimensions+1,int> strides    = getStrides(nodesPerAxis);
      const tarch::la::Vector<Dimensions+1,int> indexQHull = delineariseIndex(scalarIndexHull,strides); // (t,y,z,face) , (t,x,z,face), (t,x,y,face)
     
      const int d  = indexQHull[Dimensions] / 2;
      const int lr = indexQHull[Dimensions] - 2*d;
    
      // clear
      for (int var=0; var < unknowns; var++) {
        QHullOut[ scalarIndexHull * strideQ + var ] = 0.0; 
      }
      
      for (int id=0; id<nodesPerAxis; id++) {
        const int scalarIndexQ = mapSpaceTimeFaceIndexToScalarCellIndex(indexQHull,d,lr,id);
    
        const double coeff = FCoeff[lr][id];
        for (int var=0; var < unknowns; var++) {
          QHullOut[ scalarIndexHull * strideQ + var ] += coeff * QIn[ scalarIndexQ*strideQ + var ]; 
        }
      }
    }
    #if defined(GPUOffloading)
    #pragma omp end declare target
    #endif
  }
}

#ifdef __HIP__

// kernels

// launcher

#endif
