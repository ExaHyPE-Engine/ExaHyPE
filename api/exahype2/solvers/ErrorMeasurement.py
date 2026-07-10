# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import jinja2

from .PDETerms import PDETerms


def build_measured_unknowns_initializer_list(solver, measured_unknowns):
    """!

    Resolve `measured_unknowns` into a C++ array initialiser list of unknown
    indices (e.g. "{0, 2, 3}"). None selects all unknowns; otherwise an int/str
    or any iterable of unknown indices and/or variable names.
    """
    shortcuts = solver._variable_shortcuts
    number_of_unknowns = shortcuts.unknowns

    if measured_unknowns is None:
        indices = list(range(number_of_unknowns))
    else:
        if isinstance(measured_unknowns, (int, str)):
            measured_unknowns = [measured_unknowns]

        indices = []
        for item in measured_unknowns:
            if isinstance(item, str):
                if item not in shortcuts.names:
                    raise ValueError(
                        "measured_unknowns: unknown variable name '{}'. "
                        "Known names are {}.".format(item, shortcuts.names)
                    )
                position = shortcuts.names.index(item)
                indices.extend(
                    range(
                        shortcuts.positions[position],
                        shortcuts.positions[position + 1],
                    )
                )
            elif isinstance(item, bool):
                # bool is a subclass of int, so exclude it before the int branch.
                raise TypeError(
                    "measured_unknowns must contain ints or strs, not bool."
                )
            elif isinstance(item, int):
                indices.append(item)
            else:
                raise TypeError(
                    "measured_unknowns entries must be int or str, got {}.".format(
                        type(item).__name__
                    )
                )

    # De-duplicate while preserving order.
    seen = set()
    unique_indices = []
    for index in indices:
        if index not in seen:
            seen.add(index)
            unique_indices.append(index)
    indices = unique_indices

    if not indices:
        raise ValueError("measured_unknowns resolved to an empty selection.")

    for index in indices:
        if index < 0 or index >= number_of_unknowns:
            raise ValueError(
                "measured_unknowns: index {} is out of range [0, {}).".format(
                    index, number_of_unknowns
                )
            )

    return "{" + ", ".join(str(index) for index in indices) + "}"


def add_error_database_to_solver(solver):
    """!

    Add the shared error database, the per-rank error accumulator and the
    setError() method to a solver. Common to every error-measurement variant,
    so both the cell-based and the particle-based tools reuse it.
    """
    solver.add_user_solver_includes(
        """
#include "tarch/database/GlobalTimeSeriesDatabase.h"
#include "runner/RuntimeParameters.h"
#include "tarch/mpi/Rank.h"
#include "tarch/mpi/mpi.h"
"""
    )

    solver._abstract_solver_user_declarations += """
public:
  tarch::database::GlobalTimeSeriesDatabase _errorDatabase;
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


class ErrorMeasurement:
    """!

    Error measurement tool for ExaHyPE 2 solvers, this is the base class that must
    be specialised for different solvers.

    Used by calling the corresponding specialisation by calling it using the solver
    instance as an argument.

    When called, it adds an additional function analyticalSolution() to the user-defined
    code snippets, through which the user can define what the solution should be.

    The error measurement also adds a postprocessing step to the corresponding solver,
    which computes the L1, L2 and Linf error between the computed solution and the
    analytical solution at each data point over the domain. These are then summed up
    over the entire volume to get the integrated errors over the entire domain.

    Finally, these are added to a GlobalTimeSeriesDatabase object by the global master thread, and
    outputted to csv files at regular intervals.

    @parameters:

    solver: the solver for which the errors should be measured

    error_measurement_implementation: allows one to specify the analytical
      solution directly instead of manually filling it in the user-defined
      code snippets, analogously to fluxes and such in the solvers.

    output_file_name: specify what the output .csv files should be named

    time_delta_between_snapshots: record a measurement only once this much
      simulation time has elapsed since the last (default 0 = every step).

    measured_unknowns: restrict the error norms to a subset of the unknowns.
      None (default) selects all unknowns; otherwise an int/str or iterable of
      unknown indices and/or variable names. See
      build_measured_unknowns_initializer_list.
    """

    def __init__(
        self,
        solver,
        error_measurement_implementation,
        output_file_name="Error",
        time_delta_between_snapshots=0.0,
        measured_unknowns=None,
    ):
        solver._error_norm_measured_unknowns = build_measured_unknowns_initializer_list(
            solver, measured_unknowns
        )

        solver._start_simulation_implementation += (
            create_start_simulation_implementation_for_error_measurement(
                output_file_name=output_file_name,
                time_delta_between_snapshots=time_delta_between_snapshots,
            )
        )

        add_error_database_to_solver(solver)

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
    output_file_name, time_delta_between_snapshots
):
    Template = jinja2.Template(
        """
  if (tarch::mpi::Rank::getInstance().isGlobalMaster()) {
    _errorDatabase.setOutputFileName(::runner::RuntimeParameters::getInstance().getOutputPath() + "{{FILENAME}}");
    _errorDatabase.setDataName("l1-error, l2-error, linf-error");
    _errorDatabase.setTimeDeltaBetweenTwoSnapshots({{TIME_DELTA}});
    std::fill_n(_errors, 3, 0.0);
  }
