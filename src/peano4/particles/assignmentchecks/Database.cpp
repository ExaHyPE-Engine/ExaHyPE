// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "Database.h"

#include "peano4/particles/assignmentchecks/Event.h"
#include "peano4/particles/assignmentchecks/ParticleIdentifier.h"
#include "tarch/instrumentation/Debug.h"
#include "tarch/la/Vector.h"

namespace {
  /**
   * Compares the key the map stores against the key the caller looks up. The
   * comparison of ParticleIdentifier holds two keys equal that differ in their
   * particle position, so a lookup can return an entry of another particle.
   */
  void assertStoredIdentifierMatches(
    [[maybe_unused]] const peano4::particles::assignmentchecks::ParticleIdentifier& storedIdentifier,
    [[maybe_unused]] const peano4::particles::assignmentchecks::ParticleIdentifier& identifier
  ) {
#if PEANO_DEBUG
    assertion(
      storedIdentifier.particleId == identifier.particleId,
      "The stored particle id differs from the one of the key.",
      storedIdentifier.toString(),
      identifier.toString()
    );
    assertion(
      storedIdentifier.treeId == identifier.treeId,
      "The stored tree id differs from the one of the key.",
      storedIdentifier.toString(),
      identifier.toString()
    );
    for (int dimension = 0; dimension < DIMENSIONS; dimension++) {
      assertion(
        storedIdentifier.vertexIndex(dimension) == identifier.vertexIndex(dimension),
        "The stored vertex index differs from the one of the key.",
        storedIdentifier.toString(),
        identifier.toString()
      );
    }
    assertion(
      storedIdentifier.vertexDepth == identifier.vertexDepth,
      "The stored vertex depth differs from the one of the key.",
      storedIdentifier.toString(),
      identifier.toString()
    );
#endif
  }
} // namespace

/**
 * Returns the one assignment-history database for this process.
 */
peano4::particles::assignmentchecks::Database& peano4::particles::assignmentchecks::Database::getInstance() {
  static Database instance;
  return instance;
}

peano4::particles::assignmentchecks::Database::Database() { _meshSweepData.emplace_back("initial"); }

/**
 * Sets the history limit applied independently to each particle.
 */
void peano4::particles::assignmentchecks::Database::setMaxParticleSnapshotsToKeepTrackOf(
  const std::size_t maximumNumberOfSnapshots
) {
  const std::unique_lock<std::shared_mutex> lock(_mutex);
  _maxParticleEventsToKeepTrackOf = maximumNumberOfSnapshots;
}

/**
 * Starts a mesh sweep and makes it the target for subsequently added events.
 */
void peano4::particles::assignmentchecks::Database::startMeshSweep(const std::string& meshSweepName) {
  logDebug("Finish the current mesh sweep and start " << meshSweepName);

  const std::unique_lock<std::shared_mutex> lock(_mutex);

  _meshSweepData.emplace_back(meshSweepName);
  _currentMeshSweepIndex++;
}

/**
 * Returns the number of mesh-sweep snapshots retained by the database.
 */
int peano4::particles::assignmentchecks::Database::getNumberOfSnapshots() const {
  const std::shared_lock<std::shared_mutex> lock(_mutex);
  return static_cast<int>(_meshSweepData.size());
}

/**
 * Returns the mesh-sweep entries without locking _mutex, so no other method of
 * this object may run while the caller holds the reference.
 */
std::vector<peano4::particles::assignmentchecks::MeshSweepData>& peano4::particles::assignmentchecks::Database::
  getMeshSweepData() {
  return _meshSweepData;
}

std::size_t peano4::particles::assignmentchecks::Database::getCurrentMeshSweepIndex() const {
  const std::shared_lock<std::shared_mutex> lock(_mutex);
  return static_cast<std::size_t>(_currentMeshSweepIndex);
}

int peano4::particles::assignmentchecks::Database::getNumberOfTracedParticles() const {
  const std::shared_lock<std::shared_mutex> lock(_mutex);
  return static_cast<int>(_data.size());
}

