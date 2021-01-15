#include "PredictorAoS.h"

#include "KernelUtils.h"

#include "Generic.h"

#include "tarch/la/Vector.h"

#include "tarch/logging/Log.h"

#if defined(OpenMPGPUOffloading)
#pragma omp declare target
#endif
GPUCallableMethod void exahype2::aderdg::spaceTimePredictor_initialGuess_body_AoS(
  double * __restrict__       QOut,
  const double * __restrict__ UIn,
  const int                   nodesPerAxis,
  const int                   strideQ,
  const int                   scalarIndex) {
  for (int var = 0; var < strideQ; var++) {
    QOut[ scalarIndex*strideQ + var ] = UIn[ ( scalarIndex / nodesPerAxis )*strideQ + var ];
  }
}

#if defined(OpenMPGPUOffloading)
#pragma omp end declare target
#endif

#if defined(OpenMPGPUOffloading)
#pragma omp declare target
#endif
GPUCallableMethod void exahype2::aderdg::spaceTimePredictor_PicardLoop_initialiseRhs_AoS(
  double * __restrict__       rhsOut,
  const double * __restrict__ UIn,
  const double * __restrict__ FLCoeff,
  const int                   nodesPerAxis,
  const int                   strideQ,
  const int                   strideRhs,
  const int                   scalarIndex) {
  const int it = delineariseIndex(scalarIndex,getStrides(nodesPerAxis))[0];
  const double coeff = FLCoeff[ it ];
  
  for (int var = 0; var < strideQ; var++) {
    rhsOut[ scalarIndex*strideRhs + var ] = coeff * UIn[ ( scalarIndex / nodesPerAxis ) * strideQ + var ];
  }
}

#if defined(OpenMPGPUOffloading)
#pragma omp end declare target
#endif

#if defined(OpenMPGPUOffloading)
#pragma omp declare target
#endif
GPUCallableMethod void exahype2::aderdg::spaceTimePredictor_PicardLoop_addFluxContributionsToRhs_body_AoS (
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
      const double coeff = coeff0 * Kxi[ a*nodesPerAxis + index[d] ]; // @todo: provide transposed variant
      const double * Q = &QIn[ ( scalarIndex + (a - index[d])*strides[d] )*strideQ ];
      flux(Q, x, time, d-1, FAux);
      for (int var=0; var < unknowns; var++) {
        rhsOut[ scalarIndex*strideRhs+var ] -= coeff * FAux[ var ];
      }
    }
  }
}

#if defined(OpenMPGPUOffloading)
#pragma omp end declare target
#endif

#if defined(OpenMPGPUOffloading)
#pragma omp declare target
#endif
GPUCallableMethod void exahype2::aderdg::spaceTimePredictor_PicardLoop_addSourceContributionToRhs_body_AoS(
    std::function< void(
      const double * __restrict__                 Q,
      const tarch::la::Vector<Dimensions,double>& x,
      double                                      t,
      double * __restrict__                       S
    ) >   algebraicSource,
    double* __restrict__                        rhsOut,
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
    const int                                   scalarIndex) {
  const tarch::la::Vector<Dimensions+1,int>     index  = delineariseIndex(scalarIndex, getStrides(nodesPerAxis));
  const tarch::la::Vector<Dimensions+1, double> coords = getCoordinates(index,cellCentre,dx,t,dt,nodes);
  const tarch::la::Vector<Dimensions, double> x( ( coords.data() + 1 ) );
  const double time = coords[0];
  
  const double coeff = dt * weights[index[0]];
  
  const double* Q = &QIn[ scalarIndex*strideQ ];

  algebraicSource(Q, x, time, SAux);
  for (int var = 0; var < unknowns; var++) {
    rhsOut[ scalarIndex*strideRhs + var ] += coeff * SAux[var];
  }
}
#if defined(OpenMPGPUOffloading)
#pragma omp end declare target
#endif

