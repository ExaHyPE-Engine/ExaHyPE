#include "KernelUtils.h"

#if defined(OpenMPGPUOffloading)
#pragma omp declare target
#endif
GPUCallableMethod int exahype2::aderdg::getNodesPerCell(int nodesPerAxis) {
  int nodesPerCell = 1;
  for ( int d = 0; d < Dimensions; d++ ) { nodesPerCell *= nodesPerAxis; }
  return nodesPerCell;
}
#if defined(OpenMPGPUOffloading)
#pragma omp end declare target
#endif

#if defined(OpenMPGPUOffloading)
#pragma omp declare target
#endif
GPUCallableMethod int exahype2::aderdg::getSpaceTimeNodesPerCell(int nodesPerAxis) {
  int nodesPerCell = 1;
  for ( int d = 0; d < Dimensions+1; d++ ) { nodesPerCell *= nodesPerAxis; }
  return nodesPerCell;
}
#if defined(OpenMPGPUOffloading)
#pragma omp end declare target
#endif

#if defined(OpenMPGPUOffloading)
#pragma omp declare target
#endif
GPUCallableMethod tarch::la::Vector<Dimensions+1,int> exahype2::aderdg::getStrides(
   int nodesPerAxis,
   bool strides4SpaceTimeQuantity) {
  tarch::la::Vector<Dimensions+1,int> strides(1);
  for ( int d = 0; d < Dimensions; d++ ) { 
    strides[d+1] *= strides[d]*nodesPerAxis;
  }
  if ( !strides4SpaceTimeQuantity ) {
    for ( int d = 0; d < Dimensions+1; d++ ) { 
      strides[d] /= nodesPerAxis;
    }
  }
  return strides;
}
#if defined(OpenMPGPUOffloading)
#pragma omp end declare target
#endif

#if defined(OpenMPGPUOffloading)
#pragma omp declare target
#endif
GPUCallableMethod int exahype2::aderdg::lineariseIndex(
   const tarch::la::Vector<Dimensions+1,int>& index,
   const tarch::la::Vector<Dimensions+1,int>& strides
) {
  int scalarIndex = 0;
  for ( int d = 0; d < Dimensions+1; d++ ) { // t -> x -> y -> z
    scalarIndex += strides[d]*index[d];
  }
  return scalarIndex;
}
#if defined(OpenMPGPUOffloading)
#pragma omp end declare target
#endif
  
#if defined(OpenMPGPUOffloading)
#pragma omp declare target
#endif
GPUCallableMethod tarch::la::Vector<Dimensions+1,int> exahype2::aderdg::delineariseIndex(
  int scalarIndex,
  const tarch::la::Vector<Dimensions+1,int>& strides) {
  tarch::la::Vector<Dimensions+1,int> index(-1); // valid index component values are non-negative
  int tmp = 0;
  if ( strides[0] > 0 ) {
    for ( int d = Dimensions; d >= 0; d-- ) { // z -> y -> x -> t
      index[d] = (scalarIndex - tmp) / strides[d]; 
      tmp += strides[d]*index[d]; // iz -> iy -> ix -> it
    }
  } else {
    for ( int d = Dimensions; d >= 1; d-- ) { // z -> y -> x
      index[d] = (scalarIndex - tmp) / strides[d]; 
      tmp += strides[d]*index[d]; // iz -> iy -> ix
    }
  }
  return index;
}
#if defined(OpenMPGPUOffloading)
#pragma omp end declare target
#endif
  
#if defined(OpenMPGPUOffloading)
#pragma omp declare target
#endif
GPUCallableMethod tarch::la::Vector<Dimensions+1,double> exahype2::aderdg::getCoordinates(
  const tarch::la::Vector<Dimensions+1,int>& index,
  const tarch::la::Vector<Dimensions,double>& centre,
  const double                              dx,
  const double                              t, 
  const double                              dt, 
  const double* __restrict__                nodes) {
  tarch::la::Vector<Dimensions+1, double> coords;

  coords[0]= t;
  if ( index[0] >= 0 ) {
    coords[0] += nodes[index[0]] * dt;
  } 
  for ( int d = 1; d < Dimensions+1; d++ ) { // x -> y -> z
    coords[d] = centre[d-1] + dx/*[d-1]*/ * (nodes[index[d]] - 0.5);
  }
  return coords;
}
#if defined(OpenMPGPUOffloading)
#pragma omp end declare target
#endif

