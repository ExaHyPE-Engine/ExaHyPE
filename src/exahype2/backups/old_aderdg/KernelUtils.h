// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/accelerator/accelerator.h"
#include "tarch/la/Vector.h"

#define FLCoeff(i) FLCoeff[i]
#define FRCoeff(i) FLCoeff[nodesPerAxis - 1 - i]

namespace exahype2 {
  namespace aderdg {

#if defined(GPUOffloadingOMP)
#pragma omp declare target
#endif
#if defined(GPUOffloadingCPP)
    int getNodesPerCell(int nodesPerAxis);
#else
    GPUCallableMethod int getNodesPerCell(int nodesPerAxis);
#endif
#if defined(GPUOffloadingOMP)
#pragma omp end declare target
#endif

#if defined(GPUOffloadingOMP)
#pragma omp declare target
#endif
#if defined(GPUOffloadingCPP)
    int getSpaceTimeNodesPerCell(int nodesPerAxis);
#else
    GPUCallableMethod int getSpaceTimeNodesPerCell(int nodesPerAxis);
#endif
#if defined(GPUOffloadingOMP)
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
     * @param[in] nodesPerAxis              nodes per coordinate axis nodes/Lagrange basis functions per coordinate axis
     * (order+1)
     * @return Strides per direction (t,x,y,z), where t-direction stride has index 0.
     */
#if defined(GPUOffloadingOMP)
#pragma omp declare target
#endif
#if defined(GPUOffloadingCPP)
    tarch::la::Vector<Dimensions + 1, int> getStrides(
#else
    GPUCallableMethod tarch::la::Vector<Dimensions + 1, int> getStrides(
#endif
      int nodesPerAxis, bool strides4SpaceTimeQuantity = true
    );
#if defined(GPUOffloadingOMP)
#pragma omp end declare target
#endif

/**
 * @param[in] linearises the index with the given strides.
 * @param[in] strides strides per direction (t,x,y,z). time stamp
 * @note if stride[0] = 0 this implies that we have no time-index
 */
#if defined(GPUOffloadingOMP)
#pragma omp declare target
#endif
#if defined(GPUOffloadingCPP)
    int lineariseIndex(
#else
    GPUCallableMethod int                                    lineariseIndex(
#endif
      const tarch::la::Vector<Dimensions + 1, int>& index, const tarch::la::Vector<Dimensions + 1, int>& strides
    );
#if defined(GPUOffloadingOMP)
#pragma omp end declare target
#endif

/**
 * @param[in] scalarIndex index that we want to delinearise
 * @param[in] strides strides per direction (t,x,y,z). time stamp
 *
 * @return index that is descalar account to the strides
 *
 * @note index[0] (time index) is -1 if strides[0]=0. The latter implies that we work with a time-invariant field.
 * @see getStrides(nodesPerAxis,strides4SpaceTimeQuantity)
 */
#if defined(GPUOffloadingOMP)
#pragma omp declare target
#endif
#if defined(GPUOffloadingCPP)
    tarch::la::Vector<Dimensions + 1, int> delineariseIndex(
#else
    GPUCallableMethod tarch::la::Vector<Dimensions + 1, int> delineariseIndex(
#endif
      int scalarIndex, const tarch::la::Vector<Dimensions + 1, int>& strides
    );
#if defined(GPUOffloadingOMP)
#pragma omp end declare target
#endif

/**
 * @brief Compute coordinates from cell geometry and quadrature index
 * @return a tuple (t,x,y,z) where the t-component has index 0.
 * @param[in] index t-,x-,y-, and z-direction (reference coordinates) components of descalar scalar index time stamp
 * @note coords[0] = t if if t-direction component of index is negative.
 */
#if defined(GPUOffloadingOMP)
#pragma omp declare target
#endif
#if defined(GPUOffloadingCPP)
    tarch::la::Vector<Dimensions + 1, double> getCoordinates(
#else
    GPUCallableMethod tarch::la::Vector<Dimensions + 1, double> getCoordinates(
#endif
      const tarch::la::Vector<Dimensions + 1, int>& index,
      const tarch::la::Vector<Dimensions, double>&  centre,
      const double                                  dx,
      const double                                  t,
      const double                                  dt,
      const double* __restrict__ nodes
    );
#if defined(GPUOffloadingOMP)
#pragma omp end declare target
#endif

/**
 * @brief Compute coordinates from cell geometry and quadrature index
 * @return a tuple (t,x,y,z) where the t-component has index 0.
 * @param[in] index t-,x-,y-, and z-direction (reference coordinates) components of descalar scalar index time stamp
 * @param[in] direction encodes direction of face normal (x: 0, y: 1, z: 2)
 * @note coords[0] = t if if t-direction component of index is negative.
 */
#if defined(GPUOffloadingOMP)
#pragma omp declare target
#endif
#if defined(GPUOffloadingCPP)
    tarch::la::Vector<Dimensions + 1, double> getCoordinatesOnFace(
#else
    GPUCallableMethod tarch::la::Vector<Dimensions + 1, double> getCoordinatesOnFace(
#endif
      const tarch::la::Vector<Dimensions + 1, int>& indexOnFace,
      const tarch::la::Vector<Dimensions, double>&  faceCentre,
      const int                                     direction,
      const double                                  dx,
      const double                                  t,
      const double                                  dt,
      const double* __restrict__ nodes
    );
#if defined(GPUOffloadingOMP)
#pragma omp end declare target
#endif

/**
 * Map cell index to an index for a single face.
 *
 * @param[in] direction   coordinate direction of the (reference) element face normal (0:
 * @note Result must be scaled additionally by nodesPerAxis if it used to access space-time quantities.
 */
#if defined(GPUOffloadingOMP)
#pragma omp declare target
#endif
#if defined(GPUOffloadingCPP)
    int mapCellIndexToScalarFaceIndex(
#else
    GPUCallableMethod int                                       mapCellIndexToScalarFaceIndex(
#endif
      const tarch::la::Vector<Dimensions + 1, int>& indexCell, const int direction, const int nodesPerAxis
    );
#if defined(GPUOffloadingOMP)
#pragma omp end declare target
#endif

/**
 * Map cell index to an index for the whole hull, i.e. all 2*Dimensios faces.
 *
 * @param[in] direction   coordinate direction of the (reference) element face normal (0:
 * @param[in] orientation orientation of the (reference) element face normal (0: negative, 1: positive).
 * @note Result must be scaled additionally by nodesPerAxis if it used to access space-time quantities.
 */
#if defined(GPUOffloadingOMP)
#pragma omp declare target
#endif
#if defined(GPUOffloadingCPP)
    int mapCellIndexToScalarHullIndex(
#else
    GPUCallableMethod int                                       mapCellIndexToScalarHullIndex(
#endif
      const tarch::la::Vector<Dimensions + 1, int>& indexCell,
      const int                                     direction,
      const int                                     orientation,
      const int                                     nodesPerAxis
    );
#if defined(GPUOffloadingOMP)
#pragma omp end declare target
#endif

/**
 * @param[in] direction   coordinate direction of the (reference) element face normal (0:
 * @param[in] orientation orientation of the (reference) element face normal (0: negative, 1: positive).
 * @return scalar cell index
 */
#if defined(GPUOffloadingOMP)
#pragma omp declare target
#endif
#if defined(GPUOffloadingCPP)
    int mapSpaceTimeFaceIndexToScalarCellIndex(
#else
    GPUCallableMethod int                                       mapSpaceTimeFaceIndexToScalarCellIndex(
#endif
      const tarch::la::Vector<Dimensions + 1, int>& indexFace, const int direction, const int id, const int nodesPerAxis
    );
#if defined(GPUOffloadingOMP)
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
#if defined(GPUOffloadingOMP)
#pragma omp declare target
#endif
#if defined(GPUOffloadingCPP)
    void gradient_AoS(
#else
    GPUCallableMethod void                                      gradient_AoS(
#endif
      const double* __restrict__ const QIn,
      const double* __restrict__ const dudx,
      const double invDx,
      const int    nodesPerAxis,
      const int    strideQ,
      const int    scalarIndex,
      double* __restrict__ gradQAux
    );
#if defined(GPUOffloadingOMP)
#pragma omp end declare target
#endif

/**
 * Map point in physical domain to corresponding coordinates in reference domain.
 *
 * @param[in] x          coordinate
 * @param[in] cellCentre centre of the cell that contains x
 * @param[in] dx         extent of the cell that contains x
 * @return
 */
#if defined(GPUOffloadingOMP)
#pragma omp declare target
#endif
#if defined(GPUOffloadingCPP)
    tarch::la::Vector<Dimensions, double> mapToReferenceCoordinates(
#else
    GPUCallableMethod tarch::la::Vector<Dimensions, double> mapToReferenceCoordinates(
#endif
      const tarch::la::Vector<Dimensions, double>& x,
      const tarch::la::Vector<Dimensions, double>& cellCentre,
      const double                                 dx
    );
#if defined(GPUOffloadingOMP)
#pragma omp end declare target
#endif

/**
 * Evalutes DG polynomial at arbitrary reference space coordinate
 * in reference domain [0,1]^d.
 *
 * @see: mapToReferenceCoordinates
 */
#if defined(GPUOffloadingOMP)
#pragma omp declare target
#endif
#if defined(GPUOffloadingCPP)
    void interpolate_AoS(
#else
    GPUCallableMethod void                                  interpolate_AoS(
#endif
      const double* __restrict__ const UIn,
      const double* __restrict__ const nodes,
      const double* __restrict__ const barycentricWeights,
      const tarch::la::Vector<Dimensions, double>& referenceCoodinates,
      const int                                    nodesPerAxis,
      const int                                    strideQ,
      double* __restrict__ pointwiseQOut
    );
#if defined(GPUOffloadingOMP)
#pragma omp end declare target
#endif
  } // namespace aderdg
} // namespace exahype2
