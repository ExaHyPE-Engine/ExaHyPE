#include "RiemannAoS.h"

#include "Generic.h"

#include "KernelUtils.h"

#if defined(OpenMPGPUOffloading)
#pragma omp declare target
#endif
GPUCallableMethod void exahype2::aderdg::riemann_setBoundaryState_body_AoS(
  std::function< void(
    double * __restrict__                       QInside,
    double * __restrict__                       OOutside,
    const tarch::la::Vector<Dimensions,double>& x,
    double                                      t,
    int                                         normal
  ) >                                         boundaryState,
  double * __restrict__                       QOut,
  double * __restrict__                       QIn,
  const double * const __restrict__                 nodes,
  const tarch::la::Vector<Dimensions,double>& faceCentre,
  const double                                dx,
  const double                                t,
  const double                                dt,
  const int                                   nodesPerAxis,
  const int                                   unknowns,
  const int                                   strideQ,
  const int                                   direction,
  const int                                   scalarIndexFace) {
  const tarch::la::Vector<Dimensions+1,int> indexQFace = // (t,y,z,face=0) , (t,x,z,face=0), (t,x,y,face=0)
    delineariseIndex(scalarIndexFace,getStrides(nodesPerAxis)); 
  const tarch::la::Vector<Dimensions+1,double> coords = 
    getCoordinatesOnFace(indexQFace,faceCentre,direction,dx,t,dt,nodes);
  const tarch::la::Vector<Dimensions,double> x(&coords[1]);
  const double time = coords[0];

  boundaryState( &QIn[ scalarIndexFace*strideQ ], &QOut[ scalarIndexFace*strideQ ], x, time, direction );
}
#if defined(OpenMPGPUOffloading)
#pragma omp end declare target
#endif

#if defined(OpenMPGPUOffloading)
#pragma omp declare target
#endif
GPUCallableMethod double exahype2::aderdg::riemann_maxAbsoluteEigenvalue_body_AoS(
  std::function< double(
    const double * const __restrict__                 Q,
    const tarch::la::Vector<Dimensions,double>& x,
    double                                      t,
    const int                                   normal
  ) >                                         maxAbsoluteEigenvalue,
  const double * const __restrict__                 QLR,
  const double * const __restrict__                 nodes,
  const tarch::la::Vector<Dimensions,double>& faceCentre,
  const double                                dx,
  const double                                t,
  const double                                dt,
  const int                                   nodesPerAxis,
  const int                                   unknowns,
  const int                                   strideQ,
  const int                                   direction,
  const int                                   scalarIndexFace) {
  const tarch::la::Vector<Dimensions+1,int> strides = getStrides(nodesPerAxis);
  const tarch::la::Vector<Dimensions+1,int> indexQHull = // (t,y,z,block) , (t,x,z,block), (t,x,y,block)
    delineariseIndex(scalarIndexFace,strides);
  const tarch::la::Vector<Dimensions+1,double> coords = 
    getCoordinatesOnFace(indexQHull,faceCentre,direction,dx,t,dt,nodes); // ignores indexHull[Dimensions]
  const tarch::la::Vector<Dimensions,double> x(&coords[1]);
  const double time = coords[0];
  
  return maxAbsoluteEigenvalue( &QLR[ scalarIndexFace*strideQ ], x, time, direction );
  // @todo: check this value is > 0.
}
#if defined(OpenMPGPUOffloading)
#pragma omp end declare target
#endif

// launchers