/**
 * Clears all histories and recreates the initial mesh-sweep entry.
 */
void peano4::particles::assignmentchecks::Database::reset() {
  const std::unique_lock<std::shared_mutex> lock(_mutex);
  logDebug("Resetting database");

  _data.clear();
  _meshSweepData.clear();

  _meshSweepData.emplace_back("initial");
  _currentMeshSweepIndex = 0;
}

/**
 * Returns a copy of the particle history. An unknown identifier yields one
 * Event::Type::NotFound entry.
 */
peano4::particles::assignmentchecks::ParticleEvents peano4::particles::assignmentchecks::Database::getParticleHistory(
  const ParticleIdentifier& identifier
) const {
  const std::shared_lock<std::shared_mutex> lock(_mutex);

  auto search = _data.find(identifier);

  if (search == _data.end()) {
    ParticleEvents newEventHistory;
    newEventHistory.emplace_back(Event::Type::NotFound);
    return newEventHistory;
  }
  assertStoredIdentifierMatches(search->first, identifier);

  return search->second;
}

std::string peano4::particles::assignmentchecks::Database::toString() const {
  const std::shared_lock<std::shared_mutex> lock(_mutex);

  std::ostringstream description;

  description << "--------------------------\n";
  description << "Full database dump\n";
  description << "--------------------------\n";

  for (auto entry = _data.cbegin(); entry != _data.cend(); entry++) {
    const ParticleIdentifier& identifier = entry->first;
    const ParticleEvents&     history    = entry->second;

    description << "\n" << identifier.toString();
    int meshSweepIndex = -1;

    for (auto event = history.crbegin(); event != history.crend(); event++) {
      if (event->meshSweepIndex != meshSweepIndex) {
        const std::string meshSweepName = _meshSweepData.at(event->meshSweepIndex).getName();
        description << "\n\t[Sweep " << meshSweepName << ",index=" << event->meshSweepIndex << "]:";
      }
      meshSweepIndex = event->meshSweepIndex;
      description << "\n\t\t->" << event->toString();
    }
  }

  description << "\n";

  return description.str();
}

int peano4::particles::assignmentchecks::Database::getTotalParticleEntries(const ParticleIdentifier& identifier) const {
  const std::shared_lock<std::shared_mutex> lock(_mutex);

  auto search = _data.find(identifier);

  if (search != _data.end()) {
    assertStoredIdentifierMatches(search->first, identifier);

    return static_cast<int>(search->second.size());
  }
  return 0;
}

std::string peano4::particles::assignmentchecks::Database::lastMeshSweepSnapshot() const {
  const std::shared_lock<std::shared_mutex> lock(_mutex);

  std::ostringstream description;
  const int          currentMeshSweepIndex = _currentMeshSweepIndex;

  description << "--------------------------\n";
  description << "Last sweep dump: Sweep [" << _meshSweepData.at(currentMeshSweepIndex).getName() << "]\n";
  description << "--------------------------\n";

  for (auto entry = _data.cbegin(); entry != _data.cend(); entry++) {
    const ParticleIdentifier& identifier = entry->first;
    const ParticleEvents&     history    = entry->second;

    description << "\n" << identifier.toString();

    for (auto event = history.crbegin(); event != history.crend(); event++) {
      if (event->meshSweepIndex != currentMeshSweepIndex) {
        break;
      }
      description << "\n\t->" << event->toString();
    }
  }

  return description.str();
}

std::string peano4::particles::assignmentchecks::Database::sweepHistory() const {
  const std::shared_lock<std::shared_mutex> lock(_mutex);

  std::ostringstream description;
  int                meshSweepIndex = 0;
  for (auto meshSweep = _meshSweepData.cbegin(); meshSweep != _meshSweepData.cend(); meshSweep++) {
    description << '\n' << "sweep #" << meshSweepIndex << ": " << meshSweep->getName();
    meshSweepIndex++;
  }
  description << "\n";

  return description.str();
}

