#include "tarch/la/Vector.h"

#include <functional>

namespace exahype2 {
  namespace aderdg {
    
    /**
     * @brief Modify the solution at the given coordinates and time.
     * 
     * Writes to a single spatial degree of freedom.
     *
     * @param[in] adjustSolution
     * @param[inout] UOut
     * @param[in] nodes
     * @param[in] cellCentre
     * @param[in] dx
     * @param[in] t
     * @param[in] dt
     * @param[in] nodesPerAxis
     * @param[in] unknowns
     * @param[in] strideQ
     * @param[in] scalarIndex
     */ 
     GPUCallableMethod void corrector_adjustSolution_body_AoS(
        std::function< void(
          double * __restrict__                       Q,
          const tarch::la::Vector<Dimensions,double>& x,
          double                                      t
        ) >                                         adjustSolution,
        double * __restrict__                       UOut,
        const double * __restrict__                 nodes,
        const tarch::la::Vector<Dimensions,double>& cellCentre,
        const double                                dx,
        const double                                t,
        const double                                dt,
        const int                                   nodesPerAxis,
        const int                                   unknowns,
        const int                                   strideQ,
        const int                                   scalarIndex);

    /**
     * @brief Add space-time volume flux contributions to the solution.
     * 
     * Writes to a single spatial degree of freedom.
     *
     * @note Directly run after Picard iterations in order to not store predictor.
     *
     * @param flux
     * @param UOut
     * @param QIn
     * @param FAux
     * @param nodes
     * @param weights
     * @param Kxi
     * @param cellCentre
     * @param dx
     * @param t
     * @param dt
     * @param nodesPerAxis
     * @param unknowns
     * @param strideQ
     * @param scalarIndex
     */
    #if defined(OpenMPGPUOffloading)
    #pragma omp declare target
    #endif
    GPUCallableMethod void corrector_addFluxContributions_body_AoS(
        std::function< void(
          const double * __restrict__                 Q,
          const tarch::la::Vector<Dimensions,double>& x,
          double                                      t,
          int                                         normal,
          double * __restrict__                       F
        ) >                                         flux,
        double* __restrict__                        UOut, 
        const double* __restrict__                  QIn,
        double* __restrict__                        FAux, // must be allocated per thread as size is runtime parameter
        const double* __restrict__                  nodes,
        const double* __restrict__                  weights,
        const double* __restrict__                  Kxi,
        const tarch::la::Vector<Dimensions,double>& cellCentre,
        const double                                dx,
        const double                                t,
        const double                                dt,
        const int                                   nodesPerAxis,
        const int                                   unknowns,
        const int                                   strideQ,
        const int                                   scalarIndex);
     #if defined(OpenMPGPUOffloading)
     #pragma omp end declare target
     #endif
    
    /**
     * @brief Add source contributions to the solution.
     *
     * Writes to a single spatial degree of freedom.
     *
     * @note Directly run after Picard iterations in order to not store predictor.
     *
     * @param algebraicSource
     * @param UOut
     * @param SAux
     * @param QIn
     * @param nodes
     * @param weights
     * @param cellCentre
     * @param dx
     * @param t
     * @param dt
     * @param nodesPerAxis
     * @param unknowns
     * @param strideQ
     * @param scalarIndex
     */
    #if defined(OpenMPGPUOffloading)
    #pragma omp declare target
    #endif
    GPUCallableMethod void corrector_addSourceContributions_body_AoS(
        std::function< void(
          const double * __restrict__                 Q,
          const tarch::la::Vector<Dimensions,double>& x,
          double                                      t,
          double * __restrict__                       S
        ) >                                         algebraicSource,
        double* __restrict__                        UOut,
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
        const int                                   scalarIndex);
     #if defined(OpenMPGPUOffloading)
     #pragma omp end declare target
     #endif
    
     /**
      * @brief Add nonconservative product contributions to the solution.
      *
      * Writes to a single spatial degree of freedom.
      *
      * @note Directly run after Picard iterations in order to not store predictor.
      *
      * @param nonconservativeProduct
      * @param UOut
      * @param QIn
      * @param gradQAux
      * @param SAux
      * @param nodes
      * @param weights
      * @param dudx
      * @param cellCentre
      * @param dx
      * @param t
      * @param dt
      * @param nodesPerAxis
      * @param unknowns
      * @param strideQ
      * @param scalarIndex
      */
     #if defined(OpenMPGPUOffloading)
     #pragma omp declare target
     #endif
     GPUCallableMethod void corrector_addNcpContributions_body_AoS(
       std::function< void(
         const double * __restrict__                 Q,
         double                                      gradQ[][Dimensions],
         const tarch::la::Vector<Dimensions,double>& x,
         double                                      t,
         double * __restrict__                       BgradQ
       ) >                                         nonconservativeProduct,
       double* __restrict__                        UOut,
       double* __restrict__                        gradQAux,
       double* __restrict__                        SAux,
       const double* __restrict__                  QIn,
       const double* __restrict__                  nodes,
       const double* __restrict__                  weights,
       const double* __restrict__                  dudx,
       const tarch::la::Vector<Dimensions,double>& cellCentre,
       const double                                dx,
       const double                                t,
       const double                                dt,
       const int                                   nodesPerAxis,
       const int                                   unknowns,
       const int                                   strideQ,
       const int                                   scalarIndex);
    #if defined(OpenMPGPUOffloading)
    #pragma omp end declare target
    #endif
    
