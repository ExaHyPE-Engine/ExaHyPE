#ifndef _EXAHYPE2_ADERDG_PREDICTOR_AOS_H_
#define _EXAHYPE2_ADERDG_PREDICTOR_AOS_H_

#include "tarch/la/Vector.h"

#include <functional>

namespace exahype2 {
  namespace aderdg {
    /**
     * @name STP Picard loop kernel bodies 
     */
    //@{
    /**
     * @brief Initial guess for Q before STP Picard loop. 
     * 
     * This body writes to a single space-time volume coefficient.  
     *
     * @param[in] Qout
     * @param[in] UIn
     * @param[in] nodesPerAxis nodes/Lagrange basis functions per coordinate axis (order+1)
     * @param[in] unknowns the number of PDE unknowns that we evolve
     */
    #if defined(OpenMPGPUOffloading)
    #pragma omp declare target
    #endif
    GPUCallableMethod void spaceTimePredictor_initialGuess_body_AoS(
      double*       __restrict__ Qout,
      const double* __restrict__ UIn,
      const int                  nodesPerAxis,
      const int                  strideQ,
      const int                  scalarIndex);
    #if defined(OpenMPGPUOffloading)
    #pragma omp end declare target
    #endif
    
    /**
     * @brief Initialise RHS of Picard iterations.
     * 
     * Initialise the right-hand of the equation system that is inverted in every step of the 
     * predictor's Picard loop.
     *
     * This body writes to a single space-time volume coefficient.  
     *
     * @param[inout] rhsOut
     * @param[in] UIn
     * @param[in] FLCoeff values of basis functions evaluated at x=0.0 (left); size: order+1
     * @param[in] nodesPerAxis nodes/Lagrange basis functions per coordinate axis (order+1)
     * @param[in] strideQ
     */
    #if defined(OpenMPGPUOffloading)
    #pragma omp declare target
    #endif
    GPUCallableMethod void spaceTimePredictor_PicardLoop_initialiseRhs_AoS(
      double * __restrict__       rhsOut,
      const double * const __restrict__ UIn,
      const double * const __restrict__ FLCoeff,
      const int                  nodesPerAxis,
      const int                  strideQ,
      const int                  strideRhs,
      const int                  scalarIndex);
    #if defined(OpenMPGPUOffloading)
    #pragma omp end declare target
    #endif
    
    /**
     * @brief Add flux contributions to RHS during STP Picard loop.
     * 
     * This body writes to a single space-time volume coefficient.  
     *
     * @param[in] flux
     * @param[in] rhsOut
     * @param[in] FAux
     * @param[in] QIn
     * @param[in] nodes quadrature nodes; size: (order+1)
     * @param[in] weights quadrature weights; size: (order+1)
     * @param[in] Kxi stiffness matrix; size: (order+1)*(order+1)
     * @param[in] t time stamp
     * @param[in] dt time step size
     * @param[in] centre
     * @param[in] dx
     * @param[in] unknowns the number of PDE unknowns that we evolve
     * @param[in] nodesPerAxis nodes/Lagrange basis functions per coordinate axis (order+1)
     * @param[in] strideQ
     * @param[in] strideRhs
     * @param[in] scalarIndex
    */
    #if defined(OpenMPGPUOffloading)
    #pragma omp declare target
    #endif
    GPUCallableMethod void spaceTimePredictor_PicardLoop_addFluxContributionsToRhs_body_AoS (
        std::function< void(
          const double * const __restrict__                 Q,
          const tarch::la::Vector<Dimensions,double>& x,
          double                                      t,
          int                                         normal,
          double * __restrict__                       F
        ) >   flux,
        double* __restrict__                          rhsOut, 
        double* __restrict__                          FAux, 
        const double* __restrict__                    QIn, 
        const double* __restrict__                    nodes,
        const double* __restrict__                    weights,
        const double* __restrict__                    Kxi,
        const tarch::la::Vector<Dimensions,double>&   cellCentre,
        const double                                  dx,
        const double                                  t,
        const double                                  dt,
        const int                                     nodesPerAxis,
        const int                                     unknowns,
        const int                                     strideQ,
        const int                                     strideRhs,
        const int                                     scalarIndex);
    #if defined(OpenMPGPUOffloading)
    #pragma omp end declare target
    #endif
    
