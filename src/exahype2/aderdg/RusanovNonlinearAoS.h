#ifndef _EXAHYPE2_ADERDG_RUSANOV_NONLINEAR_AOS_H_
#define _EXAHYPE2_ADERDG_RUSANOV_NONLINEAR_AOS_H_

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
    void rusanovNonlinear_setBoundaryState_body_AoS(
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
    GPUCallableMethod double rusanovNonlinear_maxAbsoluteEigenvalue_body_AoS(
      std::function< double(
        const double * __restrict__                 Q,
        const tarch::la::Vector<Dimensions,double>& x,
        double                                      t,
        const int                                   direction
      ) >                                         maxAbsoluteEigenvalue,
      const double * __restrict__                 QLR[2],
      const double * __restrict__                 nodes,
      const double                                t,
      const double                                dt,
      const tarch::la::Vector<Dimensions,double>& faceCentre,
      const double                                dx,
      const int                                   nodesPerAxis,
      const int                                   unknowns,
      const int                                   strideQ,
      const int                                   direction,
      const int                                   scalarIndexFace);
    #if defined(OpenMPGPUOffloading)
    #pragma omp end declare target
    #endif
    
    /** 
     * compute fluxes (and fluctuations for non-conservative PDEs)
     * @note Zero out FLOut/FROut arrays before.
     */
    #if defined(OpenMPGPUOffloading)
    #pragma omp declare target
    #endif
    GPUCallableMethod void rusanovNonlinear_riemannFlux_body_AoS(
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
        double * __restrict__                       FLOut,
        double * __restrict__                       FROut,
        double * __restrict__                       FLAux,
        double * __restrict__                       FRAux,
        const double * __restrict__                 QLIn, 
        const double * __restrict__                 QRIn, 
        const double                                smax,
        const double * __restrict__                 nodes, 
        const double * __restrict__                 weights, 
        const double                                t,
        const double                                dt,
        const tarch::la::Vector<Dimensions,double>& faceCentre,
        const double                                dx,
        const int                                   nodesPerAxis,
        const int                                   unknowns,
        const int                                   strideQ,
        const int                                   strideF,
        const int                                   direction,
        const int                                   scalarIndexFace);
    #if defined(OpenMPGPUOffloading)
    #pragma omp end declare target
    #endif
    
     /** 
      * @brief Add nonconservative product contributions to the (possibly trivial)  Riemann flux.
      *
      * @param[in] nonconservativeProduct
      * @param[inout] FLOut
      * @param[inout] FROut
      * @param[in] QAvgAux
      * @param[in] gradQAux
      * @param[in] SAux
      * @param[in] QLIn
      * @param[in] QRIn
      * @param[in] smax
      * @param[in] nodes quadrature nodes; size: (order+1)
      * @param[in] weights quadrature weights; size: (order+1)
      * @param[in] t time stamp
      * @param[in] dt time step size
      * @param[in] faceCentre
      * @param[in] dx cell spacing (we assume the same spacing in all coordinate directions)
      * @param[in] nodesPerAxis nodes/Lagrange basis functions per coordinate axis (order+1)
      * @param[in] unknowns the number of PDE unknowns that we evolve
      * @param[in] strideQ
      * @param[in] strideF
      * @param[in] direction
      * @param[in] scalarIndexFace
      */
     #if defined(OpenMPGPUOffloading)
     #pragma omp declare target
     #endif
     GPUCallableMethod void rusanovNonlinear_addNcpContributionsToRiemannFlux_body_AoS(
        std::function< void(
          double * __restrict__                       Q,
          double                                      gradQ[][Dimensions],
          const tarch::la::Vector<Dimensions,double>& x,
          double                                      t,
          int                                         normal,
          double * __restrict__                       BgradQ
        ) >                                         nonconservativeProduct,
        double * __restrict__                       FLOut,
        double * __restrict__                       FROut,
        double * __restrict__                       QAvgAux,
        double * __restrict__                       gradQAux,
        double * __restrict__                       SAux,
        const double * __restrict__                 QLIn, 
        const double * __restrict__                 QRIn, 
        const double                                smax,
        const double * __restrict__                 nodes, 
        const double * __restrict__                 weights, 
        const double                                t,
        const double                                dt,
        const tarch::la::Vector<Dimensions,double>& faceCentre,
        const double                                dx,
        const int                                   nodesPerAxis,
        const int                                   unknowns,
        const int                                   strideQ,
        const int                                   strideF,
        const int                                   direction,
        const int                                   scalarIndexFace);
     #if defined(OpenMPGPUOffloading)
     #pragma omp end declare target
     #endif
   
     // launchers

     /**
      * Prescribe boundary values (QOut) and overwrite interior values (QInside). 
      *
      * @param[in] boundaryState
      * @param[inout] QOutside
      * @param[inout] QInside
      * @param[in] nodes quadrature nodes; size: (order+1)
      * @param[in] t time stamp
      * @param[in] dt time step size
      * @param[in] faceCentre
      * @param[in] dx cell spacing (we assume the same spacing in all coordinate directions)
      * @param[in] order the DG approximation order, which corresponds to order+1 DG nodes/Lagrange basis functions per coordinate axis
      * @param[in] unknowns the number of PDE unknowns that we evolve
      * @param[in] auxiliaryVariables other quantities such as material parameters that we do not evolve
      * @param[in] direction
      */
     void rusanovNonlinear_setBoundaryState_loop_AoS(
       std::function< void(
         double * __restrict__                       QInside,
         double * __restrict__                       OOutside,
         const tarch::la::Vector<Dimensions,double>& x,
         double                                      t,
         int                                         normal
       ) >                                         boundaryState,
       double * __restrict__                       QOutside,
       double * __restrict__                       QInside,
       const double * __restrict__                 nodes,
       const double                                t,
       const double                                dt,
       const tarch::la::Vector<Dimensions,double>& faceCentre,
       const double                                dx,
       const int                                   order,
       const int                                   unknowns,
       const int                                   auxiliaryVariables,
       const int                                   direction);
    
    /**
     * @brief Determine the eigenvalue with the largest absolute value among the predictor
     * that two neighbouring cells extrapolated to a common interface.
     * @return maxAbsoluteEigenvalue
     * @param[in] QL
     * @param[in] QR
     * @param[in] nodes quadrature nodes; size: (order+1)
     * @param[in] t time stamp
     * @param[in] dt time step size
     * @param[in] faceCentre
     * @param[in] dx cell spacing (we assume the same spacing in all coordinate directions)
     * @param[in] order the DG approximation order, which corresponds to order+1 DG nodes/Lagrange basis functions per coordinate axis
     * @param[in] unknowns the number of PDE unknowns that we evolve
     * @param[in] auxiliaryVariables other quantities such as material parameters that we do not evolve
     * @param[in] direction
     */
     double rusanovNonlinear_maxAbsoluteEigenvalue_loop_AoS(
        std::function< double(
          const double * __restrict__                 Q,
          const tarch::la::Vector<Dimensions,double>& x,
          double                                      t,
          const int                                   direction
        ) >                                         maxAbsoluteEigenvalue,
        const double * __restrict__                 QL,
        const double * __restrict__                 QR,
        const double * __restrict__                 nodes,
        const double                                t,
        const double                                dt,
        const tarch::la::Vector<Dimensions,double>& faceCentre,
        const double                                dx,
        const int                                   order,
        const int                                   unknowns,
        const int                                   auxiliaryVariables,
        const int                                   direction);

    /**
     * @brief Apply a Rusanov Riemann solver to the pair of extrapolated predictor
     * values from two neighbouring cells.  
     *
     * @param[inout] FLOut
     * @param[inout] FROut
     * @param[in] QLIn
     * @param[in] QRIn
     * @param[in] smax
     * @param[in] nodes quadrature nodes; size: (order+1)
     * @param[in] weights quadrature weights; size: (order+1)
     * @param[in] t time stamp
     * @param[in] dt time step size
     * @param[in] faceCentre
     * @param[in] dx cell spacing (we assume the same spacing in all coordinate directions)
     * @param[in] order the DG approximation order, which corresponds to order+1 DG nodes/Lagrange basis functions per coordinate axis
     * @param[in] unknowns the number of PDE unknowns that we evolve
     * @param[in] auxiliaryVariables other quantities such as material parameters that we do not evolve
     * @param[in] direction
     * @param[in] leftCellIsOutside
     * @param[in] rightCellIsOutside
     * @param[in] callFlux
     * @param[in] callNonconservativeProduct
     */
    void rusanovNonlinear_loop_AoS(
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
        double                                      gradQ[][Dimensions],
        const tarch::la::Vector<Dimensions,double>& x,
        double                                      t,
        int                                         normal,
        double * __restrict__                       BgradQ
      ) >                                         nonconservativeProduct,
      double * __restrict__                       FLOut,
      double * __restrict__                       FROut,
      const double * __restrict__                 QLIn, 
      const double * __restrict__                 QRIn, 
      const double                                smax,
      const double * __restrict__                 nodes, 
      const double * __restrict__                 weights, 
      const double                                t,
      const double                                dt,
      const tarch::la::Vector<Dimensions,double>& faceCentre,
      const double                                dx,
      const int                                   order,
      const int                                   unknowns,
      const int                                   auxiliaryVariables,
      const int                                   direction,
      const bool                                  leftCellIsOutside,
      const bool                                  rightCellIsOutside,
      const bool                                  callFlux,
      const bool                                  callNonconservativeProduct);

  }
}

#endif
