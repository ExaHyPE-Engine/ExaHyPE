#include "TracingAPI.h"

#include <ranges>

#include "tarch/multicore/MultiReadSingleWriteLock.h"


#if !defined(AssignmentChecks) and !defined(noAssignmentChecks) and PeanoDebug>0
#define AssignmentChecks
#endif





namespace {
  tarch::logging::Log _log("toolbox::particles::assignmentchecks");

  toolbox::particles::assignmentchecks::internal::Database _database;
} // namespace


toolbox::particles::assignmentchecks::internal::ParticleIdentifier::ParticleIdentifier(const std::string& particleName__, const tarch::la::Vector<Dimensions, double>& particleX__):
  particleName(particleName__),
  particleX(particleX__) {}

double toolbox::particles::assignmentchecks::internal::ParticleIdentifier::Precision = 1e-6;

bool toolbox::particles::assignmentchecks::internal::ParticleIdentifier::numericalEquals(const ParticleIdentifier& rhs) const {
  return (particleName == rhs.particleName) and tarch::la::equals(particleX, rhs.particleX, Precision);
}

bool toolbox::particles::assignmentchecks::internal::ParticleIdentifier::operator<(const ParticleIdentifier& rhs) const {
  if ( *this==rhs ) {
    return false;
  }
  if (particleName < rhs.particleName) {
    return true;
  }
  if (particleName > rhs.particleName) {
    return false;
  }
  for (int d = 0; d < Dimensions; d++) {
    if (particleX(d)<rhs.particleX(d)) {
      return true;
    }
    if (particleX(d)>rhs.particleX(d)) {
      return false;
    }
  }
  assertion3( false, toString(), rhs.toString(), "cannot happen" );
  return false;
}


std::string toolbox::particles::assignmentchecks::internal::ParticleIdentifier::toString() const { return "(" + particleName + "," + ::toString(particleX) + ")"; }


toolbox::particles::assignmentchecks::internal::Event::Event(
  Type type_,
  bool isLocal_,
  const tarch::la::Vector<Dimensions,
  double>& vertexX_,
  const tarch::la::Vector<Dimensions, double>& vertexH_,
  int treeId_,
  const std::string& trace_
):
  type(type_),
  isLocal(isLocal_),
  vertexX(vertexX_),
  previousParticleX(0.0),
  vertexH(vertexH_),
  treeId(treeId_),
  trace(trace_) {
  assertion(type == Type::AssignToVertex or type == Type::DetachFromVertex);
}

toolbox::particles::assignmentchecks::internal::Event::Event(
  Type                type_,
  bool                isLocal_,
  int                 treeId_,
  const std::string&  trace_
):
  type(type_),
  isLocal(isLocal_),
  vertexX(tarch::la::Vector<Dimensions, double>(0.0)),
  previousParticleX(0.0),
  vertexH(tarch::la::Vector<Dimensions, double>(0.0)),
  treeId(treeId_),
  trace(trace_) {
  assertion(type == Type::AssignToSieveSet or type == Type::Erase);
}

toolbox::particles::assignmentchecks::internal::Event::Event(Type type_):
  type(type_),
  isLocal(false),
  vertexX(tarch::la::Vector<Dimensions, double>(0.0)),
  previousParticleX(0.0),
  vertexH(tarch::la::Vector<Dimensions, double>(0.0)),
  treeId(-1),
  trace( "no-trace" ){
  assertion(type == Type::NotFound);
}

toolbox::particles::assignmentchecks::internal::Event::Event(
  Type                                          type_,
  const tarch::la::Vector<Dimensions, double>&  previousVertexX_,
  int                                           treeId_,
  const std::string&                            trace_
):
  type(type_),
  isLocal(true),
  vertexX(tarch::la::Vector<Dimensions, double>(0.0)),
  previousParticleX(previousVertexX_),
  vertexH(tarch::la::Vector<Dimensions, double>(0.0)),
  treeId(treeId_),
  trace(trace_) {
  assertion(type == Type::MoveWhileAssociatedToVertex);
}

