#ifndef _EXAHYPE2_ADERDG_RUSANOV_NONLINEAR_AOS_H_
#define _EXAHYPE2_ADERDG_RUSANOV_NONLINEAR_AOS_H_

#include "tarch/la/Vector.h"

#include <functional>

namespace exahype2 {
  namespace aderdg {
    
    #if defined(OpenMPGPUOffloading)
    #pragma omp declare target
    #endif
    GPUCallableMethod void rusanovNonlinear_body_AoS(
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
        std::function< void(
          double * __restrict__                       Q,
          double * __restrict__                       dQ_or_deltaQ,
          const tarch::la::Vector<Dimensions,double>& x,
          double                                      t,
          int                                         normal,
          double * __restrict__                       BgradQ
        ) >                                         nonconservativeProduct,
        double * __restrict__                       riemannResultOut,
        double * __restrict__                       FLAux,
        double * __restrict__                       FRAux,
        double * __restrict__                       QAvgAux,
        double * __restrict__                       dQAux,
        double * __restrict__                       SAux,
        const double * __restrict__                 QLIn, 
        const double * __restrict__                 QRIn, 
        const double                                smax,
        const double * __restrict__                 nodes, 
        const double * __restrict__                 weights, 
        const tarch::la::Vector<Dimensions,double>& faceCentre,
        const double                                dx,
        const double                                t,
        const double                                dt,
        const int                                   nodesPerAxis,
        const int                                   unknowns,
        const int                                   strideQ,
        const int                                   strideF,
        const int                                   direction,
        const bool                                  orientationToCell,
        const bool                                  callFlux,
        const bool                                  callNonconservativeProduct,
        const int                                   scalarIndexFace);
   
    // launchers
    /**
     * @brief Apply a Rusanov Riemann solver to the pair of extrapolated predictor
     * values from two neighbouring cells.  
     * @param[in]    flux,
     * @param[in]    boundaryFlux,
     * @param[in]    nonconservativeProduct,
     * @param[in]    boundaryNonconservativeProduct,
     * @param[inout] FLOut
     * @param[inout] FROut
     * @param[in]    QHullIn, 
     * @param[in]    maxEigenvaluePerFace,
     * @param[in]    smax
     * @param[in]    nodes quadrature nodes; size: (order+1)
     * @param[in]    weights quadrature weights; size: (order+1)
     * @param[in]    t time stamp
     * @param[in]    dt time step size
     * @param[in]    faceCentre
     * @param[in]    dx cell spacing (we assume the same spacing in all coordinate directions)
     * @param[in]    order the DG approximation order, which corresponds to order+1 DG nodes/Lagrange basis functions per coordinate axis
     * @param[in]    unknowns the number of PDE unknowns that we evolve
     * @param[in]    auxiliaryVariables other quantities such as material parameters that we do not evolve
     * @param[in]    direction
     * @param[in]    callFlux
     * @param[in]    callNonconservativeProduct
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
        double * __restrict__                       dQ_or_deltaQ,
        const tarch::la::Vector<Dimensions,double>& x,
        double                                      t,
        int                                         normal,
        double * __restrict__                       BgradQ
      ) >                                         nonconservativeProduct,
      std::function< void(
        double * __restrict__                       Q,
        double * __restrict__                       dQ_or_deltaQ,
        const tarch::la::Vector<Dimensions,double>& x,
        double                                      t,
        int                                         normal,
        double * __restrict__                       BgradQ
      ) >                                         boundaryNonconservativeProduct,
      double * __restrict__                       riemannResultOut,
      const double * __restrict__                 QHullIn[Dimensions*2], 
      const double                                maxEigenvaluePerFace[Dimensions*2],
      const double * __restrict__                 nodes, 
      const double * __restrict__                 weights, 
      const tarch::la::Vector<Dimensions,double>& cellCentre,
      const double                                dx,
      const double                                t,
      const double                                dt,
      const int                                   order,
      const int                                   unknowns,
      const int                                   auxiliaryVariables,
      const bool                                  atBoundary[Dimensions*2],
      const bool                                  callFlux,
      const bool                                  callNonconservativeProduct);

  }
}

#endif
