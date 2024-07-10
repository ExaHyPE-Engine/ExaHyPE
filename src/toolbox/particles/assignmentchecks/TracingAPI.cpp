// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org

#include "TracingAPI.h"

#include "Database.h" // needed for defined(AssignmentChecks) macro and ::internal namespace
#include "tarch/la/VectorVectorOperations.h"


#if defined(AssignmentChecks)

namespace {
  tarch::logging::Log _log("toolbox::particles::assignmentchecks");
} // namespace


std::string toolbox::particles::assignmentchecks::sweepHistory() {
  return internal::Database::getInstance().sweepHistory();
}

void toolbox::particles::assignmentchecks::startMeshSweep(
  const std::string& meshSweepName
) {
  internal::Database& d = internal::Database::getInstance();
  d.startMeshSweep(meshSweepName);
}

void toolbox::particles::assignmentchecks::eraseParticle(
  const std::string&                           particleName,
  const tarch::la::Vector<Dimensions, double>& particleX,
  const int                                    particleID,
  bool                                         isLocal,
  const tarch::la::Vector<Dimensions, double>& vertexH,
  int                                          treeId,
  const std::string&                           trace
) {

  logTraceInWith4Arguments(
    "eraseParticle(...)",
    particleName,
    particleX,
    isLocal,
    treeId
  );

  using namespace internal;

  Database& database = Database::getInstance();

  ParticleSearchIdentifier identifier = ParticleSearchIdentifier(
    particleName,
    particleX,
    particleID,
    tarch::la::max(vertexH)
  );

  Event event(Event::Type::Erase, isLocal, treeId, trace);

  ParticleEvents& history       = database.addEvent(identifier, event);
  Event           previousEvent = getPreviousEvent(history, treeId, 1);


  assertion5(
    previousEvent.type == internal::Event::Type::DetachFromVertex,
    identifier.toString(),
    event.toString(),
    previousEvent.toString(),
    treeId,
    database.particleHistory(identifier)
  );

  logTraceOut("eraseParticle(...)");
}


void toolbox::particles::assignmentchecks::assignParticleToVertex(
  const std::string&                           particleName,
  const tarch::la::Vector<Dimensions, double>& particleX,
  const int                                    particleID,

  bool                                         isLocal,
  const tarch::la::Vector<Dimensions, double>& vertexX,
  const tarch::la::Vector<Dimensions, double>& vertexH,
  int                                          treeId,
  const std::string&                           trace,
  bool                                         particleIsNew,
  bool reassignmentOnSameTreeDepthAllowed
) {

  logTraceInWith6Arguments(
    "assignParticleToVertex(...)",
    particleName,
    particleX,
    isLocal,
    vertexX,
    vertexH,
    treeId
  );

  using namespace internal;

  constexpr bool checkNewParticles = false;
  Database&      database          = Database::getInstance();

  ParticleSearchIdentifier identifier = ParticleSearchIdentifier(
    particleName,
    particleX,
    particleID,
    tarch::la::max(vertexH)
  );

  Event event(
    Event::Type::AssignToVertex,
    isLocal,
    vertexX,
    particleX,
    vertexH,
    treeId,
    trace
  );

  ParticleEvents& history = database.addEvent(identifier, event);

  if (not(particleIsNew and (not checkNewParticles))) {
    // skip the newly added event.
    Event previousEvent = getPreviousEvent(history, treeId, 1);

    const bool isDropping = previousEvent.type
                              == internal::Event::Type::DetachFromVertex
                            and tarch::la::
                              allGreater(previousEvent.vertexH, vertexH);
    const bool isLifting = previousEvent.type
                             == internal::Event::Type::DetachFromVertex
                           and tarch::la::
                             allSmaller(previousEvent.vertexH, vertexH);
    const bool isDroppingFromSieveSet = previousEvent.type
                                        == internal::Event::Type::
                                          AssignToSieveSet;

    const bool isDetached
      = reassignmentOnSameTreeDepthAllowed
          ? (previousEvent.type == internal::Event::Type::DetachFromVertex)
          : (
            previousEvent.type == internal::Event::Type::DetachFromVertex
            and not previousEvent.isLocal
          );

    const bool virtualPartThatHasBeenErased =
        (previousEvent.type == internal::Event::Type::Erase and (not previousEvent.isLocal));

    if (isLocal) {
      assertion7(
        previousEvent.type == internal::Event::Type::NotFound
          or isDroppingFromSieveSet or (isLifting and previousEvent.isLocal)
          or (isDropping and previousEvent.isLocal) or (isDetached)
          or virtualPartThatHasBeenErased,
          identifier.toString(),
        event.toString(),
        previousEvent.toString(),
        treeId,
        database.getNumberOfSnapshots(),
        trace,
        database.particleHistory(identifier)
      );
    } else {
      assertion7(
          (previousEvent.type == internal::Event::Type::NotFound)
          or
          isDroppingFromSieveSet
          or
          (isDropping and not previousEvent.isLocal)
          or
          (previousEvent.type == internal::Event::Type::DetachFromVertex and previousEvent.isLocal)
          or virtualPartThatHasBeenErased,
          identifier.toString(),
          event.toString(),
          previousEvent.toString(),
          treeId,
          database.getNumberOfSnapshots(),
          trace,
          database.particleHistory(identifier)
        );
    }
  }

  logTraceOut("assignParticleToVertex(...)");
}