    /**
     * @brief Add source contributions to RHS during STP Picard loop.
     * 
     * This body writes to a single space-time volume coefficient.  
     * 
     * @param[in] algebraicSource
     * @param[in] rhsOut
     * @param[in] SAux
     * @param[in] QIn
     * @param[in] nodes quadrature nodes; size: (order+1)
     * @param[in] weights quadrature weights; size: (order+1)
     * @param[in] cellCentre
     * @param[in] dx cell spacing (we assume the same spacing in all coordinate directions)
     * @param[in] t time stamp
     * @param[in] dt time step size
     * @param[in] unknowns the number of PDE unknowns that we evolve
     * @param[in] nodesPerAxis nodes/Lagrange basis functions per coordinate axis (order+1)
     * @param[in] strideQ
     * @param[in] strideRhs
     * @param[in] scalarIndex
     */
    #if defined(OpenMPGPUOffloading)
    #pragma omp declare target
    #endif
    GPUCallableMethod void spaceTimePredictor_PicardLoop_addSourceContributionToRhs_body_AoS(
        std::function< void(
          const double * const __restrict__                 Q,
          const tarch::la::Vector<Dimensions,double>& x,
          double                                      t,
          double * __restrict__                       S
        ) >   algebraicSource,
        double* __restrict__                        rhsOut,
        double* __restrict__                        SAux,
        const double* __restrict__                  QIn,
        const double* __restrict__                  nodes,
        const double* __restrict__                  weights,
        const tarch::la::Vector<Dimensions,double>& cellCentre,
        const double                                dx,
        const double                                t,
        const double                                dt,
        const int                                   nodesPerAxis,
        const int                                   unknowns,
        const int                                   strideQ,
        const int                                   strideRhs,
        const int                                   scalarIndex);
    #if defined(OpenMPGPUOffloading)
    #pragma omp end declare target
    #endif
    
    /**
     * 
     */
    #if defined(OpenMPGPUOffloading)
    #pragma omp declare target
    #endif
    GPUCallableMethod void spaceTimePredictor_PicardLoop_addNcpContributionToRhs_body_AoS(
      std::function< void(
        const double * const __restrict__                 Q,
        double * __restrict__                       dQ_or_deltaQ,
        const tarch::la::Vector<Dimensions,double>& x,
        double                                      t,
        int                                         normal,
        double * __restrict__                       BgradQ
      ) >                                         nonconservativeProduct,
      double * __restrict__                       rhsOut,
      double * __restrict__                       gradQAux,
      double * __restrict__                       SAux,
      const double * const __restrict__                 QIn,
      const double * const __restrict__                 nodes,
      const double * const __restrict__                 weights,
      const double * const __restrict__                 dudx, 
      const tarch::la::Vector<Dimensions,double>& cellCentre,
      const double                                dx,
      const double                                t,
      const double                                dt,
      const int                                   nodesPerAxis,
      const int                                   unknowns,
      const int                                   strideQ,
      const int                                   strideRhs,
      const int                                   scalarIndex);
    #if defined(OpenMPGPUOffloading)
    #pragma omp end declare target
    #endif
    
    /**
     * Invert the Picard space-time system to compute the next solution.  
     *
     * @note Unlike the kernel bodies that write to the RHS, 
     * this kernel cannot be fused with the other space-time predictor Picard loop kernel bodies due to the neighbour lookups
     * of flux and gradient.
     * 
     * @param[inout] QOut
     * @param[inout] squaredResiduumOut
     * @param[in] rhsIn
     * @param[in] iK1 inverse of the predictor left-hand side operator; size: (order+1)*(order+1)
     * @param[in] nodesPerAxis nodes/Lagrange basis functions per coordinate axis (order+1)
     * @param[in] unknowns the number of PDE unknowns that we evolve
     * @param[in] strideQ
     */
    #if defined(OpenMPGPUOffloading)
    #pragma omp declare target
    #endif
    GPUCallableMethod void spaceTimePredictor_PicardLoop_invert_body_AoS(
      double * __restrict__       QOut,
      double&                     squaredResiduumOut,
      const double * const __restrict__ rhsIn,
      const double * const __restrict__ iK1,
      const int                   nodesPerAxis,
      const int                   unknowns,
      const int                   strideQ,
      const int                   strideRhs,
      const int                   scalarIndex);
    #if defined(OpenMPGPUOffloading)
    #pragma omp end declare target
    #endif
    
