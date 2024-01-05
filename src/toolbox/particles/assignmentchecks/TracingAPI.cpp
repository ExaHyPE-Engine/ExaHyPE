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
  if (particleName < rhs.particleName)
    return true;
  if (particleName > rhs.particleName)
    return false;
  for (int d = 0; d < Dimensions; d++) {
    if (tarch::la::smaller(particleX(d), rhs.particleX(d)))
      return true;
    if (tarch::la::greater(particleX(d), rhs.particleX(d)))
      return false;
  }
  return false;
}

std::string toolbox::particles::assignmentchecks::internal::ParticleIdentifier::toString() const { return "(" + particleName + "," + ::toString(particleX) + ")"; }


toolbox::particles::assignmentchecks::internal::Event::Event(
  Type type_, bool isLocal_, const tarch::la::Vector<Dimensions, double>& vertexX_, const tarch::la::Vector<Dimensions, double>& vertexH_, int treeId_
):
  type(type_),
  isLocal(isLocal_),
  vertexX(vertexX_),
  vertexH(vertexH_),
  treeId(treeId_) {
  assertion(type == Type::AssignToVertex or type == Type::RemoveFromVertex);
}

toolbox::particles::assignmentchecks::internal::Event::Event(Type type_, bool isLocal_, int treeId_):
  type(type_),
  isLocal(isLocal_),
  vertexX(tarch::la::Vector<Dimensions, double>(0.0)),
  vertexH(tarch::la::Vector<Dimensions, double>(0.0)),
  treeId(treeId_) {
  assertion(type == Type::AssignToSieveSet or type == Type::Erase);
}

toolbox::particles::assignmentchecks::internal::Event::Event(Type type_):
  type(type_),
  isLocal(false),
  vertexX(tarch::la::Vector<Dimensions, double>(0.0)),
  vertexH(tarch::la::Vector<Dimensions, double>(0.0)),
  treeId(-1) {
  assertion(type == Type::NotFound);
}

std::string toolbox::particles::assignmentchecks::internal::Event::toString() const {
  std::ostringstream msg;
  msg << "(";
  switch (type) {
  case Type::AssignToSieveSet:
    msg << "assign-to-sieve-set";
    break;
  case Type::AssignToVertex:
    msg << "assign-to-vertex";
    break;
  case Type::Erase:
    msg << "erase";
    break;
  case Type::RemoveFromVertex:
    msg << "remove-from-vertex";
    break;
  case Type::NotFound:
    msg << "not-found";
    break;
  };
  msg << ",local=" << isLocal << ",x=" << vertexX << ",h=" << vertexH << ",tree=" << treeId << ")";
  return msg.str();
}


toolbox::particles::assignmentchecks::internal::Database::Database() { _data.push_back(MeshSweepData()); };

void toolbox::particles::assignmentchecks::internal::Database::addMeshSweep() {
  logInfo("addMeshSweep()", "end mesh sweep with " << _data.rbegin()->size() << " event(s)");
  _data.push_back(MeshSweepData());
};

toolbox::particles::assignmentchecks::internal::Event toolbox::particles::assignmentchecks::internal::Database::getEntry(
  const ParticleIdentifier& identifier, int spacetreeId, bool hasToBeLocal
) const {
  auto currentSnapshot = _data.crbegin();

  while (currentSnapshot != _data.crend()) {
    const MeshSweepData& meshSweepData = *currentSnapshot;
    if (meshSweepData.count(identifier) > 0) {
      auto event = meshSweepData.at(identifier).crbegin();
      while (event != meshSweepData.at(identifier).crend()) {
        if ((event->isLocal or not hasToBeLocal) and event->type == Event::Type::Erase and event->treeId == spacetreeId) {
          return Event(Event::Type::NotFound);
        }
        if ((event->isLocal or not hasToBeLocal) and event->type == Event::Type::RemoveFromVertex and event->treeId == spacetreeId) {
          return Event(Event::Type::NotFound);
        }
        if ((event->isLocal or not hasToBeLocal) and event->type == Event::Type::AssignToVertex and event->treeId == spacetreeId) {
          return *event;
        }
        event++;
      }
    }
    currentSnapshot++;
  }

  return Event(Event::Type::NotFound);
}