std::string toolbox::particles::assignmentchecks::internal::Event::toString() const {
  std::ostringstream msg;

  msg << "(";
  switch (type) {
  case Type::AssignToSieveSet:
    msg << "assign-to-sieve-set" << ",local=" << isLocal << ",tree=" << treeId << ",trace=" << trace;
    break;
  case Type::AssignToVertex:
    msg << "assign-to-vertex" << ",local=" << isLocal << ",x=" << vertexX << ",h=" << vertexH << ",tree=" << treeId << ",trace=" << trace;
    break;
  case Type::Erase:
    msg << "erase" << ",local=" << isLocal << ",tree=" << treeId << ",trace=" << trace;
    break;
  case Type::DetachFromVertex:
    msg << "detach-from-vertex" << ",local=" << isLocal << ",x=" << vertexX << ",h=" << vertexH << ",tree=" << treeId << ",trace=" << trace;
    break;
  case Type::NotFound:
    msg << "not-found";
    break;
  case Type::MoveWhileAssociatedToVertex:
    msg << "moved-while-associated-to-vertex" << "," << previousParticleX << "->x_new,tree=" << treeId << ",trace=" << trace;
    break;
  };
  msg << ")";

  return msg.str();
}


toolbox::particles::assignmentchecks::internal::MeshSweepData::MeshSweepData(const std::string& meshSweepName):
  _meshSweepName(meshSweepName) {}


std::string toolbox::particles::assignmentchecks::internal::MeshSweepData::getName() const {
  return _meshSweepName;
}


toolbox::particles::assignmentchecks::internal::ParticleIdentifier toolbox::particles::assignmentchecks::internal::Database::createParticleIdentifier(
  const std::string&                            particleName,
  const tarch::la::Vector<Dimensions, double>&  particleX,
  double                                        searchTolerance
) {
  ParticleIdentifier result(particleName, particleX);

  if (searchTolerance>0.0) {
    tarch::multicore::MultiReadSingleWriteLock lock(_semaphore,tarch::multicore::MultiReadSingleWriteLock::Read);
    auto currentSnapshot = _data.crbegin();
    while (currentSnapshot != _data.crend()) {
      for (const auto& eventsForOneParticle: *currentSnapshot) {
        // use floating-point aware comparison operator
        if (
          eventsForOneParticle.first.particleName==result.particleName
          and
          tarch::la::equals(eventsForOneParticle.first.particleX, result.particleX, searchTolerance)
        ) {
          logDebug(
            "createParticleIdentifier()",
            "found entry for " << particleX << " given tolerance of " << searchTolerance << ": will copy data over bit-wisely which biases identifier by "
            << (eventsForOneParticle.first.particleX - result.particleX)
          );
          // This is a bit-wise copy and biases the result towards an
          // existing entry.
          result = eventsForOneParticle.first;
          assertion(currentSnapshot->count(result) > 0);
          return result;
        }
      }
      currentSnapshot++;
    }
  }

  return result;
}


toolbox::particles::assignmentchecks::internal::Database::Database( int maxParticleSnapshotsToKeepTrackOf ):
  _maxParticleSnapshotsToKeepTrackOf( maxParticleSnapshotsToKeepTrackOf ) {
  _data.push_back(MeshSweepData("initial"));
};


void toolbox::particles::assignmentchecks::internal::Database::startMeshSweep(const std::string& meshSweepName) {
  logInfo("startMeshSweep()", "finish old mesh sweep with " << _data.rbegin()->size() << " event(s) and start new one for " << meshSweepName );
  tarch::multicore::MultiReadSingleWriteLock lock(_semaphore,tarch::multicore::MultiReadSingleWriteLock::Write);
  _data.push_back(MeshSweepData(meshSweepName));
};


int toolbox::particles::assignmentchecks::internal::Database::getNumberOfSnapshots() const {
  return _data.size()-1;
}



void toolbox::particles::assignmentchecks::internal::Database::eliminateExistingParticles() {
  tarch::multicore::MultiReadSingleWriteLock lock(_semaphore,tarch::multicore::MultiReadSingleWriteLock::Write);

  bool hasEliminated = true;

  while (hasEliminated) {
    removeEmptyDatabaseSnapshots();

    hasEliminated        = false;
    auto lastSnapshot = _data.rbegin();

    auto particleTrajectory = lastSnapshot->begin();
    while (particleTrajectory != lastSnapshot->end() and not hasEliminated) {
      if (
        particleTrajectory->second.back().type == Event::Type::MoveWhileAssociatedToVertex
        or
        particleTrajectory->second.back().type == Event::Type::AssignToVertex
        or
        not particleTrajectory->second.back().isLocal
      ) {
        removeTrajectory(
          particleTrajectory->first,
          particleTrajectory->second.back().treeId
        );
        hasEliminated = true;
      }
      else {
        particleTrajectory++;
      }
    }

    if (lastSnapshot->empty()) {
      _data.pop_back();
      hasEliminated = true;
    }
  }
}


