// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org

#include "TracingAPI.h"

#include "Database.h" // needed for defined(AssignmentChecks) macro and ::internal namespace


#if defined(AssignmentChecks)

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
  const tarch::la::Vector<DIMENSIONS, double>& particleX,
  const int                                    particleID,
  bool                                         isLocal,
  const tarch::la::Vector<DIMENSIONS, double>& vertexX,
  const tarch::la::Vector<DIMENSIONS, double>& vertexH,
  int                                          treeId,
  const std::string&                           trace
) {


  using namespace internal;

  Database& database = Database::getInstance();

  auto identifier = ParticleIdentifier(
    particleName,
    particleX,
    vertexX,
    vertexH,
    particleID,
    treeId
  );

  Event event(Event::Type::Erase, isLocal, treeId, trace);
  Event previousEvent = database.addEvent(identifier, event);


  assertion(
    previousEvent.type == internal::Event::Type::DetachFromVertex,
    identifier.toString(),
    event.toString(),
    previousEvent.toString(),
    treeId,
    database.particleHistory(identifier)
  );

}


void toolbox::particles::assignmentchecks::assignParticleToVertex(
  const std::string&                           particleName,
  const tarch::la::Vector<DIMENSIONS, double>& particleX,
  const int                                    particleID,
  bool                                         isLocal,
  const tarch::la::Vector<DIMENSIONS, double>& vertexX,
  const tarch::la::Vector<DIMENSIONS, double>& vertexH,
  int                                          treeId,
  const std::string&                           trace,
  const tarch::la::Vector<DIMENSIONS, double>& prevVertexX,
  const tarch::la::Vector<DIMENSIONS, double>& prevVertexH,
  bool                                         particleIsNew,
  bool reassignmentOnSameTreeDepthAllowed
) {



  using namespace internal;

  constexpr bool checkNewParticles = false;
  Database&      database          = Database::getInstance();

  auto vertexIndNew   = ParticleIdentifier::getVertexInd(vertexX, vertexH);
  int  vertexDepthNew = ParticleIdentifier::getVertexDepth(vertexX, vertexH);
  auto vertexIndOld   = ParticleIdentifier::getVertexInd(
    prevVertexX,
    prevVertexH
  );
  int vertexDepthOld = ParticleIdentifier::getVertexDepth(
    prevVertexX,
    prevVertexH
  );

  ParticleIdentifier identifier = ParticleIdentifier(
    particleName,
    particleX,
    vertexIndOld,
    vertexDepthOld,
    particleID,
    treeId
  );
  if (particleIsNew) {
    identifier = ParticleIdentifier(
      particleName,
      particleX,
      vertexIndNew,
      vertexDepthOld,
      particleID,
      treeId
    );
  }

  Event event(
    Event::Type::AssignToVertex,
    isLocal,
    vertexX,
    particleX,
    vertexH,
    treeId,
    trace
  );

  Event previousEvent = database.addEvent(identifier, event);

  if (not(particleIsNew and (not checkNewParticles))) {
    // skip the newly added event.

    [[maybe_unused]] const bool isDropping = previousEvent.type
                              == internal::Event::Type::DetachFromVertex
                            and tarch::la::
                              allGreater(previousEvent.vertexH, vertexH);
    [[maybe_unused]] const bool isLifting = previousEvent.type
                             == internal::Event::Type::DetachFromVertex
                           and tarch::la::
                             allSmaller(previousEvent.vertexH, vertexH);
    [[maybe_unused]] const bool isDroppingFromSieveSet = previousEvent.type
                                        == internal::Event::Type::
                                          AssignToSieveSet;
    [[maybe_unused]] const bool isDetached
      = reassignmentOnSameTreeDepthAllowed
          ? (previousEvent.type == internal::Event::Type::DetachFromVertex)
          : (
            previousEvent.type == internal::Event::Type::DetachFromVertex
            and not previousEvent.isLocal
          );

    if (isLocal) {
      assertion(
        (previousEvent.type == internal::Event::Type::NotFound
        ) or (previousEvent.type == internal::Event::Type::Erase)
          or isDroppingFromSieveSet or (isLifting and previousEvent.isLocal)
          or (isDropping and previousEvent.isLocal) or (isDetached),
        identifier.toString(),
        event.toString(),
        previousEvent.toString(),
        treeId,
        database.getNumberOfSnapshots(),
        trace,
        database.particleHistory(identifier)
      );
    } else {
      assertion(
          (previousEvent.type == internal::Event::Type::NotFound)
          or
          (previousEvent.type == internal::Event::Type::Erase)
          or
          isDroppingFromSieveSet
          or
          (isDropping and not previousEvent.isLocal)
          or
          (previousEvent.type == internal::Event::Type::DetachFromVertex and previousEvent.isLocal),
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

  // Do we need to shift the identifier?

  bool shift = false;

  for (int i = 0; i < DIMENSIONS; i++) {
    if (vertexIndNew(i) != vertexIndOld(i)) {
      shift = true;
      break;
    }
  }

  if (vertexDepthNew != vertexDepthOld)
    shift = true;

  if (shift) {
    database.shiftIdentifierVertexInd(identifier, vertexIndNew, vertexDepthNew);
  }

}

void toolbox::particles::assignmentchecks::moveParticle(
  const std::string&                           particleName,
  const tarch::la::Vector<DIMENSIONS, double>& oldParticleX,
  const tarch::la::Vector<DIMENSIONS, double>& newParticleX,
  const int                                    particleID,
  const tarch::la::Vector<DIMENSIONS, double>& vertexX,
  const tarch::la::Vector<DIMENSIONS, double>& vertexH,
  int                                          treeId,
  const std::string&                           trace
) {


  using namespace internal;
  Database& database = Database::getInstance();

  // If we break this, the particle search will fail.
  // TODO: This may not fail any more, as we don't use the floating point based
  // fuzzy search in the map any more. However, needs to be checked. For now, it
  // won't harm to keep this.
  [[maybe_unused]] tarch::la::Vector dx = tarch::la::abs(oldParticleX - newParticleX);
  assertion(
    tarch::la::allSmaller(dx, tarch::la::max(vertexH)),
    "particle moved more than vertex size",
    oldParticleX,
    newParticleX,
    dx,
    vertexH
  );

  // use old particle position to find history.
  ParticleIdentifier identifier = ParticleIdentifier(
    particleName,
    oldParticleX,
    vertexX,
    vertexH,
    particleID,
    treeId
  );

  ParticleEvents history = database.getParticleHistory(identifier);
  // We assume that we can't be moving a particle without having done anything
  // else first.
  assertion(history.size() > 0);

  Event previousEvent = database.getPreviousEvent(history, treeId);

  // Can't be sieve event, particle needs to have been assigned to vertex
  // first
  assertion(
    previousEvent.type == Event::Type::AssignToVertex
      or previousEvent.type == Event::Type::MoveWhileAssociatedToVertex
      or previousEvent.type
           == Event::Type::ConsecutiveMoveWhileAssociatedToVertex,
    "Invalid previous event type",
    previousEvent.toString(),
    database.particleHistory(identifier)
  );

  assertion(
    tarch::la::equals(previousEvent.vertexH, vertexH),
    "Vertices not the same",
    previousEvent.vertexH,
    vertexH,
    treeId,
    database.particleHistory(identifier)
  );
  assertion(
    tarch::la::equals(previousEvent.vertexX, vertexX),
    "Vertices not the same",
    previousEvent.vertexX,
    vertexX,
    treeId,
    database.particleHistory(identifier)
  );

  // Now actually write the move event down.
  if ((previousEvent.type == Event::Type::MoveWhileAssociatedToVertex) or (previousEvent.type == Event::Type::ConsecutiveMoveWhileAssociatedToVertex)){

    // Either way, the event we'll write down is a consecutive move
    Event newEvent = Event(
      Event::Type::ConsecutiveMoveWhileAssociatedToVertex,
      previousEvent.isLocal,
      vertexX,
      previousEvent.previousParticleX,
      vertexH,
      treeId,
      previousEvent.trace,
      -1 // will be modified in database.addEvent
    );

    if (previousEvent.type == Event::Type::MoveWhileAssociatedToVertex) {
      // Rename the trace.
      std::ostringstream pastTrace;
      pastTrace
        << "Consecutive Move starting at x=["
        << ::toString(previousEvent.previousParticleX) << "] at sweep "
        << database.getMeshSweepData().at(previousEvent.meshSweepIndex).getName()
        << " | old trace: " << previousEvent.trace << " | new trace: " + trace;
      newEvent.trace = pastTrace.str();
    }

    // And replace old event now.
    database.replaceEvent(identifier, previousEvent, newEvent);

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

}


void toolbox::particles::assignmentchecks::detachParticleFromVertex(
  const std::string&                           particleName,
  const tarch::la::Vector<DIMENSIONS, double>& particleX,
  const int                                    particleID,
  bool                                         isLocal,
  const tarch::la::Vector<DIMENSIONS, double>& vertexX,
  const tarch::la::Vector<DIMENSIONS, double>& vertexH,
  int                                          treeId,
  const std::string&                           trace
) {



  using namespace internal;
  Database& database = Database::getInstance();

  ParticleIdentifier identifier = ParticleIdentifier(
    particleName,
    particleX,
    vertexX,
    vertexH,
    particleID,
    treeId
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


  Event previousEvent = database.addEvent(identifier, event);

  assertion(
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
    trace,
    database.toString()
  );
  assertion(
    tarch::la::equals(previousEvent.vertexX, vertexX),
    identifier.toString(),
    event.toString(),
    previousEvent.toString(),
    treeId,
    database.getNumberOfSnapshots(),
    trace,
    database.particleHistory(identifier)
  );
  assertion(
    tarch::la::equals(previousEvent.vertexH, vertexH),
    identifier.toString(),
    event.toString(),
    previousEvent.toString(),
    treeId,
    trace,
    database.particleHistory(identifier)
  );

}

void toolbox::particles::assignmentchecks::assignParticleToSieveSet(
  const std::string&                           particleName,
  const tarch::la::Vector<DIMENSIONS, double>& particleX,
  const int                                    particleID,
  bool                                         isLocal,
  const tarch::la::Vector<DIMENSIONS, double>& vertexX,
  const tarch::la::Vector<DIMENSIONS, double>& vertexH,
  int                                          treeId,
  const std::string&                           trace
) {



  using namespace internal;
  Database& database = Database::getInstance();

  ParticleIdentifier identifier = ParticleIdentifier(
    particleName,
    particleX,
    vertexX,
    vertexH,
    particleID,
    treeId
  );


  Event event{internal::Event::Type::AssignToSieveSet, isLocal, treeId, trace};

  Event previousEvent = database.addEvent(identifier, event);

  assertion(
    previousEvent.type == internal::Event::Type::DetachFromVertex,
    identifier.toString(),
    event.toString(),
    previousEvent.toString(),
    treeId,
    database.particleHistory(identifier)
  );

}

void toolbox::particles::assignmentchecks::eliminateExistingParticles() {

  internal::Database& database = internal::Database::getInstance();
  database.eliminateExistingParticles();
}

void toolbox::particles::assignmentchecks::ensureDatabaseIsEmpty() {

  internal::Database& database = internal::Database::getInstance();

  // During a reset or initialization, we always add the "initial"
  // mesh sweep, so the database should have at least 1.
  if (database.getNumberOfSnapshots() != 1) {
    logInfo("database still holds " << database.getNumberOfSnapshots() << " snapshots");
    logError(database.toString());
  }
}

#else

void toolbox::particles::assignmentchecks::startMeshSweep(
  const std::string& meshSweepName
) {}

void toolbox::particles::assignmentchecks::eraseParticle(
  const std::string&                           particleName,
  const tarch::la::Vector<DIMENSIONS, double>& particleX,
  const int                                    particleID,
  bool                                         isLocal,
  const tarch::la::Vector<DIMENSIONS, double>& vertexX,
  const tarch::la::Vector<DIMENSIONS, double>& vertexH,
  int                                          treeId,
  const std::string&                           trace
) {}

void toolbox::particles::assignmentchecks::assignParticleToVertex(
  const std::string&                           particleName,
  const tarch::la::Vector<DIMENSIONS, double>& particleX,
  const int                                    particleID,
  bool                                         isLocal,
  const tarch::la::Vector<DIMENSIONS, double>& vertexX,
  const tarch::la::Vector<DIMENSIONS, double>& vertexH,
  int                                          treeId,
  const std::string&                           trace,
  const tarch::la::Vector<DIMENSIONS, double>& prevVertexX,
  const tarch::la::Vector<DIMENSIONS, double>& prevVertexH,
  bool                                         particleIsNew,
  bool reassignmentOnSameTreeDepthAllowed
) {}

void toolbox::particles::assignmentchecks::detachParticleFromVertex(
  const std::string&                           particleName,
  const tarch::la::Vector<DIMENSIONS, double>& particleX,
  const int                                    particleID,
  bool                                         isLocal,
  const tarch::la::Vector<DIMENSIONS, double>& vertexX,
  const tarch::la::Vector<DIMENSIONS, double>& vertexH,
  int                                          treeId,
  const std::string&                           trace
) {}

void toolbox::particles::assignmentchecks::assignParticleToSieveSet(
  const std::string&                           particleName,
  const tarch::la::Vector<DIMENSIONS, double>& particleX,
  const int                                    particleID,
  bool                                         isLocal,
  const tarch::la::Vector<DIMENSIONS, double>& vertexX,
  const tarch::la::Vector<DIMENSIONS, double>& vertexH,
  int                                          treeId,
  const std::string&                           trace
) {}

void toolbox::particles::assignmentchecks::moveParticle(
  const std::string&                           particleName,
  const tarch::la::Vector<DIMENSIONS, double>& oldParticleX,
  const tarch::la::Vector<DIMENSIONS, double>& newParticleX,
  const int                                    particleID,
  const tarch::la::Vector<DIMENSIONS, double>& vertexX,
  const tarch::la::Vector<DIMENSIONS, double>& vertexH,
  int                                          treeId,
  const std::string&                           trace
) {}

void toolbox::particles::assignmentchecks::ensureDatabaseIsEmpty() {}

#endif