void exahype2::aderdg::riemann_setBoundaryState_loop_AoS(
  std::function< void(
    const double * const __restrict__                 QInside,
    double * __restrict__                       OOutside,
    const tarch::la::Vector<Dimensions,double>& x,
    double                                      t,
    int                                         normal
  ) >                                         boundaryState,
  double *                                    QHullOut[Dimensions*2],
  const double * const __restrict__                 nodes,
  const tarch::la::Vector<Dimensions,double>& cellCentre,
  const double                                dx,
  const double                                t,
  const double                                dt,
  const int                                   order,
  const int                                   unknowns,
  const int                                   auxiliaryVariables,
  const bool                                  atBoundary[Dimensions*2]) {
  const int nodesPerAxis = order + 1;

  const int strideQ = unknowns + auxiliaryVariables; 
  
  const int spaceTimeNodesOnFace = getNodesPerCell(nodesPerAxis); // nodesPerAxis^d
  
  const int strideQLR = spaceTimeNodesOnFace;

  for (int orientation=0; orientation < 2; orientation++) {
    for (int direction = 0; direction < Dimensions; direction++) {
      const int face = Dimensions*orientation + direction; 
      tarch::la::Vector<Dimensions,double> faceCentre = cellCentre;
      faceCentre[ direction ] += (-1+2*orientation)*0.5*dx;    
      
      // set Dirichlet boundary conditions
      if ( atBoundary[ face ] ) {
        double* QL  = QHullOut[ face ];
        double* QR  = QHullOut[ face ] + strideQLR*strideQ;
        double* QOut = ( orientation == 0 ) ? QL : QR;
        double* QIn  = ( orientation == 0 ) ? QR : QL;
        for ( int scalarIndexFace = 0; scalarIndexFace < spaceTimeNodesOnFace; scalarIndexFace++ ) {
          riemann_setBoundaryState_body_AoS(
            boundaryState,
            QOut,
            QIn,
            nodes,
            faceCentre,
            dx,
            t,
            dt,
            nodesPerAxis,
            unknowns,
            strideQ,
            direction,
            scalarIndexFace);
        }
      }
    }
  } 
}
    
void exahype2::aderdg::riemann_maxAbsoluteEigenvalue_loop_AoS(
  std::function< double(
    const double * const __restrict__                 Q,
    const tarch::la::Vector<Dimensions,double>& x,
    double                                      t,
    const int                                   direction
  ) >                                         maxAbsoluteEigenvalue,
  double                                      maxEigenvaluePerFaceOut[Dimensions*2],
  double * const                              QHullIn[Dimensions*2],
  const double * const __restrict__                 nodes,
  const tarch::la::Vector<Dimensions,double>& cellCentre,
  const double                                dx,
  const double                                t,
  const double                                dt,
  const int                                   order,
  const int                                   unknowns,
  const int                                   auxiliaryVariables) {
  const int nodesPerAxis = order+1;

  const int strideQ = unknowns+auxiliaryVariables;
  
  // Each face stores first: the degrees of freedom of QL ("left") and then of QR ("right")
  const int strideQLR = getNodesPerCell(nodesPerAxis); 

  // 6144 work items for order 7 in 3D 
  const int collocatedSpaceTimeNodesOnCellHull = Dimensions*2 * 2 * strideQLR; // #faces * 2 * nodesPerAxis^d,
 
  // init output 
  for (int face=0; face<2*Dimensions; face++) {
    maxEigenvaluePerFaceOut[ face ] = 0.0;
  }
  // compute
  // scalarIndexHull = face*2*scalarIndexFace
  for ( int scalarIndexHull = 0; scalarIndexHull < collocatedSpaceTimeNodesOnCellHull; scalarIndexHull++ ) {
    const int face        = scalarIndexHull / (2*strideQLR);
    // face = Dimensions*orientation + direction, direction < Dimensions
    const int orientation = face/Dimensions;
    const int direction   = face-Dimensions*orientation; 
    // strip off face index info to get  DoF index on face
    const int scalarIndexFace = scalarIndexHull - face*2*strideQLR;
    
    tarch::la::Vector<Dimensions,double> faceCentre = cellCentre;
    faceCentre[ direction ] += (-1+2*orientation)*0.5*dx;    
    const double smax = riemann_maxAbsoluteEigenvalue_body_AoS(
      maxAbsoluteEigenvalue,
      QHullIn[ face ],
      nodes,
      faceCentre,
      dx,
      t,
      dt,
      nodesPerAxis,
      unknowns,
      strideQ,
      direction,
      scalarIndexFace);
    maxEigenvaluePerFaceOut[ face ] = std::max( maxEigenvaluePerFaceOut[ face ], smax );
  }
}
