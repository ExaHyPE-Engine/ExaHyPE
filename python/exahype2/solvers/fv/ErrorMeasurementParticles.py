# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
from exahype2.solvers.ErrorMeasurementParticles import (
    ErrorMeasurementParticles as BaseErrorMeasurementParticles,
)
from exahype2.solvers.ErrorMeasurementParticles import (
    create_finish_time_step_implementation_for_error_measurement,
)

from exahype2.solvers.PDETerms import PDETerms

from exahype2.solvers.fv.actionsets import ParticleWisePostprocessSolution


class ErrorMeasurementParticles(BaseErrorMeasurementParticles):
    def __init__(
        self,
        solver,
        error_measurement_implementation=PDETerms.User_Defined_Implementation,
        delta_between_database_flushes=0,
        output_file_name="FVErrorMeasurement",
        data_delta_between_snapshots="1e+16",
        time_delta_between_snapshots=0.0,
        clear_database_after_flush=True,
        particle_set=None,
        descend_invocation_order=12,
    ):
        super(ErrorMeasurementParticles, self).__init__(
            solver=solver,
            error_measurement_implementation=error_measurement_implementation,
            delta_between_database_flushes=delta_between_database_flushes,
            output_file_name=output_file_name,
            data_delta_between_snapshots=data_delta_between_snapshots,
            time_delta_between_snapshots=time_delta_between_snapshots,
            clear_database_after_flush=clear_database_after_flush,
        )
        solver._finish_time_step_implementation += create_finish_time_step_implementation_for_error_measurement(
            guard="_solverState == SolverState::GridInitialisation or (_solverState == SolverState::IsSolving and isLastGridSweepOfTimeStep())"
        )

        solver._action_set_postprocess_solution = ParticleWisePostprocessSolution(
            solver, particle_set, descend_invocation_order=descend_invocation_order
        )
        solver._action_set_postprocess_solution.guard = """
            (repositories::{{SOLVER_INSTANCE}}.getSolverState() == {{SOLVER_NAME}}::SolverState::GridInitialisation or
             repositories::{{SOLVER_INSTANCE}}.isLastGridSweepOfTimeStep())"""
