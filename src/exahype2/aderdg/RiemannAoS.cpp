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
  const double * __restrict__                 nodes,
  const double                                t,
  const double                                dt,
  const tarch::la::Vector<Dimensions,double>& faceCentre,
  const double                                dx,
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
    const double * __restrict__                 Q,
    const tarch::la::Vector<Dimensions,double>& x,
    double                                      t,
    const int                                   normal
  ) >                                         maxAbsoluteEigenvalue,
  const double * __restrict__                 QLR,
  const double * __restrict__                 nodes,
  const double                                t,
  const double                                dt,
  const tarch::la::Vector<Dimensions,double>& faceCentre,
  const double                                dx,
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
    double * __restrict__                       QInside,
    double * __restrict__                       OOutside,
    const tarch::la::Vector<Dimensions,double>& x,
    double                                      t,
    int                                         normal
  ) >                                         boundaryState,
  double * __restrict__                       QOut,
  double * __restrict__                       QIn,
  const double * __restrict__                 nodes,
  const double                                t,
  const double                                dt,
  const tarch::la::Vector<Dimensions,double>& faceCentre,
  const double                                dx,
  const int                                   order,
  const int                                   unknowns,
  const int                                   auxiliaryVariables,
  const int                                   direction) {
  const int nodesPerAxis = order + 1;

  const int strideQ = unknowns + auxiliaryVariables; 

  const int spaceTimeNodesOnFace = getNodesPerCell(nodesPerAxis); // nodesPerAxis^d

  for ( int scalarIndexFace = 0; scalarIndexFace < spaceTimeNodesOnFace; scalarIndexFace++ ) {
    riemann_setBoundaryState_body_AoS(
      boundaryState,
      QOut,
      QIn,
      nodes,
      t,
      dt,
      faceCentre,
      dx,
      nodesPerAxis,
      unknowns,
      strideQ,
      direction,
      scalarIndexFace);
  } 
}
    
void exahype2::aderdg::riemann_maxAbsoluteEigenvalue_loop_AoS(
  std::function< double(
    const double * __restrict__                 Q,
    const tarch::la::Vector<Dimensions,double>& x,
    double                                      t,
    const int                                   direction
  ) >                                        maxAbsoluteEigenvalue,
  double                                     maxEigenvaluePerFaceOut[Dimensions*2],
  const double * __restrict__                QHullIn[Dimensions*2],
  const double * __restrict__                nodes,
  const double                               t,
  const double                               dt,
  const tarch::la::Vector<Dimensions,double> faceCentres[Dimensions*2],
  const double                               dx,
  const int                                  order,
  const int                                  unknowns,
  const int                                  auxiliaryVariables,
  const int                                  direction) {
  const int nodesPerAxis = order+1;

  const int strideQ = unknowns+auxiliaryVariables;
  
  // Each face stores first: the degrees of freedom of QL ("left") and then of QR ("right")
  const int strideBlock = getNodesPerCell(nodesPerAxis); 

  // 6144 work items for order 7 in 3D 
  const int collocatedSpaceTimeNodesOnCellHull = Dimensions*2 * 2 * strideBlock; // 2 * #faces * nodesPerAxis^d

  for ( int scalarIndexHull = 0; scalarIndexHull < collocatedSpaceTimeNodesOnCellHull; scalarIndexHull++ ) {
    const int face            = scalarIndexHull / (2*strideBlock);
    const int scalarIndexFace = scalarIndexHull - face*2*strideBlock;
    const double smax = riemann_maxAbsoluteEigenvalue_body_AoS(
      maxAbsoluteEigenvalue,
      QHullIn[ face ],
      nodes,
      t,
      dt,
      faceCentres[ face ],
      dx,
      nodesPerAxis,
      unknowns,
      strideQ,
      direction,
      scalarIndexFace);
    maxEigenvaluePerFaceOut[ face ] = std::max( maxEigenvaluePerFaceOut[ face ], smax );
  }
}