std::string peano4::particles::assignmentchecks::Database::particleHistory(const ParticleIdentifier& identifier) const {
  const std::shared_lock<std::shared_mutex> lock(_mutex);

  std::ostringstream description;
  description << "\n============================\n" << identifier.toString() << "\n============================\n";

  auto search = _data.find(identifier);
  if (search != _data.end()) {
    assertStoredIdentifierMatches(search->first, identifier);

    const ParticleEvents& history       = search->second;
    auto                  eventIterator = history.crbegin();

    int previousMeshSweepIndex = -1;

    while (eventIterator != history.crend()) {
      const int meshSweepIndex = eventIterator->meshSweepIndex;

      if (meshSweepIndex != previousMeshSweepIndex) {
        description << "\nsweep #" << meshSweepIndex << " (" << _meshSweepData.at(meshSweepIndex).getName() << "):";
      }
      description << "\n\t->" << eventIterator->toString();

      previousMeshSweepIndex = meshSweepIndex;
      eventIterator++;
    }
  }

  description << "\n";

  return description.str();
}

/**
 * Appends an event to the particle history and returns its previous event.
 * When the history limit is reached, the newest retained event first becomes
 * the substitute for the discarded history.
 */
peano4::particles::assignmentchecks::Event peano4::particles::assignmentchecks::Database::addEvent(
  const ParticleIdentifier& identifier,
  Event&                    event
) {
  logDebug("Adding event for " << identifier.toString());

  const std::unique_lock<std::shared_mutex> lock(_mutex);

  event.meshSweepIndex = _currentMeshSweepIndex;

  auto search = _data.find(identifier);

  if (search == _data.end()) {
    _data.emplace(identifier, ParticleEvents{event});
    logDebug("Add a particle history for " << identifier.toString() << ", event: " << event.toString());
    return Event(Event::Type::NotFound);
  }
  assertStoredIdentifierMatches(search->first, identifier);

  ParticleEvents& history = search->second;

  if (history.size() >= _maxParticleEventsToKeepTrackOf) {
    Event lastEvent = history.back();

#if PEANO_DEBUG
    if (lastEvent.type == Event::Type::NotFound) {
      logDebug("The history of " << identifier.toString() << " holds an event of the type NotFound");
    }
#endif

    static const std::string SubstitutionPrefix = "substitute-for-whole-trajectory/";
    if (not lastEvent.trace.starts_with(SubstitutionPrefix)) {
      lastEvent.trace = SubstitutionPrefix + lastEvent.trace;
    }

    logDebug("Shorten the history of " << identifier.toString() << " to " << lastEvent.toString());

    history.clear();
    history.push_back(lastEvent);
  }

  const Event previousEvent = history.back();
  history.push_back(event);
  logDebug("Added event " << event.toString() << " " << identifier.toString());

  return previousEvent;
}

/**
 * Replaces a matching event and assigns the current mesh-sweep index to the
 * replacement.
 */
void peano4::particles::assignmentchecks::Database::replaceEvent(
  const ParticleIdentifier& identifier,
  const Event&              oldEvent,
  Event&                    newEvent
) {
  const std::unique_lock<std::shared_mutex> lock(_mutex);

  newEvent.meshSweepIndex = _currentMeshSweepIndex;

  auto search = _data.find(identifier);

  [[maybe_unused]] const bool particleHasHistory = search != _data.end();
  assertion(particleHasHistory, "The database holds no history for the particle.", identifier.toString());

  assertStoredIdentifierMatches(search->first, identifier);

  ParticleEvents& history = search->second;

  for (auto eventIterator = history.begin(); eventIterator != history.end(); eventIterator++) {
    if (
      (eventIterator->meshSweepIndex == oldEvent.meshSweepIndex) and (eventIterator->type == oldEvent.type)
      and (eventIterator->treeId == oldEvent.treeId) and (eventIterator->isLocal == oldEvent.isLocal)
    ) {
      eventIterator->type           = newEvent.type;
      eventIterator->isLocal        = newEvent.isLocal;
      eventIterator->vertexX        = newEvent.vertexX;
      eventIterator->vertexH        = newEvent.vertexH;
      eventIterator->treeId         = newEvent.treeId;
      eventIterator->trace          = newEvent.trace;
      eventIterator->meshSweepIndex = newEvent.meshSweepIndex;
      return;
    }
  }
  assertion(false, "The event to replace is not part of the particle history.", oldEvent.toString());
}

