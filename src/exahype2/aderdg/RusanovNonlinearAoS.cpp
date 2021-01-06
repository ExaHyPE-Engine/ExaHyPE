#include "RusanovNonlinearAoS.h"

#include "Generic.h"

#include "KernelUtils.h"

namespace exahype2 {
  namespace aderdg {

    #if defined(OpenMPGPUOffloading)
    #pragma omp declare target
    #endif
    GPUCallableMethod void rusanovNonlinear_maxAbsoluteEigenvalue_body_AoS(
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
    #if defined(OpenMPGPUOffloading)
    #pragma omp end declare target
    #endif
    
    #if defined(OpenMPGPUOffloading)
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
        double * __restrict__                       FLAux,
        double * __restrict__                       FRAux,
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
    #if defined(OpenMPGPUOffloading)
    #pragma omp end declare target
    #endif
    
    #if defined(OpenMPGPUOffloading)
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
    #if defined(OpenMPGPUOffloading)
    #pragma omp end declare target
    #endif
    
    double rusanovNonlinear_maxAbsoluteEigenvalue_loop_AoS(
      std::function< double(
        const double * __restrict__                 Q,
        const tarch::la::Vector<Dimensions,double>& x,
        double                                      t,
        const int                                   direction
      ) >                                         maxAbsoluteEigenvalue,
      const double * __restrict__                 QL,
      const double * __restrict__                 QR,
      const double * __restrict__                 nodes,
      const double                                t,
      const double                                dt,
      const tarch::la::Vector<Dimensions,double>& faceCentre,
      const double                                dx,
      const int                                   order,
      const int                                   unknowns,
      const int                                   auxiliaryVariables,
      const int                                   direction) {
      const unsigned int nodesPerAxis = order + 1;

      const unsigned int strideQ = unknowns + auxiliaryVariables; 
      const unsigned int strideF = unknowns;

      const unsigned int spaceTimeNodesOnInterface = getNodesPerCell(nodesPerAxis) * 2; // nodesPerAxis^d * 2

      const double* QLR[2] = {QL,QR};
     
      double maxAbsEigenvalue = 0.0;  // strictly positive
      for ( unsigned int scalarIndexFace = 0; scalarIndexFace < spaceTimeNodesOnInterface; scalarIndexFace++ ) {
        double smax = 0.0; // strictly positive
        rusanovNonlinear_maxAbsoluteEigenvalue_body_AoS(
          maxAbsoluteEigenvalue,
          smax,
          QLR,
          nodes,
          t,
          dt,
          faceCentre,
          dx,
          nodesPerAxis,
          unknowns,
          strideQ,
          strideF,
          direction,
          scalarIndexFace);
        maxAbsEigenvalue = std::max( maxAbsEigenvalue, smax );
      } 
      return maxAbsEigenvalue;
    }
    
    void rusanovNonlinear_loop_AoS(
      std::function< void(
        const double * __restrict__                 Q,
        const tarch::la::Vector<Dimensions,double>& x,
        double                                      t,
        int                                         normal,
        double * __restrict__                       F
      ) >                                         flux,
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
      const double * __restrict__                 QLIn, 
      const double * __restrict__                 QRIn, 
      const double                                smax,
      const double * __restrict__                 nodes, 
      const double * __restrict__                 weights, 
      const double                                t,
      const double                                dt,
      const tarch::la::Vector<Dimensions,double>& faceCentre,
      const double                                dx,
      const int                                   order,
      const int                                   unknowns,
      const int                                   auxiliaryVariables,
      const int                                   direction,
      const bool                                  callFlux,
      const bool                                  callNonconservativeProduct) {
      const unsigned int nodesPerAxis = order + 1;

      const int strideQ     = unknowns+auxiliaryVariables;
      const int strideS     = unknowns;
      const int strideF     = unknowns;
      const int strideGradQ = strideQ*Dimensions; // gradient of auxiliary variables needed for some apps

      const unsigned int nodesOnFace = getNodesPerCell(nodesPerAxis)/nodesPerAxis; // nodesPerAxis^(d-1)

      const double* QLR[2] = {QLIn,QRIn};
      
      double* FAux     = new double[2*nodesOnFace*strideF]{0.0}; 
      double* QAvgAux  = new double[nodesOnFace*strideQ]{0.0};
      double* SAux     = new double[nodesOnFace*strideS]{0.0};
      double* gradQAux = new double[nodesOnFace*strideGradQ]{0.0};
 
      for ( unsigned int scalarIndexFace = 0; scalarIndexFace < nodesOnFace; scalarIndexFace++ ) {
        if ( callFlux ) {
          rusanovNonlinear_riemannFlux_body_AoS(
            flux,
            FLOut,
            FROut,
            &FAux[ scalarIndexFace*2*strideF ],
            &FAux[ scalarIndexFace*2*strideF + strideF ],
            QLIn, 
            QRIn, 
            smax,
            nodes, 
            weights, 
            t,
            dt,
            faceCentre,
            dx,
            nodesPerAxis,
            unknowns,
            strideQ,
            strideF,
            direction,
            scalarIndexFace);
        }
        if ( callNonconservativeProduct ) {
          rusanovNonlinear_addNcpContributionsToRiemannFlux_body_AoS(
            nonconservativeProduct,
            FLOut,
            FROut,
            &QAvgAux[ scalarIndexFace*strideQ ],
            &gradQAux[ scalarIndexFace*strideGradQ ],
            &SAux[ scalarIndexFace*strideS ],
            QLIn, 
            QRIn, 
            smax,
            nodes, 
            weights, 
            t,
            dt,
            faceCentre,
            dx,
            nodesPerAxis,
            unknowns,
            strideQ,
            strideF,
            direction,
            scalarIndexFace);
        }
      }

      delete [] FAux;
      delete [] QAvgAux;
      delete [] SAux;
      delete [] gradQAux;
    }  
  }
}
