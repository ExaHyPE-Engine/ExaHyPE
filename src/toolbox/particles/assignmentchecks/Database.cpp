// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org

#include "Database.h"

#include "Event.h"
#include "ParticleIdentifier.h"
#include "tarch/la/Vector.h"
#include "tarch/multicore/MultiReadSingleWriteLock.h"

#include <iterator>
#include <limits>
#include <ranges>
#include <vector>


namespace {
  tarch::logging::Log _log("toolbox::particles::assignmentchecks");
} // namespace


void toolbox::particles::assignmentchecks::internal::Database::
  setMaxParticleSnapshotsToKeepTrackOf(const size_t n) {
  _maxParticleEventsToKeepTrackOf = n;
}


void toolbox::particles::assignmentchecks::internal::Database::startMeshSweep(
  const std::string& meshSweepName
) {
  logDebug(
    "startMeshSweep()",
    "finish old mesh sweep with and start new one for " << meshSweepName
  );

  tarch::multicore::MultiReadSingleWriteLock
    lock(_semaphore, tarch::multicore::MultiReadSingleWriteLock::Write);

  _meshSweepData.push_back(MeshSweepData(meshSweepName));
  _currentMeshSweepIndex++;
};


int toolbox::particles::assignmentchecks::internal::Database::
  getNumberOfSnapshots() const {
  return _meshSweepData.size();
}


std::vector<toolbox::particles::assignmentchecks::internal::MeshSweepData>& toolbox::
  particles::assignmentchecks::internal::Database::getMeshSweepData() {
  return _meshSweepData;
}


size_t toolbox::particles::assignmentchecks::internal::Database::
  getCurrentMeshSweepIndex() const {
  return _currentMeshSweepIndex;
}


int toolbox::particles::assignmentchecks::internal::Database::
  getNumberOfTracedParticles() const {
  return _data.size();
}


void toolbox::particles::assignmentchecks::internal::Database::reset() {

  logDebug("reset()", "Resetting database");

  _data.clear();
  _meshSweepData.clear();

  // and re-initialize.
  _meshSweepData.push_back(MeshSweepData("initial"));
  _currentMeshSweepIndex = 0;
}


toolbox::particles::assignmentchecks::internal::ParticleEvents toolbox::
  particles::assignmentchecks::internal::Database::getParticleHistory(
    const ParticleIdentifier& identifier
  ) {

  tarch::multicore::MultiReadSingleWriteLock
    lock(_semaphore, tarch::multicore::MultiReadSingleWriteLock::Read);

  auto search = _data.find(identifier);

  if (search == _data.end()) {
    ParticleEvents* newEventHistory = new ParticleEvents();
    newEventHistory->push_back(Event(Event::Type::NotFound));
    return *newEventHistory;
  } else {

#if PeanoDebug > 3
    assertion3(
      search->first.particleID == identifier.particleID,
      "Particle identifier ID not identical",
      search->first.particleID,
      identifier.particleID
    );
    assertion3(
      search->first.treeID == identifier.treeID,
      "Particle treeID not identical",
      search->first.treeID,
      identifier.treeID
    );
    for (int i = 0; i < Dimensions; i++) {
      assertion3(
        search->first.vertexInd(i) == identifier.vertexInd(i),
        "Particle treeID not identical",
        search->first.vertexInd(i),
        identifier.vertexInd(i)
      );
    }
    assertion3(
      search->first.vertexDepth == identifier.vertexDepth,
      "Particle depth not identical",
      search->first.vertexDepth,
      identifier.vertexDepth
    );
#endif

    ParticleEvents history = search->second;
    return history;
  }
}


std::string toolbox::particles::assignmentchecks::internal::Database::toString(
) {
  tarch::multicore::MultiReadSingleWriteLock
    lock(_semaphore, tarch::multicore::MultiReadSingleWriteLock::Read);

  std::ostringstream msg;

  msg << "--------------------------\n";
  msg << "Full database dump\n";
  msg << "--------------------------\n";

  for (auto entry = _data.cbegin(); entry != _data.cend(); entry++) {
    ParticleIdentifier identifier = entry->first;
    ParticleEvents     history    = entry->second;

    msg << "\n" << identifier.toString();
    int meshSweepIndex = -1;

    for (auto event = history.crbegin(); event != history.crend(); event++) {
      if (event->meshSweepIndex != meshSweepIndex) {
        std::string sweepname = _meshSweepData.at(event->meshSweepIndex)
                                  .getName();
        msg << "\n\t[Sweep " << sweepname << ",index=" << event->meshSweepIndex
            << "]:";
      }
      meshSweepIndex = event->meshSweepIndex;
      msg << "\n\t\t->" << event->toString();
    }
  }

  msg << "\n";

  return msg.str();
}


