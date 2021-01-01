#include "KernelUtils.h"

namespace exahype2 {
  namespace aderdg {
    
    GPUCallableMethod int getNodesPerCell(int nodesPerAxis) {
      int nodesPerCell = 1;
      for ( int d = 0; d < Dimensions; d++ ) { nodesPerCell *= nodesPerAxis; }
      return nodesPerCell;
    }
    
    GPUCallableMethod int getSpaceTimeNodesPerCell(int nodesPerAxis) {
      int nodesPerCell = 1;
      for ( int d = 0; d < Dimensions+1; d++ ) { nodesPerCell *= nodesPerAxis; }
      return nodesPerCell;
    }
    
    GPUCallableMethod tarch::la::Vector<Dimensions+1,int> getStrides(
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
    
    GPUCallableMethod int lineariseIndex(
       const tarch::la::Vector<Dimensions+1,int>& index,
       const tarch::la::Vector<Dimensions+1,int>& strides
    ) {
      int scalarIndex = 0;
      for ( int d = 0; d < Dimensions+1; d++ ) { // t -> x -> y -> z
        scalarIndex += strides[d]*index[d];
      }
      return scalarIndex;
    }
      
    GPUCallableMethod tarch::la::Vector<Dimensions+1,int> delineariseIndex(
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
      
    GPUCallableMethod tarch::la::Vector<Dimensions+1,double> getCoordinates(
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
    
    GPUCallableMethod tarch::la::Vector<Dimensions+1,double> getCoordinatesOnFace(
      const tarch::la::Vector<Dimensions+1,int>& indexOnFace,
      const tarch::la::Vector<Dimensions,double>& faceCentre,
      const int                                 direction,
      const double                              dx,
      const double                              t, 
      const double                              dt, 
      const double* __restrict__                nodes) {
      tarch::la::Vector<Dimensions+1, double> coords =
        getCoordinates(indexOnFace,faceCentre,dx,t,dt,nodes);
      coords[direction+1] = faceCentre[direction];
      return coords;
    }
    
    GPUCallableMethod int mapCellIndexToScalarHullIndex(
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
    
    GPUCallableMethod int mapSpaceTimeFaceIndexToScalarCellIndex(
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

    GPUCallableMethod void gradient_AoS(
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
        for (int a=0; a < nodesPerAxis; a++) { 
          const double coeff = invDx/*[d]*/ * dudx[ a*nodesPerAxis + index[d+1] ];
          for (int var=0; var < strideQ; var++) {
            //gradQ[ d*Dimensions + var ] += coeff * QIn[ ( scalarIndex + (a-index[d+1])*strides[d+1] )*strideQ + var ]; 
            gradQ[ d + var*Dimensions ] += coeff * QIn[ ( scalarIndex + (a-index[d+1])*strides[d+1] )*strideQ + var ]; 
          }
        }
      }
    }
    
    GPUCallableMethod size_t alignment() {
      //return 64;
      return -1;
    }
    
    GPUCallableMethod size_t paddedSize(size_t numElements, size_t sizeofType) {
      //int align = alignment() / sizeofType; // 8 elements per cache line for double 
      //return ( ( numElements + align - 1 ) / align ) * align;  // @todo: padding
      return numElements;
    }
    
    double* allocateBuffer_cpu(size_t unpaddedSizeMultiplier, size_t paddedSizeMultiplier) {
      //double* ptr = nullptr;
      //int ierr = posix_memalign( (void**) &ptr, alignment(), sizeof(double) * unpaddedSizeMultiplier * paddedSizeMultiplier );
      //return ptr;
      return new double[ unpaddedSizeMultiplier * paddedSizeMultiplier ]; 
    }

  }
}
