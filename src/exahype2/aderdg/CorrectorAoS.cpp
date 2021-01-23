#include "CorrectorAoS.h"
//#include <functional>

#include "KernelUtils.h"

#include "Generic.h"

#include "tarch/la/Vector.h"

#if defined(OpenMPGPUOffloading)
#pragma omp declare target
#endif
GPUCallableMethod void exahype2::aderdg::corrector_adjustSolution_body_AoS(
    std::function< void(
      double * __restrict__                       Q,
      const tarch::la::Vector<Dimensions,double>& x,
      double                                      t
    ) >                                         adjustSolution,
    double * __restrict__                       UOut,
    const double * __restrict__                 nodes,
    const tarch::la::Vector<Dimensions,double>& cellCentre,
    const double                                dx,
    const double                                t,
    const int                                   nodesPerAxis,
    const int                                   unknowns,
    const int                                   strideQ,
    const int                                   scalarIndex) {
  const tarch::la::Vector<Dimensions+1,int>     index  = delineariseIndex(scalarIndex,getStrides(nodesPerAxis,false));
  const tarch::la::Vector<Dimensions+1, double> coords = getCoordinates(index,cellCentre,dx,t,0,nodes);
  const tarch::la::Vector<Dimensions, double>   x( ( coords.data() + 1 ) );
 
  adjustSolution(&UOut[ scalarIndex * strideQ ], x, t);
}
#if defined(OpenMPGPUOffloading)
#pragma omp end declare target
#endif

