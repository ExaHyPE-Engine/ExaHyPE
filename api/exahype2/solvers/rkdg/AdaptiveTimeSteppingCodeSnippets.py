# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import exahype2.solvers.AdaptiveTimeSteppingCodeSnippets


class AdaptiveTimeSteppingCodeSnippets( exahype2.solvers.AdaptiveTimeSteppingCodeSnippets ):
  """
  Code snippet generator for adaptive time stepping in the Runge-Kutta schemes.
  """
  def __init__(self, time_step_relaxation):
    self._time_step_relaxation = time_step_relaxation

  def create_abstract_solver_user_declarations(self):
    return super( AdaptiveTimeSteppingCodeSnippets, self ).create_abstract_solver_user_declarations() + """
protected:
  double _maxScaledEigenvalueThisTimeStep;

public:
  void setMaxScaledEigenvalue(double eigenvalue);
"""

  def create_abstract_solver_user_definitions(self):
    return super( AdaptiveTimeSteppingCodeSnippets, self ).create_abstract_solver_user_definitions() + """
void {{FULL_QUALIFIED_NAMESPACE}}::{{CLASSNAME}}::setMaxScaledEigenvalue(double eigenvalue) {
  if (tarch::la::greater(eigenvalue, 0.0)) {
    tarch::multicore::Lock lock(_semaphore);
    _maxScaledEigenvalueThisTimeStep = std::max(_maxScaledEigenvalueThisTimeStep, eigenvalue);
  }
}
"""

  def create_abstract_solver_constructor_statements(self):
    return super( AdaptiveTimeSteppingCodeSnippets, self ).create_abstract_solver_constructor_statements() + """
    _maxScaledEigenvalueThisTimeStep = 0.0;
"""

  def create_compute_new_time_step_size(self):
    """
    Record both eigenvalues and derive the new time step size.

    The physical eigenvalue feeds the solver statistics, while the scaled eigenvalue
    drives the admissible time step size.
    """
    return """
    repositories::{{SOLVER_INSTANCE}}.setMaxEigenvalue(maxPhysicalEigenvalue);
    repositories::{{SOLVER_INSTANCE}}.setMaxScaledEigenvalue(maxScaledEigenvalue);
    const double newTimeStepSize = repositories::{{SOLVER_INSTANCE}}.getAdmissibleTimeStepSize();
"""

  def create_start_time_step_implementation(self):
    return """
  if (isFirstGridSweepOfTimeStep()) {
    _maxEigenvalueThisTimeStep       = 0.0;
    _maxScaledEigenvalueThisTimeStep = 0.0;
  }
"""

  def create_finish_time_step_implementation(self):
    """
    Derive the admissible time step size from the reduced scaled eigenvalue.

    The Runge-Kutta and polynomial-degree factors damp the resulting size.
    """
    return super( AdaptiveTimeSteppingCodeSnippets, self ).create_finish_time_step_implementation() + """
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
      // The scaled eigenvalue sums the directional eigenvalues over the cell width, so it
      // carries the cell size and the number of space dimensions; the relaxation is the
      // fraction of the resulting stability limit.
      const     double waveSpeedDamping   = 1.0 / _maxScaledEigenvalueThisTimeStep;
      constexpr double CockburnShuDamping = 1.0 / (2.0*{{RK_ORDER}}-1.0);
      constexpr double MeshDamping        = {{DG_ORDER}}==0 ? 1.0 : 1.0 / ({{DG_ORDER}} * {{DG_ORDER}});
      _admissibleTimeStepSize = _admissibleTimeStepSizeThisTimeStep;
      _admissibleTimeStepSizeThisTimeStep = """ + str(self._time_step_relaxation) + """ * waveSpeedDamping * CockburnShuDamping * MeshDamping;
      if (tarch::la::greater(_admissibleTimeStepSizeThisTimeStep, 0.0)) {
        if (_minTimeStepSizeThisTimeStep >= std::numeric_limits<double>::max() / 10.0) {
          _minTimeStepSizeThisTimeStep = _admissibleTimeStepSizeThisTimeStep;
        }
        if (_maxTimeStepSizeThisTimeStep <= std::numeric_limits<double>::min() * 10.0) {
          _maxTimeStepSizeThisTimeStep = _admissibleTimeStepSizeThisTimeStep;
        }
      }
      if ( std::isnan(_admissibleTimeStepSizeThisTimeStep) or std::isinf(_admissibleTimeStepSizeThisTimeStep) ) {
        ::tarch::triggerNonCriticalAssertion( __FILE__, __LINE__, "_admissibleTimeStepSizeThisTimeStep>0", "Invalid (NaN or inf) time step size: " + std::to_string(_admissibleTimeStepSizeThisTimeStep) );
      }
      if (tarch::la::smallerEquals(_admissibleTimeStepSizeThisTimeStep,0.0,1e-10) ) {
        logWarning("Degenerated time step size of " << std::to_string(_admissibleTimeStepSizeThisTimeStep) << ". The problem might be extremely stiff (and cannot be solved), or there could be a bug in the eigenvalue computation." );
      }
    }
  }
"""
