// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org

#include "TracingAPI.h"

#include "Database.h" // needed for defined(AssignmentChecks) macro and ::internal namespace


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

std::cerr << std::setprecision(16) << "TRACINGAPI:startMeshSweep," << meshSweepName << std::endl;

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

std::cerr << std::setprecision(16) << "TRACINGAPI:eraseParticle," <<
  particleName << "," << particleX << "," <<  particleID << "," <<  isLocal << "," <<  vertexH << "," <<  treeId << "," <<  trace
  << std::endl;

  logTraceInWith5Arguments(
    "eraseParticle(...)",
    particleName,
    particleX,
    particleID,
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
  Event previousEvent = database.addEvent(identifier, event, treeId);


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

std::cerr << std::setprecision(16) << "TRACINGAPI:assignParticleToVertex," <<
  particleName << "," <<  particleX << "," <<  particleID << "," <<  isLocal << "," <<
  vertexX  << "," << vertexH << "," <<  treeId << "," <<  trace << "," <<  particleIsNew << "," << reassignmentOnSameTreeDepthAllowed
  << std::endl;




  logTraceInWith7Arguments(
    "assignParticleToVertex(...)",
    particleName,
    particleID,
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

  Event previousEvent = database.addEvent(identifier, event, treeId);

  if (not(particleIsNew and (not checkNewParticles))) {
    // skip the newly added event.

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

    if (isLocal) {
      assertion7(
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
      assertion7(
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

std::cerr << std::setprecision(16) << "TRACINGAPI:moveParticle," <<
  particleName  << "," <<  oldParticleX << "," << newParticleX << "," <<  particleID << "," <<  vertexX << "," <<  vertexH << "," <<  treeId << "," <<  trace
  << std::endl;

  logTraceInWith5Arguments(
    "moveParticle(...)",
    particleName,
    particleID,
    oldParticleX,
    newParticleX,
    treeId
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
  );

  // use old particle position to find history.
  ParticleSearchIdentifier identifier = ParticleSearchIdentifier(
    particleName,
    oldParticleX,
    particleID,
    tarch::la::max(vertexH)
  );

  ParticleEvents history = database.getParticleHistory(identifier);
  // We assume that we can't be moving a particle without having done anything
  // else first.
  assertion(history.size() > 0);

  Event previousEvent = database.getPreviousEvent(history, treeId);

if (particleID == 2325473) {
  std::cout << "==================== IN MOVE: Previous Event" << previousEvent.toString() << "\n";
  std::cout << "==================== IN MOVE: ID=" << particleID << " dx=" << newParticleX - oldParticleX << "\n";
  std::cout << database.toString();
  std::cout << std::endl;
}

  // Can't be sieve event, particle needs to have been assigned to vertex
  // first
  assertion4(
    previousEvent.type == Event::Type::AssignToVertex
      or previousEvent.type == Event::Type::MoveWhileAssociatedToVertex
      or previousEvent.type == Event::Type::ConsecutiveMoveWhileAssociatedToVertex,
    "Invalid previous event type",
    previousEvent.toString(),
    database.particleHistory(identifier),
    database.toString()
  );

  assertion5(
    tarch::la::equals(previousEvent.vertexH, vertexH),
    "Vertices not the same",
    previousEvent.vertexH,
    vertexH,
    treeId,
    database.particleHistory(identifier)
  );
  assertion5(
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

    database.addEvent(identifier, newEvent, treeId);
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


std::cerr << std::setprecision(16) << "TRACINGAPI:detachParticleFromVertex," <<
  particleName << "," <<  particleX << "," <<  particleID << "," << isLocal << "," <<  vertexX << "," <<  vertexH << "," <<  treeId << "," <<  trace
  << std::endl;

  logTraceInWith7Arguments(
    "detachParticleFromVertex(...)",
    particleName,
    particleID,
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


  Event previousEvent = database.addEvent(identifier, event, treeId);

  assertion7(
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


std::cerr << std::setprecision(16) << "TRACINGAPI:assignParticleToSieveSet," <<
  particleName << "," <<  particleX << "," <<  particleID << "," << isLocal << "," <<  vertexH << "," <<  treeId << "," <<  trace
  << std::endl;

  logTraceInWith5Arguments(
    "assignParticleToSieveSet(...)",
    particleName,
    particleID,
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


  Event event{internal::Event::Type::AssignToSieveSet, isLocal, treeId, trace};

  Event previousEvent = database.addEvent(identifier, event, treeId);

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

void toolbox::particles::assignmentchecks::eliminateExistingParticles() {

std::cerr << std::setprecision(16) << "TRACINGAPI:eliminateExistingParticles"
  << std::endl;


  internal::Database& database = internal::Database::getInstance();
  database.eliminateExistingParticles();
}

void toolbox::particles::assignmentchecks::ensureDatabaseIsEmpty() {

std::cerr << std::setprecision(16) << "TRACINGAPI:ensureDatabaseIsEmpty"
  << std::endl;


  internal::Database& database = internal::Database::getInstance();

  // During a reset or initialization, we always add the "initial"
  // mesh sweep, so the database should have at least 1.
  if (database.getNumberOfSnapshots() != 1) {
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