toolbox::particles::assignmentchecks::internal::Event toolbox::particles::assignmentchecks::internal::Database::getEntry(const ParticleIdentifier& identifier) const {
  auto currentSnapshot = _data.crbegin();

  while (currentSnapshot != _data.crend()) {
    const MeshSweepData& meshSweepData = *currentSnapshot;
    if (meshSweepData.count(identifier) > 0) {
      assertion(meshSweepData.at(identifier).size() > 0);
      return *meshSweepData.at(identifier).rbegin();
    }
    currentSnapshot++;
  }

  return Event(Event::Type::NotFound);
}

std::string toolbox::particles::assignmentchecks::internal::Database::particleHistory(const ParticleIdentifier& identifier) const {
  std::ostringstream msg;
  msg << identifier.toString() << ":";
  bool firstEntry = false;
  int  snapshot   = 0;
  for (const auto& p : _data) {
    if (p.count(identifier) > 0) {
      if (not firstEntry) {
        firstEntry = true;
      } else {
        msg << "->";
      }
      msg << "#" << snapshot << ":{";
      for (const auto& event : p.at(identifier)) {
        msg << event.toString();
      }
      msg << "}";
    }
    snapshot++;
  }
  return msg.str();
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

void toolbox::particles::assignmentchecks::endMeshSweep() { _database.addMeshSweep(); }


void toolbox::particles::assignmentchecks::eraseParticle(const std::string& particleName, const tarch::la::Vector<Dimensions, double>& particleX, bool isLocal, int treeId) {
  logDebug("eraseParticle()", "erase " << particleName << " particle at " << particleX << " on tree " << treeId);

  tarch::multicore::Lock lock(_semaphore);

  internal::ParticleIdentifier identifier(particleName, particleX);
  internal::Event              event(internal::Event::Type::Erase, isLocal, treeId);

  internal::Event previousLocalParticle = _database.getEntry(identifier, treeId, true);
  // @todo Comment: Kann sich halt bewegt haben?
  assertion5(
    previousLocalParticle.type == internal::Event::Type::NotFound
    //    or
    //    _database.getEntry( identifier, treeId, false ).type == internal::Event::Type::NotFound
    ,
    identifier.toString(),
    event.toString(),
    previousLocalParticle.toString(),
    treeId,
    _database.particleHistory(identifier)
  );

  _database.addEvent(identifier, event);
}

void toolbox::particles::assignmentchecks::assignParticleToVertex(
  const std::string&                           particleName,
  const tarch::la::Vector<Dimensions, double>& particleX,
  bool                                         isLocal,
  const tarch::la::Vector<Dimensions, double>& vertexX,
  const tarch::la::Vector<Dimensions, double>& vertexH,
  int                                          treeId
) {
  tarch::multicore::Lock lock(_semaphore);

  internal::ParticleIdentifier identifier(particleName, particleX);
  internal::Event              event(internal::Event::Type::AssignToVertex, isLocal, vertexX, vertexH, treeId);

  assertion5(
    _database.getEntry(identifier, treeId, true).type == internal::Event::Type::NotFound,
    identifier.toString(),
    event.toString(),
    _database.getEntry(identifier, treeId, true).toString(),
    treeId,
    _database.particleHistory(identifier)
  );

  internal::Event previousEvent = _database.getEntry(identifier);
  logDebug(
    "assignParticleToVertex()",
    "assign "
      << particleName << " particle at " << particleX << " to vertex " << vertexX << " on mesh level " << vertexH << " on tree " << treeId
      << ". Previous event=" << previousEvent.toString()
  );

  assertion4(
    _database.getEntry(identifier, treeId, true).type == internal::Event::Type::NotFound or tarch::la::equals(previousEvent.vertexX, vertexX),
    identifier.toString(),
    event.toString(),
    previousEvent.toString(),
    _database.particleHistory(identifier)
  );
  assertion4(
    _database.getEntry(identifier, treeId, true).type == internal::Event::Type::NotFound or tarch::la::equals(previousEvent.vertexH, vertexH),
    identifier.toString(),
    event.toString(),
    previousEvent.toString(),
    _database.particleHistory(identifier)
  );

  _database.addEvent(identifier, event);
}

void toolbox::particles::assignmentchecks::removeParticleFromVertex(
  const std::string&                           particleName,
  const tarch::la::Vector<Dimensions, double>& particleX,
  bool                                         isLocal,
  const tarch::la::Vector<Dimensions, double>& vertexX,
  const tarch::la::Vector<Dimensions, double>& vertexH,
  int                                          treeId
) {
  tarch::multicore::Lock lock(_semaphore);

  internal::ParticleIdentifier identifier(particleName, particleX);
  internal::Event              event{internal::Event::Type::RemoveFromVertex, isLocal, vertexX, vertexH, treeId};

  internal::Event previousEvent = _database.getEntry(identifier, treeId, true);

  assertion5(
    previousEvent.type == internal::Event::Type::AssignToVertex or previousEvent.type == internal::Event::Type::NotFound,
    identifier.toString(),
    event.toString(),
    _database.getEntry(identifier, treeId, true).toString(),
    treeId,
    _database.particleHistory(identifier)
  );

  logDebug(
    "removeParticleFromVertex()",
    "remove "
      << particleName << " particle at " << particleX << " from vertex " << vertexX << " on mesh level " << vertexH << " on tree " << treeId
      << ". Previous entry=" << previousEvent.toString()
  );

  assertion6(
    previousEvent.type == internal::Event::Type::NotFound or tarch::la::equals(previousEvent.vertexX, vertexX),
    identifier.toString(),
    event.toString(),
    treeId,
    _database.getEntry(identifier, treeId, true).toString(),
    previousEvent.toString(),
    _database.particleHistory(identifier)
  );
  assertion6(
    previousEvent.type == internal::Event::Type::NotFound or tarch::la::equals(previousEvent.vertexH, vertexH),
    identifier.toString(),
    event.toString(),
    treeId,
    _database.getEntry(identifier, treeId, true).toString(),
    previousEvent.toString(),
    _database.particleHistory(identifier)
  );

  _database.addEvent(identifier, event);
}

void toolbox::particles::assignmentchecks::assignParticleToSieveSet(
  const std::string& particleName, const tarch::la::Vector<Dimensions, double>& particleX, bool isLocal, int treeId
) {
  tarch::multicore::Lock lock(_semaphore);

  logDebug("assignParticleToSieveSet()", "assign " << particleName << " particle at " << particleX << " to global sieve set on tree " << treeId);

  internal::ParticleIdentifier identifier(particleName, particleX);
  internal::Event              event{internal::Event::Type::AssignToSieveSet, isLocal, treeId};

  assertion4(
    _database.getEntry(identifier, treeId, true).type == internal::Event::Type::NotFound, identifier.toString(), event.toString(), treeId, _database.particleHistory(identifier)
  );

  _database.addEvent(identifier, event);
}


#else

void toolbox::particles::assignmentchecks::endMeshSweep() {}

void toolbox::particles::assignmentchecks::eraseParticle(const std::string& particleName, const tarch::la::Vector<Dimensions, double>& particleX, bool isLocal, int treeId) {}

void toolbox::particles::assignmentchecks::assignParticleToVertex(
  const std::string&                           particleName,
  const tarch::la::Vector<Dimensions, double>& particleX,
  bool                                         isLocal,
  const tarch::la::Vector<Dimensions, double>& vertexX,
  const tarch::la::Vector<Dimensions, double>& vertexH,
  int                                          treeId
) {}

void toolbox::particles::assignmentchecks::removeParticleFromVertex(
  const std::string&                           particleName,
  const tarch::la::Vector<Dimensions, double>& particleX,
  bool                                         isLocal,
  const tarch::la::Vector<Dimensions, double>& vertexX,
  const tarch::la::Vector<Dimensions, double>& vertexH,
  int                                          treeId
) {}

void toolbox::particles::assignmentchecks::assignParticleToSieveSet(
  const std::string& particleName, const tarch::la::Vector<Dimensions, double>& particleX, bool isLocal, int treeId
) {}


#endif
