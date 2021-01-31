#include "RusanovNonlinearAoS.h"

#include "Generic.h"

#include "KernelUtils.h"

#if defined(OpenMPGPUOffloading)
#pragma omp declare target
#endif
GPUCallableMethod void exahype2::aderdg::rusanovNonlinear_body_AoS(
    std::function< void(
      const double * __restrict__                 Q,
      const tarch::la::Vector<Dimensions,double>& x,
      double                                      t,
      int                                         normal,
      double * __restrict__                       F
    ) >                                         fluxL,
    std::function< void(
      const double * __restrict__                 Q,
      const tarch::la::Vector<Dimensions,double>& x,
      double                                      t,
      int                                         normal,
      double * __restrict__                       F
    ) >                                         fluxR,
    std::function< void(
      double * __restrict__                       Q,
      double * __restrict__                       dQ_or_deltaQ,
      const tarch::la::Vector<Dimensions,double>& x,
      double                                      t,
      int                                         normal,
      double * __restrict__                       BgradQ
    ) >                                         nonconservativeProduct,
    double * __restrict__                       riemannResultOut,
    double * __restrict__                       FLAux,
    double * __restrict__                       FRAux,
    double * __restrict__                       QAvgAux,
    double * __restrict__                       dQAux,
    double * __restrict__                       SAux,
    const double * __restrict__                 QLIn, 
    const double * __restrict__                 QRIn, 
    const double                                smax,
    const double * __restrict__                 nodes, 
    const double * __restrict__                 weights, 
    const tarch::la::Vector<Dimensions,double>& faceCentre,
    const double                                dx,
    const double                                t,
    const double                                dt,
    const int                                   nodesPerAxis,
    const int                                   unknowns,
    const int                                   strideQ,
    const int                                   strideF,
    const int                                   direction,
    const bool                                  orientationToCell,
    const bool                                  callFlux,
    const bool                                  callNonconservativeProduct,
    const int                                   scalarIndexHull) {
  const tarch::la::Vector<Dimensions+1,int> strides = getStrides(nodesPerAxis,false); 
  const tarch::la::Vector<Dimensions+1,int> indexUHull = // (y,z,face) , (x,z,face), (x,y,face)
    delineariseIndex(scalarIndexHull,strides); 
  const tarch::la::Vector<Dimensions+1,double> coords = 
    getCoordinatesOnFace(indexUHull,faceCentre,direction,dx,t,dt,nodes);
  const tarch::la::Vector<Dimensions,double> x(&coords[1]);

  const int scalarIndexFace = scalarIndexHull - indexUHull[Dimensions]*strides[Dimensions];

  // zero result vector
  for (int var = 0; var < unknowns; var++) {
    riemannResultOut[ scalarIndexHull*strideF + var ] = 0;
  }
  for (int it = 0; it < nodesPerAxis; it++) { // time integration 
    const int offsetQ   = (scalarIndexFace*nodesPerAxis + it)*strideQ;
    const double time = t + nodes[it] * dt;
    
    // time averaged flux contributions
    if ( callFlux ) {
      const double coeff1 = weights[it] * 0.5;
      const double coeff2 = coeff1 * smax;
  
      fluxL( QLIn + offsetQ, x, time, direction, FLAux ); 
      fluxR( QRIn + offsetQ, x, time, direction, FRAux );
      for (int var = 0; var < unknowns; var++) {
        riemannResultOut[ scalarIndexHull*strideF + var ] += coeff1 * (FRAux[ var ] + FLAux[ var ]);
      }
      for (int var = 0; var < unknowns; var++) {
        riemannResultOut[ scalarIndexHull*strideF + var ] -= coeff2 * (QRIn[ offsetQ + var ] - QLIn[ offsetQ + var ]);
      }
    }
    // nonconservative product contirbutions
    if ( callNonconservativeProduct ) {
      // orientationToCell = 0 => face is left to cell  => cell is right to face => cell gets FR => sign is negative
      // orientationToCell = 1 => face is right to cell => cell is left to face  => cell gets FL => sign is positive
      const double coeff0 = (-1.0 + 2.0*orientationToCell) * 0.5;
      for (int var=0; var < strideQ; var++) {
        dQAux[ var ] = QRIn[ offsetQ + var ] - QLIn[ offsetQ + var ];
      }
      for (int var=0; var < strideQ; var++) {
        QAvgAux[var] = 0.5 * (QRIn[ offsetQ + var ] + QLIn[ offsetQ + var ]);
      }
      
      nonconservativeProduct(QAvgAux,  dQAux, x, time, direction, SAux);

      const double coeff = coeff0*weights[it];
      for (int var = 0; var < unknowns; var++) {
        riemannResultOut[ scalarIndexHull*strideF + var ] += coeff * SAux[var];
      }
    }
  }
}
#if defined(OpenMPGPUOffloading)
#pragma omp end declare target
#endif

