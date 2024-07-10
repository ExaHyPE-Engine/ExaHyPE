// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org

#include "Database.h"

#include <iterator>
#include <limits>
#include <ranges>
#include <vector>

#include "Event.h"
#include "ParticleIdentifier.h"
#include "tarch/la/Vector.h"
#include "tarch/multicore/MultiReadSingleWriteLock.h"


namespace {
  tarch::logging::Log _log("toolbox::particles::assignmentchecks");
} // namespace


void toolbox::particles::assignmentchecks::internal::Database::
  setMaxParticleSnapshotsToKeepTrackOf(const size_t n) {
  _maxParticleSnapshotsToKeepTrackOf = n;
}


void toolbox::particles::assignmentchecks::internal::Database::startMeshSweep(
  const std::string& meshSweepName
) {

  logInfo(
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
  return _data.size();
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

  _data.clear();
  _meshSweepData.clear();

  // and re-initialize.
  _meshSweepData.push_back(MeshSweepData("initial"));
  _currentMeshSweepIndex = 0;
}


toolbox::particles::assignmentchecks::internal::ParticleEvents& toolbox::
  particles::assignmentchecks::internal::Database::getParticleHistory(
    const ParticleSearchIdentifier& identifier
  ) {
  tarch::multicore::MultiReadSingleWriteLock
    lock(_semaphore, tarch::multicore::MultiReadSingleWriteLock::Read);

  auto search = _data.find(identifier);

  if (search == _data.end()) {
    ParticleEvents* newEventHistory = new ParticleEvents();
    newEventHistory->push_back(Event(Event::Type::NotFound));
    return *newEventHistory;
  } else {

    ParticleEvents& history = search->second;
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
        msg << "\n\t[Sweep " << sweepname << "]:";
      }
      meshSweepIndex = event->meshSweepIndex;
      msg << "\n\t\t->" << event->toString();
    }
  }

  msg << "\n";

  return msg.str();
}


int toolbox::particles::assignmentchecks::internal::Database::
  getTotalParticleEntries(const ParticleSearchIdentifier& identifier) {
  tarch::multicore::MultiReadSingleWriteLock
    lock(_semaphore, tarch::multicore::MultiReadSingleWriteLock::Read);

  auto search = _data.find(identifier);
  if (search != _data.end()) {
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

  msg << "--------------------------\n";
  msg << "Last sweep dump: Sweep ["
      << _meshSweepData.at(_currentMeshSweepIndex).getName() << "]\n";
  msg << "--------------------------\n";

  for (auto entry = _data.cbegin(); entry != _data.cend(); entry++) {
    ParticleIdentifier identifier = entry->first;
    ParticleEvents     history    = entry->second;

    msg << "\n" << identifier.toString();

    for (auto event = history.crbegin(); event != history.crend(); event++) {
      if (event->meshSweepIndex != _currentMeshSweepIndex) {
        break;
      }
      msg << "\n\t->" << event->toString();
    }
  }

  return msg.str();
}


std::string toolbox::particles::assignmentchecks::internal::Database::
  sweepHistory() const {

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
  particleHistory(const ParticleSearchIdentifier& identifier) {

  tarch::multicore::MultiReadSingleWriteLock
    lock(_semaphore, tarch::multicore::MultiReadSingleWriteLock::Read);

  std::ostringstream msg;
  msg << "\n============================\n"
      << identifier.toString() << "\n============================\n";

  auto search = _data.find(identifier);
  if (search != _data.end()) {

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


toolbox::particles::assignmentchecks::internal::ParticleEvents& toolbox::
  particles::assignmentchecks::internal::Database::
    addEvent(ParticleSearchIdentifier identifier, Event& event) {

  tarch::multicore::MultiReadSingleWriteLock
    lock(_semaphore, tarch::multicore::MultiReadSingleWriteLock::Write);

  ParticleEvents output;

  // Take note of the current mesh sweep index.
  event.meshSweepIndex = getCurrentMeshSweepIndex();

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
    );
    return _data.at(identifier);

  } else {

    ParticleEvents& history = search->second;

    // First: Do we need to downsize the history?
    if (history.size() >= _maxParticleSnapshotsToKeepTrackOf) {

      Event last = history.back();

      logDebug("addEvent(...) ", "shortening history to " + last.toString());

      last.trace = "substitute-for-whole-trajectory/" + last.trace;
      history.clear();
      history.push_back(last);
    }

    // Add the new event now.
    history.push_back(event);
    logDebug(
      "addEvent(...) ",
      "added event " + event.toString() + " "
        + ::toString((identifier.particleX));
    );

    return history;
  }
}


void toolbox::particles::assignmentchecks::internal::Database::
  shiftIdentifierCoordinates(
    toolbox::particles::assignmentchecks::internal::ParticleSearchIdentifier
                                          identifier,
    tarch::la::Vector<Dimensions, double> newParticleX
  ) {

  auto search = _data.find(identifier);

  assertion1(
    search != _data.end(),
    "Particle not found through its identifier"
  );

  // Do we need to shift the coordinates of the identifier?  Use the one the
  // search gives you back, because we allow for a fuzzy search.  You need to
  // compare to the actually stored coordinates, not to whatever you think they
  // currently may be.
  ParticleIdentifier key     = search->first;
  ParticleEvents&    history = search->second;

  bool shift = tarch::la::oneGreater(
    tarch::la::abs(key.particleX - newParticleX),
    ParticleSearchIdentifier::shiftTolerance * identifier.positionTolerance
  );

  if (shift) {
    // delete old entry in database and add the new one.
    logDebug(
      "addEvent(...) ",
      "shifting particle identifier from "
        << key.particleX << " to " << newParticleX
    );

    ParticleEvents historyCopy = history;
    _data.erase(key);
    ParticleIdentifier newIdentifier = ParticleIdentifier(
      identifier.particleName,
      newParticleX,
      identifier.particleID
    );
    _data.insert(
      std::pair<ParticleIdentifier, ParticleEvents>(newIdentifier, historyCopy)
    );
  }
}


toolbox::particles::assignmentchecks::internal::Event toolbox::particles::
  assignmentchecks::internal::getPreviousEvent(
    ParticleEvents& particleHistory,
    int             spacetreeId,
    size_t          nFirstEventsToSkip
  ) {

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
