# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import jinja2

from .SolverCodeSnippets import SolverCodeSnippets


class AdaptiveTimeSteppingCodeSnippets(SolverCodeSnippets):
    """
    Code snippet generator for all adaptive time stepping solvers
    """

    def create_abstract_solver_user_declarations(self):
        return """
protected:
  double _maxEigenvalue;
  double _maxEigenvalueThisTimeStep;

  double _admissibleTimeStepSize;
  double _admissibleTimeStepSizeThisTimeStep;

public:
  void setMaxEigenvalue(double eigenvalue);
  double getMaxEigenvalue() const;

  /**
   * @return Admissible time step size for the current sweep, i.e.
   *         return _admissibleTimeStepSize. This value always refers
   *         to the minimum mesh volume size. If you use subcycling,
   *         you have to scale it for cells that are not on the finest
   *         mesh resolution.
   */
  virtual double getAdmissibleTimeStepSize() const;
"""

    def create_abstract_solver_user_definitions(self):
        return """
void {{FULL_QUALIFIED_NAMESPACE}}::{{CLASSNAME}}::setMaxEigenvalue(double eigenvalue) {
  if (tarch::la::greater(eigenvalue, 0.0)) {
    tarch::multicore::Lock lock(_semaphore);
    _maxEigenvalueThisTimeStep = std::max(_maxEigenvalueThisTimeStep, eigenvalue);
  }
}

double {{FULL_QUALIFIED_NAMESPACE}}::{{CLASSNAME}}::getMaxEigenvalue() const {
  return _maxEigenvalue;
}

double {{FULL_QUALIFIED_NAMESPACE}}::{{CLASSNAME}}::getAdmissibleTimeStepSize() const {
  return _admissibleTimeStepSize;
}
"""

    def create_abstract_solver_constructor_statements(self):
        return """
    _admissibleTimeStepSize = 0.0;
    _admissibleTimeStepSizeThisTimeStep = 0.0;

    _maxEigenvalue = 0.0;
    _maxEigenvalueThisTimeStep = 0.0;
"""

    def create_compute_time_step_size(self):
        return """
    const double timeStepSize = repositories::{{SOLVER_INSTANCE}}.getAdmissibleTimeStepSize();
"""

    def create_compute_new_time_step_size(self):
        """
        This is global, fixed time stepping, i.e. the new time step size will likely
        be the same as the previous one, unless the mesh changes, as we work with
        normalised time step sizes, i.e. in this case the time step size might change.
        Anyway, the new time step size is only for stats anyway, as we'll pick a
        global one when we determine timeStepSize the next time step.
        """
        return """
    repositories::{{SOLVER_INSTANCE}}.setMaxEigenvalue(maxEigenvalue);
    const double newTimeStepSize = repositories::{{SOLVER_INSTANCE}}.getAdmissibleTimeStepSize();
"""

    def create_reduced_eigenvalues(self):
        """
        Names of the eigenvalue members that the end-of-time-step reduction covers.

        A solver that tracks further eigenvalues extends this list rather than adding a
        reduction of its own.
        """
        return ["_maxEigenvalueThisTimeStep"]

    def create_max_eigenvalue_reduction(self):
        """
        Turn the rank-local eigenvalue maxima into global maxima with one allreduce.

        Every reduction synchronises all ranks, so all eigenvalues share one buffer.
        """
        eigenvalues = self.create_reduced_eigenvalues()
        width = max(len(eigenvalue) for eigenvalue in eigenvalues)

        Template = jinja2.Template(
            """
    const double   localMaxEigenvalues[]  = {{INITIALISER}};
    double         globalMaxEigenvalues[] = {{ZEROS}};
    const MPI_Comm communicator           = tarch::mpi::Rank::getInstance().getCommunicator();

    tarch::mpi::allreduce(
      localMaxEigenvalues,
      globalMaxEigenvalues,
      {{COUNT}},
      MPI_DOUBLE,
      MPI_MAX,
      communicator
    );

{{ASSIGNMENTS}}
""",
            keep_trailing_newline=True,
        )

        d = {}
        d["INITIALISER"] = "{" + ", ".join(eigenvalues) + "}"
        d["ZEROS"] = "{" + ", ".join(["0.0"] * len(eigenvalues)) + "}"
        d["COUNT"] = len(eigenvalues)
        d["ASSIGNMENTS"] = "\n".join(
            "    {:<{}} = globalMaxEigenvalues[{}];".format(eigenvalue, width, index)
            for index, eigenvalue in enumerate(eigenvalues)
        )
        return Template.render(**d)

    def create_finish_time_step_implementation(self):
        """
        This routine is inserted after we have reduced all global quantities. These
        are the quantities with the postfix ThisTimeStep.
        """
        return (
            """
  if (isLastGridSweepOfTimeStep()) {"""
            + self.create_max_eigenvalue_reduction()
            + """
    if (tarch::la::smaller(_maxEigenvalueThisTimeStep, 0.0)) {
      tarch::triggerNonCriticalAssertion(__FILE__, __LINE__, "_maxEigenvalueThisTimeStep >= 0", "Invalid max eigenvalue: " + std::to_string(_maxEigenvalueThisTimeStep));
    } else if (tarch::la::equals(_maxEigenvalueThisTimeStep, 0.0)) {
      tarch::triggerNonCriticalAssertion(__FILE__, __LINE__, "_maxEigenvalueThisTimeStep > 0", "Max eigenvalue has reached zero, which means that the simulation cannot successfully continue: " + std::to_string(_maxEigenvalueThisTimeStep));
      _admissibleTimeStepSizeThisTimeStep = 0.0;
    }

    _maxEigenvalue = _maxEigenvalueThisTimeStep;
  }
"""
        )
