#include "Generic.h"
#include "tarch/la/Vector.h"

namespace exahype2 {
  namespace aderdg {
    /**
     * @name STP Picard loop kernel bodies 
     */
    //@{
    /**
     * \brief Initial guess for Q before STP Picard loop. 
     * 
     * This body writes to a single space-time volume coefficient.  
     *
     * @param[in] Qout
     * @param[in] UIn
     * @param[in] nodesPerAxis
     * @param[in] unknowns
     * @param[in] auxiliaryVariables
     */
    GPUCallableMethod void spaceTimePredictor_initialGuess_body_AoS(
      double*       __restrict__ Qout,
      const double* __restrict__ UIn,
      const int                  nodesPerAxis,
      const int                  strideQ,
      const int                  scalarIndex);
      for (int var = 0; var < strideQ; var++) {
        Qout[ scalarIndex*strideQ + var ] = UIn[ ( scalarIndex / nodesPerAxis )*strideQ + var ];
      }
    }
    
    /**
     * \brief Initialise RHS of Picard iterations.
     * 
     * Initialise the right-hand of the equation system that is inverted in every step of the 
     * predictor's Picard loop.
     *
     * This body writes to a single space-time volume coefficient.  
     *
     * @param rhsOut
     * @param UIn
     * @param FLCoeff
     * @param nodesPerAxis
     * @param strideQ
     */
    GPUCallableMethod void spaceTimePredictor_PicardLoop_initialiseRhs_AoS(
      double* __restrict__       rhsOut,
      const double* __restrict__ UIn,
      const double* __restrict__ FLCoeff,
      const int                  nodesPerAxis,
      const int                  strideQ,
      const int                  scalarIndex);
    
    /**
     * \brief Add flux contributions to RHS during STP Picard loop.
     * 
     * This body writes to a single space-time volume coefficient.  
     *
     * @param[in] flux
     * @param[in] rhsOut
     * @param[in] FAux
     * @param[in] QIn
     * @param[in] nodes
     * @param[in] weights
     * @param[in] Kxi
     * @param[in] t
     * @param[in] dt
     * @param[in] centre
     * @param[in] invDx
     * @param[in] unknowns
     * @param[in] nodesPerAxis
     * @param[in] strideQ
     * @param[in] strideRhs
     * @param[in] scalarIndex
    */
    GPUCallableMethod void spaceTimePredictor_PicardLoop_addFluxContributionsToRhs_body_AoS (
        std::function< void(
          double * __restrict__                       Q,
          const tarch::la::Vector<Dimensions,double>& x,
          double                                      t,
          int                                         normal,
        ) >   flux,
        double* __restrict__                          rhsOut, 
        double* __restrict__                          FAux, 
        const double* __restrict__                    nodes,
        const double* __restrict__                    weights,
        const double* __restrict__                    Kxi,
        const double                                  t,
        const double                                  dt,
        const tarch::la::Vector<Dimensions+1,double>& centre,
        const double                                  invDx,
        const int                                     nodesPerAxis,
        const int                                     unknowns,
        const int                                     strideQ,
        const int                                     strideRhs,
        const int                                     scalarIndex);
    
    /**
     * \brief Add source contributions to RHS during STP Picard loop.
     * 
     * This body writes to a single space-time volume coefficient.  
     * 
     * @param[in] algebraicSource
     * @param[in] rhsOut
     * @param[in] SAux
     * @param[in] QIn
     * @param[in] nodes
     * @param[in] weights
     * @param[in] cellCentre
     * @param[in] dx
     * @param[in] t
     * @param[in] dt
     * @param[in] unknowns
     * @param[in] nodesPerAxis
     * @param[in] strideQ
     * @param[in] strideRhs
     * @param[in] scalarIndex
     */
    GPUCallableMethod void spaceTimePredictor_PicardLoop_addSourceContributionToRhs_body_AoS(
        std::function< void(
          double * __restrict__                       Q,
          const tarch::la::Vector<Dimensions,double>& x,
          double                                      t,
        ) >   algebraicSource,
        double* __restrict__                       rhsOut,
        double* __restrict__                       SAux,
        const double* __restrict__                 QIn,
        const double* __restrict__                 nodes,
        const double* __restrict__                 weights,
        const tarch::la::Vector<Dimensions,double> cellCentre,
        const double                               dx,
        const double                               t,
        const double                               dt,
        const int                                  nodesPerAxis,
        const int                                  unknowns,
        const int                                  strideQ,
        const int                                  strideRhs,
        const int                                  scalarIndex);
    
    /**
     * 
     */
    GPUCallableMethod void spaceTimePredictor_PicardLoop_addNcpContributionToRhs_body_AoS(
      std::function< void(
        double * __restrict__                       Q,
        double                                      gradQ[][Dimensions],
        const tarch::la::Vector<Dimensions,double>& x,
        double                                      t,
      ) >                                         nonconservativeProduct,
      double* __restrict__                        rhsOut,
      double* __restrict__                        gradQAux,
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
      const int                                   strideRhs,
      const int                                   strideQ,
      const int                                   scalarIndex);
    
    /**
     * Invert the Picard space-time system to compute the next solution.  
     *
     * @note Unlike the kernel bodies that write to the RHS, 
     * this kernel cannot be fused with the other space-time predictor Picard loop kernel bodies due to the neighbour lookups
     * of flux and gradient.
     * 
     * @param QOut
     * @param squaredResiduumOut
     * @param rhsIn
     * @param iK1
     * @param nodesPerAxis
     * @param unknowns
     * @param strideQ
     */
    GPUCallableMethod void spaceTimePredictor_PicardLoop_invert_body_AoS(
      double * __restrict__       QOut,
      double&                     squaredResiduumOut,
      const double * __restrict__ rhsIn,
      const double * __restrict__ iK1,
      const int                   nodesPerAxis,
      const int                   unknowns,
      const int                   strideQ);
    
    //@}

    /**
     * Extrapolate the predictor onto DoF associated with the 
     * faces of a cell (cell hull). These serve as Riemann
     * solver inputs.
     * 
     * @param QHullOut
     * @param QIn
     * @param FCoeff
     * @param invDx
     * @param nodesPerAxis
     * @param unknowns
     * @param strideQ
     * @param scalarIndexHull
     */
    GPUCallableMethod void spaceTimePredictor_extrapolate_body_AoS(
      double * __restrict__       QHullOut,
      const double * __restrict__ QIn,
      const double * __restrict__ FCoeff[2],
      const double                invDx,
      const int                   nodesPerAxis,
      const int                   unknowns,
      const int                   strideQ,
      const int                   scalarIndexHull);

  } // aderdg
} // exahype2
