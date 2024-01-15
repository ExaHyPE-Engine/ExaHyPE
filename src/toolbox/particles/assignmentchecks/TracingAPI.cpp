#include "TracingAPI.h"

#include <ranges>

#include "tarch/multicore/BooleanSemaphore.h"
#include "tarch/multicore/Lock.h"


namespace {
  tarch::logging::Log _log("toolbox::particles::assignmentchecks");

  toolbox::particles::assignmentchecks::internal::Database _database;
  tarch::multicore::BooleanSemaphore                       _semaphore;
} // namespace


toolbox::particles::assignmentchecks::internal::ParticleIdentifier::ParticleIdentifier(const std::string& particleName__, const tarch::la::Vector<Dimensions, double>& particleX__):
  particleName(particleName__),
  particleX(particleX__) {}

bool toolbox::particles::assignmentchecks::internal::ParticleIdentifier::operator==(const ParticleIdentifier& rhs) const {
  return (particleName == rhs.particleName) and tarch::la::equals(particleX, rhs.particleX);
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

  auto reportErase =[&]() -> void {
    msg << ",local=" << isLocal << ",tree=" << treeId << ",trace=" << trace;
  };

  auto reportDetach =[&]() -> void {
    msg << ",local=" << isLocal << ",x=" << vertexX << ",h=" << vertexH << ",tree=" << treeId << ",trace=" << trace;
  };

  auto reportOthers = [&]() -> void {
    msg << ",local=" << isLocal << ",x=" << vertexX << ",h=" << vertexH << ",tree=" << treeId << ",trace=" << trace;
  };

  auto reportMove = [&]() -> void {
    msg << "," << previousParticleX << "->x_new,tree=" << treeId << ",trace=" << trace;
  };

  msg << "(";
  switch (type) {
  case Type::AssignToSieveSet:
    msg << "assign-to-sieve-set";
    reportOthers();
    break;
  case Type::AssignToVertex:
    msg << "assign-to-vertex";
    reportOthers();
    break;
  case Type::Erase:
    msg << "erase";
    reportErase();
    break;
  case Type::DetachFromVertex:
    msg << "detach-from-vertex";
    reportDetach();
    break;
  case Type::NotFound:
    msg << "not-found";
    break;
  case Type::MoveWhileAssociatedToVertex:
    msg << "moved-while-associated-to-vertex";
    reportMove();
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


toolbox::particles::assignmentchecks::internal::Database::Database() { _data.push_back(MeshSweepData("initial")); };

void toolbox::particles::assignmentchecks::internal::Database::startMeshSweep(const std::string& meshSweepName) {
  tarch::multicore::Lock lock(_semaphore);
  logDebug("addMeshSweep()", "end mesh sweep with " << _data.rbegin()->size() << " event(s)");
  _data.push_back(MeshSweepData(meshSweepName));
};


int toolbox::particles::assignmentchecks::internal::Database::getCurrentMeshSweep() const {
  return _data.size()-1;
}


toolbox::particles::assignmentchecks::internal::Event toolbox::particles::assignmentchecks::internal::Database::getEntry(
  const ParticleIdentifier&  identifier,
  int                        spacetreeId,
  int                        firstNRecentEntriesToSkip
) const {
  auto currentSnapshot = _data.crbegin();

  std::advance( currentSnapshot, firstNRecentEntriesToSkip );

  int countSnapshotsBackwards = 0;
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
          return Event(Event::Type::NotFound);
        }
        else if (
          event->type == Event::Type::MoveWhileAssociatedToVertex
          and
          firstNRecentEntriesToSkip!=DoNotFollowParticleMovementsInDatabase
        ) {
          ParticleIdentifier previousIdentifier{identifier};
          previousIdentifier.particleX = event->previousParticleX;
          assertion3(
            previousIdentifier!=identifier,
            previousIdentifier.toString(),
            identifier.toString(),
            event->toString()
          );
          logDebug( "getEntry()", "rerun with " << previousIdentifier.toString() << " distilled from " << identifier.toString() );
          return getEntry(previousIdentifier, spacetreeId, countSnapshotsBackwards);
        }
        else if (event->type != Event::Type::Erase and treeIsAFit) {
          return *event;
        }
        event++;
      }
    }
    currentSnapshot++;
    countSnapshotsBackwards++;
  }

  return Event(Event::Type::NotFound);
}


std::string toolbox::particles::assignmentchecks::internal::Database::toString() const {
  std::ostringstream msg;

  int  snapshotCounter   = 0;
  for (auto& snapshot: _data) {
    msg << std::endl << "sweep #" << snapshotCounter << " (" << snapshot.getName() << "):";
    for (const auto& identifier: snapshot) {
      msg << std::endl << "- " << identifier.first.toString() << ": ";
      for (const auto& event : identifier.second) {
        msg << event.toString();
      }
    }
    snapshotCounter++;
  }

  return msg.str();
}


int toolbox::particles::assignmentchecks::internal::Database::totalEntries(const ParticleIdentifier& identifier) const {
  int result = 0;
  for (auto& p: _data) {
    result += p.count(identifier);
  }
  return result;
}