void toolbox::particles::assignmentchecks::internal::Database::removeEmptyDatabaseSnapshots() {
  auto snapshot = _data.begin();
  int currentSnapshot = 0;
  while (snapshot!=_data.end()) {
    auto trajectory = snapshot->begin();
    while (trajectory!=snapshot->end()) {
      if (trajectory->second.empty()) {
        logDebug( "removeEmptyDatabaseSnapshots()", "removed entry for particle " << trajectory->first.toString() );
        trajectory = snapshot->erase(trajectory);
      }
      else {
        trajectory++;
      }
    }
    snapshot++;
  }


  snapshot = _data.begin();
  const int size = _data.size();
  while (snapshot!=_data.end()) {
    if (snapshot->empty() and currentSnapshot<size-1) {
      logDebug( "removeEmptyDatabaseSnapshots()", "removed whole snapshot as it was empty" );
      snapshot = _data.erase(snapshot);
    }
    else {
      snapshot++;
    }
    currentSnapshot++;
  }
}


void toolbox::particles::assignmentchecks::internal::Database::removeTrajectory(
  const ParticleIdentifier&  identifier,
  int                        spacetreeId,
  int                        firstNRecentEntriesToSkip
) {
  assertion( spacetreeId>=0 );

  auto currentSnapshot = _data.rbegin();
  std::advance( currentSnapshot, firstNRecentEntriesToSkip );

  while (currentSnapshot != _data.rend()) {
    MeshSweepData& meshSweepData = *currentSnapshot;

    if (meshSweepData.count(identifier) > 0) {
      auto historyEventIterator = meshSweepData.at(identifier).rbegin();
      while (historyEventIterator != meshSweepData.at(identifier).rend()) {
        if (
          historyEventIterator->treeId == spacetreeId
          and
          historyEventIterator->type == Event::Type::MoveWhileAssociatedToVertex
        ) {
          ParticleIdentifier previousIdentifier = identifier;
          previousIdentifier.particleX = historyEventIterator->previousParticleX;
          logDebug( "removeTrajectory(...)", "first erase historic data of " << previousIdentifier.toString() << " due to " << historyEventIterator->toString() );
          removeTrajectory(previousIdentifier, spacetreeId, firstNRecentEntriesToSkip);
        }
        historyEventIterator++;
      }

      auto forwardEventIterator = meshSweepData.at(identifier).begin();
      while (forwardEventIterator != meshSweepData.at(identifier).end()) {
        if ( forwardEventIterator->treeId == spacetreeId ) {
          logDebug( "removeTrajectory(...)", "erase event " << forwardEventIterator->toString() );
          forwardEventIterator = meshSweepData[identifier].erase(forwardEventIterator);
        }
        else {
          forwardEventIterator++;
        }
      }
    }
    currentSnapshot++;
    firstNRecentEntriesToSkip++;
  }
}


std::pair< toolbox::particles::assignmentchecks::internal::Event, toolbox::particles::assignmentchecks::internal::ParticleIdentifier >  toolbox::particles::assignmentchecks::internal::Database::getEntry(
  const ParticleIdentifier&  identifier,
  int                        spacetreeId,
  int                        firstNRecentEntriesToSkip
) {
  tarch::multicore::MultiReadSingleWriteLock lock(_semaphore,tarch::multicore::MultiReadSingleWriteLock::Read);

  auto currentSnapshot = _data.crbegin();

  std::advance( currentSnapshot, firstNRecentEntriesToSkip );

  while (currentSnapshot != _data.crend()) {
    const MeshSweepData& meshSweepData = *currentSnapshot;

    if (meshSweepData.count(identifier) > 0) {
      auto event = meshSweepData.at(identifier).crbegin();
      while (event != meshSweepData.at(identifier).crend()) {
        bool treeIsAFit = event->treeId == spacetreeId
                       or spacetreeId==AnyTree;
        if (
          event->type == Event::Type::Erase
          and
          treeIsAFit
        ) {
          return { Event(Event::Type::NotFound), identifier };
        }
        else if (
          event->type == Event::Type::MoveWhileAssociatedToVertex
          and
          treeIsAFit
          and
          firstNRecentEntriesToSkip!=DoNotFollowParticleMovementsInDatabase
        ) {
          const ParticleIdentifier previousIdentifier = _database.createParticleIdentifier(identifier.particleName, event->previousParticleX);
          assertion3(
            not (previousIdentifier==identifier),
            previousIdentifier.toString(),
            identifier.toString(),
            event->toString()
          );
          logDebug( "getEntry()", "rerun with " << previousIdentifier.toString() << " distilled from " << identifier.toString() << " on iteration " << firstNRecentEntriesToSkip );
          return getEntry(previousIdentifier, spacetreeId, firstNRecentEntriesToSkip);
        }
        else if (event->type != Event::Type::Erase and treeIsAFit) {
          return { *event, identifier };
        }
        event++;
      }
    }
    currentSnapshot++;
    firstNRecentEntriesToSkip++;
  }

  return { Event(Event::Type::NotFound), identifier };
}


