# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import jinja2

from .PDETerms import PDETerms


class ErrorMeasurement:
    """!
    Error measurement tool for various ExaHyPE 2 solvers.
    Takes as arguments some information about what errors should be measured, then
    at each timestep of the solver this measures the absolute error at each data
    point of the solver and, using this, computes the L0, L2 and L_inf errors integrated
    over each cell. Finally, it sums these up over the entire volume to get the integrated
    errors over the whole domain.
    These are then added to a GlobalDatabase object by the global master thread, and
    outputted to csv files at regular intervals.
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
        solver._constructor_implementation += (
            create_constructor_implementation_for_error_measurement(
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

        if error_measurement_implementation == PDETerms.User_Defined_Implementation:
            print(error_measurement_implementation)
            print(PDETerms.User_Defined_Implementation)
            solver._solver_user_declarations += create_solver_user_declarations()
            solver._solver_user_definitions += create_solver_user_definitions()
            solver._abstract_solver_user_declarations += (
                create_solver_user_abstract_declarations(True)
            )
        else:
            solver._abstract_solver_user_definitions += (
                create_solver_user_abstract_definitions(
                    error_measurement_implementation
                )
            )
            solver._abstract_solver_user_declarations += (
                create_solver_user_abstract_declarations(False)
            )


def create_constructor_implementation_for_error_measurement(
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
    _errorDatabase.setDataName("l0-error,l2-error,l_inf-error");
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
      #ifdef Parallel
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
        _errorDatabase.addGlobalSnapshot(_minTimeStampThisTimeStep + _maxTimeStepSizeThisTimeStep, 3, _errors);
      }

      std::fill_n(_errors, 3, 0.0);
    }
"""
    )


def create_solver_user_abstract_declarations(user_defined):
    return (
        """
    /**
     * Exact solution of the equation at a given point and time
     *
     * @param x             Position
     * @param t             Time
     * @param dt            Timestep size
     * @param sol           Analytical solution of the equation
     */
    virtual void analyticalSolution(
      const tarch::la::Vector<Dimensions, double>&  x,
      const double                                  t,
      const double                                  dt,
      const double*                                 Q, // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
      double*                                       solution // solution[{{NUMBER_OF_UNKNOWNS}}]
    )"""
        + (" = 0" if user_defined else " final")
        + ";"
    )


def create_solver_user_abstract_definitions(error_measurement_implementation):
    return (
        """
    void {{"{{FULL_QUALIFIED_NAMESPACE}}::{{CLASSNAME}}"}}::analyticalSolution(
      const tarch::la::Vector<Dimensions, double>&  x,
      const double                                  t,
      const double                                  dt,
      const double*                                 Q, // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
      double*                                       solution // solution[{{NUMBER_OF_UNKNOWNS}}]
    ) {
"""
        + error_measurement_implementation
        + """
}
"""
    )


def create_solver_user_declarations():
    return """
    virtual void analyticalSolution(
      const tarch::la::Vector<Dimensions, double>&  x,
      const double                                  t,
      const double                                  dt,
      const double*                                 Q, // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
      double*                                       solution // solution[{{NUMBER_OF_UNKNOWNS}}]
    ) override;
"""


def create_solver_user_definitions():
    return """
    void {{"{{FULL_QUALIFIED_NAMESPACE}}::{{CLASSNAME}}"}}::analyticalSolution(
      const tarch::la::Vector<Dimensions, double>&  x,
      const double                                  t,
      const double                                  dt,
      const double*                                 Q, // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
      double*                                       solution // solution[{{NUMBER_OF_UNKNOWNS}}]
    ) {
      logTraceInWith2Arguments("analyticalSolution(...)", x, t);

      // @todo Implement your stuff here

      logTraceOut("analyticalSolution(...)");
    }
"""
