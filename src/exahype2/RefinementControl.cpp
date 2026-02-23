// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "RefinementControl.h"

exahype2::RefinementCommand exahype2::getDefaultRefinementCommand() { return RefinementCommand::Erase; }

exahype2::RefinementCommand operator&&(exahype2::RefinementCommand lhs, exahype2::RefinementCommand rhs) {
  if (lhs == exahype2::RefinementCommand::Refine or rhs == exahype2::RefinementCommand::Refine) {
    return exahype2::RefinementCommand::Refine;
  } else if (lhs == exahype2::RefinementCommand::Keep or rhs == exahype2::RefinementCommand::Keep) {
    return exahype2::RefinementCommand::Keep;
  } else {
    return exahype2::RefinementCommand::Erase;
  }
}

std::string toString(exahype2::RefinementCommand value) {
  switch (value) {
  case exahype2::RefinementCommand::Refine:
    return "Refine";
  case exahype2::RefinementCommand::Keep:
    return "Keep";
  case exahype2::RefinementCommand::Erase:
    return "Erase";
  }
  return "<undef>";
}

tarch::logging::Log exahype2::RefinementControl::_log("exahype2::RefinementControl");

exahype2::RefinementControl::RefinementControl(double tolerance):
  _tolerance(tolerance) {}

void exahype2::RefinementControl::clear() { _newEvents.clear(); }

void exahype2::RefinementControl::addCommand(
  const tarch::la::Vector<DIMENSIONS, double>& x,
  const tarch::la::Vector<DIMENSIONS, double>& h,
  exahype2::RefinementCommand                  command,
  int                                          lifetime
) {
  assertion(lifetime >= 1);

  switch (command) {
  case exahype2::RefinementCommand::Refine: {
    tarch::la::Vector<DIMENSIONS, double> targetH       = (1.0 + _tolerance) * 1.0 / 3.0 * h;
    tarch::la::Vector<DIMENSIONS, double> expandedWidth = (1.0 + _tolerance) * h;

    peano4::grid::GridControlEvent newEvent;
    newEvent.setRefinementControl(peano4::grid::GridControlEvent::RefinementControl::Refine);
    newEvent.offset  = x - 0.5 * expandedWidth;
    newEvent.width   = expandedWidth;
    newEvent.targetH = targetH;

    assertion1(newEvent.width[0] > 0.0, newEvent.toString());
    assertion1(newEvent.targetH[0] > 0.0, newEvent.toString());

    _newEvents.push_back(std::pair<peano4::grid::GridControlEvent, int>(newEvent, lifetime));

    logDebug(
      "addCommand()",
      "Added refinement for x="
        << x << ", h=" << h << ": " << newEvent.toString() << " (total of " << _newEvents.size() << " instructions)"
    );
  } break;
  case exahype2::RefinementCommand::Keep:
    break;
  case exahype2::RefinementCommand::Erase: {
    tarch::la::Vector<DIMENSIONS, double> targetH = 3.0 * h;
    // Erase is evaluated on the parent cells via a containment check.
    // A leaf centre can be shifted by up to ±h from the parent centre,
    // so the event must be wider than the parent to always contain it:
    // width = targetH + 2*h (= 5*h);
    // parent half-width = targetH/2 = 1.5h;
    // max centre shift leaf↔parent = h;
    // required event half-width = 1.5h + h = 2.5h;
    // required event width = 5h = targetH + 2h.
    tarch::la::Vector<DIMENSIONS, double> expandedWidth = (1.0 + _tolerance) * (targetH + 2.0 * h);

    peano4::grid::GridControlEvent newEvent;
    newEvent.setRefinementControl(peano4::grid::GridControlEvent::RefinementControl::Erase);
    newEvent.offset  = x - 0.5 * expandedWidth;
    newEvent.width   = expandedWidth;
    newEvent.targetH = targetH;

    assertion1(newEvent.width[0] > 0.0, newEvent.toString());
    assertion1(newEvent.targetH[0] > 0.0, newEvent.toString());

    _newEvents.push_back(std::pair<peano4::grid::GridControlEvent, int>(newEvent, lifetime));

    logDebug(
      "addCommand()",
      "Added erase for x="
        << x << ", h=" << h << ": " << newEvent.toString() << " (total of " << _newEvents.size() << " instructions)"
    );
  } break;
  }
}

std::string exahype2::RefinementControl::toString() const {
  std::ostringstream msg;
  msg << "RefinementControl{"
      << "#Events=" << _newEvents.size() << "}";
  return msg.str();
}