std::string toolbox::particles::assignmentchecks::internal::Database::toString() {
  tarch::multicore::MultiReadSingleWriteLock lock(_semaphore,tarch::multicore::MultiReadSingleWriteLock::Read);

  std::ostringstream msg;

  int  snapshotCounter   = 0;
  for (auto& snapshot: _data) {
    msg << std::endl << "sweep #" << snapshotCounter << " (" << snapshot.getName() << "):";
    for (const auto& identifier: snapshot) {
      msg << std::endl << "- " << identifier.first.toString() << ": ";
      bool firstEntry = false;
      for (const auto& event : identifier.second) {
        if (firstEntry) {
          firstEntry = true;
        }
        else {
          msg << "->";
        }
        msg << event.toString();
      }
    }
    snapshotCounter++;
  }

  return msg.str();
}


int toolbox::particles::assignmentchecks::internal::Database::totalEntries(const ParticleIdentifier& identifier) {
  tarch::multicore::MultiReadSingleWriteLock lock(_semaphore,tarch::multicore::MultiReadSingleWriteLock::Read);

  int result = 0;
  for (auto& p: _data) {
    result += p.count(identifier);
  }
  return result;
}

std::string toolbox::particles::assignmentchecks::internal::Database::lastMeshSweepSnapshot() {
  tarch::multicore::MultiReadSingleWriteLock lock(_semaphore,tarch::multicore::MultiReadSingleWriteLock::Read);

  std::ostringstream msg;
  if (not _data.empty()) {
    const auto& lastMeshSnapshot = *_data.crbegin();
    msg << "#" << (_data.size()-1) << "(" << lastMeshSnapshot.getName() << "):";
    for (const auto& particleTrace: lastMeshSnapshot ) {
      msg << std::endl << "-" << particleTrace.first.toString() << ": ";
      for (const auto& event: particleTrace.second) {
        msg << event.toString();
      }
    }
  }
  return msg.str();
}

std::string toolbox::particles::assignmentchecks::internal::Database::particleHistory(const ParticleIdentifier& identifier) {
  tarch::multicore::MultiReadSingleWriteLock lock(_semaphore,tarch::multicore::MultiReadSingleWriteLock::Read);

  std::ostringstream msg;
  msg << std::endl
      << "============================" << std::endl
      << identifier.toString() << std::endl
      << "============================";
  int  snapshot   = _data.size()-1;

  bool                hasPredecessor = false;
  ParticleIdentifier  predecessor(identifier);
  auto                p = _data.crbegin();

  while (p!=_data.crend()) {
    if ((*p).count(identifier) > 0) {
      msg << std::endl << "sweep #" << snapshot << " (" << p->getName() << "):";
      bool firstEntry = false;
      for (const auto& event : p->at(identifier)) {
        if (not firstEntry) {
          firstEntry = true;
        } else {
          msg << "->";
        }
        msg << event.toString();
        if (event.type==Event::Type::MoveWhileAssociatedToVertex and hasPredecessor) {
          msg << " [particle has been moved on multiple ranks - only one taken into account]";
        }
        else if (event.type==Event::Type::MoveWhileAssociatedToVertex and not hasPredecessor) {
          hasPredecessor = true;
          predecessor    = _database.createParticleIdentifier( predecessor.particleName, event.previousParticleX );
        }
      }
    }
    snapshot--;
    p++;
  }

  if (hasPredecessor) {
    return msg.str() + particleHistory(predecessor);
  }
  else return msg.str();
}

