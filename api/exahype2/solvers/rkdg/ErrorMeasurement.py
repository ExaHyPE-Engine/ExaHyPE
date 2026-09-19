# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import exahype2.solvers.ErrorMeasurement
from exahype2.solvers.ErrorMeasurement import (
    create_finish_time_step_implementation_for_error_measurement,
)

from exahype2.solvers.PDETerms import PDETerms

from exahype2.solvers.rkdg.GlobalFixedTimeStep import GlobalFixedTimeStep
from exahype2.solvers.rkdg.GlobalAdaptiveTimeStep import GlobalAdaptiveTimeStep

from exahype2.solvers.rkdg.actionsets import (
    EmptyPostprocessSolution,
    CellWisePostprocessSolution,
)


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

        if solver._action_set_postprocess_solution is None or isinstance(
            solver._action_set_postprocess_solution, EmptyPostprocessSolution
        ):
            solver._action_set_postprocess_solution = CellWisePostprocessSolution(
                solver=solver
            )

        if isinstance(solver, GlobalAdaptiveTimeStep):
            solver._action_set_postprocess_solution.add_postprocessing_kernel(
                create_postprocessing_kernel_for_error_measurement_for_adaptive_timestepping(
                    solver._provide_cell_data_to_compute_kernels_default_guard()
                )
            )
        elif isinstance(solver, GlobalFixedTimeStep):
            solver._action_set_postprocess_solution.add_postprocessing_kernel(
                create_postprocessing_kernel_for_error_measurement_for_fixed_timestepping(
                    solver._provide_cell_data_to_compute_kernels_default_guard()
                )
            )


def _create_postprocessing_kernel_for_error_measurement(guard, time_step_size_accessor):
    """!

    Returns the RKDG error-measurement kernel. `time_step_size_accessor` names the solver
    method the kernel calls to read the time step size.
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
      double errors[3]{0.};

#if DIMENSIONS==2
      const double cellVolume = marker.getH()[0] * marker.getH()[1];
#else
      const double cellVolume = marker.getH()[0] * marker.getH()[1] * marker.getH()[2];
#endif

      double solution[{{NUMBER_OF_UNKNOWNS}}];
      int linearisedIndex = 0;

      peano4::dForEach({{DG_ORDER}} + 1, [&](const tarch::la::Vector<DIMENSIONS, int>& index) {
#if DIMENSIONS==2
        const double weight = cellVolume * kernels::{{SOLVER_NAME}}::Quadrature::QuadratureWeights1D[index[0]] * kernels::{{SOLVER_NAME}}::Quadrature::QuadratureWeights1D[index[1]];
#else
        const double weight = cellVolume * kernels::{{SOLVER_NAME}}::Quadrature::QuadratureWeights1D[index[0]] * kernels::{{SOLVER_NAME}}::Quadrature::QuadratureWeights1D[index[1]] * kernels::{{SOLVER_NAME}}::Quadrature::QuadratureWeights1D[index[2]];
#endif

        repositories::{{SOLVER_INSTANCE}}.analyticalSolution(
          ::exahype2::dg::getQuadraturePoint(
            marker.getX(),
            marker.getH(),
            index,
            kernels::{{SOLVER_NAME}}::Quadrature::QuadraturePoints1D
          ),
          fineGridCell{{SOLVER_NAME}}CellLabel.getTimeStamp(),
          repositories::{{SOLVER_INSTANCE}}."""
        + time_step_size_accessor
        + """,
          &fineGridCell{{UNKNOWN_IDENTIFIER}}.data()[linearisedIndex],
          solution
        );

        constexpr int MeasuredUnknowns[] = {{MEASURED_UNKNOWNS}};
        for (const int unknown : MeasuredUnknowns) {
          const double error = std::abs(fineGridCell{{UNKNOWN_IDENTIFIER}}.data()[linearisedIndex + unknown] - solution[unknown]);
          errors[0] += weight * error; // L1 norm
          errors[1] += weight * error * error; // L2 norm
          errors[2] = std::max(errors[2], error); // Linf norm
        }

        linearisedIndex += {{NUMBER_OF_UNKNOWNS}} + {{NUMBER_OF_AUXILIARY_VARIABLES}};
      });
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
