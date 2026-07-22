// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "RefinementControl.h"

#include "tarch/instrumentation/Log.h"

exahype2::RefinementCommand exahype2::getDefaultRefinementCommand() { return RefinementCommand::Erase; }

/**
 * Create a container for the requests of one solver.
 *
 * @param solverNumber Identifies the solver whose criterion fills this container. The
 *   service resolves coarsening as a consensus across all solvers, so every erase
 *   request carries its author.
 */
exahype2::RefinementControl::RefinementControl(int solverNumber) :
  _solverNumber(solverNumber) {}

/**
 * Drop the events collected so far.
 *
 * Only this container's events are affected; the events the service has already
 * committed survive. An action set calls this on its own local assembly once the
 * service has merged it. The service exposes no counterpart, as it re-delivers a
 * committed event for the traversal that realises the topology change and drops
 * it afterwards, so there is no user-managed lifetime to reset.
 */
void exahype2::RefinementControl::clear() { _newEvents.clear(); }

/**
 * Add a refine, keep or erase request for one cell.
 *
 * A refine command becomes a 3h box centred on the cell, i.e. the requester plus one
 * full cell of every neighbour, asking for a third of the cell size. An erase command
 * becomes an event covering exactly the cell and asking for three times its size, which
 * RefinementControlService::finaliseCoarseningEvents() promotes to the cell's parent
 * once the whole sibling group agrees.
 *
 * @param x       Centre of the cell for which this command is triggered. This is
 *                typically marker.getX().
 * @param h       Size of this cell. Typically marker.getH().
 * @param command Request raised by the solver's refinement criterion.
 */
void exahype2::RefinementControl::addCommand(
  const tarch::la::Vector<DIMENSIONS, double>& x,
  const tarch::la::Vector<DIMENSIONS, double>& h,
  exahype2::RefinementCommand                  command
) {
  addCommand(x, h, command, NewEvent::RealisationPhase::TriggerTopologyChange);
}

void exahype2::RefinementControl::addCommand(
  const tarch::la::Vector<DIMENSIONS, double>& x,
  const tarch::la::Vector<DIMENSIONS, double>& h,
  exahype2::RefinementCommand                  command,
  NewEvent::RealisationPhase                   phase
) {
  switch (command) {
  case exahype2::RefinementCommand::Refine: {
    // The event box deliberately covers the requester and one full cell of every
    // neighbour: with the positive-volume overlap test in
    // Spacetree::evaluateGridControlEvents this refines a protective band around the
    // requester. A moving feature then stays inside its refined region for several
    // steps, the re-committed merged event sets stay identical, and the service
    // suppresses the mesh-update drains in between; without the band nearly every step
    // commits a one-cell change and pays a full drain. The full-cell width keeps the
    // boxes cell-aligned, so neighbouring requesters fuse into few large events.
    const tarch::la::Vector<DIMENSIONS, double> targetH = 1.0 / 3.0 * h;

    peano4::grid::GridControlEvent newEvent;
    newEvent.setRefinementControl(peano4::grid::GridControlEvent::RefinementControl::Refine);
    newEvent.offset  = x - 1.5 * h;
    newEvent.width   = 3.0 * h;
    newEvent.targetH = targetH;

    assertion(newEvent.width[0] > 0.0, newEvent.toString());
    assertion(newEvent.targetH[0] > 0.0, newEvent.toString());

    _newEvents.push_back(NewEvent{newEvent, phase, _solverNumber});

    logDebug(
      "Added refinement for x=" << x << ", h=" << h << ": " << newEvent.toString() << " (total of " << _newEvents.size()
                                << " instructions)"
    );
  } break;
  case exahype2::RefinementCommand::Keep:
    break;
  case exahype2::RefinementCommand::Erase: {
    // Coarsening is finalised by RefinementControlService as a sibling-group decision: we
    // emit a leaf-sized erase request, and the service promotes it to a parent-sized event
    // once the whole 3^d sibling group requests it. This keeps a single smooth fine patch
    // from erasing its parent while siblings still carry the feature.
    const tarch::la::Vector<DIMENSIONS, double> targetH = 3.0 * h;

    peano4::grid::GridControlEvent newEvent;
    newEvent.setRefinementControl(peano4::grid::GridControlEvent::RefinementControl::Erase);
    newEvent.offset  = x - 0.5 * h;
    newEvent.width   = h;
    newEvent.targetH = targetH;

    assertion(newEvent.width[0] > 0.0, newEvent.toString());
    assertion(newEvent.targetH[0] > 0.0, newEvent.toString());

    _newEvents.push_back(NewEvent{newEvent, phase, _solverNumber});

    logDebug(
      "Added erase for x=" << x << ", h=" << h << ": " << newEvent.toString() << " (total of " << _newEvents.size()
                           << " instructions)"
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