void toolbox::particles::assignmentchecks::moveParticle(
  const std::string&                           particleName,
  const tarch::la::Vector<Dimensions, double>& oldParticleX,
  const tarch::la::Vector<Dimensions, double>& newParticleX,
  const int                                    particleID,
  const tarch::la::Vector<Dimensions, double>& vertexX,
  const tarch::la::Vector<Dimensions, double>& vertexH,
  int                                          treeId,
  const std::string&                           trace
) {

  logTraceInWith3Arguments(
    "moveParticle(...)",
    particleName,
    oldParticleX,
    newParticleX
  );

  using namespace internal;
  Database& database = Database::getInstance();

  // If we break this, the particle search will fail.
  tarch::la::Vector dx = tarch::la::abs(oldParticleX - newParticleX);
  assertion5(
      tarch::la::allSmaller(dx, tarch::la::max(vertexH)),
      "particle moved more than vertex size",
      oldParticleX,
      newParticleX,
      dx,
      vertexH
      )

  // use old particle position to find history.
  ParticleSearchIdentifier identifier = ParticleSearchIdentifier(
    particleName,
    oldParticleX,
    particleID,
    tarch::la::max(vertexH)
  );

  ParticleEvents& history = database.getParticleHistory(identifier);
  // We assume that we can't be moving a particle without having done anything
  // else first.
  assert(history.size() > 0);

  Event previousEvent    = getPreviousEvent(history, treeId);
  Event anyPreviousEvent = getPreviousEvent(history, Database::AnyTree);

  // We need to have at least 1 event somewhere.
  // Even if the database had been trimmed, at least 1 event remains. But the
  // remaining event may have been on a different tree.
  assertion(anyPreviousEvent.type != internal::Event::Type::NotFound);

  // We must still be on the same vertex, or something is wrong.
  assertion3(
    tarch::la::equals(anyPreviousEvent.vertexH, vertexH),
    "Vertices not the same",
    anyPreviousEvent.vertexH,
    vertexH
  );
  assertion3(
    tarch::la::equals(anyPreviousEvent.vertexX, vertexX),
    "Vertices not the same",
    anyPreviousEvent.vertexX,
    vertexX
  );

  if (previousEvent.type != Event::Type::NotFound) {
    // This must be on the same tree then.
    // We can apply stricter checks.

    // Can't be sieve event, particle needs to have been assigned to vertex first
    assertion(
      previousEvent.type == Event::Type::AssignToVertex
      or previousEvent.type == Event::Type::MoveWhileAssociatedToVertex
      or previousEvent.type
           == Event::Type::ConsecutiveMoveWhileAssociatedToVertex
    );

    // We must still be on the same vertex, or something is wrong.
    assertion3(
      tarch::la::equals(previousEvent.vertexH, vertexH),
      "Vertices not the same",
      previousEvent.vertexH,
      vertexH
    );
    assertion3(
      tarch::la::equals(previousEvent.vertexX, vertexX),
      "Vertices not the same",
      previousEvent.vertexX,
      vertexX
    );
    assertion3(
      previousEvent.treeId == treeId,
      "Tree not the same",
      previousEvent.treeId,
      treeId
    );
  }

  if ((previousEvent.type == Event::Type::MoveWhileAssociatedToVertex) or (previousEvent.type == Event::Type::ConsecutiveMoveWhileAssociatedToVertex)){

    Event newEvent = Event(Event::Type::NotFound);

    if (previousEvent.type == Event::Type::MoveWhileAssociatedToVertex) {
      // we turn it into a ConsecutiveMoveWhileAssociatedToVertex and
      // store the current particle position.
      std::ostringstream pastTrace;
      pastTrace
        << "Consecutive Move starting at x=[" <<
        ::toString(previousEvent.previousParticleX) << "] at sweep " <<
        database.getMeshSweepData().at(previousEvent.meshSweepIndex).getName()
        << " | old trace: " << previousEvent.trace;
      " | new trace: " + trace;

      // Create new event.
      newEvent = Event(
        Event::Type::ConsecutiveMoveWhileAssociatedToVertex,
        previousEvent.isLocal,
        vertexX,
        oldParticleX,
        vertexH,
        treeId,
        pastTrace.str(),
        -1 // will be modified in database.addEvent
      );
    } else {
      newEvent = Event(
        Event::Type::ConsecutiveMoveWhileAssociatedToVertex,
        previousEvent.isLocal,
        vertexX,
        previousEvent.previousParticleX,
        vertexH,
        treeId,
        previousEvent.trace,
        -1 // will be modified in database.addEvent
      );
    }

    // In either case, delete old event from history and then re-add it so that
    // all the mechanisms in addEvent (shortening history, modifying
    // identifier coordinates) trigger.
    for (auto it = history.begin(); it != history.end(); it++) {
      if ((it->meshSweepIndex == previousEvent.meshSweepIndex)
        and (it->type == previousEvent.type)
          and (it->treeId == previousEvent.treeId)
          and (it->isLocal == previousEvent.isLocal)){
        history.erase(it);
        break;
      }
    }

    // Add new event now.
    database.addEvent(identifier, newEvent);
  } else {
    // Add a new move event.
    Event newEvent = Event(
      Event::Type::MoveWhileAssociatedToVertex,
      vertexX,
      oldParticleX,
      vertexH,
      treeId,
      trace
    );

    database.addEvent(identifier, newEvent);
  }

  // Finally, since the particle moved, do we need to modify the coordinates
  // of the identifier? We need to use the coordinates as well to ensure the
  // correct particle identity. If the particle has moved too far, we need
  // to update that information.
  database.shiftIdentifierCoordinates(identifier, newParticleX);

  logTraceOut("moveParticle(...)");
}