void toolbox::particles::assignmentchecks::internal::Database::addEvent(ParticleIdentifier identifier, const Event& event) {
  tarch::multicore::MultiReadSingleWriteLock lock(_semaphore,tarch::multicore::MultiReadSingleWriteLock::Write);

  assertion(not _data.empty());
  MeshSweepData& snapshot = *_data.rbegin();
  if (snapshot.count(identifier) == 0) {
    snapshot.insert(std::pair<ParticleIdentifier, ParticleEvents>(identifier, ParticleEvents()));
    logDebug( "addEvent(...)", "add new particle history thread in this snapshot for " << identifier.toString() );
  }

  // We first have to push it. Otherwise, the susequent getEntry() won't work.
  snapshot[identifier].push_back(event);

  if ( event.type == Event::Type::AssignToVertex and _data.size() > _maxParticleSnapshotsToKeepTrackOf ) {
    removeTrajectory( identifier, event.treeId );
    removeEmptyDatabaseSnapshots();

    // re-add element
    if (snapshot.count(identifier) == 0) {
      snapshot.insert(std::pair<ParticleIdentifier, ParticleEvents>(identifier, ParticleEvents()));
      logDebug( "addEvent(...)", "re-add particle history in this snapshot for " << identifier.toString() );
    }
    snapshot[identifier].push_back(event);
  }
  if ( event.type == Event::Type::MoveWhileAssociatedToVertex and _data.size() > _maxParticleSnapshotsToKeepTrackOf ) {
    lock.free();

    auto rootEntryOfLatestTrajectory = getEntry(identifier, event.treeId);
    assertion4(
      rootEntryOfLatestTrajectory.first.type!=Event::Type::NotFound,
      rootEntryOfLatestTrajectory.first.toString(),
      rootEntryOfLatestTrajectory.second.toString(),
      event.toString(),
      identifier.toString()
    );

    Event substituteEntryForTrajectory(
        Event::Type::MoveWhileAssociatedToVertex,
        rootEntryOfLatestTrajectory.second.particleX,
        event.treeId,
        "substitute-for-whole-trajectory"
    );
    rootEntryOfLatestTrajectory.first.trace = "substitute-trajectory-start-from-original-point-" + ::toString( rootEntryOfLatestTrajectory.second.particleX );


    lock.lock();
    removeTrajectory( identifier, event.treeId);
    removeEmptyDatabaseSnapshots();

    // re-add element
    if (snapshot.count(identifier) == 0) {
      snapshot.insert(std::pair<ParticleIdentifier, ParticleEvents>(identifier, ParticleEvents()));
      logDebug( "addEvent(...)", "re-add particle history in this snapshot for " << identifier.toString() );
    }
    snapshot[rootEntryOfLatestTrajectory.second].push_back(rootEntryOfLatestTrajectory.first);
    snapshot[identifier].push_back(substituteEntryForTrajectory);
  }
}


#if defined(AssignmentChecks)

void toolbox::particles::assignmentchecks::startMeshSweep(
  const std::string&  meshSweepName
) {
  _database.startMeshSweep( meshSweepName );
}


void toolbox::particles::assignmentchecks::eraseParticle(
  const std::string&                           particleName,
  const tarch::la::Vector<Dimensions, double>& particleX,
  bool                                         isLocal,
  int                                          treeId,
  const std::string&                           trace
) {
  logTraceInWith4Arguments("eraseParticle(...)", particleName, particleX, isLocal, treeId );



  internal::ParticleIdentifier identifier = _database.createParticleIdentifier(particleName, particleX);
  internal::Event              event(internal::Event::Type::Erase, isLocal, treeId, trace);

  internal::Event previousLocalParticle = _database.getEntry(identifier, treeId).first;
  assertion5(
    previousLocalParticle.type==internal::Event::Type::DetachFromVertex,
    identifier.toString(),
    event.toString(),
    previousLocalParticle.toString(),
    treeId,
    _database.particleHistory(identifier)
  );

  _database.addEvent(identifier, event);
  logTraceOut("eraseParticle(...)" );
}



