# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import exahype2.solvers.ErrorMeasurement
from exahype2.solvers.ErrorMeasurement import (
    create_finish_time_step_implementation_for_error_measurement,
)

from exahype2.solvers.PDETerms import PDETerms

from exahype2.solvers.aderdg.GlobalFixedTimeStep import GlobalFixedTimeStep
from exahype2.solvers.aderdg.GlobalAdaptiveTimeStep import GlobalAdaptiveTimeStep

from exahype2.solvers.aderdg.actionsets import EmptyPostprocessSolution


class ErrorMeasurement(exahype2.solvers.ErrorMeasurement):
    def __init__(
        self,
        solver,
        error_measurement_implementation=PDETerms.User_Defined_Implementation,
        output_file_name="Error",
        time_delta_between_snapshots=0.0,
        measured_unknowns=None,
    ):
        super(ErrorMeasurement, self).__init__(
            solver=solver,
            error_measurement_implementation=error_measurement_implementation,
            output_file_name=output_file_name,
            time_delta_between_snapshots=time_delta_between_snapshots,
            measured_unknowns=measured_unknowns,
        )

        solver._finish_time_step_implementation += create_finish_time_step_implementation_for_error_measurement(
            guard="_solverState == SolverState::GridInitialisation or _solverState == SolverState::TimeStep"
        )

        if solver._action_set_postprocess_solution == None:
            solver._action_set_postprocess_solution = EmptyPostprocessSolution(
                solver=solver
            )

        if isinstance(solver, GlobalAdaptiveTimeStep):
            solver._action_set_postprocess_solution._cell_compute_kernel += create_postprocessing_kernel_for_error_measurement_for_adaptive_timestepping(
                solver._provide_cell_data_to_compute_kernels_default_guard()
            )
        elif isinstance(solver, GlobalFixedTimeStep):
            solver._action_set_postprocess_solution._cell_compute_kernel += create_postprocessing_kernel_for_error_measurement_for_fixed_timestepping(
                solver._provide_cell_data_to_compute_kernels_default_guard()
            )

        solver._action_set_postprocess_solution.descend_invocation_order = 3


def _create_postprocessing_kernel_for_error_measurement(guard, time_step_size_accessor):
    """!

    Shared ADER-DG error-measurement kernel. Fixed and adaptive time stepping
    differ only in the accessor for the time step size ("getTimeStepSize()" vs
    "getAdmissibleTimeStepSize()").
    """
    return (
        """
    if ((
      repositories::{{SOLVER_INSTANCE}}.getSolverState() == {{SOLVER_NAME}}::SolverState::GridInitialisation
      or
      repositories::{{SOLVER_INSTANCE}}.isFirstGridSweepOfTimeStep())
      and """
        + guard
        + """
    ) {
      double errors[3]{};

      kernels::{{SOLVER_NAME}}::cellErrorIntegral(
        [&](
          const tarch::la::Vector<DIMENSIONS, double>&  x,
          const double                                  t,
          const double                                  dt,
          const {{SOLUTION_STORAGE_PRECISION}}*         Q_,
          double*                                       solution
        ) -> void {
          double Q[{{NUMBER_OF_UNKNOWNS}} + {{NUMBER_OF_AUXILIARY_VARIABLES}}];
          for(int i=0; i<{{NUMBER_OF_UNKNOWNS}} + {{NUMBER_OF_AUXILIARY_VARIABLES}}; i++){
            Q[i] = (double) Q_[i];
          }
          repositories::{{SOLVER_INSTANCE}}.analyticalSolution(x, t, dt, Q, solution);
        },
        marker.getX(),
        marker.getH(),
        fineGridCell{{SOLVER_NAME}}CellLabel.getTimeStamp(),
        repositories::{{SOLVER_INSTANCE}}."""
        + time_step_size_accessor
        + """,
        fineGridCell{{UNKNOWN_IDENTIFIER}}_old.data(),
        errors
      );

      repositories::{{SOLVER_INSTANCE}}.setError(errors);
    }
"""
    )


def create_postprocessing_kernel_for_error_measurement_for_fixed_timestepping(guard):
    return _create_postprocessing_kernel_for_error_measurement(
        guard, "getTimeStepSize()"
    )


def create_postprocessing_kernel_for_error_measurement_for_adaptive_timestepping(guard):
    return _create_postprocessing_kernel_for_error_measurement(
        guard, "getAdmissibleTimeStepSize()"
    )
