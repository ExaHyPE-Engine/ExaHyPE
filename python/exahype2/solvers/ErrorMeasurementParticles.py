# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import jinja2

from .PDETerms import PDETerms


# TODO: Derive from ErrorMeasurement
class ErrorMeasurementParticles:
    """!
    Error measurement tool for ExaHyPE 2 tracer particles.
    """

    def __init__(
        self,
        solver,
        error_measurement_implementation,
        delta_between_database_flushes,
        output_file_name,
        data_delta_between_snapshots,
        time_delta_between_snapshots,
        clear_database_after_flush,
    ):
        solver._start_simulation_implementation += (
            create_start_simulation_implementation_for_error_measurement(
                delta_between_database_flushes=delta_between_database_flushes,
                output_file_name=output_file_name,
                data_delta_between_snapshots=data_delta_between_snapshots,
                time_delta_between_snapshots=time_delta_between_snapshots,
                clear_database_after_flush=clear_database_after_flush,
            )
        )

        solver.add_user_solver_includes(
            """
#include "toolbox/blockstructured/GlobalDatabase.h"
#include "vertexdata/{{PARTICLES_CONTAINER}}.h"
#include "globaldata/{{PARTICLE}}.h"
"""
        )

        solver._abstract_solver_user_declarations += """
public:
  toolbox::blockstructured::GlobalDatabase _errorDatabase;
  double _errors[3] = {0.0, 0.0, 0.0};
  void setError(double* error);
"""

        solver._abstract_solver_user_definitions += """
void {{"{{FULL_QUALIFIED_NAMESPACE}}::{{CLASSNAME}}"}}::setError(double* error) {
  tarch::multicore::Lock lock(_semaphore);
  _errors[0] += error[0];
  _errors[1] += error[1];
  _errors[2] = std::max(_errors[2], error[2]);
}
"""

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


def create_start_simulation_implementation_for_error_measurement(
    delta_between_database_flushes,
    output_file_name,
    data_delta_between_snapshots,
    time_delta_between_snapshots,
    clear_database_after_flush,
):
    Template = jinja2.Template(
        """
  if (tarch::mpi::Rank::getInstance().isGlobalMaster()) {
    _errorDatabase.setDeltaBetweenTwoDatabaseFlushes({{FLUSH_DELTA}});
    _errorDatabase.setOutputFileName("{{FILENAME}}");
    _errorDatabase.setDataName("l0-error, l2-error, l_inf-error");
    // _errorDatabase.setOutputPrecision({{OUTPUT_PRECISION}});
    _errorDatabase.setDataDeltaBetweenTwoSnapshots({{DATA_DELTA}}, {{USE_RELATIVE_DELTAS}});
    _errorDatabase.setTimeDeltaBetweenTwoSnapshots({{TIME_DELTA}});
    _errorDatabase.clearDatabaseAfterFlush( {{CLEAR_DATABASE_AFTER_FLUSH}});
    std::fill_n(_errors, 3, 0.0);
  }
""",
        undefined=jinja2.DebugUndefined,
    )

    d = {}
    d["FLUSH_DELTA"] = delta_between_database_flushes
    d["FILENAME"] = output_file_name
    d["DATA_DELTA"] = data_delta_between_snapshots
    d["USE_RELATIVE_DELTAS"] = "false"
    d["TIME_DELTA"] = time_delta_between_snapshots
    d["CLEAR_DATABASE_AFTER_FLUSH"] = "true" if clear_database_after_flush else "false"
    return Template.render(**d)


def create_finish_time_step_implementation_for_error_measurement(guard):
    return (
        """
    if ("""
        + guard
        + """) {
      #ifdef WITH_MPI
      double errorsThisTimeStamp[3] = {_errors[0], _errors[1], _errors[2]};

      tarch::mpi::Rank::getInstance().allReduce(
          &errorsThisTimeStamp[0],
          &_errors[0],
          1,
          MPI_DOUBLE,
          MPI_SUM,
          [&]() -> void { tarch::services::ServiceRepository::getInstance().receiveDanglingMessages(); }
      );

      tarch::mpi::Rank::getInstance().allReduce(
          &errorsThisTimeStamp[1],
          &_errors[1],
          1,
          MPI_DOUBLE,
          MPI_SUM,
          [&]() -> void { tarch::services::ServiceRepository::getInstance().receiveDanglingMessages(); }
      );

      tarch::mpi::Rank::getInstance().allReduce(
          &errorsThisTimeStamp[2],
          &_errors[2],
          1,
          MPI_DOUBLE,
          MPI_MAX,
          [&]() -> void { tarch::services::ServiceRepository::getInstance().receiveDanglingMessages(); }
      );
      #endif

      if (tarch::mpi::Rank::getInstance().isGlobalMaster()) {
        _errors[1] = std::sqrt(_errors[1]);
        _errorDatabase.addGlobalSnapshot(_minTimeStampThisTimeStep, 3, _errors);
      }

      std::fill_n(_errors, 3, 0.0);
    }
"""
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