""",
        undefined=jinja2.DebugUndefined,
    )

    d = {}
    d["FILENAME"] = output_file_name
    d["TIME_DELTA"] = time_delta_between_snapshots
    return Template.render(**d)


def create_finish_time_step_implementation_for_error_measurement(guard):
    return (
        """
    if ("""
        + guard
        + """) {
      #ifdef PEANO_WITH_MPI
      double localSummedErrors[2] = {_errors[0], _errors[1]};
      double globalSummedErrors[2] = {0.0, 0.0};
      double localMaxError[1] = {_errors[2]};
      double globalMaxError[1] = {0.0};
      MPI_Comm communicator = tarch::mpi::Rank::getInstance().getCommunicator();
      auto progress = [&]() -> void { tarch::services::ServiceRepository::getInstance().receiveDanglingMessages(); };

      tarch::mpi::allreduce(
          localSummedErrors,
          globalSummedErrors,
          2,
          MPI_DOUBLE,
          MPI_SUM,
          communicator,
          progress
      );

      tarch::mpi::allreduce(
          localMaxError,
          globalMaxError,
          1,
          MPI_DOUBLE,
          MPI_MAX,
          communicator,
          progress
      );
      _errors[0] = globalSummedErrors[0];
      _errors[1] = globalSummedErrors[1];
      _errors[2] = globalMaxError[0];
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
     * @param x             Position
     * @param t             Time
     * @param dt            Timestep size
     * @param sol           Analytical solution of the equation
     */
    virtual void analyticalSolution(
      const tarch::la::Vector<DIMENSIONS, double>&  x,
      const double                                  t,
      const double                                  dt,
      const double*                                 Q, // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
      double*                                       solution // solution[{{NUMBER_OF_UNKNOWNS}}]
    ) = 0;
"""


def create_solver_user_declarations():
    return """
      virtual void analyticalSolution(
        const tarch::la::Vector<DIMENSIONS, double>&  x,
        const double                                  t,
        const double                                  dt,
        const double*                                 Q, // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
        double*                                       solution // solution[{{NUMBER_OF_UNKNOWNS}}]
      ) override;
"""


def create_solver_user_definitions(user_defined, error_measurement_implementation):
    if not user_defined:
        return (
            """
    void {{"{{FULL_QUALIFIED_NAMESPACE}}::{{CLASSNAME}}"}}::analyticalSolution(
      const tarch::la::Vector<DIMENSIONS, double>&  x,
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
    else:
        return """
    void {{"{{FULL_QUALIFIED_NAMESPACE}}::{{CLASSNAME}}"}}::analyticalSolution(
      const tarch::la::Vector<DIMENSIONS, double>&  x,
      const double                                  t,
      const double                                  dt,
      const double*                                 Q, // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
      double*                                       solution // solution[{{NUMBER_OF_UNKNOWNS}}]
    ) {
      // @todo Implement
    }
"""
