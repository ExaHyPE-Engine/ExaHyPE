#include "RusanovNonlinearAoS.h"

#include "KernelUtils.h"

namespace exahype2 {
  namespace aderdg {

    #if defined(GPUOffloading)
    #pragma omp declare target
    #endif
    GPUCallableMethod void rusanovNonlinear_maxEigenvalue_body_AoS(
        std::function< double(
          const double * __restrict__                 Q,
          const tarch::la::Vector<Dimensions,double>& x,
          double                                      t,
          const int                                   direction
        ) >                                         maxAbsoluteEigenvalue,
        double&                                     smax,
        const double * __restrict__                 QLR[2],
        const double * __restrict__                 nodes,
        const double                                t,
        const double                                dt,
        const tarch::la::Vector<Dimensions,double>& faceCentre,
        const double                                dx,
        const int                                   nodesPerAxis,
        const int                                   unknowns,
        const int                                   strideQ,
        const int                                   strideF,
        const int                                   direction,
        const int                                   scalarIndexFace) {
      const tarch::la::Vector<Dimensions+1,int> indexQFace = // (t,y,z,face) , (t,x,z,face), (t,x,y,face)
        delineariseIndex(scalarIndexFace,getStrides(nodesPerAxis)); 
      const tarch::la::Vector<Dimensions+1,double> coords = 
        getCoordinatesOnFace(indexQFace,faceCentre,direction,dx,t,dt,nodes);
      const tarch::la::Vector<Dimensions,double> x(&coords[1]);
      const double time = coords[0];
    
      const int lr = indexQFace[Dimensions];
     
      // hyperbolic eigenvalues
      smax = std::max( smax, maxAbsoluteEigenvalue( &QLR[lr][ scalarIndexFace*strideQ ], x, time, direction ) );
    }
    #if defined(GPUOffloading)
    #pragma omp end declare target
    #endif
    
    #if defined(GPUOffloading)
    #pragma omp declare target
    #endif
    GPUCallableMethod void rusanovNonlinear_riemannFlux_body_AoS(
        std::function< void(
          const double * __restrict__                 Q,
          const tarch::la::Vector<Dimensions,double>& x,
          double                                      t,
          int                                         normal,
          double * __restrict__                       F
        ) >                                         flux,
        double * __restrict__                       FLOut,
        double * __restrict__                       FROut,
        const double * __restrict__                 QLIn, 
        const double * __restrict__                 QRIn, 
        double * __restrict__                       FLAux,
        double * __restrict__                       FRAux,
        const double&                               smax,
        const double * __restrict__                 nodes, 
        const double * __restrict__                 weights, 
        const double                                t,
        const double                                dt,
        const tarch::la::Vector<Dimensions,double>& faceCentre,
        const double                                dx,
        const int                                   nodesPerAxis,
        const int                                   unknowns,
        const int                                   strideQ,
        const int                                   strideF,
        const int                                   direction,
        const int                                   scalarIndexFace) {
      const tarch::la::Vector<Dimensions+1,int> indexQFace = // (t,y,z,face=0) , (t,x,z,face=0), (t,x,y,face=0)
        delineariseIndex(scalarIndexFace,getStrides(nodesPerAxis)); 
      const tarch::la::Vector<Dimensions+1,double> coords = 
        getCoordinatesOnFace(indexQFace,faceCentre,direction,dx,t,dt,nodes);
      const tarch::la::Vector<Dimensions,double> x(&coords[1]);
      const double time = coords[0];
    
      // time-averaged flux FLOut
      for (int a = 0; a < nodesPerAxis; a++) { // time integration
        const int offsetQ = (scalarIndexFace*nodesPerAxis + a)*strideQ;
        const double coeff1 = weights[a] * 0.5;
        const double coeff2 = coeff1 * smax;
        
        flux( QLIn + offsetQ, x, time, direction, FLAux ); 
        flux( QRIn + offsetQ, x, time, direction, FRAux );
        for (int var = 0; var < unknowns; var++) {
          FLOut[ scalarIndexFace*strideF + var ] += coeff1 * (FRAux[ var ] + FLAux[ var ]);
        }
        
        for (int var = 0; var < unknowns; var++) {
          FLOut[ scalarIndexFace*strideF + var ] -= coeff2 * (QRIn[ offsetQ + var ] - QLIn[ offsetQ + var ]);
        }
      }
      
      // copy FLOut to FROut
      for (int var = 0; var < unknowns; var++) {
        FROut[ scalarIndexFace*strideF + var ] = FLOut[ scalarIndexFace*strideF + var ]; 
      }
    }
    #if defined(GPUOffloading)
    #pragma omp end declare target
    #endif
    
    #if defined(GPUOffloading)
    #pragma omp declare target
    #endif
    GPUCallableMethod void rusanovNonlinear_addNcpContributionsToRiemannFlux_body_AoS(
        std::function< void(
          double * __restrict__                       Q,
          double                                      gradQ[][Dimensions],
          const tarch::la::Vector<Dimensions,double>& x,
          double                                      t,
          int                                         normal,
          double * __restrict__                       BgradQ
        ) >                                         nonconservativeProduct,
        double * __restrict__                       FLOut,
        double * __restrict__                       FROut,
        double * __restrict__                       QAvgAux,
        double * __restrict__                       gradQAux,
        double * __restrict__                       SAux,
        const double * __restrict__                 QLIn, 
        const double * __restrict__                 QRIn, 
        const double                                smax,
        const double * __restrict__                 nodes, 
        const double * __restrict__                 weights, 
        const double                                t,
        const double                                dt,
        const tarch::la::Vector<Dimensions,double>& faceCentre,
        const double                                dx,
        const int                                   nodesPerAxis,
        const int                                   unknowns,
        const int                                   strideQ,
        const int                                   strideF,
        const int                                   direction,
        const int                                   scalarIndexFace) {
      const tarch::la::Vector<Dimensions+1,int> indexQFace = // (t,y,z,face=0) , (t,x,z,face=0), (t,x,y,face=0)
        delineariseIndex(scalarIndexFace,getStrides(nodesPerAxis)); 
      const tarch::la::Vector<Dimensions+1,double> coords = 
        getCoordinatesOnFace(indexQFace,faceCentre,direction,dx,t,dt,nodes);
      const tarch::la::Vector<Dimensions,double> x(&coords[1]);
      const double time = coords[0];
    
      for (int it = 0; it < nodesPerAxis; it++) { // time integration
        const int offsetQ  = (scalarIndexFace*nodesPerAxis + it)*strideQ;
        
        for (int var=0; var < strideQ; var++) {
          gradQAux[ Dimensions*var + direction ] = QRIn[ offsetQ + var ] - QLIn[ offsetQ + var ];
        }
        
        
        for (int var=0; var < strideQ; var++) {
          QAvgAux[var] = 0.5 * (QRIn[ offsetQ + var ] + QLIn[ offsetQ + var ]);
        }
        
          
        nonconservativeProduct(QAvgAux, (double (*)[Dimensions]) gradQAux, x, time, direction, SAux);
    
        const double coeff = weights[it] * 0.5;
        for (int var = 0; var < unknowns; var++) {
          FROut[ scalarIndexFace*strideF + var ] -= coeff * SAux[var];
        }
        
        for (int var = 0; var < unknowns; var++) {
          FLOut[ scalarIndexFace*strideF + var ] += coeff * SAux[var];
        }
      }
    }
    #if defined(GPUOffloading)
    #pragma omp end declare target
    #endif
  }
}
