#include "tarch/la/Vector.h"

#include <functional>

namespace exahype2 {
  namespace aderdg {

    /** 
     * Determines the maximum absolute value among the eigenvalues computed 
     * at a space-time quadrature point on a face.
     *
     * @note: Different to the ExaHyPE1 implementation, we compute the
     * maximum of the absolute values of the eigenvalues using
     * all space-time quadrature points.
     * @param maxAbsoluteEigenvalue
     * @param smax
     * @param QLR,
     * @param t
     * @param dt
     * @param faceCentre
     * @param dx
     * @param nodesPerAxis
     * @param unknowns
     * @param strideQ
     * @param direction
     * @param scalarIndexFace
     */
    #if defined(OpenMPGPUOffloading)
    #pragma omp declare target
    #endif
    GPUCallableMethod void rusanovNonlinear_maxAbsoluteEigenvalue_body_AoS(
        std::function< double(
          double * __restrict__                        Q,
          const tarch::la::Vector<Dimensions,double>&  x,
          double                                       t
        ) >                                         maxAbsoluteEigenvalue,
        double&                                     smax,
        const double * __restrict__                 QLR[2],
        const double * __restrict__                 nodes,
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
     * compute fluxes (and fluctuations for non-conservative PDEs)
     * @note Zero out FLOut/FROut arrays before.
     */
    #if defined(OpenMPGPUOffloading)
    #pragma omp declare target
    #endif
    GPUCallableMethod void rusanovNonlinear_riemannFlux_body_AoS(
        std::function< void(
          double * __restrict__                        Q,
          const tarch::la::Vector<Dimensions,double>&  faceCentre,
          double                                       t,
          double                                       dt,
          int                                          normal
        ) >                                         flux,
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
      * compute fluxes (and fluctuations for non-conservative PDEs)
      * @note Zero out gradient and FLOut, FROut or call flux function before.
      * @todo also offer an option with a only the flux
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

  }
}