// launchers
void exahype2::aderdg::rusanovNonlinear_loop_AoS(
  std::function< void(
    const double * __restrict__                 Q,
    const tarch::la::Vector<Dimensions,double>& x,
    double                                      t,
    int                                         normal,
    double * __restrict__                       F
  ) >                                         flux,
  std::function< void(
    const double * __restrict__                 Q,
    const tarch::la::Vector<Dimensions,double>& x,
    double                                      t,
    int                                         normal,
    double * __restrict__                       F
  ) >                                         boundaryFlux,
  std::function< void(
    double * __restrict__                       Q,
    double * __restrict__                       dQ_or_deltaQ,
    const tarch::la::Vector<Dimensions,double>& x,
    double                                      t,
    int                                         normal,
    double * __restrict__                       BgradQ
  ) >                                         nonconservativeProduct,
  std::function< void(
    double * __restrict__                       Q,
    double * __restrict__                       dQ_or_deltaQ,
    const tarch::la::Vector<Dimensions,double>& x,
    double                                      t,
    int                                         normal,
    double * __restrict__                       BgradQ
  ) >                                         boundaryNonconservativeProduct,
  double * __restrict__                       riemannResultOut,
  const double * __restrict__                 QHullIn[Dimensions*2], 
  const double                                maxEigenvaluePerFace[Dimensions*2],
  const double * __restrict__                 nodes, 
  const double * __restrict__                 weights, 
  const tarch::la::Vector<Dimensions,double>& cellCentre,
  const double                                dx,
  const double                                t,
  const double                                dt,
  const int                                   order,
  const int                                   unknowns,
  const int                                   auxiliaryVariables,
  const bool                                  atBoundary[Dimensions*2],
  const bool                                  callFlux,
  const bool                                  callNonconservativeProduct) {
  const int nodesPerAxis = order + 1;

  const int strideQ  = unknowns+auxiliaryVariables;
  const int strideS  = unknowns;
  const int strideF  = unknowns;
  const int strideDQ = strideQ; // gradient of auxiliary variables needed for some apps

  // 384 work items for order 7 in 3D
  const int nodesOnFace = getNodesPerCell(nodesPerAxis)/nodesPerAxis; 
  const int nodesOnHull = Dimensions*2 * nodesOnFace; // 2*d*nodesPerAxis^(d-1)
  
  const int strideQLR = nodesOnFace*nodesPerAxis; 
 
  double * FAux     = nullptr;
  double * QAvgAux  = nullptr;
  double * SAux     = nullptr;
  double * dQAux    = nullptr;
  if ( callFlux ) {
    FAux     = new double[2*nodesOnHull*strideF]{0.0}; 
  }
  if ( callNonconservativeProduct ) {
    QAvgAux  = new double[nodesOnHull*strideQ]{0.0};
    SAux     = new double[nodesOnHull*strideS]{0.0};
    dQAux    = new double[nodesOnHull*strideDQ]{0.0};
  }
  
  for ( int scalarIndexHull = 0; scalarIndexHull < nodesOnHull; scalarIndexHull++ ) {
    const int face              = scalarIndexHull / nodesOnFace;
    const int orientationToCell = face/Dimensions;
    const int direction         = face-Dimensions*orientationToCell;
    const int scalarIndexFace   = scalarIndexHull - face*nodesOnFace;
      
    const bool leftCellIsOutside  = atBoundary[ face ] && orientationToCell == 0; 
    const bool rightCellIsOutside = atBoundary[ face ] && orientationToCell == 1;
     
    tarch::la::Vector<Dimensions,double> faceCentre = cellCentre;
    faceCentre[ direction ] += (-1+2*orientationToCell)*0.5*dx;    
 
    rusanovNonlinear_body_AoS(
      ( leftCellIsOutside )  ? boundaryFlux : flux,
      ( rightCellIsOutside ) ? boundaryFlux : flux,
      ( leftCellIsOutside || rightCellIsOutside ) ? boundaryNonconservativeProduct : nonconservativeProduct,
      riemannResultOut + face*nodesOnFace,
      FAux + scalarIndexHull*2*strideF,
      FAux + scalarIndexHull*2*strideF + strideF,
      QAvgAux + scalarIndexHull*strideQ,
      dQAux   + scalarIndexHull*strideDQ,
      SAux    + scalarIndexHull*strideS,
      QHullIn[ face ], 
      QHullIn[ face ] + strideQLR, 
      maxEigenvaluePerFace[ face ],
      nodes, 
      weights, 
      faceCentre,
      dx,
      t,
      dt,
      nodesPerAxis,
      unknowns,
      strideQ,
      strideF,
      direction,
      orientationToCell,
      callFlux,
      callNonconservativeProduct,
      scalarIndexHull);
  }

  if ( callFlux ) {
    delete [] FAux;
  }
  if ( callNonconservativeProduct ) {
    delete [] QAvgAux;
    delete [] SAux;
    delete [] dQAux;
  }
}  
