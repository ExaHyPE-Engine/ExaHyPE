# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import exahype2.solvers.AdaptiveTimeSteppingCodeSnippets


class AdaptiveTimeSteppingCodeSnippets(
    exahype2.solvers.AdaptiveTimeSteppingCodeSnippets
):
    def __init__(self):
        pass

    def create_abstract_solver_user_declarations(self):
        return (
            super(
                AdaptiveTimeSteppingCodeSnippets, self
            ).create_abstract_solver_user_declarations()
            + """
protected:
  double _maxScaledEigenvalueThisTimeStep;

public:
  void setMaxScaledEigenvalue(double eigenvalue);
"""
        )

    def create_abstract_solver_user_definitions(self):
        return (
            super(
                AdaptiveTimeSteppingCodeSnippets, self
            ).create_abstract_solver_user_definitions()
            + """
void {{FULL_QUALIFIED_NAMESPACE}}::{{CLASSNAME}}::setMaxScaledEigenvalue(double eigenvalue) {
  if (tarch::la::greater(eigenvalue, 0.0)) {
    tarch::multicore::Lock lock(_semaphore);
    _maxScaledEigenvalueThisTimeStep = std::max(_maxScaledEigenvalueThisTimeStep, eigenvalue);
  }
}
"""
        )

    def create_abstract_solver_constructor_statements(self):
        return (
            super(
                AdaptiveTimeSteppingCodeSnippets, self
            ).create_abstract_solver_constructor_statements()
            + """
    _maxScaledEigenvalueThisTimeStep = 0.0;
"""
        )

    def create_compute_new_time_step_size(self):
        return """
    const auto eigenvalues = kernels::{{SOLVER_NAME}}::reduceMaxEigenvalues(
      repositories::{{SOLVER_INSTANCE}},
      QNew,
      marker.getX(),
      marker.getH(),
      timeStamp,
      timeStepSize
    );

    repositories::{{SOLVER_INSTANCE}}.setMaxEigenvalue(eigenvalues.maxPhysicalEigenvalue);
    repositories::{{SOLVER_INSTANCE}}.setMaxScaledEigenvalue(eigenvalues.maxScaledEigenvalue);
    const double newTimeStepSize = repositories::{{SOLVER_INSTANCE}}.getMinCellSize() > 0.0 ?
      repositories::{{SOLVER_INSTANCE}}.getAdmissibleTimeStepSize() * tarch::la::min(marker.getH()) / repositories::{{SOLVER_INSTANCE}}.getMinCellSize() :
      0.0;
"""

    def create_start_time_step_implementation(self):
        return """
  if (isFirstGridSweepOfTimeStep()) {
    _maxEigenvalueThisTimeStep = 0.0;
    _maxScaledEigenvalueThisTimeStep = 0.0;
  }
"""

    def create_finish_time_step_implementation(self):
        return (
            super(
                AdaptiveTimeSteppingCodeSnippets, self
            ).create_finish_time_step_implementation()
            + """
      if (isLastGridSweepOfTimeStep()) {
#ifdef PEANO_WITH_MPI
        double reducedMaxScaledEigenvalue = _maxScaledEigenvalueThisTimeStep;
        MPI_Comm communicator = tarch::mpi::Rank::getInstance().getCommunicator();
        auto progress = [&]() -> void { tarch::services::ServiceRepository::getInstance().receiveDanglingMessages(); };
        tarch::mpi::allreduce(
          &reducedMaxScaledEigenvalue,
          &_maxScaledEigenvalueThisTimeStep,
          1,
          MPI_DOUBLE,
          MPI_MAX,
          communicator,
          progress
        );
#endif

        if (tarch::la::greater(_maxScaledEigenvalueThisTimeStep, 0.0)) {
          _admissibleTimeStepSize = _admissibleTimeStepSizeThisTimeStep;

          const auto dtCFL = CFL * PNPM / _maxScaledEigenvalueThisTimeStep;

          if (not std::isfinite(dtCFL)) {
            tarch::triggerNonCriticalAssertion(__FILE__, __LINE__,
              "std::isfinite(dtCFL)",
              "Invalid (NaN or inf) time step size: " + std::to_string(dtCFL)
            );
          }

          if (tarch::la::smallerEquals(dtCFL, 0.0, 1e-10)) {
            logWarning("Degenerated time step size of " << std::to_string(dtCFL) << ". Problem might be extremely stiff (and can't be solved) or there could be a bug (h=" << _minCellHThisTimeStep << ")");
          }

          const auto dtToEnd                  = std::max(0.0, ::runner::RuntimeParameters::getInstance().getMinTerminalTime() - _minTimeStampThisTimeStep);
          _admissibleTimeStepSizeThisTimeStep = std::min(dtToEnd, dtCFL);
          if (tarch::la::greater(_admissibleTimeStepSizeThisTimeStep, 0.0)) {
            if (_minTimeStepSizeThisTimeStep >= std::numeric_limits<double>::max() / 10.0) {
              _minTimeStepSizeThisTimeStep = _admissibleTimeStepSizeThisTimeStep;
            }
            if (_maxTimeStepSizeThisTimeStep <= std::numeric_limits<double>::min() * 10.0) {
              _maxTimeStepSizeThisTimeStep = _admissibleTimeStepSizeThisTimeStep;
            }
          }
        }
      }
"""
        )
