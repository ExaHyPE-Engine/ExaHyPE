# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org

from peano4.solversteps.ActionSet import ActionSet
from exahype2.solvers.PDETerms import PDETerms
import jinja2

from .kernels import (
  create_constructor_implementation_for_error_measurement,
  create_solver_user_declarations, create_solver_user_definitions,
  create_solver_user_abstract_declarations, create_solver_user_abstract_definitions
)

class ErrorMeasurement:
  """!
  Error measurement tool for various ExaHyPE2 solvers.
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
      error_measurement_implementation=PDETerms.User_Defined_Implementation,
      deltaBetweenDatabaseFlushes = 0,
      outputFileName = "errorMeasurement",
      dataDeltaBetweenSnapshots = "1e+16",
      timeDeltaBetweenSnapshots = 0.,
      clearDatabaseAfterFlush = True
  ):
    
    solver._constructor_implementation += create_constructor_implementation_for_error_measurement(
      deltaBetweenDatabaseFlushes, outputFileName, dataDeltaBetweenSnapshots, timeDeltaBetweenSnapshots, clearDatabaseAfterFlush
    )

    solver.add_user_solver_includes("""
#include "toolbox/blockstructured/GlobalDatabase.h"
""")
    
    solver._abstract_solver_user_declarations += """
public:
  toolbox::blockstructured::GlobalDatabase _errorDatabase;
  double _errors[3] = {0.0, 0.0, 0.0};
  void setError(double* err);
"""

    solver._abstract_solver_user_definitions  += """
void {{"{{FULL_QUALIFIED_NAMESPACE}}::{{CLASSNAME}}"}}::setError(double* err){
  tarch::multicore::Lock lock(_semaphore);
  _errors[0] += err[0];
  _errors[1] += err[1];
  _errors[2] = std::max(_errors[2],err[2]);
}
"""

    if error_measurement_implementation==PDETerms.User_Defined_Implementation:
      print(error_measurement_implementation)
      print(PDETerms.User_Defined_Implementation)
      solver._solver_user_declarations  += create_solver_user_declarations()
      solver._solver_user_definitions   += create_solver_user_definitions()
      solver._abstract_solver_user_declarations += create_solver_user_abstract_declarations(True)
    else:
      solver._abstract_solver_user_definitions  += create_solver_user_abstract_definitions(error_measurement_implementation)
      solver._abstract_solver_user_declarations += create_solver_user_abstract_declarations(False)