    //@}

    /**
     * Extrapolate the predictor onto DoF associated with the 
     * faces of a cell (cell hull). These serve as Riemann
     * solver inputs.
     * 
     * @param[inout] QHullOut
     * @param[in] QIn
     * @param[in] FLRCoeff Basis functions evaluated at reference coordinates 0.0 (L,component 0) and 1.0 (R, component 1).
     * @param[in] nodesPerAxis nodes/Lagrange basis functions per coordinate axis (order+1)
     * @param[in] strideQLR
     * @param[in] strideQ
     * @param[in] scalarIndexHull
     */
    #if defined(OpenMPGPUOffloading)
    #pragma omp declare target
    #endif
    GPUCallableMethod void spaceTimePredictor_extrapolate_body_AoS(
      double * __restrict__       QHullOut[Dimensions*2],
      const double * const __restrict__ QIn,
      const double * const __restrict__ FLRCoeff[2],
      const int                   nodesPerAxis,
      const int                   strideQLR,
      const int                   strideQ,
      const int                   scalarIndexHull);
    #if defined(OpenMPGPUOffloading)
    #pragma omp end declare target
    #endif
    
    /**
     * Extrapolate the predictor onto DoF associated with the 
     * faces of a cell (cell hull). These serve as Riemann
     * solver inputs.
     * 
     * Simplified version of spaceTimePredictor_extrapolate_body_AoS specifically for Lobatto nodes.
     * 
     * @param[inout] QHullOut
     * @param[in] QIn
     * @param[in] nodesPerAxis nodes/Lagrange basis functions per coordinate axis (order+1)
     * @param[in] strideQLR
     * @param[in] strideQ
     * @param[in] scalarIndexHull
     */
    #if defined(OpenMPGPUOffloading)
    #pragma omp declare target
    #endif
    GPUCallableMethod void spaceTimePredictor_extrapolate_Lobatto_body_AoS(
      double * __restrict__             QHullOut[Dimensions*2],
      const double * const __restrict__ QIn,
      const int                         nodesPerAxis,
      const int                         strideQLR,
      const int                         strideQ,
      const int                         scalarIndexHull);
    #if defined(OpenMPGPUOffloading)
    #pragma omp end declare target
    #endif
    
    /** 
     * @brief Extrapolates the predictor to time t+dt. 
     * 
     * @param[inout] UOut
     * @param[in] QIn
     * @param[in] FRCoeff values of basis functions evaluated at x=1.0 (right); size: order+1
     * @param[in] nodesPerAxis nodes/Lagrange basis functions per coordinate axis (order+1)
     * @param[in] unknowns the number of PDE unknowns that we evolve
     * @param[in] strideQ
     * @param[in] scalarIndexHull
     */
    #if defined(OpenMPGPUOffloading)
    #pragma omp declare target
    #endif
    void spaceTimePredictor_extrapolateInTime_body_AoS(
      double * __restrict__             UOut,
      const double * const __restrict__ QIn,
      const double * const __restrict__ FRCoeff,
      const double                      nodesPerAxis,
      const int                         strideQ,
      const int                         scalarIndexCell);
    #if defined(OpenMPGPUOffloading)
    #pragma omp end declare target
    #endif
    