void toolbox::particles::assignmentchecks::assignParticleToVertex(
  const std::string&                           particleName,
  const tarch::la::Vector<Dimensions, double>& particleX,
  bool                                         isLocal,
  const tarch::la::Vector<Dimensions, double>& vertexX,
  const tarch::la::Vector<Dimensions, double>& vertexH,
  int                                          treeId,
  const std::string&                           trace,
  bool                                         particleIsNew
) {
  logTraceInWith6Arguments( "assignParticleToVertex(...)", particleName, particleX, isLocal, vertexX, vertexH, treeId );

  constexpr bool checkNewParticles = false;

  if ((not checkNewParticles) and particleIsNew) {
    internal::ParticleIdentifier identifier = _database.createParticleIdentifier(particleName, particleX, 0.0);
    internal::Event              event(internal::Event::Type::AssignToVertex, isLocal, vertexX, vertexH, treeId, trace);

    _database.addEvent(identifier, event);
  }
  else {
    internal::ParticleIdentifier identifier = _database.createParticleIdentifier(particleName, particleX);
    internal::Event              event(internal::Event::Type::AssignToVertex, isLocal, vertexX, vertexH, treeId, trace);

    #if PeanoDebug>0
    internal::Event              previousEvent = _database.getEntry(identifier, treeId).first;

    const bool isDropping = previousEvent.type == internal::Event::Type::DetachFromVertex and tarch::la::allGreater( previousEvent.vertexH, vertexH );
    const bool isLifting  = previousEvent.type == internal::Event::Type::DetachFromVertex and tarch::la::allSmaller( previousEvent.vertexH, vertexH );
    const bool isDroppingFromSieveSet  = previousEvent.type == internal::Event::Type::AssignToSieveSet;
    #endif

    if (isLocal) {
      assertion7(
        previousEvent.type == internal::Event::Type::NotFound
        or
        isDroppingFromSieveSet
        or
        (isLifting and previousEvent.isLocal)
        or
        (isDropping and previousEvent.isLocal)
        or
        (previousEvent.type == internal::Event::Type::DetachFromVertex and not previousEvent.isLocal),
        identifier.toString(),
        event.toString(),
        previousEvent.toString(),
        treeId,
        _database.getNumberOfSnapshots(),
        trace,
        _database.particleHistory(identifier)
      );
      assertion5(
        _database.getEntry(identifier, treeId).first.type == internal::Event::Type::NotFound
        or
        isLifting or isDropping or isDroppingFromSieveSet
        or
        tarch::la::equals(previousEvent.vertexX, vertexX),
        identifier.toString(),
        event.toString(),
        previousEvent.toString(),
        trace,
        _database.particleHistory(identifier)
      );
      assertion5(
        _database.getEntry(identifier, treeId).first.type == internal::Event::Type::NotFound
        or
        isLifting or isDropping or isDroppingFromSieveSet
        or
        tarch::la::equals(previousEvent.vertexH, vertexH),
        identifier.toString(),
        event.toString(),
        previousEvent.toString(),
        trace,
        _database.particleHistory(identifier)
      );
    }
    else {
      assertion7(
        (previousEvent.type == internal::Event::Type::NotFound)
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
        _database.getNumberOfSnapshots(),
        trace,
        _database.particleHistory(identifier)
      );
    }

    _database.addEvent(identifier, event);
  }
  logTraceOut( "assignParticleToVertex(...)" );
}


