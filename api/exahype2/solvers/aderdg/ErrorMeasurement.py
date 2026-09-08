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
        super().__init__(
            solver=solver,
            error_measurement_implementation=error_measurement_implementation,
            output_file_name=output_file_name,
            time_delta_between_snapshots=time_delta_between_snapshots,
            measured_unknowns=measured_unknowns,
        )

        # A limiting step closes at either an accepted correction or subcell
        # recomputation, so both paths reduce the errors at the common completion point.
        solver._finish_time_step_implementation += create_finish_time_step_implementation_for_error_measurement(
            guard=(
                "{% if SUBCELL_LIMITER %}"
                "getSolverState() == SolverState::GridInitialisation"
                " or isLastGridSweepOfTimeStep()"
                "{% else %}"
                "_solverState == SolverState::GridInitialisation or _solverState == SolverState::TimeStep"
                "{% endif %}"
            )
        )

        if solver._action_set_postprocess_solution is None:
            solver._action_set_postprocess_solution = EmptyPostprocessSolution(
                solver=solver
            )

        if solver.subcell_limiter_is_enabled:
            solver.add_user_action_set_includes(
                """
#include "exahype2/dg/LimiterStatus.h"
#include "exahype2/fv/PatchUtils.h"
"""
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
    """Build an ADER-DG error kernel with the selected time-step accessor.

    A limiter-capable solver measures the representation that owns the accepted
    state: DG nodes on regular/halo cells and finite-volume subcells on cells
    carried by the subcell scheme.
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

{% if SUBCELL_LIMITER %}
      const bool measureSubcellSolution =
        repositories::{{SOLVER_INSTANCE}}.getLimitingSolverState()
          != ::exahype2::dg::LimitingSolverState::GridInitialisation
        and ::exahype2::dg::runsSubcellScheme(
          fineGridCell{{SOLVER_NAME}}CellLabel.getLimiterStatus()
        );
      if (measureSubcellSolution) {
        constexpr int NumberOfSubcellsPerAxis = 2 * {{ORDER}} + 1;
        constexpr int NumberOfData =
          {{NUMBER_OF_UNKNOWNS}} + {{NUMBER_OF_AUXILIARY_VARIABLES}};

        double subcellVolume = 1.0;
        for (int dimension = 0; dimension < DIMENSIONS; dimension++) {
          subcellVolume *= marker.getH()[dimension] / NumberOfSubcellsPerAxis;
        }

        ::peano4::dForEach(
          NumberOfSubcellsPerAxis,
          [&](const tarch::la::Vector<DIMENSIONS, int>& subcellIndex) {
            const auto subcellCentre =
              ::exahype2::fv::getVolumeSizeAndCentre<
                DIMENSIONS,
                NumberOfSubcellsPerAxis
              >(marker.getX(), marker.getH(), subcellIndex).volumeCentre;
            const auto* const numericalSolution =
              fineGridCell{{SOLVER_NAME}}SubcellOld.data()
              + ::peano4::dLinearised(subcellIndex, NumberOfSubcellsPerAxis)
                * NumberOfData;
            double solutionValues[NumberOfData];
            double analyticalSolution[{{NUMBER_OF_UNKNOWNS}}];
            std::copy_n(numericalSolution, NumberOfData, solutionValues);
            repositories::{{SOLVER_INSTANCE}}.analyticalSolution(
              subcellCentre,
              fineGridCell{{SOLVER_NAME}}CellLabel.getTimeStamp(),
              repositories::{{SOLVER_INSTANCE}}."""
        + time_step_size_accessor
        + """,
              solutionValues,
              analyticalSolution
            );

            constexpr int MeasuredUnknowns[] = {{MEASURED_UNKNOWNS}};
            for (const int unknown : MeasuredUnknowns) {
              const double error = std::abs(
                static_cast<double>(numericalSolution[unknown])
                - analyticalSolution[unknown]
              );
              errors[0] += subcellVolume * error;
              errors[1] += subcellVolume * error * error;
              errors[2] = std::max(errors[2], error);
            }
          }
        );
      } else {
{% endif %}
      kernels::{{SOLVER_NAME}}::cellErrorIntegral(
        [&](
          const tarch::la::Vector<DIMENSIONS, double>& position,
          const double                                 evaluationTimeStamp,
          const double                                 evaluationTimeStepSize,
          const {{SOLUTION_STORAGE_PRECISION}}* const  numericalSolution,
          double* const                                analyticalSolution
        ) -> void {
          double solutionValues[{{NUMBER_OF_UNKNOWNS}} + {{NUMBER_OF_AUXILIARY_VARIABLES}}];
          for(int i=0; i<{{NUMBER_OF_UNKNOWNS}} + {{NUMBER_OF_AUXILIARY_VARIABLES}}; i++){
            solutionValues[i] = (double) numericalSolution[i];
          }
          repositories::{{SOLVER_INSTANCE}}.analyticalSolution(
            position,
            evaluationTimeStamp,
            evaluationTimeStepSize,
            solutionValues,
            analyticalSolution
          );
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
{% if SUBCELL_LIMITER %}
      }
{% endif %}

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