    /** 
     * @brief Extrapolates the predictor to time t+dt. 
     *
     * Simplified version of spaceTimePredictor_extrapolate_body_AoS specifically for Lobatto nodes.
     * 
     * @param[inout] UOut
     * @param[in] QIn
     * @param[in] FRCoeff values of basis functions evaluated at x=1.0 (right); size: order+1
     * @param[in] nodesPerAxis nodes/Lagrange basis functions per coordinate axis (order+1)
     * @param[in] unknowns the number of PDE unknowns that we evolve
     * @param[in] strideQ
     * @param[in] scalarIndexHull
     */
    #if defined(OpenMPGPUOffloading)
    #pragma omp declare target
    #endif
    void spaceTimePredictor_extrapolateInTime_Lobatto_body_AoS(
      double * __restrict__             UOut,
      const double * const __restrict__ QIn,
      const double * const __restrict__ FRCoeff,
      const double                      nodesPerAxis,
      const int                         strideQ,
      const int                         scalarIndexCell);
    #if defined(OpenMPGPUOffloading)
    #pragma omp end declare target
    #endif

    /**
     * @brief Compute the space-time predictor (Qout) from the current solution (UIn). 
     * 
     * @param[in] flux
     * @param[in] algebraicSource
     * @param[in] nonconservativeProduct
     * @param[inout] QOut
     * @param[in] UIn
     * @param[in] weights quadrature weights; size: (order+1)
     * @param[in] nodes quadrature nodes; size: (order+1)
     * @param[in] Kxi stiffness matrix; size: (order+1)*(order+1)
     * @param[in] iK1 inverse of the predictor left-hand side operator; size: (order+1)*(order+1)
     * @param[in] FLCoeff values of basis functions evaluated at x=0.0 (left); size: order+1
     * @param[in] dudx derivative operator; size: (order+1)*(order+1)
     * @param[in] dx cell spacing (we assume the same spacing in all coordinate directions)
     * @param[in] cellCentre
     * @param[in] t time stamp
     * @param[in] dt time step size
     * @param[in] order the DG approximation order, which corresponds to order+1 DG nodes/Lagrange basis functions per coordinate axis
     * @param[in] unknowns the number of PDE unknowns that we evolve
     * @param[in] auxiliaryVariables other quantities such as material parameters that we do not evolve
     * @param[in] atol
     * @param[in] callFlux call the flux function
     * @param[in] callSource call the algebraicSource function
     * @param[in] callNonconservativeProduct call the nonconservativeProduct
     */ 
    void spaceTimePredictor_PicardLoop_loop_AoS(
      std::function< void(
        const double * const __restrict__                 Q,
        const tarch::la::Vector<Dimensions,double>& x,
        double                                      t,
        int                                         normal,
        double * __restrict__                       F
      ) >   flux,
      std::function< void(
        const double * const __restrict__                 Q,
        const tarch::la::Vector<Dimensions,double>& x,
        double                                      t,
        double * __restrict__                       S
      ) >   algebraicSource,
      std::function< void(
        const double * const __restrict__                 Q,
        const double * const __restrict__                 dQ_or_deltaQ,
        const tarch::la::Vector<Dimensions,double>& x,
        double                                      t,
        int                                         normal,
        double * __restrict__                       BgradQ
      ) >                                         nonconservativeProduct,
      double * __restrict__                       QOut, 
      const double * const __restrict__                 UIn, 
      const double * const __restrict__                 nodes,
      const double * const __restrict__                 weights,
      const double * const __restrict__                 Kxi,
      const double * const __restrict__                 iK1,
      const double * const __restrict__                 FLCoeff,
      const double * const __restrict__                 dudx, 
      const tarch::la::Vector<Dimensions,double>& cellCentre,
      const double                                dx,
      const double                                t,
      const double                                dt,
      const int                                   order,
      const int                                   unknowns,
      const int                                   auxiliaryVariables,
      const double                                atol,
      const bool                                  callFlux,
      const bool                                  callSource,
      const bool                                  callNonconservativeProduct);

    /**
     * @brief Extrapolate all of cell's predictor DOF to the hull of the element.
     *
     * @note This version works for Gauss-Legendre and Gauss-Lobatto nodes.
     *
     * @param[inout] QHullOut
     * @param[in] QIn
     * @param[in] FLCoeff values of basis functions evaluated at x=0.0 (left); size: order+1
     * @param[in] FRCoeff values of basis functions evaluated at x=1.0 (right); size: order+1
     * @param[in] order the DG approximation order, which corresponds to order+1 DG nodes/Lagrange basis functions per coordinate axis
     * @param[in] unknowns the number of PDE unknowns that we evolve
     * @param[in] auxiliaryVariables other quantities such as material parameters that we do not evolve
     */ 
    void spaceTimePredictor_extrapolate_loop_AoS(
        double * __restrict__       QHullOut[Dimensions*2],
        const double * const __restrict__ QIn,
        const double * const __restrict__ FLCoeff,
        const double * const __restrict__ FRCoeff,
        const int                   order,
        const int                   unknowns,
        const int                   auxiliaryVariables);
    
