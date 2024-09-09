# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import exahype2.solvers.ErrorMeasurement
from exahype2.solvers.ErrorMeasurement import (
    create_finish_time_step_implementation_for_error_measurement,
)

from exahype2.solvers.PDETerms import PDETerms

from exahype2.solvers.aderdg.actionsets import EmptyPostprocessSolution


class ErrorMeasurement(exahype2.solvers.ErrorMeasurement):
    def __init__(
        self,
        solver,
        error_measurement_implementation=PDETerms.User_Defined_Implementation,
        delta_between_database_flushes=0,
        output_file_name="ADERDGErrorMeasurement",
        data_delta_between_snapshots="1e+16",
        time_delta_between_snapshots=0.0,
        clear_database_after_flush=True,
    ):
        super(ErrorMeasurement, self).__init__(
            solver=solver,
            error_measurement_implementation=error_measurement_implementation,
            delta_between_database_flushes=delta_between_database_flushes,
            output_file_name=output_file_name,
            data_delta_between_snapshots=data_delta_between_snapshots,
            time_delta_between_snapshots=time_delta_between_snapshots,
            clear_database_after_flush=clear_database_after_flush,
        )

        solver._finish_time_step_implementation += create_finish_time_step_implementation_for_error_measurement(
            guard="_solverState == SolverState::GridInitialisation or _solverState == SolverState::Timestep"
        )

        solver._action_set_postprocess_solution = EmptyPostprocessSolution(
            solver=solver
        )
        solver._action_set_postprocess_solution._compute_kernel += (
            create_postprocessing_kernel_for_error_measurement()
        )
        solver._action_set_postprocess_solution.descend_invocation_order = 3


def create_postprocessing_kernel_for_error_measurement():
    return """
    if ((repositories::{{SOLVER_INSTANCE}}.getSolverState() == {{SOLVER_NAME}}::SolverState::GridInitialisation
      or repositories::{{SOLVER_INSTANCE}}.isLastGridSweepOfTimeStep())
      and not marker.willBeRefined() and not marker.hasBeenRefined()
    ) {
      double errors[3]{};

      kernels::{{SOLVER_NAME}}::cellErrorIntegral(
        [&](
          const tarch::la::Vector<Dimensions, double>&  x,
          const double                                  t,
          const double                                  dt,
          const {{SOLUTION_STORAGE_PRECISION}}*         Q_,
          double*                                       solution
        )->void {
          double Q[{{NUMBER_OF_UNKNOWNS}} + {{NUMBER_OF_AUXILIARY_VARIABLES}}];
          std::copy_n(Q_, {{NUMBER_OF_UNKNOWNS}} + {{NUMBER_OF_AUXILIARY_VARIABLES}}, Q);
          repositories::{{SOLVER_INSTANCE}}.analyticalSolution(x, t, dt, Q, solution);
        },
        marker.x(),
        marker.h(),
        fineGridCell{{SOLVER_NAME}}CellLabel.getTimeStamp(),
        repositories::{{SOLVER_INSTANCE}}.getAdmissibleTimeStepSize(),
        fineGridCell{{UNKNOWN_IDENTIFIER}}_old.value,
        errors
      );

      repositories::{{SOLVER_INSTANCE}}.setError(errors);
    }
"""
