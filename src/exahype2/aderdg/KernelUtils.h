#include "tarch/la/Vector.h"

namespace exahype2 {
  namespace aderdg {

    GPUCallableMethod int getNodesPerCell(int nodesPerAxis);
  
    GPUCallableMethod int getSpaceTimeNodesPerCell(int nodesPerAxis);
  
    /**
     * If the stride is used for a space-time quantity, then this function generates:
     *
     * (1,nodesPerAxis,nodesPerAxis^2,nodesPerAxis^3) for strides (t,x,y,z).
     *
     * Otherwise, for time-invariant fields, it generates
     *
     * (0,1,nodesPerAxis,nodesPerAxis^2) for strides (t,x,y,z).
     *   
     * @param strides4SpaceTimeQuantity generate strides for space-time quantity (default=true).
     * @param nodesPerAxis              nodes per coordinate axis
     * @return Strides per direction (t,x,y,z), where t-direction stride has index 0.
     */
     GPUCallableMethod tarch::la::Vector<Dimensions+1,int> getStrides(
        int nodesPerAxis,
        bool strides4SpaceTimeQuantity = true);
     
     /**
      * @param linearises the index with the given strides.
      * @param strides strides per direction (t,x,y,z).
      * @note if stride[0] = 0 this implies that we have no time-index 
      */
     GPUCallableMethod int lineariseIndex(
        const tarch::la::Vector<Dimensions+1,int>& index,
        const tarch::la::Vector<Dimensions+1,int>& strides);
       
     /**
      * @param linearisedIndex index that we want to delinearise 
      * @param strides strides per direction (t,x,y,z).
      *
      * @return index that is delinearised account to the strides
      *
      * @note index[0] (time index) is -1 if strides[0]=0. The latter implies that we work with a time-invariant field.
      * @seee getStrides(nodesPerAxis,strides4SpaceTimeQuantity)
      */
     GPUCallableMethod tarch::la::Vector<Dimensions+1,int> delineariseIndex(
       int linearisedIndex,
       const tarch::la::Vector<Dimensions+1,int> strides);       
     
     /**
      * @brief Compute coordinates from cell geometry and quadrature index
      * @return a tuple (t,x,y,z) where the t-component has index 0.
      * @param index t-,x-,y-, and z-direction (reference coordinates) components of delinearised scalar index
      * @note coords[0] = t if if t-direction component of index is negative.
      */
     GPUCallableMethod tarch::la::Vector<Dimensions+1,double> getCoordinates(
       const tarch::la::Vector<Dimensions+1,int> index,
       const tarch::la::Vector<Dimensions+1,int> centre,
       const double                              dx,
       const double                              t, 
       const double                              dt, 
       const double* __restrict__                nodes);
     
     /**
      * @brief Compute coordinates from cell geometry and quadrature index
      * @return a tuple (t,x,y,z) where the t-component has index 0.
      * @param index t-,x-,y-, and z-direction (reference coordinates) components of delinearised scalar index
      * @param direction encodes direction of face normal (x: 0, y: 1, z: 2)
      * @note coords[0] = t if if t-direction component of index is negative.
      */
     GPUCallableMethod tarch::la::Vector<Dimensions+1,double> getCoordinatesOnFace(
       const tarch::la::Vector<Dimensions+1,int> indexOnFace,
       const tarch::la::Vector<Dimensions+1,int> faceCentre,
       const int                                 direction,
       const double                              dx,
       const double                              t, 
       const double                              dt, 
       const double* __restrict__                nodes);
     
     /**
      * @param direction   coordinate direction of the (reference) element face normal (0: 
      * @param orientation orientation of the (reference) element face normal (0: negative, 1: positive).
      * @note Result must be scaled additionally by nodesPerAxis if it used to access space-time quantities.
      */
     GPUCallableMethod int mapCellIndexToLinearisedHullIndex(
       const tarch::la::Vector<Dimensions+1,int> indexCell,
       const int                                 direction,
       const int                                 orientation,
       const int                                 nodesPerAxis);
     
     /**
      * @param direction   coordinate direction of the (reference) element face normal (0: 
      * @param orientation orientation of the (reference) element face normal (0: negative, 1: positive).
      * @return linearised cell index 
      */
     GPUCallableMethod int mapSpaceTimeFaceIndexToLinearisedCellIndex(
       const tarch::la::Vector<Dimensions+1,int> indexFace,
       const int                                 direction,
       const int                                 id,
       const int                                 nodesPerAxis);
     
     /** alignment in bytes
     */
     GPUCallableMethod size_t alignment();
     
     GPUCallableMethod size_t paddedSize(size_t numElements, size_t sizeofType=sizeof(double));
 
     double* allocateBuffer_cpu(size_t unpaddedSizeMultiplier, size_t paddedSizeMultiplier);  
     
     double* allocateBuffer_HIP(size_t unpaddedSizeMultiplier, size_t paddedSizeMultiplier);  
    
  } // aderdg
} // exahype2
