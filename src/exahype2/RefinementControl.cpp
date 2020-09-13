#include "RefinementControl.h"
#include "tarch/multicore/Lock.h"


exahype2::RefinementCommand exahype2::getDefaultRefinementCommand() {
  return RefinementCommand::Coarsen;
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
	return exahype2::RefinementCommand::Coarsen;
  }
}


std::string toString( exahype2::RefinementCommand value ) {
  switch (value) {
    case exahype2::RefinementCommand::Refine:  return "refine";
    case exahype2::RefinementCommand::Keep:    return "keep";
    case exahype2::RefinementCommand::Coarsen: return "coarsen";
  }
  return "<undef>";
}


tarch::logging::Log  exahype2::RefinementControl::_log( "exahype2::RefinementControl" );


tarch::multicore::BooleanSemaphore  exahype2::RefinementControl::_semaphore;


exahype2::RefinementControl::RefinementControl(double tolerance):
  _Tolerance( tolerance ) {
}


void exahype2::RefinementControl::clear() {
  logDebug( "clear()", "clear list of control events" );
  _newEvents.clear();
}


std::vector< peano4::grid::GridControlEvent >  exahype2::RefinementControl::getGridControlEvents() {
  logDebug( "getGridControlEvents()", "return " << _committedEvents.size() << " grid control events" );
  std::vector< peano4::grid::GridControlEvent > result = _committedEvents;
  _committedEvents.clear();
  return result;
}


void exahype2::RefinementControl::addCommand(
  const tarch::la::Vector<Dimensions,double>&  x,
  const tarch::la::Vector<Dimensions,double>&  h,
  exahype2::RefinementCommand                  command,
  bool                                         invokedByGridConstruction
) {
  logTraceInWith4Arguments( "addCommand()", x, h, ::toString(command), invokedByGridConstruction );
  switch (command) {
    case ::exahype2::RefinementCommand::Refine:
      {
        tarch::la::Vector<Dimensions,double> expandedH = (1.0+_Tolerance) * h;
        tarch::la::Vector<Dimensions,double> shift     = 0.5 * _Tolerance * h;

        peano4::grid::GridControlEvent newEvent(
          peano4::grid::GridControlEvent::RefinementControl::Refine,
          x-0.5 * h - shift,
          expandedH,
          (1.0+_Tolerance) * 1.0/3.0 * h
        );
        assertionNumericalEquals1( newEvent.getWidth(0), newEvent.getWidth(1), newEvent.toString() );
        assertionNumericalEquals1( newEvent.getH(0), newEvent.getH(1), newEvent.toString() );
        _newEvents.push_back( newEvent );
        logDebug( "addCommend()", "added refinement for x=" << x << ", h=" << h << ": " << newEvent.toString() << " (total of " << _newEvents.size() << " instructions)" );
      }
      break;
    case ::exahype2::RefinementCommand::Keep:
      break;
    case ::exahype2::RefinementCommand::Coarsen:
      if (not invokedByGridConstruction) {
        assertionMsg(false, "not implemented yet");
      }
      break;
  }
  logTraceOutWith1Argument( "addCommand()", _newEvents.size() );
}


void exahype2::RefinementControl::finishStep() {
  logInfo( "finishStep()", "roll over " << _newEvents.size() << " events" );
  _committedEvents.insert( _committedEvents.end(), _newEvents.begin(), _newEvents.end() );
  _newEvents.clear();
}


void exahype2::RefinementControl::merge( const RefinementControl& control ) {
  tarch::multicore::Lock lock(_semaphore);
  _newEvents.insert( _newEvents.end(), control._newEvents.begin(), control._newEvents.end() );
}


std::string exahype2::RefinementControl::toString() const {
  std::ostringstream msg;
  msg << "("
      << "#new-events=" << _newEvents.size()
      << ",#committed-events=" << _committedEvents.size()
      << ")";
  return msg.str();
}