#if defined(OpenMPGPUOffloading)
#pragma omp declare target
#endif
GPUCallableMethod void exahype2::aderdg::spaceTimePredictor_PicardLoop_addNcpContributionToRhs_body_AoS(
  std::function< void(
    const double * __restrict__                 Q,
    double * __restrict__                       dQ_or_dQdn,
    const tarch::la::Vector<Dimensions,double>& x,
    double                                      t,
    int                                         normal,
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
  const int                                   strideQ,
  const int                                   strideRhs,
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
  for ( int direction = 0; direction < Dimensions; direction++ ) {
    nonconservativeProduct(Q, &gradQAux[ direction*strideQ ], x, time, direction, SAux );
    for(int var=0; var < unknowns; var++) {
      rhsOut[ scalarIndex*strideRhs + var ] += coeff * SAux[var];
    }
  }
}
#if defined(OpenMPGPUOffloading)
#pragma omp end declare target
#endif

#if defined(OpenMPGPUOffloading)
#pragma omp declare target
#endif
GPUCallableMethod void exahype2::aderdg::spaceTimePredictor_PicardLoop_invert_body_AoS(
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

#if defined(OpenMPGPUOffloading)
#pragma omp end declare target
#endif

#if defined(OpenMPGPUOffloading)
#pragma omp declare target
#endif
GPUCallableMethod void exahype2::aderdg::spaceTimePredictor_extrapolate_body_AoS(
    double * __restrict__       QHullOut,
    const double * __restrict__ QIn,
    const double * __restrict__ FLRCoeff[2],
    const int                   nodesPerAxis,
    const int                   strideQ,
    const int                   scalarIndexHull) {
  const tarch::la::Vector<Dimensions+1,int> strides    = getStrides(nodesPerAxis);
  const tarch::la::Vector<Dimensions+1,int> indexQHull = delineariseIndex(scalarIndexHull,strides); // (t,y,z,face) , (t,x,z,face), (t,x,y,face)
 
  const int d  = indexQHull[Dimensions] / 2;
  const int lr = indexQHull[Dimensions] - 2*d;

  for (int id=0; id<nodesPerAxis; id++) {
    const int scalarIndexQ = mapSpaceTimeFaceIndexToScalarCellIndex(indexQHull,d,lr,id);

    const double coeff = FLRCoeff[lr][id];
    for (int var=0; var < strideQ; var++) {
      QHullOut[ scalarIndexHull * strideQ + var ] += coeff * QIn[ scalarIndexQ*strideQ + var ]; 
    }
  }
}

GPUCallableMethod void exahype2::aderdg::spaceTimePredictor_extrapolate_Lobatto_body_AoS(
    double * __restrict__       QHullOut,
    const double * __restrict__ QIn,
    const int                   nodesPerAxis,
    const int                   strideQ,
    const int                   scalarIndexHull) {
  const tarch::la::Vector<Dimensions+1,int> strides    = getStrides(nodesPerAxis);
  const tarch::la::Vector<Dimensions+1,int> indexQHull = delineariseIndex(scalarIndexHull,strides); // (t,y,z,face) , (t,x,z,face), (t,x,y,face)
 
  const int d  = indexQHull[Dimensions] / 2;
  const int lr = indexQHull[Dimensions] - 2*d;

  const int id = ( lr == 0 ) ? 0 : (nodesPerAxis-1);
  const int scalarIndexQ = mapSpaceTimeFaceIndexToScalarCellIndex(indexQHull,d,lr, id );

  for (int var=0; var < strideQ; var++) {
    QHullOut[ scalarIndexHull * strideQ + var ] = QIn[ scalarIndexQ*strideQ + var ]; 
  }
}
#if defined(OpenMPGPUOffloading)
#pragma omp end declare target
#endif

#if defined(OpenMPGPUOffloading)
#pragma omp declare target
#endif
GPUCallableMethod void exahype2::aderdg::spaceTimePredictor_extrapolateInTime_body_AoS(
  double * __restrict__       UOut,
  const double * __restrict__ QIn,
  const double * __restrict__ FRCoeff,
  const double                nodesPerAxis,
  const int                   strideQ,
  const int                   scalarIndexCell) {
  // clear
  for (int var = 0; var < strideQ; var++ ) {
    UOut[ scalarIndexCell*strideQ + var ] = 0;
  }
  // compute
  for (int it = 0; it < nodesPerAxis; it++ ) {
    const int indexQ = (scalarIndexCell*nodesPerAxis + it)*strideQ;
    const double coeff = FRCoeff[it];
    for (int var = 0; var < strideQ; var++ ) {
      UOut[ scalarIndexCell*strideQ + var ] += coeff * QIn[ indexQ + var ];
    }
  }
}
#if defined(OpenMPGPUOffloading)
#pragma omp end declare target
#endif

#if defined(OpenMPGPUOffloading)
#pragma omp declare target
#endif
GPUCallableMethod void exahype2::aderdg::spaceTimePredictor_extrapolateInTime_Lobatto_body_AoS(
  double * __restrict__       UOut,
  const double * __restrict__ QIn,
  const double * __restrict__ FRCoeff,
  const double                nodesPerAxis,
  const int                   strideQ,
  const int                   scalarIndexCell) {
 const int it = nodesPerAxis-1;
 const int indexQ = (scalarIndexCell*nodesPerAxis + it)*strideQ;
 for (int var = 0; var < strideQ; var++ ) {
   UOut[ scalarIndexCell*strideQ + var ] = QIn[ indexQ + var ];
 }
}
#if defined(OpenMPGPUOffloading)
#pragma omp end declare target
#endif

#if defined(__HIPCC__) or defined(__NVCC__)

// kernels
__global__ void exahype2::aderdg::spaceTimePredictor_initialGuess_krnl_AoS(
  const double * __restrict__ QOut,
  const double * __restrict__ UIn,
  const int                   nodesPerAxis,
  const int                   strideQ) {
  const int spaceTimeNodesPerCell = getSpaceTimeNodesPerCell(nodesPerAxis);
  const int scalarIndexCell       = threadIdx.x + blockDim.x * blockIdx.x;
 
  if ( scalarIndexCell < spaceTimeNodesPerCell ) {
    spaceTimePredictor_initialGuess_body_AoS(
      QOut,
      UIn,
      nodesPerAxis,
      strideQ,
      scalarIndexCell);
  }
}

__global__ void exahype2::aderdg::spaceTimePredictor_PicardLoop_assembleRhs_krnl_AoS(
  std::function< void(
    const double * __restrict__                 Q,
    const tarch::la::Vector<Dimensions,double>& x,
    double                                      t,
    int                                         normal,
    double * __restrict__                       F
  ) >   flux,
  std::function< void(
    const double * __restrict__                 Q,
    const tarch::la::Vector<Dimensions,double>& x,
    double                                      t,
    double * __restrict__                       S
  ) >   algebraicSource,
  std::function< void(
    const double * __restrict__                 Q,
    double * __restrict__                       dQ_or_dQdn,
    const tarch::la::Vector<Dimensions,double>& x,
    double                                      t,
    int                                         normal,
    double * __restrict__                       BgradQ
  ) >                                         nonconservativeProduct,
  double * __restrict__                       rhsOut, 
  double * __restrict__                       SAux,
  double * __restrict__                       gradQAux,
  double * __restrict__                       FAux, 
  const double * __restrict__                 QIn, 
  const double * __restrict__                 weights,
  const double * __restrict__                 nodes,
  const double * __restrict__                 Kxi,
  const double * __restrict__                 FLCoeff,
  const double * __restrict__                 dudx, 
  const double                                dx,
  const tarch::la::Vector<Dimensions,double>& cellCentre,
  const double                                t,
  const double                                dt,
  const int                                   nodesPerAxis,
  const int                                   unknowns,
  const int                                   strideQ,
  const int                                   strideS,
  const int                                   strideGradQ,
  const int                                   strideRhs,
  const bool                                  callFlux,
  const bool                                  callSource,
  const bool                                  callNonconservativeProduct) {
  const int spaceTimeNodesPerCell = getSpaceTimeNodesPerCell(nodesPerAxis);
  const int scalarIndexCell       = threadIdx.x + blockDim.x * blockIdx.x;
  
  if ( scalarIndexCell < spaceTimeNodesPerCell ) {
    spaceTimePredictor_PicardLoop_initialiseRhs_AoS(
      rhsOut,
      UIn,
      FLCoeff,
      nodesPerAxis,
      strideQ,
      strideRhs,
      scalarIndex);

      if ( callFlux ) { 
        
        spaceTimePredictor_PicardLoop_addFluxContributionsToRhs_body_AoS(
          flux,
          rhsOut, 
          FAux, 
          QIn, 
          nodes,
          weights,
          Kxi,
          cellCentre,
          dx,
          t,
          dt,
          nodesPerAxis,
          unknowns,
          strideQ,
          strideRhs,
          scalarIndex);
      }
      if ( callSource ) { 
        
        spaceTimePredictor_PicardLoop_addSourceContributionToRhs_body_AoS(
          algebraicSource,
          rhsOut,
          SAux,
          QIn,
          nodes,
          weights,
          cellCentre,
          dx,
          t,
          dt,
          nodesPerAxis,
          unknowns,
          strideQ,
          strideRhs,
          scalarIndex);
      }
      if ( callNonconservativeProduct ) { 
        
        spaceTimePredictor_PicardLoop_addNcpContributionToRhs_body_AoS(
          nonconservativeProduct,
          rhsOut,
          gradQAux,
          SAux,
          QIn,
          nodes,
          weights,
          dudx, 
          cellCentre,
          dx,
          t,
          dt,
          nodesPerAxis,
          unknowns,
          strideQ,
          strideRhs,
          scalarIndex);
      }
    }
  }
}

__global__ void exahype2::aderdg::spaceTimePredictor_PicardLoop_invert_krnl_AoS(
       double * __restrict__       QOut,
       double * __restrict__       partial_squaredResiduumOut,
       const double * __restrict__ rhsIn,
       const double * __restrict__ iK1,
       const int                   nodesPerAxis,
       const int                   unknowns,
       const int                   strideQ,
       const int                   strideRhs,
       const int                   scalarIndex) {
   HIP_DYNAMIC_SHARED( double, sdata) 
   const int threadIdxInBlock      = threadIdx.x;
   const int scalarIndexCell       = threadIdxInBlock + blockDim.x * blockIdx.x;
   const int spaceTimeNodesPerCell = getSpaceTimeNodesPerCell(nodesPerAxis);
   
   double squaredResiduumOut = 0.0;
   if ( scalarIndexCell < spaceTimeNodesPerCell ) {
     spaceTimePredictor_PicardLoop_invert_body_AoS(
       QOut,
       squaredResiduumOut,
       rhsIn,
       iK1,
       nodesPerAxis,
       unknowns,
       strideQ,
       strideRhs,
       scalarIndex);
   }
  
   // below: (unoptimised) block-wide reduction; final result computed on host (as result is needed on host side)
   sdata[ threadIdxInBlock ] = squaredResiduumOut;
   __syncthreads();

   for (unsigned int s = 1; s < blockDim.x; s *= 2) {
     int index = 2 * s * threadIdxInBlock;
     if (index < blockDim.x) {
       sdata[index] += sdata[index + s];
     }
     __syncthreads();
   }
  
   if (threadIdxInBlock == 0) { // write partial result for this block to global memory
     partial_squaredResiduumOut[ blockIdx.x ] = squaredResiduumOut;
   }
 }  
    
 __global__ void exahype2::aderdg::spaceTimePredictor_extrapolate_krnl_AoS(
    double * __restrict__       QHullOut,
    const double * __restrict__ QIn,
    const double * __restrict__ FLCoeff,
    const double * __restrict__ FRCoeff,
    const int                   nodesPerAxis,
    const int                   unknowns,
    const int                   strideQ) {
   const int spaceTimeNodesOnCellHull = getNodesPerCell(nodesPerAxis)/* nodesPerAxis^d */ * 2 * Dimensions;
   
  const double* FLRCoeff[2] = {FLCoeff, FRCoeff};
 
   const int scalarIndexHull = threadIdx.x + blockDim.x * blockIdx.x;
   if ( scalarIndexHull < spaceTimeNodesOnCellHull ) {
     spaceTimePredictor_extrapolate_body_AoS(
       QHullOut,
       QIn,
       FLRCoeff,
       nodesPerAxis,
       unknowns,
       strideQ,
       scalarIndexHull);
   }
}
// launcher

#endif
 
// CPU launchers
void exahype2::aderdg::spaceTimePredictor_PicardLoop_loop_AoS(
  std::function< void(
    const double * __restrict__                 Q,
    const tarch::la::Vector<Dimensions,double>& x,
    double                                      t,
    int                                         normal,
    double * __restrict__                       F
  ) >   flux,
  std::function< void(
    const double * __restrict__                 Q,
    const tarch::la::Vector<Dimensions,double>& x,
    double                                      t,
    double * __restrict__                       S
  ) >   algebraicSource,
  std::function< void(
    const double * __restrict__                 Q,
    const double * __restrict__                 dQ_or_dQdn,
    const tarch::la::Vector<Dimensions,double>& x,
    double                                      t,
    int                                         normal,
    double * __restrict__                       BgradQ
  ) >                                         nonconservativeProduct,
  double * __restrict__                       QOut, 
  const double * __restrict__                 UIn, 
  const double * __restrict__                 nodes,
  const double * __restrict__                 weights,
  const double * __restrict__                 Kxi,
  const double * __restrict__                 iK1,
  const double * __restrict__                 FLCoeff,
  const double * __restrict__                 dudx, 
  const double                                dx,
  const tarch::la::Vector<Dimensions,double>& cellCentre,
  const double                                t,
  const double                                dt,
  const int                                   order,
  const int                                   unknowns,
  const int                                   auxiliaryVariables,
  const double                                atol,
  const bool                                  callFlux,
  const bool                                  callSource,
  const bool                                  callNonconservativeProduct) {
  const int nodesPerAxis = order+1;

  const int spaceTimeNodesPerCell = getSpaceTimeNodesPerCell(nodesPerAxis);
  
  const int strideQ      = unknowns+auxiliaryVariables;
  const int strideRhs    = unknowns;
  const int strideS      = unknowns;
  const int strideF      = unknowns;
  const int strideGradQ  = strideQ*Dimensions; // gradient of auxiliary variables needed for some apps
  
  double* rhs      = new double[spaceTimeNodesPerCell*strideRhs]{0.0}; 
  
  double* SAux     = new double[spaceTimeNodesPerCell*strideS]{0.0};
  double* gradQAux = new double[spaceTimeNodesPerCell*strideGradQ]{0.0};
  double* FAux     = new double[spaceTimeNodesPerCell*strideF]{0.0}; 
  
  // initial guess  
  for ( unsigned int scalarIndexCell = 0; scalarIndexCell < spaceTimeNodesPerCell; scalarIndexCell++ ) {
    spaceTimePredictor_initialGuess_body_AoS(
      QOut,
      UIn,
      nodesPerAxis,
      strideQ,
      scalarIndexCell);
  }

  int iter = 0;
  for ( ;iter < nodesPerAxis; iter++ ) {
    for ( unsigned int scalarIndexCell = 0; scalarIndexCell < spaceTimeNodesPerCell; scalarIndexCell++ ) {
      spaceTimePredictor_PicardLoop_initialiseRhs_AoS(
        rhs,
        UIn,
        FLCoeff,
        nodesPerAxis,
        strideQ,
        strideRhs,
        scalarIndexCell);

      if ( callFlux ) { 
        spaceTimePredictor_PicardLoop_addFluxContributionsToRhs_body_AoS(
          flux,
          rhs, 
          &FAux[ scalarIndexCell*strideF ], 
          QOut, 
          nodes,
          weights,
          Kxi,
          cellCentre,
          dx,
          t,
          dt,
          nodesPerAxis,
          unknowns,
          strideQ,
          strideRhs,
          scalarIndexCell);
      }
      if ( callSource ) { 
        spaceTimePredictor_PicardLoop_addSourceContributionToRhs_body_AoS(
          algebraicSource,
          rhs,
          &SAux[ scalarIndexCell*strideS ],
          QOut,
          nodes,
          weights,
          cellCentre,
          dx,
          t,
          dt,
          nodesPerAxis,
          unknowns,
          strideQ,
          strideRhs,
          scalarIndexCell);
      }
      if ( callNonconservativeProduct ) { 
        spaceTimePredictor_PicardLoop_addNcpContributionToRhs_body_AoS(
          nonconservativeProduct,
          rhs,
          &gradQAux[ scalarIndexCell*strideGradQ ],
          &SAux[ scalarIndexCell*strideS ],
          QOut,
          nodes,
          weights,
          dudx, 
          cellCentre,
          dx,
          t,
          dt,
          nodesPerAxis,
          unknowns,
          strideQ,
          strideRhs,
          scalarIndexCell);
      }
    } // scalarIndexCell

    // 3. Multiply with (K1)^(-1) to get the discrete time integral of the discrete Picard iteration
    double sq_res = 0.0;
    for ( unsigned int scalarIndexCell = 0; scalarIndexCell < spaceTimeNodesPerCell; scalarIndexCell++ ) {
      double squaredResiduum = 0.0;
      spaceTimePredictor_PicardLoop_invert_body_AoS(
        QOut,
        squaredResiduum,
        rhs,
        iK1,
        nodesPerAxis,
        unknowns,
        strideQ,
        strideRhs,
        scalarIndexCell);
      sq_res += squaredResiduum;
    }

    // 4. Exit condition
    if ( atol > 0 && sq_res < atol * atol) {
      iter = nodesPerAxis + 1; // break
    }
    if (iter == nodesPerAxis) { // No convergence after last iteration
      static tarch::logging::Log _log("kernels::aderdg::generic::c");
      logWarning("aderPicardLoopNonlinear(...)","|res|^2=" << sq_res << " > |atol|^2=" << atol * atol << " after "
        << iter << " iterations. Solver seems not to have converged properly within maximum number of iteration steps");
    }
  } // iter
  
  delete [] rhs;
   
  delete [] FAux;
  delete [] SAux;
  delete [] gradQAux;
}

void exahype2::aderdg::spaceTimePredictor_extrapolate_loop_AoS(
    double * __restrict__       QHullOut,
    const double * __restrict__ QIn,
    const double * __restrict__ FLCoeff,
    const double * __restrict__ FRCoeff,
    const int                   order,
    const int                   unknowns,
    const int                   auxiliaryVariables) {
  const int nodesPerAxis = order + 1;
  
  const int strideQ = unknowns+auxiliaryVariables;
  
  const int spaceTimeNodesOnCellHull = getNodesPerCell(nodesPerAxis)/* nodesPerAxis^d */ * 2 * Dimensions;
  
  const double* FLRCoeff[2] = {FLCoeff, FRCoeff};
 
  for ( unsigned int scalarIndexHull = 0; scalarIndexHull < spaceTimeNodesOnCellHull; scalarIndexHull++ ) {
    spaceTimePredictor_extrapolate_body_AoS(
      QHullOut,
      QIn,
      FLRCoeff,
      nodesPerAxis,
      strideQ,
      scalarIndexHull);
  }
}

void exahype2::aderdg::spaceTimePredictor_extrapolate_Lobatto_loop_AoS(
    double * __restrict__       QHullOut,
    const double * __restrict__ QIn,
    const double * __restrict__ FLCoeff, // just to have same signature as other routine
    const double * __restrict__ FRCoeff, // just to have same signature as other routine
    const int                   order,
    const int                   unknowns,
    const int                   auxiliaryVariables) {
  const int nodesPerAxis = order + 1;
  
  const int strideQ = unknowns+auxiliaryVariables;
  
  const int spaceTimeNodesOnCellHull = getNodesPerCell(nodesPerAxis)/* nodesPerAxis^d */ * 2 * Dimensions;
 
  for ( unsigned int scalarIndexHull = 0; scalarIndexHull < spaceTimeNodesOnCellHull; scalarIndexHull++ ) {
    spaceTimePredictor_extrapolate_Lobatto_body_AoS(
      QHullOut,
      QIn,
      nodesPerAxis,
      strideQ,
      scalarIndexHull);
  }
}
    
void exahype2::aderdg::spaceTimePredictor_extrapolateInTime_loop_AoS(
  double * __restrict__       UOut,
  const double * __restrict__ QIn,
  const double * __restrict__ FRCoeff,
  const int                   order,
  const int                   unknowns,
  const int                   auxiliaryVariables) {
  const int nodesPerAxis = order + 1;
  
  const int strideQ = unknowns+auxiliaryVariables;
  
  const int nodesPerCell = getNodesPerCell(nodesPerAxis);
 
  for ( unsigned int scalarIndexCell = 0; scalarIndexCell < nodesPerCell; scalarIndexCell++ ) {
    exahype2::aderdg::spaceTimePredictor_extrapolateInTime_body_AoS(
      UOut,
      QIn,
      FRCoeff,
      nodesPerAxis,
      strideQ,
      scalarIndexCell);
  }
}

void exahype2::aderdg::spaceTimePredictor_extrapolateInTime_Lobatto_loop_AoS(
  double * __restrict__       UOut,
  const double * __restrict__ QIn,
  const double * __restrict__ FRCoeff,
  const int                   order,
  const int                   unknowns,
  const int                   auxiliaryVariables) {
  const int nodesPerAxis = order + 1;
  
  const int strideQ = unknowns+auxiliaryVariables;
  
  const int nodesPerCell = getNodesPerCell(nodesPerAxis);
 
  for ( unsigned int scalarIndexCell = 0; scalarIndexCell < nodesPerCell; scalarIndexCell++ ) {
    exahype2::aderdg::spaceTimePredictor_extrapolateInTime_Lobatto_body_AoS(
      UOut,
      QIn,
      FRCoeff,
      nodesPerAxis,
      strideQ,
      scalarIndexCell);
  }
}