/**
 * Moves the history of the particle to the key that carries newVertexIndex and
 * newVertexDepth. A history under the target key is overwritten.
 */
void peano4::particles::assignmentchecks::Database::shiftIdentifierVertexIndex(
  const ParticleIdentifier&                       identifier,
  const tarch::la::Vector<DIMENSIONS, long long>& newVertexIndex,
  const int                                       newVertexDepth
) {
  const std::unique_lock<std::shared_mutex> lock(_mutex);

  auto search = _data.find(identifier);

  [[maybe_unused]] const bool particleHasHistory = search != _data.end();
  assertion(particleHasHistory, "The database holds no history for the particle.", identifier.toString());

  assertStoredIdentifierMatches(search->first, identifier);

  // The heterogeneous lookup may be fuzzy, so re-key the stored identifier.
  const ParticleIdentifier storedIdentifier = search->first;
  const ParticleEvents     history          = search->second;

  logDebug("Shift particle identifier from " << storedIdentifier.vertexIndex << " to " << newVertexIndex);

  _data.erase(storedIdentifier);
  const ParticleIdentifier newIdentifier(
    identifier.particleName,
    identifier.particleX,
    newVertexIndex,
    newVertexDepth,
    identifier.particleId,
    identifier.treeId
  );
  _data.insert_or_assign(newIdentifier, history);
}

/**
 * Deletes every particle whose newest event neither assigns it to a vertex nor
 * moves it along a vertex, and keeps every particle that a remote rank owns.
 */
void peano4::particles::assignmentchecks::Database::eliminateExistingParticles() {
  const std::unique_lock<std::shared_mutex> lock(_mutex);

  for (auto particleIterator = _data.begin(); particleIterator != _data.end();) {
    const peano4::particles::assignmentchecks::Event& lastEvent = particleIterator->second.back();

    if (
      lastEvent.type == peano4::particles::assignmentchecks::Event::Type::AssignToVertex
      or lastEvent.type == peano4::particles::assignmentchecks::Event::Type::ConsecutiveMoveWhileAssociatedToVertex
      or lastEvent.type == peano4::particles::assignmentchecks::Event::Type::MoveWhileAssociatedToVertex
      or (not lastEvent.isLocal)
    ) {
      ++particleIterator;
    } else {
      particleIterator = _data.erase(particleIterator);
    }
  }
}

/**
 * Returns the newest event for the requested spacetree after skipping the
 * requested number of newest entries. AnyTree accepts the first candidate.
 */
peano4::particles::assignmentchecks::Event peano4::particles::assignmentchecks::Database::getPreviousEvent(
  const ParticleEvents& particleHistory,
  const int             treeId,
  const std::size_t     numberOfFirstEventsToSkip
) const {
  if (particleHistory.size() <= numberOfFirstEventsToSkip) {
    return Event(Event::Type::NotFound);
  }

  auto eventIterator = particleHistory.crbegin();
  std::advance(eventIterator, numberOfFirstEventsToSkip);

  if (eventIterator == particleHistory.crend()) {
    return Event(Event::Type::NotFound);
  }

  if (treeId == Database::AnyTree) {
    return *eventIterator;
  }

  while (eventIterator != particleHistory.crend()) {
    if (eventIterator->treeId == treeId) {
      return *eventIterator;
    }
    eventIterator++;
  }

  return Event(Event::Type::NotFound);
}