    /**
     * @brief Extrapolate all of cell's predictor DOF to the hull of the element.
     *
     * @note assumes that Gauss-Lobatto nodes are used as support points for the basis functions.
     *       In this case, the outermost nodes are located directly at the boundary of 
     *       the reference element. Boundary-extrapolation thus becomes a simple copy of some predictor coefficients.
     * @note FLCoeff,FRCoeff are arguments in order to have the same signature as
     *       the generic routine. They are not actually needed.
     * @see spaceTimePredictor_extrapolate_loop_AoS, spaceTimePredictor_extrapolate_body_AoS, spaceTimePredictor_extrapolate_Lobatto_body_AoS
     *
     * @param[inout] QHullOut
     * @param[in] QIn
     * @param[in] FLCoeff values of basis functions evaluated at x=0.0 (left); size: order+1
     * @param[in] FRCoeff values of basis functions evaluated at x=1.0 (right); size: order+1
     * @param[in] order the DG approximation order, which corresponds to order+1 DG nodes/Lagrange basis functions per coordinate axis
     * @param[in] unknowns the number of PDE unknowns that we evolve
     * @param[in] auxiliaryVariables other quantities such as material parameters that we do not evolve
     */ 
    void spaceTimePredictor_extrapolate_Lobatto_loop_AoS(
        double * __restrict__       QHullOut[Dimensions*2],
        const double * const __restrict__ QIn,
        const double * const __restrict__ FLCoeff,
        const double * const __restrict__ FRCoeff,
        const int                   order,
        const int                   unknowns,
        const int                   auxiliaryVariables);
    
    /** 
     * @brief Extrapolates the predictor to t+dt. 
     *
     * The output variable QIn will be completely overwritten. QIn is the space-time
     * polynomial.
     * 
     * @param[inout] UOut
     * @param[in] QIn
     * @param[in] FRCoeff values of basis functions evaluated at x=1.0 (right); size: order+1
     * @param[in] dt time step size
     * @param[in] order the DG approximation order, which corresponds to order+1 DG nodes/Lagrange basis functions per coordinate axis
     * @param[in] unknowns the number of PDE unknowns that we evolve
     * @param[in] auxiliaryVariables other quantities such as material parameters that we do not evolve
     */
    void spaceTimePredictor_extrapolateInTime_loop_AoS(
      double * __restrict__             UOut,
      const double * const __restrict__ QIn,
      const double * const __restrict__ FRCoeff,
      const int                         order,
      const int                         unknowns,
      const int                         auxiliaryVariables);
    
    /** 
     * @brief Extrapolates the predictor to t+dt. 
     * 
     * Simplified variant for Lobatto nodes.
     * 
     * @note FRCoeff is passed as argument in order to have the same signature as
     *       the generic routine. This argument is not actually needed.
     * 
     * @param[inout] UOut
     * @param[in] QIn
     * @param[in] FRCoeff values of basis functions evaluated at x=1.0 (right); size: order+1
     * @param[in] dt time step size
     * @param[in] order the DG approximation order, which corresponds to order+1 DG nodes/Lagrange basis functions per coordinate axis
     * @param[in] unknowns the number of PDE unknowns that we evolve
     * @param[in] auxiliaryVariables other quantities such as material parameters that we do not evolve
     */
    void spaceTimePredictor_extrapolateInTime_Lobatto_loop_AoS(
      double * __restrict__             UOut,
      const double * const __restrict__ QIn,
      const double * const __restrict__ FRCoeff,
      const int                         order,
      const int                         unknowns,
      const int                         auxiliaryVariables);

  } // aderdg
} // exahype2

#endif