void toolbox::particles::assignmentchecks::moveParticle(
  const std::string&                            particleName,
  const tarch::la::Vector<Dimensions, double>&  oldParticleX,
  const tarch::la::Vector<Dimensions, double>&  newParticleX,
  int                                           treeId,
  const std::string&                            trace
) {
  logTraceInWith3Arguments( "moveParticle(...)", particleName, oldParticleX, newParticleX );

  if (not tarch::la::equals(oldParticleX, newParticleX, internal::ParticleIdentifier::Precision)) {
    internal::ParticleIdentifier newIdentifier = _database.createParticleIdentifier(particleName, newParticleX);
    internal::ParticleIdentifier oldIdentifier = _database.createParticleIdentifier(particleName, oldParticleX, internal::ParticleIdentifier::Precision * 4.0);
    if (
      newIdentifier != oldIdentifier
    ) {
      internal::Event              newEvent(internal::Event::Type::MoveWhileAssociatedToVertex, oldIdentifier.particleX, treeId, trace );

      #if PeanoDebug>0
      internal::Event              previousEvent               = _database.getEntry(oldIdentifier, treeId ).first;
      internal::Event              existingNewEventOnAnyTree   = _database.getEntry(newIdentifier, internal::Database::AnyTree ).first;
      internal::Event              existingNewEventOnLocalTree = _database.getEntry(newIdentifier, treeId ).first;
      #endif

      const std::string errorMessage0 = R"(
=============
Explanation
=============
The tracer has been informed of a particle movement. When it tried to bookmark
the particle with its new position, it found out that there is already a
particle registered at this place. It seems that a particle overlaps with
another one.

This might mean that there is actually a particle here, but could also result
from two other situations:

- We trace position updates one after the other. If particle A takes the
  position of a particle B, we might simply not have updated B yet.
- We trace position updates only if positions have changed significantly.
  Significantly here is formalised via

      toolbox::particles::assignmentchecks::internal::ParticleIdentifier::Precision

  That is, if particles are closer together than this delta, we do not write
  logs into our database. This ensures that the database is not filled with
  tiny update entries.

As the tracing cannot handle either situation, we are left with two options.
We can dramatically reduce Precision at the cost of a higher overhead.
Alternatively, it might be appropriate to check the time step sizes
employed: If particles move too fast, the probability that A ends up at a
position just previously held by B (which is not yet updated) is higher.

)";
      assertion13(
        existingNewEventOnLocalTree.type == internal::Event::Type::NotFound,
        oldIdentifier.toString(),
        newIdentifier.toString(),
        previousEvent.toString(),
        newEvent.toString(),
        existingNewEventOnLocalTree.toString(),
        existingNewEventOnAnyTree.toString(),
        _database.getNumberOfSnapshots(),
        treeId,
        trace,
        internal::ParticleIdentifier::Precision,
        _database.totalEntries(newIdentifier),
        _database.particleHistory(newIdentifier),
        errorMessage0
      );
      const std::string errorMessage1 = R"(
=============
Explanation
=============
The tracer has been informed of a particle movement. When it tried to bookmark
the particle with its new position, it found out that there is already a
particle registered at this place. That is fine, as particles might be held
redundantly on different trees - either as halo copies or as they sit exactly
on the face between two subdomains.

If that happens, they however have to be tied to the same vertex in the domain
although the vertex might be replicated on a different tree. Alternatively, the
other rank might already have moved it and come to the conclusion that it has
to be assigned to the sieve set. The present tree is not there yet, i.e. is
just about to move it, but will eventually also raise its particle to the
sieve set.
)";
      assertion13(
        existingNewEventOnAnyTree.type == internal::Event::Type::NotFound
        or
        existingNewEventOnAnyTree.type == internal::Event::Type::AssignToSieveSet
        or
        (
          existingNewEventOnAnyTree.type == internal::Event::Type::AssignToVertex
          and
          existingNewEventOnAnyTree.vertexX == previousEvent.vertexX
          and
          existingNewEventOnAnyTree.vertexH == previousEvent.vertexH
        ),
        oldIdentifier.toString(),
        newIdentifier.toString(),
        previousEvent.toString(),
        newEvent.toString(),
        existingNewEventOnLocalTree.toString(),
        existingNewEventOnAnyTree.toString(),
        _database.getNumberOfSnapshots(),
        treeId,
        trace,
        internal::ParticleIdentifier::Precision,
        _database.totalEntries(newIdentifier),
        _database.particleHistory(newIdentifier),
        errorMessage1
      );
      assertion9(
        previousEvent.type == internal::Event::Type::AssignToVertex,
        oldIdentifier.toString(),
        previousEvent.toString(),
        newIdentifier.toString(),
        newEvent.toString(),
        _database.getNumberOfSnapshots(),
        treeId,
        trace,
        _database.totalEntries(oldIdentifier),
        _database.particleHistory(oldIdentifier)
      );

      _database.addEvent(newIdentifier, newEvent);
    }
  }

  logTraceOut( "moveParticle(...)" );
}


void toolbox::particles::assignmentchecks::detachParticleFromVertex(
  const std::string&                           particleName,
  const tarch::la::Vector<Dimensions, double>& particleX,
  bool                                         isLocal,
  const tarch::la::Vector<Dimensions, double>& vertexX,
  const tarch::la::Vector<Dimensions, double>& vertexH,
  int                                          treeId,
  const std::string&                           trace
) {
  logTraceInWith6Arguments( "detachParticleFromVertex(...)", particleName, particleX, isLocal, vertexX, vertexH, treeId );

  internal::ParticleIdentifier identifier = _database.createParticleIdentifier(particleName, particleX);
  internal::Event              event{internal::Event::Type::DetachFromVertex, isLocal, vertexX, vertexH, treeId, trace};

  #if PeanoDebug>0
  internal::Event previousEvent = _database.getEntry(identifier, treeId).first;
  #endif

  assertion7(
    previousEvent.type == internal::Event::Type::AssignToVertex,
    identifier.toString(),
    event.toString(),
    previousEvent.toString(),
    treeId,
    _database.getNumberOfSnapshots(),
    trace,
    _database.particleHistory(identifier)
  );
  assertion7(
    tarch::la::equals(previousEvent.vertexX, vertexX),
    identifier.toString(),
    event.toString(),
    previousEvent.toString(),
    treeId,
    _database.getNumberOfSnapshots(),
    trace,
    _database.particleHistory(identifier)
  );
  assertion6(
    tarch::la::equals(previousEvent.vertexH, vertexH),
    identifier.toString(),
    event.toString(),
    previousEvent.toString(),
    treeId,
    trace,
    _database.particleHistory(identifier)
  );

  _database.addEvent(identifier, event);
  logTraceOut( "detachParticleFromVertex(...)" );
}