int toolbox::particles::assignmentchecks::internal::Database::
  getTotalParticleEntries(const ParticleIdentifier& identifier) {

  tarch::multicore::MultiReadSingleWriteLock
    lock(_semaphore, tarch::multicore::MultiReadSingleWriteLock::Read);

  auto search = _data.find(identifier);

  if (search != _data.end()) {

#if PeanoDebug > 3
    assertion3(
      search->first.particleID == identifier.particleID,
      "Particle identifier ID not identical",
      search->first.particleID,
      identifier.particleID
    );
    assertion3(
      search->first.treeID == identifier.treeID,
      "Particle treeID not identical",
      search->first.treeID,
      identifier.treeID
    );
    for (int i = 0; i < Dimensions; i++) {
      assertion3(
        search->first.vertexInd(i) == identifier.vertexInd(i),
        "Particle treeID not identical",
        search->first.vertexInd(i),
        identifier.vertexInd(i)
      );
    }
    assertion3(
      search->first.vertexDepth == identifier.vertexDepth,
      "Particle depth not identical",
      search->first.vertexDepth,
      identifier.vertexDepth
    );
#endif

    int result = search->second.size();
    return result;
  }
  return 0;
}


std::string toolbox::particles::assignmentchecks::internal::Database::
  lastMeshSweepSnapshot() {
  tarch::multicore::MultiReadSingleWriteLock
    lock(_semaphore, tarch::multicore::MultiReadSingleWriteLock::Read);

  std::ostringstream msg;
  int                cmswi = getCurrentMeshSweepIndex();

  msg << "--------------------------\n";
  msg << "Last sweep dump: Sweep [" << _meshSweepData.at(cmswi).getName()
      << "]\n";
  msg << "--------------------------\n";

  for (auto entry = _data.cbegin(); entry != _data.cend(); entry++) {
    ParticleIdentifier identifier = entry->first;
    ParticleEvents     history    = entry->second;

    msg << "\n" << identifier.toString();

    for (auto event = history.crbegin(); event != history.crend(); event++) {
      if (event->meshSweepIndex != cmswi) {
        break;
      }
      msg << "\n\t->" << event->toString();
    }
  }

  return msg.str();
}


std::string toolbox::particles::assignmentchecks::internal::Database::
  sweepHistory() {

  tarch::multicore::MultiReadSingleWriteLock
    lock(_semaphore, tarch::multicore::MultiReadSingleWriteLock::Read);

  std::ostringstream msg;
  int                counter = 0;
  for (auto sweep = _meshSweepData.cbegin(); sweep != _meshSweepData.cend();
       sweep++) {
    msg << std::endl << "sweep #" << counter << ": " << sweep->getName();
    counter++;
  }
  msg << "\n";

  return msg.str();
}


std::string toolbox::particles::assignmentchecks::internal::Database::
  particleHistory(const ParticleIdentifier& identifier) {

  tarch::multicore::MultiReadSingleWriteLock
    lock(_semaphore, tarch::multicore::MultiReadSingleWriteLock::Read);

  std::ostringstream msg;
  msg << "\n============================\n"
      << identifier.toString() << "\n============================\n";

  auto search = _data.find(identifier);
  if (search != _data.end()) {

#if PeanoDebug > 3
    assertion3(
      search->first.particleID == identifier.particleID,
      "Particle identifier ID not identical",
      search->first.particleID,
      identifier.particleID
    );
    assertion3(
      search->first.treeID == identifier.treeID,
      "Particle treeID not identical",
      search->first.treeID,
      identifier.treeID
    );
    for (int i = 0; i < Dimensions; i++) {
      assertion3(
        search->first.vertexInd(i) == identifier.vertexInd(i),
        "Particle treeID not identical",
        search->first.vertexInd(i),
        identifier.vertexInd(i)
      );
    }
    assertion3(
      search->first.vertexDepth == identifier.vertexDepth,
      "Particle depth not identical",
      search->first.vertexDepth,
      identifier.vertexDepth
    );
#endif

    ParticleEvents& history = search->second;
    auto            ev      = history.crbegin();

    // mesh sweep index of the previous event.
    int prevMeshSweepInd = -1;

    while (ev != history.crend()) {

      int meshSweepInd = ev->meshSweepIndex;

      if (meshSweepInd != prevMeshSweepInd) {
        // We're starting new sweep. Print header.
        msg << "\nsweep #" << meshSweepInd << " ("
            << _meshSweepData.at(meshSweepInd).getName() << "):";
      }
      msg << "\n\t->" << ev->toString();

      prevMeshSweepInd = meshSweepInd;
      ev++;
    }
  }

  msg << "\n";

  return msg.str();
}


