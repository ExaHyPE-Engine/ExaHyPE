// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org

#include "TracingAPI.h"

#include "tarch/instrumentation/Error.h"
#include "tarch/instrumentation/Info.h"
#include "toolbox/particles/assignmentchecks/Database.h" // Defines ASSIGNMENT_CHECKS and the namespace internal

#if defined(ASSIGNMENT_CHECKS)

std::string toolbox::particles::assignmentchecks::sweepHistory() {
  return internal::Database::getInstance().sweepHistory();
}

void toolbox::particles::assignmentchecks::startMeshSweep(const std::string& meshSweepName) {
  internal::Database& database = internal::Database::getInstance();
  database.startMeshSweep(meshSweepName);
}

/**
 * Records the removal of a virtual particle. The caller detaches the particle
 * from its vertex before it calls this function.
 */
void toolbox::particles::assignmentchecks::eraseParticle(
  const std::string&                           particleName,
  const tarch::la::Vector<DIMENSIONS, double>& particleX,
  const int                                    particleId,
  bool                                         isLocal,
  const tarch::la::Vector<DIMENSIONS, double>& vertexX,
  const tarch::la::Vector<DIMENSIONS, double>& vertexH,
  int                                          treeId,
  const std::string&                           trace
) {
  internal::Database& database = internal::Database::getInstance();

  auto identifier = internal::ParticleIdentifier(particleName, particleX, vertexX, vertexH, particleId, treeId);

  internal::Event event(internal::Event::Type::Erase, isLocal, treeId, trace);
  internal::Event previousEvent = database.addEvent(identifier, event);

  assertion(
    previousEvent.type == internal::Event::Type::DetachFromVertex,
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
void toolbox::particles::assignmentchecks::assignParticleToVertex(
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
  constexpr bool      CheckNewParticles = false;
  internal::Database& database          = internal::Database::getInstance();

  auto vertexIndNew   = internal::ParticleIdentifier::getVertexInd(vertexX, vertexH);
  int  vertexDepthNew = internal::ParticleIdentifier::getVertexDepth(vertexX, vertexH);
  auto vertexIndOld   = internal::ParticleIdentifier::getVertexInd(prevVertexX, prevVertexH);
  int  vertexDepthOld = internal::ParticleIdentifier::getVertexDepth(prevVertexX, prevVertexH);

  internal::ParticleIdentifier identifier =
    internal::ParticleIdentifier(particleName, particleX, vertexIndOld, vertexDepthOld, particleId, treeId);
  if (particleIsNew) {
    identifier =
      internal::ParticleIdentifier(particleName, particleX, vertexIndNew, vertexDepthOld, particleId, treeId);
  }

  internal::Event event(internal::Event::Type::AssignToVertex, isLocal, vertexX, particleX, vertexH, treeId, trace);

  internal::Event previousEvent = database.addEvent(identifier, event);

  if (not(particleIsNew and (not CheckNewParticles))) {
    [[maybe_unused]] const bool isDropping = previousEvent.type == internal::Event::Type::DetachFromVertex
      and tarch::la::allGreater(previousEvent.vertexH, vertexH);
    [[maybe_unused]] const bool isLifting = previousEvent.type == internal::Event::Type::DetachFromVertex
      and tarch::la::allSmaller(previousEvent.vertexH, vertexH);
    [[maybe_unused]] const bool isDroppingFromSieveSet = previousEvent.type == internal::Event::Type::AssignToSieveSet;
    [[maybe_unused]] const bool isDetached             = reassignmentOnSameTreeDepthAllowed
                  ? (previousEvent.type == internal::Event::Type::DetachFromVertex)
                  : (previousEvent.type == internal::Event::Type::DetachFromVertex and not previousEvent.isLocal);

    if (isLocal) {
      assertion(
        (previousEvent.type == internal::Event::Type::NotFound) or (previousEvent.type == internal::Event::Type::Erase)
          or isDroppingFromSieveSet or (isLifting and previousEvent.isLocal) or (isDropping and previousEvent.isLocal)
          or (isDetached),
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
        (previousEvent.type == internal::Event::Type::NotFound) or (previousEvent.type == internal::Event::Type::Erase)
          or isDroppingFromSieveSet or (isDropping and not previousEvent.isLocal)
          or (previousEvent.type == internal::Event::Type::DetachFromVertex and previousEvent.isLocal),
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
    if (vertexIndNew(i) != vertexIndOld(i)) {
      shift = true;
      break;
    }
  }

  if (vertexDepthNew != vertexDepthOld) {
    shift = true;
  }

  if (shift) {
    database.shiftIdentifierVertexInd(identifier, vertexIndNew, vertexDepthNew);
  }
}

/**
 * Records the movement of a particle from oldParticleX to newParticleX. A
 * sequence of movements of one particle along one vertex collapses into a
 * single event of the type ConsecutiveMoveWhileAssociatedToVertex, which keeps
 * the position from before the first movement of that sequence.
 */
void toolbox::particles::assignmentchecks::moveParticle(
  const std::string&                           particleName,
  const tarch::la::Vector<DIMENSIONS, double>& oldParticleX,
  const tarch::la::Vector<DIMENSIONS, double>& newParticleX,
  const int                                    particleId,
  const tarch::la::Vector<DIMENSIONS, double>& vertexX,
  const tarch::la::Vector<DIMENSIONS, double>& vertexH,
  int                                          treeId,
  const std::string&                           trace
) {
  internal::Database& database = internal::Database::getInstance();

  [[maybe_unused]] tarch::la::Vector dx = tarch::la::abs(oldParticleX - newParticleX);
  assertion(
    tarch::la::allSmaller(dx, tarch::la::max(vertexH)),
    "particle moved more than vertex size",
    oldParticleX,
    newParticleX,
    dx,
    vertexH
  );

  // The key of the history holds the position from before the movement.
  internal::ParticleIdentifier identifier =
    internal::ParticleIdentifier(particleName, oldParticleX, vertexX, vertexH, particleId, treeId);

  internal::ParticleEvents history = database.getParticleHistory(identifier);
  assertion(history.size() > 0);

  internal::Event previousEvent = database.getPreviousEvent(history, treeId);

  assertion(
    previousEvent.type == internal::Event::Type::AssignToVertex
      or previousEvent.type == internal::Event::Type::MoveWhileAssociatedToVertex
      or previousEvent.type == internal::Event::Type::ConsecutiveMoveWhileAssociatedToVertex,
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

  if ((previousEvent.type == internal::Event::Type::MoveWhileAssociatedToVertex)
      or (previousEvent.type == internal::Event::Type::ConsecutiveMoveWhileAssociatedToVertex)) {
    internal::Event newEvent = internal::Event(
      internal::Event::Type::ConsecutiveMoveWhileAssociatedToVertex,
      previousEvent.isLocal,
      vertexX,
      previousEvent.previousParticleX,
      vertexH,
      treeId,
      previousEvent.trace,
      -1 // replaceEvent() overwrites the mesh sweep index
    );

    if (previousEvent.type == internal::Event::Type::MoveWhileAssociatedToVertex) {
      std::ostringstream pastTrace;
      pastTrace << "Consecutive Move starting at x=[" << ::toString(previousEvent.previousParticleX) << "] at sweep "
                << database.getMeshSweepData().at(previousEvent.meshSweepIndex).getName()
                << " | old trace: " << previousEvent.trace << " | new trace: " + trace;
      newEvent.trace = pastTrace.str();
    }

    database.replaceEvent(identifier, previousEvent, newEvent);
  } else {
    internal::Event newEvent = internal::Event(
      internal::Event::Type::MoveWhileAssociatedToVertex,
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
void toolbox::particles::assignmentchecks::detachParticleFromVertex(
  const std::string&                           particleName,
  const tarch::la::Vector<DIMENSIONS, double>& particleX,
  const int                                    particleId,
  bool                                         isLocal,
  const tarch::la::Vector<DIMENSIONS, double>& vertexX,
  const tarch::la::Vector<DIMENSIONS, double>& vertexH,
  int                                          treeId,
  const std::string&                           trace
) {
  internal::Database& database = internal::Database::getInstance();

  internal::ParticleIdentifier identifier =
    internal::ParticleIdentifier(particleName, particleX, vertexX, vertexH, particleId, treeId);

  internal::Event event(internal::Event::Type::DetachFromVertex, isLocal, vertexX, particleX, vertexH, treeId, trace);

  internal::Event previousEvent = database.addEvent(identifier, event);

  const bool particleWasAssignedToVertex = previousEvent.type == internal::Event::Type::AssignToVertex
    or previousEvent.type == internal::Event::Type::MoveWhileAssociatedToVertex
    or previousEvent.type == internal::Event::Type::ConsecutiveMoveWhileAssociatedToVertex;
  const bool repeatedVirtualParticleCleanup =
    not isLocal and previousEvent.type == internal::Event::Type::Erase and not previousEvent.isLocal;

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
  assertion(
    repeatedVirtualParticleCleanup or tarch::la::equals(previousEvent.vertexX, vertexX),
    identifier.toString(),
    event.toString(),
    previousEvent.toString(),
    treeId,
    database.getNumberOfSnapshots(),
    trace,
    database.particleHistory(identifier)
  );
  assertion(
    repeatedVirtualParticleCleanup or tarch::la::equals(previousEvent.vertexH, vertexH),
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
void toolbox::particles::assignmentchecks::assignParticleToSieveSet(
  const std::string&                           particleName,
  const tarch::la::Vector<DIMENSIONS, double>& particleX,
  const int                                    particleId,
  bool                                         isLocal,
  const tarch::la::Vector<DIMENSIONS, double>& vertexX,
  const tarch::la::Vector<DIMENSIONS, double>& vertexH,
  int                                          treeId,
  const std::string&                           trace
) {
  internal::Database& database = internal::Database::getInstance();

  internal::ParticleIdentifier identifier =
    internal::ParticleIdentifier(particleName, particleX, vertexX, vertexH, particleId, treeId);

  internal::Event event{internal::Event::Type::AssignToSieveSet, isLocal, treeId, trace};

  internal::Event previousEvent = database.addEvent(identifier, event);

  assertion(
    previousEvent.type == internal::Event::Type::DetachFromVertex,
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
void toolbox::particles::assignmentchecks::eliminateExistingParticles() {
  internal::Database& database = internal::Database::getInstance();
  database.eliminateExistingParticles();
}

/**
 * Writes the database to the error log if it holds more than the snapshot that
 * a reset creates. A caller runs eliminateExistingParticles() before, so the
 * log names the particles that the mesh has lost.
 */
void toolbox::particles::assignmentchecks::checkDatabaseIsEmpty() {
  internal::Database& database = internal::Database::getInstance();

  if (database.getNumberOfSnapshots() != 1) {
    logInfo("database still holds " << database.getNumberOfSnapshots() << " snapshots");
    logError(database.toString());
  }
}

#else

void toolbox::particles::assignmentchecks::startMeshSweep(const std::string& meshSweepName) {}

void toolbox::particles::assignmentchecks::eraseParticle(
  const std::string&                           particleName,
  const tarch::la::Vector<DIMENSIONS, double>& particleX,
  const int                                    particleId,
  bool                                         isLocal,
  const tarch::la::Vector<DIMENSIONS, double>& vertexX,
  const tarch::la::Vector<DIMENSIONS, double>& vertexH,
  int                                          treeId,
  const std::string&                           trace
) {}

void toolbox::particles::assignmentchecks::assignParticleToVertex(
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

void toolbox::particles::assignmentchecks::detachParticleFromVertex(
  const std::string&                           particleName,
  const tarch::la::Vector<DIMENSIONS, double>& particleX,
  const int                                    particleId,
  bool                                         isLocal,
  const tarch::la::Vector<DIMENSIONS, double>& vertexX,
  const tarch::la::Vector<DIMENSIONS, double>& vertexH,
  int                                          treeId,
  const std::string&                           trace
) {}

void toolbox::particles::assignmentchecks::assignParticleToSieveSet(
  const std::string&                           particleName,
  const tarch::la::Vector<DIMENSIONS, double>& particleX,
  const int                                    particleId,
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
  const int                                    particleId,
  const tarch::la::Vector<DIMENSIONS, double>& vertexX,
  const tarch::la::Vector<DIMENSIONS, double>& vertexH,
  int                                          treeId,
  const std::string&                           trace
) {}

void toolbox::particles::assignmentchecks::checkDatabaseIsEmpty() {}

#endif