void toolbox::particles::assignmentchecks::assignParticleToSieveSet(
  const std::string& particleName, const tarch::la::Vector<Dimensions, double>& particleX, bool isLocal,
  int treeId,
  const std::string&                           trace
) {
  logTraceInWith4Arguments( "assignParticleToSieveSet(...)", particleName, particleX, isLocal, treeId );

  logDebug("assignParticleToSieveSet()", "assign " << particleName << " particle at " << particleX << " to global sieve set on tree " << treeId);

  internal::ParticleIdentifier identifier = _database.createParticleIdentifier(particleName, particleX);
  internal::Event              event{internal::Event::Type::AssignToSieveSet, isLocal, treeId, trace};

  #if PeanoDebug>0
  internal::Event              previousEvent = _database.getEntry(identifier, treeId).first;
  #endif

  assertion5(
    previousEvent.type == internal::Event::Type::DetachFromVertex,
    identifier.toString(),
    event.toString(), previousEvent.toString(),
    treeId, _database.particleHistory(identifier)
  );

  _database.addEvent(identifier, event);
  logTraceOut( "assignParticleToSieveSet(...)" );
}


void toolbox::particles::assignmentchecks::eliminateExistingParticles() {
  _database.eliminateExistingParticles();
}


void toolbox::particles::assignmentchecks::ensureDatabaseIsEmpty() {
  if (_database.getNumberOfSnapshots()!=0) {
    logInfo( "ensureDatabaseIsEmpty()", "database still holds " << _database.getNumberOfSnapshots() << " snapshots" );
    logError( "ensureDatabaseIsEmpty()", _database.toString() );
    assertion(false);
    exit(-1);
  }
}

#else

void toolbox::particles::assignmentchecks::startMeshSweep(const std::string& meshSweepName) {}

void toolbox::particles::assignmentchecks::eraseParticle(
  const std::string& particleName, const tarch::la::Vector<Dimensions, double>& particleX, bool isLocal, int treeId,
  const std::string&                           trace
) {}

void toolbox::particles::assignmentchecks::assignParticleToVertex(
  const std::string&                           particleName,
  const tarch::la::Vector<Dimensions, double>& particleX,
  bool                                         isLocal,
  const tarch::la::Vector<Dimensions, double>& vertexX,
  const tarch::la::Vector<Dimensions, double>& vertexH,
  int                                          treeId,
  const std::string&                           trace,
  bool                                         particleIsNew
) {}

void toolbox::particles::assignmentchecks::detachParticleFromVertex(
  const std::string&                           particleName,
  const tarch::la::Vector<Dimensions, double>& particleX,
  bool                                         isLocal,
  const tarch::la::Vector<Dimensions, double>& vertexX,
  const tarch::la::Vector<Dimensions, double>& vertexH,
  int                                          treeId,
  const std::string&                           trace
) {}

void toolbox::particles::assignmentchecks::assignParticleToSieveSet(
  const std::string& particleName, const tarch::la::Vector<Dimensions, double>& particleX, bool isLocal, int treeId,   const std::string& trace
) {}

void toolbox::particles::assignmentchecks::moveParticle(
    const std::string&                            particleName,
    const tarch::la::Vector<Dimensions, double>&  oldParticleX,
    const tarch::la::Vector<Dimensions, double>&  newParticleX,
    int                                           treeId,
    const std::string&                            trace
) {}

void toolbox::particles::assignmentchecks::ensureDatabaseIsEmpty() {}

void toolbox::particles::assignmentchecks::eliminateExistingParticles() {

}

#endif


bool operator==(const toolbox::particles::assignmentchecks::internal::ParticleIdentifier& lhs, const toolbox::particles::assignmentchecks::internal::ParticleIdentifier& rhs ) {
  return lhs.numericalEquals(rhs);
}


bool operator!=(const toolbox::particles::assignmentchecks::internal::ParticleIdentifier& lhs, const toolbox::particles::assignmentchecks::internal::ParticleIdentifier& rhs ) {
  return not lhs.numericalEquals(rhs);
}
