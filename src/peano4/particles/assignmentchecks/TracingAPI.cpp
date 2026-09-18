// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "TracingAPI.h"

#include "peano4/particles/assignmentchecks/Database.h" // Defines ASSIGNMENT_CHECKS
#include "tarch/instrumentation/Error.h"
#include "tarch/instrumentation/Info.h"

#if defined(ASSIGNMENT_CHECKS)

std::string peano4::particles::assignmentchecks::sweepHistory() {
  return peano4::particles::assignmentchecks::Database::getInstance().sweepHistory();
}

void peano4::particles::assignmentchecks::startMeshSweep(const std::string& meshSweepName) {
  auto& database = peano4::particles::assignmentchecks::Database::getInstance();
  database.startMeshSweep(meshSweepName);
}

/**
 * Records the removal of a virtual particle. The caller detaches the particle
 * from its vertex before it calls this function.
 */
void peano4::particles::assignmentchecks::eraseParticle(
  const std::string&                           particleName,
  const tarch::la::Vector<DIMENSIONS, double>& particleX,
  const int                                    particleId,
  bool                                         isLocal,
  const tarch::la::Vector<DIMENSIONS, double>& vertexX,
  const tarch::la::Vector<DIMENSIONS, double>& vertexH,
  int                                          treeId,
  const std::string&                           trace
) {
  auto& database = peano4::particles::assignmentchecks::Database::getInstance();

  auto identifier = peano4::particles::assignmentchecks::ParticleIdentifier(
    particleName,
    particleX,
    vertexX,
    vertexH,
    particleId,
    treeId
  );

  peano4::particles::assignmentchecks::Event
    event(peano4::particles::assignmentchecks::Event::Type::Erase, isLocal, treeId, trace);
  const peano4::particles::assignmentchecks::Event previousEvent = database.addEvent(identifier, event);

  assertion(
    previousEvent.type == peano4::particles::assignmentchecks::Event::Type::DetachFromVertex,
    identifier.toString(),
    event.toString(),
    previousEvent.toString(),
    treeId,
    database.particleHistory(identifier)
  );
}

/**
 * Records the assignment of a particle to the vertex at vertexX and checks it
 * against the previous event of the particle. The key of the history holds the
 * vertex of the previous assignment, so the caller passes that vertex in
 * prevVertexX and prevVertexH, and passes any value once particleIsNew is set.
 * reassignmentOnSameTreeDepthAllowed accepts a move from one vertex to another
 * vertex of the same depth.
 */
