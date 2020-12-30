namespace exahype2 {
  namespace aderdg {

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
   * @param centre
   * @param dx
   * @param t
   * @param dt
   * @param nodesPerAxis
   * @param unknowns
   * @param strideQ
   * @param linearisedIndex
   */
  GPUCallableMethod void corrector_addFluxContributions_body_AoS(
      std::function< void(
        double * __restrict__                       Q,
        const tarch::la::Vector<Dimensions,double>& x,
        double                                      t,
        int                                         normal,
      ) >                        flux,
      double* __restrict__       UOut, 
      const double* __restrict__ QIn,
      const double* __restrict__ nodes,
      const double* __restrict__ weights,
      const double* __restrict__ Kxi,
      const double               centre,
      const double               dx,
      const double               t,
      const double               dt,
      const int                  nodesPerAxis,
      const int                  unknowns,
      const int                  strideQ,
      const int                  linearisedIndex);
  
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
   * @param linearisedIndex
   */
  GPUCallableMethod void corrector_addSourceContribution_body_AoS(
      std::function< void(
        double * __restrict__                       Q,
        const tarch::la::Vector<Dimensions,double>& x,
        double                                      t,
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
      const int                                   linearisedIndex);
  
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
   * @param linearisedIndex
   */
  GPUCallableMethod void corrector_addNcpContribution_body_AoS(
      std::function< void(
        double * __restrict__                       Q,
        double                                      gradQ[][Dimensions],
        const tarch::la::Vector<Dimensions,double>& x,
        double                                      t,
      ) >                                         nonconservativeProduct,
      double* __restrict__                        UOut,
      const double* __restrict__                  QIn,
      const double* __restrict__                  gradQAux,
      const double* __restrict__                  SAux,
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
      const int                                   linearisedIndex);
   
   /**
    * @brief Add Riemann flux contributions to the solution.
    * 
    * Writes to a single spatial degree of freedom.
    *
    * @param UOut
    * @param riemannResultIn
    * @param weights
    * @param FCoeff
    * @param invDx
    * @param nodesPerAxis
    * @param unknowns
    * @param strideQ
    * @param linearisedIndex
    */
   GPUCallableMethod void corrector_addRiemannContributions_body_AoS(
       double * __restrict__       UOut,
       const double * __restrict__ riemannResultIn,
       const double * __restrict__ weights,
       const double * __restrict__ FCoeff[2],
       const double                invDx,
       const int                   nodesPerAxis,
       const int                   unknowns,
       const int                   strideQ,
       const int                   linearisedIndex):
  }
}
