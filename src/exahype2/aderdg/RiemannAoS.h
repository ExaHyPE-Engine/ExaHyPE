#ifndef _EXAHYPE2_ADERDG_RIEMANN_AOS_H_
#define _EXAHYPE2_ADERDG_RIEMANN_AOS_H_

/**
 * Common routines required for a variety of Riemann solvers.
 */

#include "tarch/la/Vector.h"

#include <functional>

namespace exahype2 {
  namespace aderdg {
      
    /** 
     * @brief Set boundary state at given coordinates and time.
     *
     * @param[in] boundaryState
     * @param[inout] QOut
     * @param[inout] QIn
     * @param[in] nodes quadrature nodes; size: (order+1)
     * @param[in] t time stamp
     * @param[in] dt time step size
     * @param[in] faceCentre
     * @param[in] dx cell spacing (we assume the same spacing in all coordinate directions)
     * @param[in] nodesPerAxis nodes/Lagrange basis functions per coordinate axis (order+1)
     * @param[in] unknowns the number of PDE unknowns that we evolve
     * @param[in] strideQ
     * @param[in] direction
     * @param[in] orientation
     * @param[in] scalarIndexFace
     */
    #if defined(OpenMPGPUOffloading)
    #pragma omp declare target
    #endif
    void riemann_setBoundaryState_body_AoS(
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
      const tarch::la::Vector<Dimensions,double>& faceCentre,
      const double                                dx,
      const double                                t,
      const double                                dt,
      const int                                   nodesPerAxis,
      const int                                   unknowns,
      const int                                   strideQ,
      const int                                   direction,
      const int                                   scalarIndexFace);
    #if defined(OpenMPGPUOffloading)
    #pragma omp end declare target
    #endif

    /** 
     * Determines the maximum absolute value among the eigenvalues computed 
     * at a space-time quadrature point on a face.
     *
     * @note: Different to the ExaHyPE1 implementation, we compute the
     * maximum of the absolute values of the eigenvalues using
     * all space-time quadrature points.
     *
     * @return the largest absolute eigenvalue
     *
     * @param[in] maxAbsoluteEigenvalue
     * @param[in] QLR,
     * @param[in] t time stamp
     * @param[in] dt time step size
     * @param[in] faceCentre
     * @param[in] dx cell spacing (we assume the same spacing in all coordinate directions)
     * @param[in] nodesPerAxis nodes/Lagrange basis functions per coordinate axis (order+1)
     * @param[in] unknowns the number of PDE unknowns that we evolve
     * @param[in] strideQ
     * @param[in] direction
     * @param[in] scalarIndexFace
     */
    #if defined(OpenMPGPUOffloading)
    #pragma omp declare target
    #endif
    GPUCallableMethod double riemann_maxAbsoluteEigenvalue_body_AoS(
    std::function< double(
      const double * __restrict__                 Q,
      const tarch::la::Vector<Dimensions,double>& x,
      double                                      t,
      const int                                   normal
    ) >                                         maxAbsoluteEigenvalue,
    const double * __restrict__                 QLR,
    const double * __restrict__                 nodes,
    const tarch::la::Vector<Dimensions,double>& faceCentre,
    const double                                dx,
    const double                                t,
    const double                                dt,
    const int                                   nodesPerAxis,
    const int                                   unknowns,
    const int                                   strideQ,
    const int                                   direction,
    const int                                   scalarIndexFace);
    #if defined(OpenMPGPUOffloading)
    #pragma omp end declare target
    #endif

     /**
      * Prescribe boundary values (QOut) and overwrite interior values (QInside). 
      *
      * @param[in] boundaryState
      * @param[inout] QHullOut
      * @param[in] nodes quadrature nodes; size: (order+1)
      * @param[in] t time stamp
      * @param[in] dt time step size
      * @param[in] faceCentre
      * @param[in] dx cell spacing (we assume the same spacing in all coordinate directions)
      * @param[in] order the DG approximation order, which corresponds to order+1 DG nodes/Lagrange basis functions per coordinate axis
      * @param[in] unknowns the number of PDE unknowns that we evolve
      * @param[in] auxiliaryVariables other quantities such as material parameters that we do not evolve
      * @param[in] atBoundary
      */
     void riemann_setBoundaryState_loop_AoS(
       std::function< void(
         const double * __restrict__                 QInside,
         double * __restrict__                       OOutside,
         const tarch::la::Vector<Dimensions,double>& x,
         double                                      t,
         int                                         normal
       ) >                                         boundaryState,
       double * __restrict__                       QHullOut[Dimensions*2],
       const double * __restrict__                 nodes,
       const tarch::la::Vector<Dimensions,double>& cellCentre,
       const double                                dx,
       const double                                t,
       const double                                dt,
       const int                                   order,
       const int                                   unknowns,
       const int                                   auxiliaryVariables,
       const bool                                  atBoundary[Dimensions*2]);
    
    /**
     * @brief Per face, determine the eigenvalue with the largest absolute value
     * among the boundary-extrapolated predictor values of the 
     * current cell and its face neighbour(s).
     *
     * @return maxAbsoluteEigenvalue
     * @param[inout] maxEigenvaluePerFaceOut
     * @param[in]    QHullIn
     * @param[in]    nodes quadrature nodes; size: (order+1)
     * @param[in]    t time stamp
     * @param[in]    dt time step size
     * @param[in]    faceCentre
     * @param[in]    dx cell spacing (we assume the same spacing in all coordinate directions)
     * @param[in]    order the DG approximation order, which corresponds to order+1 DG nodes/Lagrange basis functions per coordinate axis
     * @param[in]    unknowns the number of PDE unknowns that we evolve
     * @param[in]    auxiliaryVariables other quantities such as material parameters that we do not evolve
     * @param[in]    direction
     */
    void riemann_maxAbsoluteEigenvalue_loop_AoS(
      std::function< double(
        const double * __restrict__                 Q,
        const tarch::la::Vector<Dimensions,double>& x,
        double                                      t,
        const int                                   direction
      ) >                                         maxAbsoluteEigenvalue,
      double                                      maxEigenvaluePerFaceOut[Dimensions*2],
      const double * __restrict__                 QHullIn[Dimensions*2],
      const double * __restrict__                 nodes,
      const tarch::la::Vector<Dimensions,double>& cellCentre,
      const double                                dx,
      const double                                t,
      const double                                dt,
      const int                                   order,
      const int                                   unknowns,
      const int                                   auxiliaryVariables);
  }
}

#endif