toolbox::particles::assignmentchecks::internal::Event toolbox::particles::
  assignmentchecks::internal::Database::
    addEvent(ParticleIdentifier identifier, Event& event) {

  logDebug("addEvent()", "Adding event for " << identifier.toString());

  tarch::multicore::MultiReadSingleWriteLock
    lock(_semaphore, tarch::multicore::MultiReadSingleWriteLock::Write);

  // Take note of the current mesh sweep index.
  event.meshSweepIndex = getCurrentMeshSweepIndex();

  // Try finding existing previous event.
  auto search = _data.find(identifier);

  if (search == _data.end()) {
    // This is a new particle.

    ParticleEvents newEventHistory = ParticleEvents();
    newEventHistory.push_back(event);
    _data.insert(
      std::pair<ParticleIdentifier, ParticleEvents>(identifier, newEventHistory)
    );
    logDebug(
      "addEvent(...) ",
      "add new particle history thread for " + identifier.toString()
        + ",Event:" + event.toString()
    );
    // there is no previous event.
    return Event(Event::Type::NotFound);

  } else {

#if PeanoDebug > 3
    assertion3(
      search->first.particleID == identifier.particleID,
      "Particle identifier ID not identical",
      search->first.particleID,
      identifier.particleID
    );
    assertion3(
      search->first.treeID == identifier.treeID,
      "Particle treeID not identical",
      search->first.treeID,
      identifier.treeID
    );
    for (int i = 0; i < Dimensions; i++) {
      assertion3(
        search->first.vertexInd(i) == identifier.vertexInd(i),
        "Particle treeID not identical",
        search->first.vertexInd(i),
        identifier.vertexInd(i)
      );
    }
    assertion3(
      search->first.vertexDepth == identifier.vertexDepth,
      "Particle depth not identical",
      search->first.vertexDepth,
      identifier.vertexDepth
    );
#endif


    ParticleEvents& history = search->second;

    // First: Do we need to downsize the history?
    if (history.size() >= _maxParticleEventsToKeepTrackOf) {

      // make copy, not reference
      Event lastEvent = history.back();

#if PeanoDebug > 3
      if (lastEvent.type == Event::Type::NotFound) {
        logDebug(
          "addEvent(...)",
          "Particle histroy shouldn't contain an invalid event"
        );
      }
#endif

      // In certain cases, especially for virtual particles, the last saved
      // event may be a substitution already. We don't want to keep
      // prepending the string with the notifier, so check for it.
      if (not lastEvent.trace.starts_with("substitute-for-whole-trajectory/")) {
        lastEvent.trace = "substitute-for-whole-trajectory/" + lastEvent.trace;
      }

      logDebug(
        "addEvent(...) ",
        "shortening history of " << identifier.toString(
        ) << " to " << lastEvent.toString()
      );

      history.clear();
      history.push_back(lastEvent);
    }

    // Add the new event now.
    Event previousEvent = history.back();
    history.push_back(event);
    logDebug(
      "addEvent(...) ",
      "added event " + event.toString() + " " + identifier.toString()
    );

    return previousEvent;
  }
}


void toolbox::particles::assignmentchecks::internal::Database::replaceEvent(
  ParticleIdentifier identifier,
  Event&             oldEvent,
  Event&             newEvent
) {

  tarch::multicore::MultiReadSingleWriteLock
    lock(_semaphore, tarch::multicore::MultiReadSingleWriteLock::Write);

  // Take note of the current mesh sweep index.
  newEvent.meshSweepIndex = getCurrentMeshSweepIndex();

  auto search = _data.find(identifier);

  assertion1(
    search != _data.end(),
    "Trying to replace event of particle with no history"
  );

#if PeanoDebug > 3
  assertion3(
    search->first.particleID == identifier.particleID,
    "Particle identifier ID not identical",
    search->first.particleID,
    identifier.particleID
  );
  assertion3(
    search->first.treeID == identifier.treeID,
    "Particle treeID not identical",
    search->first.treeID,
    identifier.treeID
  );
  for (int i = 0; i < Dimensions; i++) {
    assertion3(
      search->first.vertexInd(i) == identifier.vertexInd(i),
      "Particle treeID not identical",
      search->first.vertexInd(i),
      identifier.vertexInd(i)
    );
  }
  assertion3(
    search->first.vertexDepth == identifier.vertexDepth,
    "Particle depth not identical",
    search->first.vertexDepth,
    identifier.vertexDepth
  );
#endif

  ParticleEvents& history = search->second;

  // Get previous event
  Event previousEventOnThisTree = Event(Event::Type::NotFound);

  for (auto it = history.begin(); it != history.end(); it++) {
    if ((it->meshSweepIndex == oldEvent.meshSweepIndex) and
        (it->type == oldEvent.type) and
        (it->treeId == oldEvent.treeId) and
        (it->isLocal == oldEvent.isLocal) ){

      it->type           = newEvent.type;
      it->isLocal        = newEvent.isLocal;
      it->vertexX        = newEvent.vertexX;
      it->vertexH        = newEvent.vertexH;
      it->treeId         = newEvent.treeId;
      it->trace          = newEvent.trace;
      it->meshSweepIndex = newEvent.meshSweepIndex;

      break;
    }
  }
}


