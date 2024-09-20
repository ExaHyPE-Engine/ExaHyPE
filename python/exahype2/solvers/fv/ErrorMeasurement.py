# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import exahype2.solvers.ErrorMeasurement
from exahype2.solvers.ErrorMeasurement import (
    create_finish_time_step_implementation_for_error_measurement,
)

from exahype2.solvers.PDETerms import PDETerms

from exahype2.solvers.fv.actionsets import VolumeWisePostprocessSolution


class ErrorMeasurement(exahype2.solvers.ErrorMeasurement):
    def __init__(
        self,
        solver,
        error_measurement_implementation=PDETerms.User_Defined_Implementation,
        delta_between_database_flushes=0,
        output_file_name="FVErrorMeasurement",
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
            guard="_solverState == SolverState::GridInitialisation or (_solverState == SolverState::TimeStep and isLastGridSweepOfTimeStep())"
        )

        solver._action_set_postprocess_solution = VolumeWisePostprocessSolution(solver)
        solver._action_set_postprocess_solution.guard = """
            (repositories::{{SOLVER_INSTANCE}}.getSolverState() == {{SOLVER_NAME}}::SolverState::GridInitialisation or
             repositories::{{SOLVER_INSTANCE}}.isLastGridSweepOfTimeStep())"""
        solver._action_set_postprocess_solution.add_postprocessing_kernel(
            create_postprocessing_kernel_for_error_measurement()
        )


def create_postprocessing_kernel_for_error_measurement():
    return """
    double errors[3] = {0.0, 0.0, 0.0};
    double solution[{{NUMBER_OF_UNKNOWNS}}];

    #if Dimensions==2
    const double cellVolume = volumeH[0] * volumeH[1];
    #else
    const double cellVolume = volumeH[0] * volumeH[1] * volumeH[2];
    #endif

    repositories::{{SOLVER_INSTANCE}}.analyticalSolution(
      volumeX,
      timeStamp,
      timeStepSize,
      value,
      solution
    );

    for (int unknown = 0; unknown < {{NUMBER_OF_UNKNOWNS}}; unknown++) {
      double error = std::abs(value[unknown] - solution[unknown]);
      errors[0] += cellVolume * error; // L1 norm
      errors[1] += cellVolume * error * error; // L2 norm
      errors[2] = std::max(errors[2], error); // L_inf norm
    }

    repositories::{{SOLVER_INSTANCE}}.setError(errors);
"""