void toolbox::particles::assignmentchecks::detachParticleFromVertex(
  const std::string&                           particleName,
  const tarch::la::Vector<Dimensions, double>& particleX,
  const int                                    particleID,
  bool                                         isLocal,
  const tarch::la::Vector<Dimensions, double>& vertexX,
  const tarch::la::Vector<Dimensions, double>& vertexH,
  int                                          treeId,
  const std::string&                           trace
) {

  logTraceInWith6Arguments(
    "detachParticleFromVertex(...)",
    particleName,
    particleX,
    isLocal,
    vertexX,
    vertexH,
    treeId
  );

  using namespace internal;
  Database& database = Database::getInstance();

  ParticleSearchIdentifier identifier = ParticleSearchIdentifier(
    particleName,
    particleX,
    particleID,
    tarch::la::max(vertexH)
  );

  Event event(
    Event::Type::DetachFromVertex,
    isLocal,
    vertexX,
    particleX,
    vertexH,
    treeId,
    trace
  );

  ParticleEvents& history = Database::getInstance().addEvent(identifier, event);
  Event           previousEvent = getPreviousEvent(history, treeId, 1);


  assertion6(
    previousEvent.type == internal::Event::Type::AssignToVertex
      or previousEvent.type
           == internal::Event::Type::MoveWhileAssociatedToVertex
      or previousEvent.type
           == internal::Event::Type::ConsecutiveMoveWhileAssociatedToVertex,
    identifier.toString(),
    event.toString(),
    previousEvent.toString(),
    treeId,
    database.getNumberOfSnapshots(),
    trace
  );
  assertion7(
    tarch::la::equals(previousEvent.vertexX, vertexX),
    identifier.toString(),
    event.toString(),
    previousEvent.toString(),
    treeId,
    database.getNumberOfSnapshots(),
    trace,
    database.particleHistory(identifier)
  );
  assertion6(
    tarch::la::equals(previousEvent.vertexH, vertexH),
    identifier.toString(),
    event.toString(),
    previousEvent.toString(),
    treeId,
    trace,
    database.particleHistory(identifier)
  );

  logTraceOut("detachParticleFromVertex(...)");
}