#if defined(OpenMPGPUOffloading)
#pragma omp declare target
#endif
GPUCallableMethod tarch::la::Vector<Dimensions+1,double> exahype2::aderdg::getCoordinatesOnFace(
  const tarch::la::Vector<Dimensions+1,int>& indexOnFace,
  const tarch::la::Vector<Dimensions,double>& faceCentre,
  const int                                 direction,
  const double                              dx,
  const double                              t, 
  const double                              dt, 
  const double* __restrict__                nodes) {
  tarch::la::Vector<Dimensions+1, double> coords;

  coords[0]= t;
  if ( indexOnFace[0] >= 0 ) {
    coords[0] += nodes[indexOnFace[0]] * dt;
  }
  int e = 1;
  for ( int d = 1; d < Dimensions+1; d++ ) { // x -> y -> z
    if ( d-1 == direction ) { 
      coords[d] = faceCentre[d-1];
    } else { 
      coords[d] = faceCentre[d-1] + dx/*[d-1]*/ * (nodes[indexOnFace[e]] - 0.5);
    }
    e++;
  }
  return coords;
}
#if defined(OpenMPGPUOffloading)
#pragma omp end declare target
#endif

#if defined(OpenMPGPUOffloading)
#pragma omp declare target
#endif
GPUCallableMethod int exahype2::aderdg::mapCellIndexToScalarFaceIndex(
  const tarch::la::Vector<Dimensions+1,int>& indexCell,
  const int                                 direction,
  const int                                 nodesPerAxis
) {
  // freeze spatial dimension direction (indexCell[direction+1])
  int scalarIndexFace = 0;
  int stride = 1;
  for ( int e=0; e < Dimensions; e++ ) { // ordering (fastest running left): (y,z), (x,z), (x,y)
    if ( e != direction ) {
      scalarIndexFace += stride*indexCell[e+1];
      stride *= nodesPerAxis;
    }
  }
  return scalarIndexFace;
}
#if defined(OpenMPGPUOffloading)
#pragma omp end declare target
#endif

#if defined(OpenMPGPUOffloading)
#pragma omp declare target
#endif
GPUCallableMethod int exahype2::aderdg::mapCellIndexToScalarHullIndex(
  const tarch::la::Vector<Dimensions+1,int>& indexCell,
  const int                                 direction,
  const int                                 orientation,
  const int                                 nodesPerAxis
) {
  // freeze spatial dimension direction (indexCell[direction+1])
  int scalarIndexHull = 0;
  int stride = 1;
  for ( int e=0; e < Dimensions; e++ ) { // ordering (fastest running left): (y,z), (x,z), (x,y)
    if ( e != direction ) {
      scalarIndexHull += stride*indexCell[e+1];
      stride *= nodesPerAxis;
    }
  }
  //const int faceOffset = ( direction*2 + orientation ) * stride; // stride = nodesPerAxis^{d-1}
  const int faceOffset = ( direction + Dimensions*orientation ) * stride; // stride = nodesPerAxis^{d-1}
  scalarIndexHull += faceOffset;
  return scalarIndexHull;
}
#if defined(OpenMPGPUOffloading)
#pragma omp end declare target
#endif

#if defined(OpenMPGPUOffloading)
#pragma omp declare target
#endif
GPUCallableMethod int exahype2::aderdg::mapSpaceTimeFaceIndexToScalarCellIndex(
  const tarch::la::Vector<Dimensions+1,int>& indexFace,
  const int                                 direction,
  const int                                 id,
  const int                                 nodesPerAxis
) {
  tarch::la::Vector<Dimensions+1,int> indexCell(-1);
  indexCell[0]           = indexFace[0]; // time
  indexCell[direction+1] = id;           // node in interior 
  int i = 1;
  for ( int e = 0; e < Dimensions; e++) { // take from face
    if ( e != direction ) {
      indexCell[e+1] = indexFace[i++];
    }
  }
  return lineariseIndex(indexCell,getStrides(nodesPerAxis));
}
#if defined(OpenMPGPUOffloading)
#pragma omp end declare target
#endif

