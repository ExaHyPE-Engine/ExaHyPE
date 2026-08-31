// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "RefinementControl.h"

#include "exahype2/RefinementControlService.h"
#include "tarch/Assertions.h"
#include "tarch/instrumentation/Debug.h"

/** Returns Erase, the lowest-priority refinement command. */
exahype2::RefinementCommand exahype2::getDefaultRefinementCommand() { return RefinementCommand::Erase; }

/**
 * Creates a container for the requests of one solver.
 *
 * @param solverNumber Identifies the solver whose criterion fills this container.
 */
exahype2::RefinementControl::RefinementControl(const int solverNumber) :
  _solverNumber(solverNumber) {}

/**
 * Drops the events collected so far. The events the service has already
 * committed survive.
 */
void exahype2::RefinementControl::clear() { _newEvents.clear(); }

/**
 * Adds a refine or erase request for one cell and drops a keep request.
 *
 * A refine request refines that cell together with one full cell of every
 * neighbour. An erase request is a vote that
 * RefinementControlService::finaliseCoarseningEvents() promotes to the cell's
 * parent once the whole sibling group agrees.
 *
 * @param x       Centre of the cell for which this command is triggered.
 * @param h       Size of this cell.
 * @param command Request raised by the solver's refinement criterion.
 */
void exahype2::RefinementControl::addCommand(
  const tarch::la::Vector<DIMENSIONS, double>& x,
  const tarch::la::Vector<DIMENSIONS, double>& h,
  exahype2::RefinementCommand                  command
) {
  if (command == exahype2::RefinementCommand::Keep) {
    return;
  }

  const auto& service      = exahype2::RefinementControlService::getInstance();
  const auto& domainOffset = service.getDomainOffset();
  const auto& domainWidth  = service.getDomainWidth();

  const auto newEvent = peano4::grid::GridControlEvent::forCell(
    command == exahype2::RefinementCommand::Refine ? peano4::grid::GridControlEvent::RefinementControl::Refine
                                                   : peano4::grid::GridControlEvent::RefinementControl::Erase,
    x,
    h,
    domainOffset,
    domainWidth
  );

  _newEvents.push_back(NewEvent{newEvent, _solverNumber});

  logDebug(
    "Added instruction for x=" << x << ", h=" << h << ": " << newEvent.toString() << " (total of " << _newEvents.size()
                               << " instructions)"
  );
}

bool exahype2::RefinementControl::NewEvent::operator<(const NewEvent& other) const noexcept {
  if (event < other.event) {
    return true;
  }
  if (other.event < event) {
    return false;
  }
  return solverNumber < other.solverNumber;
}

bool exahype2::RefinementControl::NewEvent::operator==(const NewEvent& other) const noexcept {
  return event == other.event and solverNumber == other.solverNumber;
}

std::string exahype2::RefinementControl::toString() const {
  std::ostringstream msg;
  msg << "RefinementControl{"
      << "#Events=" << _newEvents.size() << "}";
  return msg.str();
}
