#ifndef _EXAHYPE2_ADERDG_KERNEL_UTILS_H_
#define _EXAHYPE2_ADERDG_KERNEL_UTILS_H_

#include "tarch/la/Vector.h"

namespace exahype2 {
  namespace aderdg {

    #if defined(OpenMPGPUOffloading)
    #pragma omp declare target
    #endif
    GPUCallableMethod int getNodesPerCell(int nodesPerAxis);
    #if defined(OpenMPGPUOffloading)
    #pragma omp end declare target
    #endif
  
    #if defined(OpenMPGPUOffloading)
    #pragma omp declare target
    #endif
    GPUCallableMethod int getSpaceTimeNodesPerCell(int nodesPerAxis);
    #if defined(OpenMPGPUOffloading)
    #pragma omp end declare target
    #endif
  
    /**
     * If the stride is used for a space-time quantity, then this function generates:
     *
     * (1,nodesPerAxis,nodesPerAxis^2,nodesPerAxis^3) for strides (t,x,y,z).
     *
     * Otherwise, for time-invariant fields, it generates
     *
     * (0,1,nodesPerAxis,nodesPerAxis^2) for strides (t,x,y,z).
     *   
     * @param[in] strides4SpaceTimeQuantity generate strides for space-time quantity (default=true).
     * @param[in] nodesPerAxis              nodes per coordinate axis nodes/Lagrange basis functions per coordinate axis (order+1)
     * @return Strides per direction (t,x,y,z), where t-direction stride has index 0.
     */
     #if defined(OpenMPGPUOffloading)
     #pragma omp declare target
     #endif
     GPUCallableMethod tarch::la::Vector<Dimensions+1,int> getStrides(
        int nodesPerAxis,
        bool strides4SpaceTimeQuantity = true);
    #if defined(OpenMPGPUOffloading)
    #pragma omp end declare target
    #endif
     
     /**
      * @param[in] linearises the index with the given strides.
      * @param[in] strides strides per direction (t,x,y,z). time stamp
      * @note if stride[0] = 0 this implies that we have no time-index 
      */
     #if defined(OpenMPGPUOffloading)
     #pragma omp declare target
     #endif
     GPUCallableMethod int lineariseIndex(
        const tarch::la::Vector<Dimensions+1,int>& index,
        const tarch::la::Vector<Dimensions+1,int>& strides);
    #if defined(OpenMPGPUOffloading)
    #pragma omp end declare target
    #endif
       
     /**
      * @param[in] scalarIndex index that we want to delinearise 
      * @param[in] strides strides per direction (t,x,y,z). time stamp
      *
      * @return index that is descalar account to the strides
      *
      * @note index[0] (time index) is -1 if strides[0]=0. The latter implies that we work with a time-invariant field.
      * @seee getStrides(nodesPerAxis,strides4SpaceTimeQuantity)
      */
     #if defined(OpenMPGPUOffloading)
     #pragma omp declare target
     #endif
     GPUCallableMethod tarch::la::Vector<Dimensions+1,int> delineariseIndex(
       int scalarIndex,
       const tarch::la::Vector<Dimensions+1,int>& strides);       
    #if defined(OpenMPGPUOffloading)
    #pragma omp end declare target
    #endif
     
     /**
      * @brief Compute coordinates from cell geometry and quadrature index
      * @return a tuple (t,x,y,z) where the t-component has index 0.
      * @param[in] index t-,x-,y-, and z-direction (reference coordinates) components of descalar scalar index time stamp
      * @note coords[0] = t if if t-direction component of index is negative.
      */
     #if defined(OpenMPGPUOffloading)
     #pragma omp declare target
     #endif
     GPUCallableMethod tarch::la::Vector<Dimensions+1,double> getCoordinates(
       const tarch::la::Vector<Dimensions+1,int>&  index,
       const tarch::la::Vector<Dimensions,double>& centre,
       const double                                dx,
       const double                                t, 
       const double                                dt, 
       const double* __restrict__                  nodes);
    #if defined(OpenMPGPUOffloading)
    #pragma omp end declare target
    #endif
     
     /**
      * @brief Compute coordinates from cell geometry and quadrature index
      * @return a tuple (t,x,y,z) where the t-component has index 0.
      * @param[in] index t-,x-,y-, and z-direction (reference coordinates) components of descalar scalar index time stamp
      * @param[in] direction encodes direction of face normal (x: 0, y: 1, z: 2)
      * @note coords[0] = t if if t-direction component of index is negative.
      */
     #if defined(OpenMPGPUOffloading)
     #pragma omp declare target
     #endif
     GPUCallableMethod tarch::la::Vector<Dimensions+1,double> getCoordinatesOnFace(
       const tarch::la::Vector<Dimensions+1,int>& indexOnFace,
       const tarch::la::Vector<Dimensions,double>& faceCentre,
       const int                                   direction,
       const double                                dx,
       const double                                t, 
       const double                                dt, 
       const double* __restrict__                  nodes);
    #if defined(OpenMPGPUOffloading)
    #pragma omp end declare target
    #endif
     