#if defined(OpenMPGPUOffloading)
#pragma omp declare target
#endif
GPUCallableMethod void exahype2::aderdg::gradient_AoS(
  const double* __restrict__ const QIn,
  const double* __restrict__ const dudx,
  const double                     invDx,
  const int                        nodesPerAxis,
  const int                        strideQ,
  const int                        scalarIndex,
  double* __restrict__             gradQ) {
  // Always works with space time input data
  const tarch::la::Vector<Dimensions+1,int> strides = getStrides(nodesPerAxis);
  const tarch::la::Vector<Dimensions+1,int> index   = delineariseIndex(scalarIndex, strides);

  for ( int d = 0; d < Dimensions; d++ ) { // x -> y -> z
    // zero
    for (int var=0; var < strideQ; var++) {
      gradQ[ d*strideQ + var ] = 0.0;
      //gradQ[ d + var*Dimensions ] = 0.0;
    }
    // sum up
    for (int a=0; a < nodesPerAxis; a++) { 
      const double coeff = invDx/*[d]*/ * dudx[ a*nodesPerAxis + index[d+1] ];
      for (int var=0; var < strideQ; var++) {
        gradQ[ d*strideQ + var ] += coeff * QIn[ ( scalarIndex + (a-index[d+1])*strides[d+1] )*strideQ + var ]; 
        //gradQ[ d + var*Dimensions ] += coeff * QIn[ ( scalarIndex + (a-index[d+1])*strides[d+1] )*strideQ + var ]; 
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
GPUCallableMethod tarch::la::Vector<Dimensions,double> exahype2::aderdg::mapToReferenceCoordinates(
  const tarch::la::Vector<Dimensions,double>& x,
  const tarch::la::Vector<Dimensions,double>& cellCentre,
  const double                                dx) {
  return (x - cellCentre + dx/2.0) / dx;
}
#if defined(OpenMPGPUOffloading)
#pragma omp end declare target
#endif

/**
 * Evalutes DG polynomial at arbitrary reference space coordinate
 * in reference domain [0,1]^d.
 *
 * @see: mapToReferenceCoordinates
 */ 
#if defined(OpenMPGPUOffloading)
#pragma omp declare target
#endif
GPUCallableMethod void exahype2::aderdg::interpolate_AoS(
  const double* __restrict__ const            UIn,
  const double* __restrict__ const            nodes,
  const double* __restrict__ const            barycentricWeights,
  const tarch::la::Vector<Dimensions,double>& referenceCoodinates,
  const int                                   nodesPerAxis,
  const int                                   strideQ,
  double* __restrict__                        pointwiseQOut) {
  double l = 1.0;
  // clear
  for ( int var = 0; var < strideQ; var++ ) {
    pointwiseQOut[ var ] = 0.0;
  } 
  // compute
  for ( int d = 0; d < Dimensions; d++ ) {
    for ( int i = 0; i < nodesPerAxis; i++ ) {
      l *= (referenceCoodinates[d] - nodes[i]);
    }
  }
  const int nodesPerCell = getNodesPerCell(nodesPerAxis);   
  for ( int scalarIndex = 0; scalarIndex < nodesPerCell; scalarIndex++ ) {
    const tarch::la::Vector<Dimensions+1,int> strides = getStrides(nodesPerAxis,false);
    const tarch::la::Vector<Dimensions+1,int> index   = delineariseIndex(scalarIndex, strides);
    
    double coeff = l;
    for ( int d = 0; d < Dimensions; d++ ){
      coeff *= barycentricWeights[index[1+d]] / (referenceCoodinates[d] - nodes[index[1+d]] );
    } 
    for ( int var = 0; var < strideQ; var++ ) {
      pointwiseQOut[ var ] += coeff * UIn[ scalarIndex*strideQ + var ];
    }
  }
}
