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
#pragma omp declare target
#endif
GPUCallableMethod int exahype2::aderdg::getSpaceTimeNodesPerCell(int nodesPerAxis) {
  int nodesPerCell = 1;
  for ( int d = 0; d < Dimensions+1; d++ ) { nodesPerCell *= nodesPerAxis; }
  return nodesPerCell;
}

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
    coords[0] + nodes[index[0]] * dt;
  } 
  for ( int d = 1; d < Dimensions+1; d++ ) { // x -> y -> z
    coords[d] = centre[d-1] + dx/*[d-1]*/ * (nodes[index[d]] - 0.5);
  }
  return coords;
}

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
    coords[0] + nodes[indexOnFace[0]] * dt;
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
#pragma omp declare target
#endif
GPUCallableMethod int exahype2::aderdg::mapCellIndexToScalarHullIndex(
  const tarch::la::Vector<Dimensions+1,int>& indexCell,
  const int                                 direction,
  const int                                 orientation,
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
  const int faceOffset = ( direction*2 + orientation ) * stride; // stride = nodesPerAxis^{direction-1}
  scalarIndexFace += faceOffset;
  return scalarIndexFace;
}

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
  indexCell[0] = indexFace[0];
  int i = 1;
  for ( int e = 0; e < Dimensions; e++) {
    if ( e != direction ) {
      indexCell[e+1] = indexFace[i++];
    }
  }
  indexCell[direction+1] = id;
  return lineariseIndex(indexCell,getStrides(nodesPerAxis));
}

#if defined(OpenMPGPUOffloading)
#pragma omp declare target
#endif
GPUCallableMethod void exahype2::aderdg::gradient_AoS(
  const double* __restrict__ QIn,
  const double* __restrict__ dudx,
  const double               invDx,
  const int                  nodesPerAxis,
  const int                  strideQ,
  const int                  scalarIndex,
  double* __restrict__       gradQ) {
  const tarch::la::Vector<Dimensions+1,int> strides = getStrides(nodesPerAxis);
  const tarch::la::Vector<Dimensions+1,int> index   = delineariseIndex(scalarIndex, strides);

  for ( int d = 0; d < Dimensions; d++ ) { // x -> y -> z
    // zero
    for (int var=0; var < strideQ; var++) {
      gradQ[ d + var*Dimensions ] = 0.0;
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
#pragma omp declare target
#endif
GPUCallableMethod size_t exahype2::aderdg::alignment() {
  //return 64;
  return -1;
}

#if defined(OpenMPGPUOffloading)
#pragma omp declare target
#endif
GPUCallableMethod size_t exahype2::aderdg::paddedSize(size_t numElements, size_t sizeofType) {
  //int align = alignment() / sizeofType; // 8 elements per cache line for double 
  //return ( ( numElements + align - 1 ) / align ) * align;  // @todo: padding
  return numElements;
}

double* exahype2::aderdg::allocateBuffer_cpu(size_t unpaddedSizeMultiplier, size_t paddedSizeMultiplier) {
  //double* ptr = nullptr;
  //int ierr = posix_memalign( (void**) &ptr, alignment(), sizeof(double) * unpaddedSizeMultiplier * paddedSizeMultiplier );
  //return ptr;
  return new double[ unpaddedSizeMultiplier * paddedSizeMultiplier ]; 
}