#if defined(OpenMPGPUOffloading)
#pragma omp declare target
#endif
GPUCallableMethod void exahype2::aderdg::corrector_addFluxContributions_body_AoS(
    std::function< void(
      const double * __restrict__                 Q,
      const tarch::la::Vector<Dimensions,double>& x,
      double                                      t,
      int                                         normal,
      double * __restrict__                       F
    ) >                                         flux,
    double* __restrict__                        UOut, 
    const double* __restrict__                  QIn,
    double* __restrict__                        FAux, // must be allocated per thread as size is runtime parameter
    const double* __restrict__                  nodes,
    const double* __restrict__                  weights,
    const double* __restrict__                  Kxi,
    const tarch::la::Vector<Dimensions,double>& cellCentre,
    const double                                dx,
    const double                                t,
    const double                                dt,
    const int                                   nodesPerAxis,
    const int                                   unknowns,
    const int                                   strideQ,
    const int                                   scalarIndex) {
  tarch::la::Vector<Dimensions+1,int> strides = getStrides(nodesPerAxis,false);
  tarch::la::Vector<Dimensions+1,int> index   = delineariseIndex(scalarIndex,strides);
  const tarch::la::Vector<Dimensions+1, double> coords = getCoordinates(index,cellCentre,dx,t,dt,nodes);
  const tarch::la::Vector<Dimensions, double>   x( ( coords.data() + 1 ) );

  const double invDx = 1.0/dx;

  for ( int d = 1; d < Dimensions+1; d++) {
    const double coeff0 = dt * invDx/*[d-1]*/ / weights[index[d]];
      
    for ( int id = 0; id < nodesPerAxis; id++ ) { // loop over spatial neighbours
      const double coeff1 = coeff0 * Kxi[ index[d]*nodesPerAxis + id ]; // note: transposed vs. predictor flux computation
      for ( int it = 0; it < nodesPerAxis; it++ ) { // time loop
        const int scalarIndexQ = ( scalarIndex + (id - index[d])*strides[d] )*nodesPerAxis + it;
        const double* Q = &QIn[ scalarIndexQ*strideQ ];
        
        const double time = t + nodes[it] * dt; 
        const double coeff = coeff1 * weights[it]; // note: transposed vs. predictor flux computation

        flux( Q, x, time, d-1, FAux ); 
        for (int var=0; var < unknowns; var++) {
          UOut[ scalarIndex*strideQ+var ] += coeff * FAux[ var ]; // note: different vs predictor flux computation
        }
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
GPUCallableMethod void exahype2::aderdg::corrector_addSourceContributions_body_AoS(
    std::function< void(
      const double * __restrict__                 Q,
      const tarch::la::Vector<Dimensions,double>& x,
      double                                      t,
      double * __restrict__                       S
    ) >                                         algebraicSource,
    double * __restrict__                       UOut,
    double * __restrict__                       SAux,
    const double * __restrict__                 QIn,
    const double * __restrict__                 nodes,
    const double * __restrict__                 weights,
    const tarch::la::Vector<Dimensions,double>& cellCentre,
    const double                                dx,
    const double                                t,
    const double                                dt,
    const int                                   nodesPerAxis,
    const int                                   unknowns,
    const int                                   strideQ,
    const int                                   scalarIndex) {
  const tarch::la::Vector<Dimensions+1,int>     index  = delineariseIndex(scalarIndex,getStrides(nodesPerAxis,false));
  const tarch::la::Vector<Dimensions+1, double> coords = getCoordinates(index,cellCentre,dx,t,dt,nodes);
  const tarch::la::Vector<Dimensions, double>   x( ( coords.data() + 1 ) );
 
  for ( int it=0; it < nodesPerAxis; it++ ) { // time-integral 
    const int scalarIndexQ = scalarIndex*nodesPerAxis + it;
    const double* Q            = &QIn[ scalarIndexQ*strideQ ];
    
    const double time = t + nodes[it] * dt; 
    algebraicSource(Q, x, time, SAux);
    
    const double coeff = dt * weights[it];
    for (int var = 0; var < unknowns; var++) {
      UOut[ scalarIndex*strideQ + var ] += coeff * SAux[var];
    }
  }
}
#if defined(OpenMPGPUOffloading)
#pragma omp end declare target
#endif

#if defined(OpenMPGPUOffloading)
#pragma omp declare target
#endif
GPUCallableMethod void exahype2::aderdg::corrector_addNcpContributions_body_AoS(
    std::function< void(
      const double * __restrict__                 Q,
      double * __restrict__                       dQ_or_deltaQ,
      const tarch::la::Vector<Dimensions,double>& x,
      double                                      t,
      int                                         normal,
      double * __restrict__                       BgradQ
    ) >                                         nonconservativeProduct,
    double * __restrict__                       UOut,
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
    const int                                   scalarIndex) {
  const tarch::la::Vector<Dimensions+1,int>     index  = delineariseIndex(scalarIndex,getStrides(nodesPerAxis,false));
  const tarch::la::Vector<Dimensions+1, double> coords = getCoordinates(index,cellCentre,dx,t,dt,nodes);
  const tarch::la::Vector<Dimensions, double>   x( ( coords.data() + 1 ) );
   
  const double invDx = 1.0/dx;
 
  for ( int it=0; it < nodesPerAxis; it++ ) { // time-integral 
    const int scalarIndexQ = scalarIndex*nodesPerAxis + it; 
    
    gradient_AoS( QIn, dudx, invDx, nodesPerAxis, strideQ, scalarIndexQ, gradQAux );
    const double time = t + nodes[it] * dt; 
    const double* Q   = &QIn[ scalarIndexQ*strideQ ];
    
    const double coeff = dt * weights[it];
    for ( int direction = 0; direction < Dimensions; direction++ ) {
      nonconservativeProduct( Q,  &gradQAux[ direction*strideQ ], x, time, direction, SAux );
      for(int var=0; var<unknowns; var++) {
        UOut[ scalarIndex*strideQ + var ] += coeff * SAux[var];
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
GPUCallableMethod void exahype2::aderdg::corrector_addRiemannContributions_body_AoS(
  double * __restrict__       UOut,
  const double * __restrict__ riemannResultIn,
  const double * __restrict__ weights,
  const double * __restrict__ FLRCoeff[2],
  const double                dx,
  const double                dt,
  const int                   nodesPerAxis,
  const int                   unknowns,
  const int                   strideQ,
  const int                   strideRiemannResult,
  const int                   scalarIndexCell) {
  const tarch::la::Vector<Dimensions+1,int> index = delineariseIndex(scalarIndexCell,getStrides(nodesPerAxis,false));
  
  const double invDx = 1.0/dx;
  
  for (int d=0; d < Dimensions; d++) {
    for (int lr=0; lr<2; lr++) {
      const int scalarIndexCellFace = mapCellIndexToScalarHullIndex(index,d,lr,nodesPerAxis);

      for (int id=0; id < nodesPerAxis; id++) { 
        const double coeff = dt * FLRCoeff[lr][index[d+1]] * invDx/*[d]*/;
        for (int var=0; var < unknowns; var++) {
          UOut[ scalarIndexCell*strideQ + var ] += coeff * riemannResultIn[ (scalarIndexCellFace*nodesPerAxis + id)*strideRiemannResult + var ]; 
        }
      }
    }
  }
}
#if defined(OpenMPGPUOffloading)
#pragma omp end declare target
#endif

// CPU launchers
void exahype2::aderdg::corrector_addCellContributions_loop_AoS(
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
    double * __restrict__                       dQ_or_deltaQ,
    const tarch::la::Vector<Dimensions,double>& x,
    double                                      t,
    int                                         normal,
    double * __restrict__                       BgradQ
  ) >                                         nonconservativeProduct,
  std::function< void(
    double * __restrict__                       Q,
    const tarch::la::Vector<Dimensions,double>& x,
    double                                      t
  ) >                                         adjustSolution,
  double * __restrict__                       UOut, 
  const double * __restrict__                 QIn, 
  const double * __restrict__                 nodes,
  const double * __restrict__                 weights,
  const double * __restrict__                 Kxi,
  const double * __restrict__                 dudx, 
  const tarch::la::Vector<Dimensions,double>& cellCentre,
  const double                                dx,
  const double                                t,
  const double                                dt,
  const int                                   order,
  const int                                   unknowns,
  const int                                   auxiliaryVariables,
  const bool                                  callFlux,
  const bool                                  callSource,
  const bool                                  callNonconservativeProduct) {
  const int nodesPerAxis = order+1;

  const int nodesPerCell = getNodesPerCell(nodesPerAxis);
  
  const int strideQ      = unknowns+auxiliaryVariables;
  const int strideRhs    = unknowns;
  const int strideS      = unknowns;
  const int strideF      = unknowns;
  const int strideGradQ  = strideQ*Dimensions; // gradient of auxiliary variables needed for some apps
  
  double* FAux     = new double[nodesPerCell*strideF]{0.0}; 
  double* SAux     = new double[nodesPerCell*strideS]{0.0};
  double* gradQAux = new double[nodesPerCell*strideGradQ]{0.0};
  
  for ( int scalarIndexCell = 0; scalarIndexCell < nodesPerCell; scalarIndexCell++ ) {
    if ( callFlux ) { 
      corrector_addFluxContributions_body_AoS(
        flux,
        UOut, 
        QIn,
        &FAux[ scalarIndexCell*strideF ],
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
        scalarIndexCell);
    }
    if ( callSource ) { 
      corrector_addSourceContributions_body_AoS(
        algebraicSource,
        UOut,
        &SAux[ scalarIndexCell*strideS ],
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
        scalarIndexCell);
    }
    if ( callNonconservativeProduct ) { 
      corrector_addNcpContributions_body_AoS(
        nonconservativeProduct,
        UOut,
        &gradQAux[ scalarIndexCell*strideGradQ ],
        &SAux[ scalarIndexCell*strideS ],
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
        scalarIndexCell);
    }
   
    corrector_adjustSolution_body_AoS(
      adjustSolution,
      UOut,
      nodes,
      cellCentre,
      dx,
      t+dt,
      nodesPerAxis,
      unknowns,
      strideQ,
      scalarIndexCell);
  } // scalarIndexCell
  
  delete [] FAux;
  delete [] SAux;
  delete [] gradQAux;
}

void exahype2::aderdg::corrector_addRiemannContributions_loop_AoS(
  double * __restrict__       UOut,
  const double * __restrict__ riemannResultIn,
  const double * __restrict__ weights,
  const double * __restrict__ FLCoeff,
  const double * __restrict__ FRCoeff,
  const double                dx,
  const double                dt,
  const int                   order,
  const int                   unknowns,
  const int                   auxiliaryVariables) {
  const int nodesPerAxis = order+1;

  const int nodesPerCell = getNodesPerCell(nodesPerAxis);
  
  const int strideQ             = unknowns+auxiliaryVariables;
  const int strideRiemannResult = unknowns;
  
  const double* FLRCoeff[2] = {FLCoeff, FRCoeff};
 
  for ( int scalarIndexCell = 0; scalarIndexCell < nodesPerCell; scalarIndexCell++ ) {
    corrector_addRiemannContributions_body_AoS(
      UOut,
      riemannResultIn,
      weights,
      FLRCoeff,
      dx,
      dt,
      nodesPerAxis,
      unknowns,
      strideQ,
      strideRiemannResult,
      scalarIndexCell);
  }
}
