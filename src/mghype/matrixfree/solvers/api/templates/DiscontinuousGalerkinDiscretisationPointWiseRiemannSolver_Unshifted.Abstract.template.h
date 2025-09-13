{% include 'DiscontinuousGalerkinDiscretisationPointWiseRiemannSolver.Abstract.generic.template.h' %}

  public:
    enum class State {
      GridConstruction,
      InitSolution,
      Suspend,
      ProjectOntoFaces,
      ComputeNumericalFlux,
      UpdateCell,
      /**
       * Used by multigrid algorithms in the last DG step to determine the
       * residual to restrict.
       */
      ComputeResidualAndSuspend
    };

    State getState() const;

    void beginInitialisationSweep();
    void beginGridConstructionSweep();

    /**
     * If this is called after the initialisation (to dump the original guess),
     * then we have to switch the solver state to ensure that all data exchange
     * along domain boundaries is correct.
     */
    void beginPlotSweep();

  protected:
    State _state;
};

