{% include 'DiscontinuousGalerkinDiscretisationPointWiseRiemannSolver.Abstract.generic.template.h' %}

  protected:
    enum class State {
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

    State _state;
};