void peano4::particles::assignmentchecks::assignParticleToVertex(
  const std::string&                           particleName,
  const tarch::la::Vector<DIMENSIONS, double>& particleX,
  const int                                    particleId,
  bool                                         isLocal,
  const tarch::la::Vector<DIMENSIONS, double>& vertexX,
  const tarch::la::Vector<DIMENSIONS, double>& vertexH,
  int                                          treeId,
  const std::string&                           trace,
  const tarch::la::Vector<DIMENSIONS, double>& prevVertexX,
  const tarch::la::Vector<DIMENSIONS, double>& prevVertexH,
  bool                                         particleIsNew,
  bool                                         reassignmentOnSameTreeDepthAllowed
) {
  constexpr bool CheckNewParticles = false;
  auto&          database          = peano4::particles::assignmentchecks::Database::getInstance();

  const auto vertexIndexNew = peano4::particles::assignmentchecks::ParticleIdentifier::getVertexIndex(vertexX, vertexH);
  const int  vertexDepthNew = peano4::particles::assignmentchecks::ParticleIdentifier::getVertexDepth(vertexX, vertexH);
  const auto vertexIndexOld =
    peano4::particles::assignmentchecks::ParticleIdentifier::getVertexIndex(prevVertexX, prevVertexH);
  const int vertexDepthOld =
    peano4::particles::assignmentchecks::ParticleIdentifier::getVertexDepth(prevVertexX, prevVertexH);

  peano4::particles::assignmentchecks::ParticleIdentifier identifier =
    peano4::particles::assignmentchecks::ParticleIdentifier(
      particleName,
      particleX,
      vertexIndexOld,
      vertexDepthOld,
      particleId,
      treeId
    );
  if (particleIsNew) {
    identifier = peano4::particles::assignmentchecks::ParticleIdentifier(
      particleName,
      particleX,
      vertexIndexNew,
      vertexDepthOld,
      particleId,
      treeId
    );
  }

  peano4::particles::assignmentchecks::Event event(
    peano4::particles::assignmentchecks::Event::Type::AssignToVertex,
    isLocal,
    vertexX,
    particleX,
    vertexH,
    treeId,
    trace
  );

  const peano4::particles::assignmentchecks::Event previousEvent = database.addEvent(identifier, event);

  if (not(particleIsNew and (not CheckNewParticles))) {
    [[maybe_unused]] const bool isDropping =
      previousEvent.type == peano4::particles::assignmentchecks::Event::Type::DetachFromVertex
      and tarch::la::allGreater(previousEvent.vertexH, vertexH);
    [[maybe_unused]] const bool isLifting =
      previousEvent.type == peano4::particles::assignmentchecks::Event::Type::DetachFromVertex
      and tarch::la::allSmaller(previousEvent.vertexH, vertexH);
    [[maybe_unused]] const bool isDroppingFromSieveSet =
      previousEvent.type == peano4::particles::assignmentchecks::Event::Type::AssignToSieveSet;
    [[maybe_unused]] const bool isDetached = reassignmentOnSameTreeDepthAllowed
      ? (previousEvent.type == peano4::particles::assignmentchecks::Event::Type::DetachFromVertex)
      : (previousEvent.type == peano4::particles::assignmentchecks::Event::Type::DetachFromVertex
         and not previousEvent.isLocal);

    if (isLocal) {
      assertion(
        (previousEvent.type == peano4::particles::assignmentchecks::Event::Type::NotFound)
          or (previousEvent.type == peano4::particles::assignmentchecks::Event::Type::Erase) or isDroppingFromSieveSet
          or (isLifting and previousEvent.isLocal) or (isDropping and previousEvent.isLocal) or (isDetached),
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
        (previousEvent.type == peano4::particles::assignmentchecks::Event::Type::NotFound)
          or (previousEvent.type == peano4::particles::assignmentchecks::Event::Type::Erase) or isDroppingFromSieveSet
          or (isDropping and not previousEvent.isLocal)
          or (previousEvent.type == peano4::particles::assignmentchecks::Event::Type::DetachFromVertex and previousEvent.isLocal),
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

  bool shift = false;

  for (int i = 0; i < DIMENSIONS; i++) {
    if (vertexIndexNew(i) != vertexIndexOld(i)) {
      shift = true;
      break;
    }
  }

  if (vertexDepthNew != vertexDepthOld) {
    shift = true;
  }

  if (shift) {
    database.shiftIdentifierVertexIndex(identifier, vertexIndexNew, vertexDepthNew);
  }
}

/**
 * Records the movement of a particle from oldParticleX to newParticleX. A
 * sequence of movements of one particle along one vertex collapses into a
 * single event of the type ConsecutiveMoveWhileAssociatedToVertex, which keeps
 * the position from before the first movement of that sequence.
 */
void peano4::particles::assignmentchecks::moveParticle(
  const std::string&                           particleName,
  const tarch::la::Vector<DIMENSIONS, double>& oldParticleX,
  const tarch::la::Vector<DIMENSIONS, double>& newParticleX,
  const int                                    particleId,
  const tarch::la::Vector<DIMENSIONS, double>& vertexX,
  const tarch::la::Vector<DIMENSIONS, double>& vertexH,
  int                                          treeId,
  const std::string&                           trace
) {
  auto& database = peano4::particles::assignmentchecks::Database::getInstance();

  [[maybe_unused]] const auto dx                        = tarch::la::abs(oldParticleX - newParticleX);
  [[maybe_unused]] const bool particleStaysInVertexCell = tarch::la::allSmaller(dx, tarch::la::max(vertexH));
  assertion(
    particleStaysInVertexCell,
    "The particle moved further than the width of its vertex.",
    oldParticleX,
    newParticleX,
    dx,
    vertexH
  );

  // The key of the history holds the position from before the movement.
  const peano4::particles::assignmentchecks::ParticleIdentifier identifier =
    peano4::particles::assignmentchecks::ParticleIdentifier(
      particleName,
      oldParticleX,
      vertexX,
      vertexH,
      particleId,
      treeId
    );

  const peano4::particles::assignmentchecks::ParticleEvents history = database.getParticleHistory(identifier);
  assertion(not history.empty());

  const peano4::particles::assignmentchecks::Event previousEvent = database.getPreviousEvent(history, treeId);

  assertion(
    previousEvent.type == peano4::particles::assignmentchecks::Event::Type::AssignToVertex
      or previousEvent.type == peano4::particles::assignmentchecks::Event::Type::MoveWhileAssociatedToVertex
      or previousEvent.type == peano4::particles::assignmentchecks::Event::Type::ConsecutiveMoveWhileAssociatedToVertex,
    "The previous event neither assigns the particle to a vertex nor moves it.",
    previousEvent.toString(),
    database.particleHistory(identifier)
  );

  [[maybe_unused]] const bool vertexHOfPreviousEventMatches = tarch::la::equals(previousEvent.vertexH, vertexH);
  assertion(
    vertexHOfPreviousEventMatches,
    "The vertex width of the previous event differs from vertexH.",
    previousEvent.vertexH,
    vertexH,
    treeId,
    database.particleHistory(identifier)
  );
  [[maybe_unused]] const bool vertexXOfPreviousEventMatches = tarch::la::equals(previousEvent.vertexX, vertexX);
  assertion(
    vertexXOfPreviousEventMatches,
    "The vertex position of the previous event differs from vertexX.",
    previousEvent.vertexX,
    vertexX,
    treeId,
    database.particleHistory(identifier)
  );

  if ((previousEvent.type == peano4::particles::assignmentchecks::Event::Type::MoveWhileAssociatedToVertex)
      or (previousEvent.type == peano4::particles::assignmentchecks::Event::Type::ConsecutiveMoveWhileAssociatedToVertex)) {
    peano4::particles::assignmentchecks::Event newEvent = peano4::particles::assignmentchecks::Event(
      peano4::particles::assignmentchecks::Event::Type::ConsecutiveMoveWhileAssociatedToVertex,
      previousEvent.isLocal,
      vertexX,
      previousEvent.previousParticleX,
      vertexH,
      treeId,
      previousEvent.trace,
      -1 // replaceEvent() overwrites the mesh sweep index
    );

    if (previousEvent.type == peano4::particles::assignmentchecks::Event::Type::MoveWhileAssociatedToVertex) {
      std::ostringstream pastTrace;
      pastTrace << "Consecutive move starting at x=[" << ::toString(previousEvent.previousParticleX) << "] at sweep "
                << database.getMeshSweepData().at(previousEvent.meshSweepIndex).getName()
                << " | old trace: " << previousEvent.trace << " | new trace: " << trace;
      newEvent.trace = pastTrace.str();
    }

    database.replaceEvent(identifier, previousEvent, newEvent);
  } else {
    peano4::particles::assignmentchecks::Event newEvent = peano4::particles::assignmentchecks::Event(
      peano4::particles::assignmentchecks::Event::Type::MoveWhileAssociatedToVertex,
      vertexX,
      oldParticleX,
      vertexH,
      treeId,
      trace
    );

    database.addEvent(identifier, newEvent);
  }
}

/**
 * Records the removal of a particle from the vertex at vertexX. The caller
 * assigns the particle to that vertex before it calls this function, and a
 * second virtual copy of one particle on one vertex passes as well.
 */
void peano4::particles::assignmentchecks::detachParticleFromVertex(
  const std::string&                           particleName,
  const tarch::la::Vector<DIMENSIONS, double>& particleX,
  const int                                    particleId,
  bool                                         isLocal,
  const tarch::la::Vector<DIMENSIONS, double>& vertexX,
  const tarch::la::Vector<DIMENSIONS, double>& vertexH,
  int                                          treeId,
  const std::string&                           trace
) {
  auto& database = peano4::particles::assignmentchecks::Database::getInstance();

  const peano4::particles::assignmentchecks::ParticleIdentifier identifier =
    peano4::particles::assignmentchecks::ParticleIdentifier(
      particleName,
      particleX,
      vertexX,
      vertexH,
      particleId,
      treeId
    );

  peano4::particles::assignmentchecks::Event event(
    peano4::particles::assignmentchecks::Event::Type::DetachFromVertex,
    isLocal,
    vertexX,
    particleX,
    vertexH,
    treeId,
    trace
  );

  const peano4::particles::assignmentchecks::Event previousEvent = database.addEvent(identifier, event);

  const bool particleWasAssignedToVertex =
    previousEvent.type == peano4::particles::assignmentchecks::Event::Type::AssignToVertex
    or previousEvent.type == peano4::particles::assignmentchecks::Event::Type::MoveWhileAssociatedToVertex
    or previousEvent.type == peano4::particles::assignmentchecks::Event::Type::ConsecutiveMoveWhileAssociatedToVertex;
  const bool repeatedVirtualParticleCleanup = not isLocal
    and previousEvent.type == peano4::particles::assignmentchecks::Event::Type::Erase and not previousEvent.isLocal;

  assertion(
    particleWasAssignedToVertex or repeatedVirtualParticleCleanup,
    identifier.toString(),
    event.toString(),
    previousEvent.toString(),
    treeId,
    database.getNumberOfSnapshots(),
    trace,
    database.toString()
  );
  [[maybe_unused]] const bool vertexXOfPreviousEventMatches =
    repeatedVirtualParticleCleanup or tarch::la::equals(previousEvent.vertexX, vertexX);
  assertion(
    vertexXOfPreviousEventMatches,
    identifier.toString(),
    event.toString(),
    previousEvent.toString(),
    treeId,
    database.getNumberOfSnapshots(),
    trace,
    database.particleHistory(identifier)
  );
  [[maybe_unused]] const bool vertexHOfPreviousEventMatches =
    repeatedVirtualParticleCleanup or tarch::la::equals(previousEvent.vertexH, vertexH);
  assertion(
    vertexHOfPreviousEventMatches,
    identifier.toString(),
    event.toString(),
    previousEvent.toString(),
    treeId,
    trace,
    database.particleHistory(identifier)
  );
}

/**
 * Records the assignment of a particle to the sieve set. The caller detaches
 * the particle from its vertex before it calls this function.
 */
void peano4::particles::assignmentchecks::assignParticleToSieveSet(
  const std::string&                           particleName,
  const tarch::la::Vector<DIMENSIONS, double>& particleX,
  const int                                    particleId,
  bool                                         isLocal,
  const tarch::la::Vector<DIMENSIONS, double>& vertexX,
  const tarch::la::Vector<DIMENSIONS, double>& vertexH,
  int                                          treeId,
  const std::string&                           trace
) {
  auto& database = peano4::particles::assignmentchecks::Database::getInstance();

  const peano4::particles::assignmentchecks::ParticleIdentifier identifier =
    peano4::particles::assignmentchecks::ParticleIdentifier(
      particleName,
      particleX,
      vertexX,
      vertexH,
      particleId,
      treeId
    );

  peano4::particles::assignmentchecks::Event event{
    peano4::particles::assignmentchecks::Event::Type::AssignToSieveSet,
    isLocal,
    treeId,
    trace
  };

  const peano4::particles::assignmentchecks::Event previousEvent = database.addEvent(identifier, event);

  assertion(
    previousEvent.type == peano4::particles::assignmentchecks::Event::Type::DetachFromVertex,
    identifier.toString(),
    event.toString(),
    previousEvent.toString(),
    treeId,
    database.particleHistory(identifier)
  );
}

/**
 * Deletes every particle from the database that a vertex still holds, so the
 * database keeps the particles that the mesh has lost.
 */
void peano4::particles::assignmentchecks::eliminateExistingParticles() {
  auto& database = peano4::particles::assignmentchecks::Database::getInstance();
  database.eliminateExistingParticles();
}

/**
 * Writes the database to the error log if it holds more than the snapshot that
 * a reset creates. A caller runs eliminateExistingParticles() before, so the
 * log names the particles that the mesh has lost.
 */
void peano4::particles::assignmentchecks::checkDatabaseIsEmpty() {
  auto& database = peano4::particles::assignmentchecks::Database::getInstance();

  if (database.getNumberOfSnapshots() != 1) {
    logInfo("The database still holds " << database.getNumberOfSnapshots() << " snapshots");
    logError(database.toString());
  }
}

#else

void peano4::particles::assignmentchecks::startMeshSweep(const std::string& meshSweepName) {}

void peano4::particles::assignmentchecks::eraseParticle(
  const std::string&                           particleName,
  const tarch::la::Vector<DIMENSIONS, double>& particleX,
  const int                                    particleId,
  bool                                         isLocal,
  const tarch::la::Vector<DIMENSIONS, double>& vertexX,
  const tarch::la::Vector<DIMENSIONS, double>& vertexH,
  int                                          treeId,
  const std::string&                           trace
) {}

void peano4::particles::assignmentchecks::assignParticleToVertex(
  const std::string&                           particleName,
  const tarch::la::Vector<DIMENSIONS, double>& particleX,
  const int                                    particleId,
  bool                                         isLocal,
  const tarch::la::Vector<DIMENSIONS, double>& vertexX,
  const tarch::la::Vector<DIMENSIONS, double>& vertexH,
  int                                          treeId,
  const std::string&                           trace,
  const tarch::la::Vector<DIMENSIONS, double>& prevVertexX,
  const tarch::la::Vector<DIMENSIONS, double>& prevVertexH,
  bool                                         particleIsNew,
  bool                                         reassignmentOnSameTreeDepthAllowed
) {}

void peano4::particles::assignmentchecks::detachParticleFromVertex(
  const std::string&                           particleName,
  const tarch::la::Vector<DIMENSIONS, double>& particleX,
  const int                                    particleId,
  bool                                         isLocal,
  const tarch::la::Vector<DIMENSIONS, double>& vertexX,
  const tarch::la::Vector<DIMENSIONS, double>& vertexH,
  int                                          treeId,
  const std::string&                           trace
) {}

void peano4::particles::assignmentchecks::assignParticleToSieveSet(
  const std::string&                           particleName,
  const tarch::la::Vector<DIMENSIONS, double>& particleX,
  const int                                    particleId,
  bool                                         isLocal,
  const tarch::la::Vector<DIMENSIONS, double>& vertexX,
  const tarch::la::Vector<DIMENSIONS, double>& vertexH,
  int                                          treeId,
  const std::string&                           trace
) {}

void peano4::particles::assignmentchecks::moveParticle(
  const std::string&                           particleName,
  const tarch::la::Vector<DIMENSIONS, double>& oldParticleX,
  const tarch::la::Vector<DIMENSIONS, double>& newParticleX,
  const int                                    particleId,
  const tarch::la::Vector<DIMENSIONS, double>& vertexX,
  const tarch::la::Vector<DIMENSIONS, double>& vertexH,
  int                                          treeId,
  const std::string&                           trace
) {}

void peano4::particles::assignmentchecks::checkDatabaseIsEmpty() {}

#endif