std::string toolbox::particles::assignmentchecks::internal::Database::particleHistory(const ParticleIdentifier& identifier) const {
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
        if (event.type==Event::Type::MoveWhileAssociatedToVertex) {
          hasPredecessor = true;
          predecessor.particleX = event.previousParticleX;
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

void toolbox::particles::assignmentchecks::internal::Database::addEvent(const ParticleIdentifier& identifier, const Event& event) {
  assertion(not _data.empty());
  MeshSweepData& snapshot = *_data.rbegin();
  if (snapshot.count(identifier) == 0) {
    snapshot.insert(std::pair<ParticleIdentifier, ParticleEvents>(identifier, ParticleEvents()));
  }
  snapshot[identifier].push_back(event);
}


#if PeanoDebug > 0

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
  tarch::multicore::Lock lock(_semaphore);

  internal::ParticleIdentifier identifier(particleName, particleX);
  internal::Event              event(internal::Event::Type::Erase, isLocal, treeId, trace);

  internal::Event previousLocalParticle = _database.getEntry(identifier, treeId);
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
  const std::string&                           trace
) {
  logTraceInWith6Arguments( "assignParticleToVertex(...)", particleName, particleX, isLocal, vertexX, vertexH, treeId );
  tarch::multicore::Lock lock(_semaphore);

  internal::ParticleIdentifier identifier(particleName, particleX);
  internal::Event              event(internal::Event::Type::AssignToVertex, isLocal, vertexX, vertexH, treeId, trace);

  if (isLocal) {
    internal::Event              previousEvent = _database.getEntry(identifier, treeId);

    assertion7(
      previousEvent.type == internal::Event::Type::NotFound
      or
      previousEvent.type == internal::Event::Type::AssignToSieveSet
      or
      previousEvent.type == internal::Event::Type::DetachFromVertex,
      identifier.toString(),
      event.toString(),
      previousEvent.toString(),
      treeId,
      _database.getCurrentMeshSweep(),
      trace,
      _database.particleHistory(identifier)
    );
    assertion5(
      _database.getEntry(identifier, treeId).type == internal::Event::Type::NotFound
      or
      previousEvent.type == internal::Event::Type::AssignToSieveSet
      or
      previousEvent.type == internal::Event::Type::DetachFromVertex
      or
      tarch::la::equals(previousEvent.vertexX, vertexX),
      identifier.toString(),
      event.toString(),
      previousEvent.toString(),
      trace,
      _database.particleHistory(identifier)
    );
    assertion5(
      _database.getEntry(identifier, treeId).type == internal::Event::Type::NotFound
      or
      previousEvent.type == internal::Event::Type::AssignToSieveSet
      or
      previousEvent.type == internal::Event::Type::DetachFromVertex
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
    internal::Event              previousEvent = _database.getEntry(identifier, treeId);

    assertion7(
      previousEvent.type == internal::Event::Type::NotFound,
      identifier.toString(),
      event.toString(),
      previousEvent.toString(),
      treeId,
      _database.getCurrentMeshSweep(),
      trace,
      _database.particleHistory(identifier)
    );
  }

  _database.addEvent(identifier, event);
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
  if (not tarch::la::equals(oldParticleX,newParticleX)) {
    tarch::multicore::Lock lock(_semaphore);

    internal::ParticleIdentifier newIdentifier(particleName, newParticleX);
    internal::Event              newEvent(internal::Event::Type::MoveWhileAssociatedToVertex, oldParticleX, treeId, trace );
    internal::ParticleIdentifier oldIdentifier(particleName, oldParticleX);
    internal::Event              previousEvent = _database.getEntry(oldIdentifier, treeId );

    assertion10(
      previousEvent.type == internal::Event::Type::AssignToVertex,
      oldIdentifier.toString(),
      previousEvent.toString(),
      newIdentifier.toString(),
      newEvent.toString(),
      _database.getCurrentMeshSweep(),
      treeId,
      trace,
      _database.particleHistory(newIdentifier),
      _database.particleHistory(oldIdentifier),
      _database.totalEntries(oldIdentifier)
    );

    _database.addEvent(newIdentifier, newEvent);

    logDebug( "moveParticle(...)", "add " << newEvent.toString() << " with identifier " << newIdentifier.toString() << " (presumably on tree " << previousEvent.treeId << ")" );
    logDebug( "moveParticle(...)", _database.particleHistory(newIdentifier) );
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
  tarch::multicore::Lock lock(_semaphore);

  internal::ParticleIdentifier identifier(particleName, particleX);
  internal::Event              event{internal::Event::Type::DetachFromVertex, isLocal, vertexX, vertexH, treeId, trace};

  internal::Event previousEvent = _database.getEntry(identifier, treeId);

  assertion7(
    previousEvent.type == internal::Event::Type::AssignToVertex,
    identifier.toString(),
    event.toString(),
    previousEvent.toString(),
    treeId,
    _database.getCurrentMeshSweep(),
    trace,
    _database.particleHistory(identifier)
  );
  assertion7(
    tarch::la::equals(previousEvent.vertexX, vertexX),
    identifier.toString(),
    event.toString(),
    treeId,
    _database.getEntry(identifier, treeId).toString(),
    previousEvent.toString(),
    trace,
    _database.particleHistory(identifier)
  );
  assertion7(
    tarch::la::equals(previousEvent.vertexH, vertexH),
    identifier.toString(),
    event.toString(),
    treeId,
    _database.getEntry(identifier, treeId).toString(),
    previousEvent.toString(),
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
  tarch::multicore::Lock lock(_semaphore);

  logDebug("assignParticleToSieveSet()", "assign " << particleName << " particle at " << particleX << " to global sieve set on tree " << treeId);

  internal::ParticleIdentifier identifier(particleName, particleX);
  internal::Event              event{internal::Event::Type::AssignToSieveSet, isLocal, treeId, trace};
  internal::Event              previousEvent = _database.getEntry(identifier, treeId);

  assertion5(
    previousEvent.type == internal::Event::Type::DetachFromVertex,
    identifier.toString(),
    event.toString(), previousEvent.toString(),
    treeId, _database.particleHistory(identifier)
  );

  _database.addEvent(identifier, event);
  logTraceOut( "assignParticleToSieveSet(...)" );
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
  const std::string&                           trace
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

#endif