    /**
     * @brief Add Riemann flux contributions to the solution.
     * 
     * Writes to a single spatial degree of freedom.
     *
     * @param UOut
     * @param riemannResultIn
     * @param weights
     * @param FCoeff
     * @param dx
     * @param nodesPerAxis
     * @param unknowns
     * @param strideQ
     * @param scalarIndex
     */
    #if defined(OpenMPGPUOffloading)
    #pragma omp declare target
    #endif
    GPUCallableMethod void corrector_addRiemannContributions_body_AoS(
       double * __restrict__       UOut,
       const double * __restrict__ riemannResultIn,
       const double * __restrict__ weights,
       const double * __restrict__ FCoeff[2],
       const double                dx,
       const double                dt,
       const int                   nodesPerAxis,
       const int                   unknowns,
       const int                   strideQ,
       const int                   strideRiemannResult,
       const int                   scalarIndexCell);
    #if defined(OpenMPGPUOffloading)
    #pragma omp end declare target
    #endif
    
    // CPU launchers
    /**
      * @brief Add cell-local contributions to new solution or update vector.
      *
      * @param[in] flux
      * @param[in] algebraicSource
      * @param[in] nonconservativeProduct
      * @param[inout] UOut
      * @param[in] QIn
      * @param[in] weights
      * @param[in] nodes
      * @param[in] Kxi
      * @param[in] dudx
      * @param[in] cellCentre
      * @param[in] dx
      * @param[in] t
      * @param[in] dt
      * @param[in] order
      * @param[in] unknowns
      * @param[in] auxiliaryVariables
      * @param[in] callFlux
      * @param[in] callSource
      * @param[in] callNonconservativeProduct
      */
    void corrector_addCellContributions_loop_AoS(
      std::function< void(
        const double * __restrict__                 Q,
        const tarch::la::Vector<Dimensions,double>& x,
        double                                      t,
        int                                         normal,
        double * __restrict__                       F
      ) >   flux,
      std::function< void(
        const double * __restrict__                 Q,
        const tarch::la::Vector<Dimensions,double>& x,
        double                                      t,
        double * __restrict__                       S
      ) >   algebraicSource,
      std::function< void(
        const double * __restrict__                 Q,
        double                                      gradQ[][Dimensions],
        const tarch::la::Vector<Dimensions,double>& x,
        double                                      t,
        double * __restrict__                       BgradQ
      ) >                                         nonconservativeProduct,
      std::function< void(
        double * __restrict__                       Q,
        const tarch::la::Vector<Dimensions,double>& x,
        double                                      t
      ) >                                         adjustSolution,
      double * __restrict__                       UOut, 
      const double * __restrict__                 QIn, 
      const double * __restrict__                 weights,
      const double * __restrict__                 nodes,
      const double * __restrict__                 Kxi,
      const double * __restrict__                 dudx, 
      const tarch::la::Vector<Dimensions,double>& cellCentre,
      const double                                dx,
      const double                                t,
      const double                                dt,
      const int                                   order,
      const int                                   unknowns,
      const int                                   auxiliaryVariables,
      const bool                                  callFlux,
      const bool                                  callSource,
      const bool                                  callNonconservativeProduct);
   
    /** 
     * @brief Add cell-local contributions to new solution or update vector.
     * 
     * @param[inout] UOut
     * @param[in] riemannResultIn
     * @param[in] weights
     * @param[in] FLCoeff
     * @param[in] FRCoeff
     * @param[in] dx
     * @param[in] dt
     * @param[in] order
     * @param[in] unknowns
     * @param[in] auxiliaryVariables
     */
    void corrector_addRiemannContributions_loop_AoS(
      double * __restrict__       UOut,
      const double * __restrict__ riemannResultIn,
      const double * __restrict__ weights,
      const double * __restrict__ FLCoeff,
      const double * __restrict__ FRCoeff,
      const double                dx,
      const double                dt,
      const int                   order,
      const int                   unknowns,
      const int                   auxiliaryVariables);
  
  }
}
