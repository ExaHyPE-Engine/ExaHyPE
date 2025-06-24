#include "Solver.h"

#include "tarch/la/ScalarOperations.h"
#include "tarch/multicore/Lock.h"
#include "tarch/mpi/Rank.h"
#include <cmath>
#include <iomanip>
#include <array>


tarch::logging::Log mghype::matrixfree::solvers::Solver::_log( "mghype::matrixfree::solvers::Solver" );


mghype::matrixfree::solvers::Solver::Solver(
  const std::string&  name,
  double              tolerance
):
  _name(name),
  SolverStatistics(tolerance) {
  const std::string      prefix = "Abstract";
  std::string::size_type index  = _name.find( prefix );
  if (index!=std::string::npos) {
    _name.erase(index, prefix.length());
  }
}


std::string mghype::matrixfree::solvers::Solver::toString() const {
  std::ostringstream msg;
  msg << std::scientific << std::setprecision(16);
  msg << _name
      << ", " << SolverStatistics::toString();
  return msg.str();
}