void toolbox::particles::assignmentchecks::assignParticleToSieveSet(
  const std::string&                           particleName,
  const tarch::la::Vector<Dimensions, double>& particleX,
  const int                                    particleID,
  bool                                         isLocal,
  const tarch::la::Vector<Dimensions, double>& vertexH,
  int                                          treeId,
  const std::string&                           trace
) {

  logTraceInWith4Arguments(
    "assignParticleToSieveSet(...)",
    particleName,
    particleX,
    isLocal,
    treeId
  );

  logDebug(
    "assignParticleToSieveSet()",
    "assign " << particleName << " particle at " << particleX << " to global sieve set on tree " << treeId
  );


  using namespace internal;
  Database& database = Database::getInstance();

  ParticleSearchIdentifier identifier = ParticleSearchIdentifier(
    particleName,
    particleX,
    particleID,
    tarch::la::max(vertexH)
  );


  internal::Event
    event{internal::Event::Type::AssignToSieveSet, isLocal, treeId, trace};

  ParticleEvents& history       = database.addEvent(identifier, event);
  Event           previousEvent = getPreviousEvent(history, treeId, 1);

  assertion5(
    previousEvent.type == internal::Event::Type::DetachFromVertex,
    identifier.toString(),
    event.toString(),
    previousEvent.toString(),
    treeId,
    database.particleHistory(identifier)
  );

  logTraceOut("assignParticleToSieveSet(...)");
}


void toolbox::particles::assignmentchecks::ensureDatabaseIsEmpty() {

  internal::Database& database = internal::Database::getInstance();

  if (database.getNumberOfSnapshots() != 0) {
    logInfo(
      "ensureDatabaseIsEmpty()",
      "database still holds " << database.getNumberOfSnapshots() << " snapshots"
    );
    logError("ensureDatabaseIsEmpty()", database.toString());
  }
}

#else

void toolbox::particles::assignmentchecks::startMeshSweep(
  const std::string& meshSweepName
) {}

void toolbox::particles::assignmentchecks::eraseParticle(
  const std::string&                           particleName,
  const tarch::la::Vector<Dimensions, double>& particleX,
  const int                                    particleID,
  bool                                         isLocal,
  const tarch::la::Vector<Dimensions, double>& vertexH,
  int                                          treeId,
  const std::string&                           trace
) {}

void toolbox::particles::assignmentchecks::assignParticleToVertex(
  const std::string&                           particleName,
  const tarch::la::Vector<Dimensions, double>& particleX,
  const int                                    particleID,
  bool                                         isLocal,
  const tarch::la::Vector<Dimensions, double>& vertexX,
  const tarch::la::Vector<Dimensions, double>& vertexH,
  int                                          treeId,
  const std::string&                           trace,
  bool                                         particleIsNew,
  bool reassignmentOnSameTreeDepthAllowed
) {}

void toolbox::particles::assignmentchecks::detachParticleFromVertex(
  const std::string&                           particleName,
  const tarch::la::Vector<Dimensions, double>& particleX,
  const int                                    particleID,
  bool                                         isLocal,
  const tarch::la::Vector<Dimensions, double>& vertexX,
  const tarch::la::Vector<Dimensions, double>& vertexH,
  int                                          treeId,
  const std::string&                           trace
) {}

void toolbox::particles::assignmentchecks::assignParticleToSieveSet(
  const std::string&                           particleName,
  const tarch::la::Vector<Dimensions, double>& particleX,
  const int                                    particleID,
  bool                                         isLocal,
  const tarch::la::Vector<Dimensions, double>& vertexH,
  int                                          treeId,
  const std::string&                           trace
) {}

void toolbox::particles::assignmentchecks::moveParticle(
  const std::string&                           particleName,
  const tarch::la::Vector<Dimensions, double>& oldParticleX,
  const tarch::la::Vector<Dimensions, double>& newParticleX,
  const int                                    particleID,
  const tarch::la::Vector<Dimensions, double>& vertexX,
  const tarch::la::Vector<Dimensions, double>& vertexH,
  int                                          treeId,
  const std::string&                           trace
) {}

void toolbox::particles::assignmentchecks::ensureDatabaseIsEmpty() {}

#endif
