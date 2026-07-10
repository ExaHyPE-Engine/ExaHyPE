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
            guard="_solverState == SolverState::GridInitialisation or isLastGridSweepOfTimeStep()"
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

    #if DIMENSIONS==2
    const double cellVolume = h[0] * h[1];
    #else
    const double cellVolume = h[0] * h[1] * h[2];
    #endif

    repositories::{{SOLVER_INSTANCE}}.analyticalSolution(
      x,
      timeStamp,
      timeStepSize,
      value,
      solution
    );

    constexpr int MeasuredUnknowns[] = {{MEASURED_UNKNOWNS}};
    for (const int unknown : MeasuredUnknowns) {
      const double error = std::abs(value[unknown] - solution[unknown]);
      errors[0] += cellVolume * error; // L1 norm
      errors[1] += cellVolume * error * error; // L2 norm
      errors[2] = std::max(errors[2], error); // Linf norm
    }

    repositories::{{SOLVER_INSTANCE}}.setError(errors);
"""
