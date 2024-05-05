# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org

from exahype2.solvers.PDETerms import PDETerms
from exahype2.solvers.aderdg.actionsets import EmptyPostprocessSolution

from .ErrorMeasurement import ErrorMeasurement

from .kernels import (
    create_postprocessing_kernel_for_ader_error_measurement,
    create_finish_time_step_implementation_for_error_measurement
)


class ADERErrorMeasurement(ErrorMeasurement):
    """!
    insert description here
    """

    def __init__(
      self,
      solver,
      error_measurement_implementation=PDETerms.User_Defined_Implementation,
      deltaBetweenDatabaseFlushes = 0,
      outputFileName = "errorMeasurement",
      dataDeltaBetweenSnapshots = "1e+16",
      timeDeltaBetweenSnapshots = 0.,
      clearDatabaseAfterFlush = True
    ):
        super(ADERErrorMeasurement, self).__init__(
          solver,
          error_measurement_implementation,
          deltaBetweenDatabaseFlushes,
          outputFileName,
          dataDeltaBetweenSnapshots,
          timeDeltaBetweenSnapshots,
          clearDatabaseAfterFlush
        )

        solver._finish_time_step_implementation += create_finish_time_step_implementation_for_error_measurement(
          guard = "_solverState==SolverState::GridInitialisation or _solverState==SolverState::Correction"
        )

        solver._action_set_postprocess_solution = (
            EmptyPostprocessSolution(solver=solver)
        )
        solver._action_set_postprocess_solution._compute_kernel += create_postprocessing_kernel_for_ader_error_measurement()
        solver._action_set_postprocess_solution.descend_invocation_order = 3