     /**
      * @param[in] direction   coordinate direction of the (reference) element face normal (0: 
      * @param[in] orientation orientation of the (reference) element face normal (0: negative, 1: positive).
      * @note Result must be scaled additionally by nodesPerAxis if it used to access space-time quantities.
      */
     #if defined(OpenMPGPUOffloading)
     #pragma omp declare target
     #endif
     GPUCallableMethod int mapCellIndexToScalarHullIndex(
       const tarch::la::Vector<Dimensions+1,int>& indexCell,
       const int                                 direction,
       const int                                 orientation,
       const int                                 nodesPerAxis);
    #if defined(OpenMPGPUOffloading)
    #pragma omp end declare target
    #endif
     
     /**
      * @param[in] direction   coordinate direction of the (reference) element face normal (0: 
      * @param[in] orientation orientation of the (reference) element face normal (0: negative, 1: positive).
      * @return scalar cell index 
      */
     #if defined(OpenMPGPUOffloading)
     #pragma omp declare target
     #endif
     GPUCallableMethod int mapSpaceTimeFaceIndexToScalarCellIndex(
       const tarch::la::Vector<Dimensions+1,int>& indexFace,
       const int                                 direction,
       const int                                 id,
       const int                                 nodesPerAxis);
    #if defined(OpenMPGPUOffloading)
    #pragma omp end declare target
    #endif
    
    /**
     * @brief Computes the gradient at a given (space-time) quadrature point.
     *
     * Computes
     *
     * u(x,y,z) = sum_ijk u_j phi_i(x) phi_j(y) phi_k(z)
     *
     * => (d/dx u) (x,y,z) = sum_ijk u_ijk (d/dx phi_i)(x) phi_j(y) phi_k(z)
     *    (d/dy u) (x,y,z) = sum_ijk u_ijk phi_i(x) (d/dy phi_j)(y) phi_k(z)
     *    (d/dz u) (x,y,z) = sum_ijk u_ijk phi_i(x) phi_k(y) (d/dz phi_k)(z) 
     *
     * Lagrange basis property simplifies this to:
     * 
     * => (d/dx u) (x_a,y_i,z_k) = sum_a u_ijk (d/dx phi_i)(x_a)
     *    (d/dy u) (x_i,y_a,z_k) = sum_a u_ijk (d/dy phi_j)(y_a)
     *    (d/dz u) (x_i,y_j,z_a) = sum_a u_ijk (d/dz phi_k)(z_a) 
     *
     * For elements that do not equal the unit cube, we write:
     * 
     * => (d/dx u) (x_a,y_i,z_k) = sum_a u_ajk 1/lx * (d/dx phi_i)(x_a)
     *    (d/dy u) (x_i,y_a,z_k) = sum_a u_iak 1/ly * (d/dy phi_j)(y_a)
     *    (d/dz u) (x_i,y_j,z_a) = sum_a u_ija 1/lz * (d/dz phi_k)(z_a)
     * 
     * where lx,ly,lz are the lengths of the element.
     *
     * @note Assumes degrees of freedom are stored in order (iz,iy,ix,it)
     * @note This function assumes that the gradient zeroed out
     *
     * @param[in] QIn
     * @param[in] dudx derivative operator; size: (order+1)*(order+1)
     * @param[in] invDx
     * @param[in] nodesPerAxis nodes/Lagrange basis functions per coordinate axis (order+1)
     * @param[in] unknowns the number of PDE unknowns that we evolve
     * @param[in] strideQ
     * @param[in] strideGradQ
     * @param[in] scalarIndex
     * @param[in] gradQAux
     */
    #if defined(OpenMPGPUOffloading)
    #pragma omp declare target
    #endif
    GPUCallableMethod void gradient_AoS(
      const double* __restrict__ QIn,
      const double* __restrict__ dudx,
      const double               invDx,
      const int                  nodesPerAxis,
      const int                  strideQ,
      const int                  scalarIndex,
      double* __restrict__       gradQAux);
    #if defined(OpenMPGPUOffloading)
    #pragma omp end declare target
    #endif
     
     /** alignment in bytes
     */
     #if defined(OpenMPGPUOffloading)
     #pragma omp declare target
     #endif
     GPUCallableMethod size_t alignment();
    #if defined(OpenMPGPUOffloading)
    #pragma omp end declare target
    #endif
     
     #if defined(OpenMPGPUOffloading)
     #pragma omp declare target
     #endif
     GPUCallableMethod size_t paddedSize(size_t numElements, size_t sizeofType=sizeof(double));
    #if defined(OpenMPGPUOffloading)
    #pragma omp end declare target
    #endif
 
     double* allocateBuffer_cpu(size_t unpaddedSizeMultiplier, size_t paddedSizeMultiplier);  
     
     double* allocateBuffer_HIP(size_t unpaddedSizeMultiplier, size_t paddedSizeMultiplier);  
    
  } // aderdg
} // exahype2

#endif