void toolbox::particles::assignmentchecks::internal::Database::
  shiftIdentifierVertexInd(
    ParticleIdentifier&                       identifier,
    tarch::la::Vector<Dimensions, long long>& newVertexInd,
    const int                                 newVertexDepth
  ) {

  tarch::multicore::MultiReadSingleWriteLock
    lock(_semaphore, tarch::multicore::MultiReadSingleWriteLock::Write);

  auto search = _data.find(identifier);

  assertion1(
    search != _data.end(),
    "Particle not found through its identifier"
  );

#if PeanoDebug > 3
  assertion3(
    search->first.particleID == identifier.particleID,
    "Particle identifier ID not identical",
    search->first.particleID,
    identifier.particleID
  );
  assertion3(
    search->first.treeID == identifier.treeID,
    "Particle treeID not identical",
    search->first.treeID,
    identifier.treeID
  );
  for (int i = 0; i < Dimensions; i++) {
    assertion3(
      search->first.vertexInd(i) == identifier.vertexInd(i),
      "Particle treeID not identical",
      search->first.vertexInd(i),
      identifier.vertexInd(i)
    );
  }
  assertion3(
    search->first.vertexDepth == identifier.vertexDepth,
    "Particle depth not identical",
    search->first.vertexDepth,
    identifier.vertexDepth
  );
#endif

  // Do we need to shift the coordinates of the identifier?  Use the one the
  // search gives you back, because we allow for a fuzzy search.  You need to
  // compare to the actually stored coordinates, not to whatever you think they
  // currently may be.
  ParticleIdentifier key     = search->first;
  ParticleEvents&    history = search->second;

  // delete old entry in database and add the new one.
  logDebug(
    "shiftIdentifierCoordinates(...) ",
    "shifting particle identifier from "
      << key.vertexInd << " to " << newVertexInd;
  );

  ParticleEvents historyCopy = history;
  _data.erase(key);
  ParticleIdentifier newIdentifier = ParticleIdentifier(
    identifier.particleName,
    identifier.particleX,
    newVertexInd,
    newVertexDepth,
    identifier.particleID,
    identifier.treeID
  );
  // Important: if particle already exists with this identifier (e.g. because
  // if was on this vertex before), map.insert() will not replace the existing
  // entry. Use insert_or_assign instead, which replaces it.
  // TODO: Instead of replacing, append history.
  _data.insert_or_assign(newIdentifier, historyCopy);
}


void toolbox::particles::assignmentchecks::internal::Database::
  eliminateExistingParticles() {

  tarch::multicore::MultiReadSingleWriteLock
    lock(_semaphore, tarch::multicore::MultiReadSingleWriteLock::Write);

  for (auto it = _data.begin(); it != _data.end();) {
    internal::Event& lastEvent = it->second.back();

    if (lastEvent.type == internal::Event::Type::AssignToVertex
        or lastEvent.type == internal::Event::Type::ConsecutiveMoveWhileAssociatedToVertex
        or lastEvent.type == internal::Event::Type::MoveWhileAssociatedToVertex
        or (not lastEvent.isLocal)) {
      ++it;
    } else {
      it = _data.erase(it);
    }
  }
}


toolbox::particles::assignmentchecks::internal::Event toolbox::particles::
  assignmentchecks::internal::Database::getPreviousEvent(
    ParticleEvents& particleHistory,
    int             spacetreeId,
    size_t          nFirstEventsToSkip
  ) {

  tarch::multicore::MultiReadSingleWriteLock
    lock(_semaphore, tarch::multicore::MultiReadSingleWriteLock::Read);

  if (particleHistory.size() <= nFirstEventsToSkip) {
    return Event(Event::Type::NotFound);
  }

  auto it = particleHistory.crbegin();
  std::advance(it, nFirstEventsToSkip);

  if (it == particleHistory.crend()) {
    return Event(Event::Type::NotFound);
  }

  if (spacetreeId == Database::AnyTree)
    return *it;

  while (it != particleHistory.crend()) {
    if (it->treeId == spacetreeId)
      return *it;
    it++;
  }

  return Event(Event::Type::NotFound);
}
