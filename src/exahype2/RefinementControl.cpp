#include "RefinementControl.h"

#include "tarch/multicore/Lock.h"
#include "peano4/grid/grid.h"
#include "peano4/parallel/SpacetreeSet.h"


exahype2::RefinementCommand exahype2::getDefaultRefinementCommand() {
  return RefinementCommand::Erase;
}


exahype2::RefinementCommand operator&&( exahype2::RefinementCommand lhs, exahype2::RefinementCommand rhs) {
  if (
    lhs == exahype2::RefinementCommand::Refine or rhs == exahype2::RefinementCommand::Refine
  ) {
    return exahype2::RefinementCommand::Refine;
  }
  else if (
    lhs == exahype2::RefinementCommand::Keep or rhs == exahype2::RefinementCommand::Keep
  ) {
    return exahype2::RefinementCommand::Keep;
  }
  else {
    return exahype2::RefinementCommand::Erase;
  }
}


std::string toString( exahype2::RefinementCommand value ) {
  switch (value) {
    case exahype2::RefinementCommand::Refine:  return "refine";
    case exahype2::RefinementCommand::Keep:    return "keep";
    case exahype2::RefinementCommand::Erase:   return "erase";
  }
  return "<undef>";
}


tarch::logging::Log  exahype2::RefinementControl::_log( "exahype2::RefinementControl" );

tarch::multicore::BooleanSemaphore  exahype2::RefinementControl::_semaphore;


exahype2::RefinementControl::RefinementControl(double tolerance):
  _Tolerance( tolerance ) {
}


exahype2::RefinementControl::~RefinementControl() {
}


void exahype2::RefinementControl::clear() {
  _newEvents.clear();
}


void exahype2::RefinementControl::addCommand(
  const tarch::la::Vector<Dimensions,double>&  x,
  const tarch::la::Vector<Dimensions,double>&  h,
  exahype2::RefinementCommand                  command,
  int                                          lifetime
) {
  logTraceInWith3Arguments( "addCommand()", x, h, ::toString(command) );
  tarch::multicore::Lock lock( _semaphore );
  
  assertion(lifetime>=1);

  switch (command) {
    case ::exahype2::RefinementCommand::Refine:
      {
        tarch::la::Vector<Dimensions,double> expandedWidth   = (1.0+_Tolerance) * h;
        tarch::la::Vector<Dimensions,double> refinedMeshSize = (1.0+_Tolerance) * 1.0/3.0 * h;

        peano4::grid::GridControlEvent newEvent(
          peano4::grid::GridControlEvent::RefinementControl::Refine,
          x-0.5*expandedWidth,
          expandedWidth,
          refinedMeshSize
        );

        assertionNumericalEquals1( newEvent.getWidth(0), newEvent.getWidth(1), newEvent.toString() );
        assertionNumericalEquals1( newEvent.getH(0), newEvent.getH(1), newEvent.toString() );
        _newEvents.push_back( std::pair<peano4::grid::GridControlEvent,int>(newEvent, lifetime) );
        logDebug( "addCommend()", "added refinement for x=" << x << ", h=" << h << ": " << newEvent.toString() << " (total of " << _newEvents.size() << " instructions)" );
      }
      break;
    case ::exahype2::RefinementCommand::Keep:
      break;
    case ::exahype2::RefinementCommand::Erase:
      {
        tarch::la::Vector<Dimensions,double> expandedWidth   = (1.0+_Tolerance) * h;
        tarch::la::Vector<Dimensions,double> shift           = 0.5 * (expandedWidth - h);
        tarch::la::Vector<Dimensions,double> refinedMeshSize = 3.1 * h;

        peano4::grid::GridControlEvent newEvent(
          peano4::grid::GridControlEvent::RefinementControl::Erase,
          x-0.5 * h - shift,
          expandedWidth,
          refinedMeshSize
        );

        assertionNumericalEquals1( newEvent.getWidth(0), newEvent.getWidth(1), newEvent.toString() );
        assertionNumericalEquals1( newEvent.getH(0), newEvent.getH(1), newEvent.toString() );
        _newEvents.push_back( std::pair<peano4::grid::GridControlEvent,int>(newEvent, lifetime) );
        logDebug( "addCommend()", "added erase for x=" << x << ", h=" << h << ": " << newEvent.toString() << " (total of " << _newEvents.size() << " instructions)" );
      }
      break;
  }
  logTraceOutWith1Argument( "addCommand()", _newEvents.size() );
}


std::string exahype2::RefinementControl::toString() const {
  std::ostringstream msg;
  msg << "("
      << "#events=" << _newEvents.size()
      << ")";
  return msg.str();
}
