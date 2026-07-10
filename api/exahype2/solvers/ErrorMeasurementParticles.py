# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
from .PDETerms import PDETerms

from exahype2.solvers.ErrorMeasurement import (
    add_error_database_to_solver,
    create_finish_time_step_implementation_for_error_measurement,
    create_start_simulation_implementation_for_error_measurement,
)


class ErrorMeasurementParticles:
    """!

    Error measurement tool for ExaHyPE 2 tracer particles.
    """

    def __init__(
        self,
        solver,
        error_measurement_implementation,
        output_file_name="Error",
        time_delta_between_snapshots=0.0,
    ):
        solver._start_simulation_implementation += (
            create_start_simulation_implementation_for_error_measurement(
                output_file_name=output_file_name,
                time_delta_between_snapshots=time_delta_between_snapshots,
            )
        )

        add_error_database_to_solver(solver)

        solver.add_user_solver_includes(
            """
#include "vertexdata/{{PARTICLES_CONTAINER}}.h"
#include "globaldata/{{PARTICLE}}.h"
"""
        )

        solver._abstract_solver_user_declarations += (
            create_solver_user_abstract_declarations()
        )
        solver._solver_user_declarations += create_solver_user_declarations()
        if error_measurement_implementation == PDETerms.User_Defined_Implementation:
            solver._solver_user_definitions += create_solver_user_definitions(
                True, error_measurement_implementation
            )
        else:
            solver._solver_user_definitions += create_solver_user_definitions(
                False, error_measurement_implementation
            )


def create_solver_user_abstract_declarations():
    return """
    /**
     * Exact solution of the equation at a given point and time
     *
     * @param t             Time
     * @param dt            Timestep size
     * @param sol           Analytical solution of the equation
     */
    virtual void analyticalSolution(
      globaldata::{{PARTICLE}}*                     particle,
      const double                                  t,
      const double                                  dt,
      double*                                       solution // solution[{{NUMBER_OF_UNKOWNS_PARTICLE}}+{{DIMENSIONS}}]
    ) = 0;
"""


def create_solver_user_declarations():
    return """
    virtual void analyticalSolution(
      globaldata::{{PARTICLE}}*                     particle,
      const double                                  t,
      const double                                  dt,
      double*                                       solution // solution[{{NUMBER_OF_UNKOWNS_PARTICLE}}+{{DIMENSIONS}}]
    ) override;
"""


def create_solver_user_definitions(user_defined, error_measurement_implementation):
    if not user_defined:
        return (
            """
    void {{"{{FULL_QUALIFIED_NAMESPACE}}::{{CLASSNAME}}"}}::analyticalSolution(
      globaldata::{{PARTICLE}}*                     particle,
      const double                                  t,
      const double                                  dt,
      double*                                       solution // solution[{{NUMBER_OF_UNKOWNS_PARTICLE}}+{{DIMENSIONS}}]
    ) {
"""
            + error_measurement_implementation
            + """
}
"""
        )
    else:
        return """
    void {{"{{FULL_QUALIFIED_NAMESPACE}}::{{CLASSNAME}}"}}::analyticalSolution(
      globaldata::{{PARTICLE}}*                     particle,
      const double                                  t,
      const double                                  dt,
      double*                                       solution // solution[{{NUMBER_OF_UNKOWNS_PARTICLE}}+{{DIMENSIONS}}]
    ) {
      // @todo Implement
    }
"""
