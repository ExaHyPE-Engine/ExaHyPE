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
std::cerr << "DATABASE_TRACE:" << "startMeshSweep," << meshSweepName << "\n";
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

std::cerr << "DATABASE_TRACE:" << "reset\n";

  logDebug("reset()", "Resetting database");

  _data.clear();
  _meshSweepData.clear();

  // and re-initialize.
  _meshSweepData.push_back(MeshSweepData("initial"));
  _currentMeshSweepIndex = 0;
}


toolbox::particles::assignmentchecks::internal::ParticleEvents toolbox::
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

    assertion1(search->first.particleID == identifier.particleID, "Particle identifier IDs not equal");
    ParticleEvents history = search->second;
    return history;
  }
}


std::string toolbox::particles::assignmentchecks::internal::Database::toString(
) {
  tarch::multicore::MultiReadSingleWriteLock
    lock(_semaphore, tarch::multicore::MultiReadSingleWriteLock::Read);

std::cerr << "DATABASE_TRACE:" << "toString\n";

  std::ostringstream msg;

  msg << "--------------------------\n";
  msg << "Full database dump\n";
  msg << "--------------------------\n";

  for (auto entry = _data.cbegin(); entry != _data.cend(); entry++) {
    ParticleSearchIdentifier identifier = entry->first;
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

std::cerr << "DATABASE_TRACE:" << "getTotalParticleEntries," << identifier.toString() << "\n";

  auto search = _data.find(identifier);

  if (search != _data.end()) {
    assertion(search->first.particleID == identifier.particleID);
    int result = search->second.size();
    return result;
  }
  return 0;
}


std::string toolbox::particles::assignmentchecks::internal::Database::
  lastMeshSweepSnapshot() {
  tarch::multicore::MultiReadSingleWriteLock
    lock(_semaphore, tarch::multicore::MultiReadSingleWriteLock::Read);

 std::cerr << "DATABASE_TRACE:" << "lastMeshSweepSnapshot\n";

  std::ostringstream msg;
  int                cmswi = getCurrentMeshSweepIndex();

  msg << "--------------------------\n";
  msg << "Last sweep dump: Sweep [" << _meshSweepData.at(cmswi).getName()
      << "]\n";
  msg << "--------------------------\n";

  for (auto entry = _data.cbegin(); entry != _data.cend(); entry++) {
    ParticleSearchIdentifier identifier = entry->first;
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

std::cerr << "DATABASE_TRACE:" << "sweepHistory\n";

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
  particleHistory(const ParticleSearchIdentifier& identifier) {

std::cerr << "DATABASE_TRACE:" << "particleHistory"<< identifier.toString() << "\n";

  tarch::multicore::MultiReadSingleWriteLock
    lock(_semaphore, tarch::multicore::MultiReadSingleWriteLock::Read);

  std::ostringstream msg;
  msg << "\n============================\n"
      << identifier.toString() << "\n============================\n";

  auto search = _data.find(identifier);
  if (search != _data.end()) {

    assertion(search->first.particleID == identifier.particleID);

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
  assignmentchecks::internal::Database::addEvent(
    ParticleSearchIdentifier identifier,
    Event&                   event,
    const int                treeId
  ) {

std::cerr << "DATABASE_TRACE:" << "addEvent," << identifier.toString() << "," << event.toString() << "," << treeId << "\n";

  logDebug("addEvent()", "Adding event for " << identifier.toString());

  tarch::multicore::MultiReadSingleWriteLock
    lock(_semaphore, tarch::multicore::MultiReadSingleWriteLock::Write);

  // Take note of the current mesh sweep index.
  event.meshSweepIndex = getCurrentMeshSweepIndex();

if (identifier.particleID == 2325473) {
  lock.~MultiReadSingleWriteLock();
  std::cout << "====================== In AddEvent: ID=" << identifier.particleID << " start search\n";
}
  auto search = _data.find(identifier);

  if (search == _data.end()) {
    // This is a new particle.

    ParticleEvents newEventHistory = ParticleEvents();
    newEventHistory.push_back(event);
    _data.insert(
      std::pair<ParticleSearchIdentifier, ParticleEvents>(identifier, newEventHistory)
    );
    logDebug(
      "addEvent(...) ",
      "add new particle history thread for " + identifier.toString()
        + ",Event:" + event.toString()
    );
    // there is no previous event.
if (identifier.particleID == 2325473) {
  std::cout << "====================== In AddEvent: NEW PARTICLE ID=" << identifier.particleID << "\n" << toString() << std::endl;
}
    return Event(Event::Type::NotFound);

  } else {

    assertion(search->first.particleID == identifier.particleID);
    ParticleEvents& history = search->second;

    // First: Do we need to downsize the history?
    if (history.size() >= _maxParticleSnapshotsToKeepTrackOf) {

      ParticleEvents   eventsToKeep;
      std::vector<int> treesToKeep;

      // Start from the back so we keep the latest event.
      auto it = history.crbegin();
      while (it != history.crend()) {

        Event past     = *it;
        int   thisTree = past.treeId;

#if PeanoDebug > 3
        if (past.type == Event::Type::NotFound) {
          logDebug(
            "addEvent(...)",
            "Particle histroy shouldn't contain an invalid event"
          );
        }
#endif

        if (std::find(treesToKeep.begin(), treesToKeep.end(), thisTree) == treesToKeep.end()){
          treesToKeep.push_back(thisTree);
          // In certain cases, especially for virtual particles, the last saved
          // event may be a substitution already. We don't want to keep
          // prepending the string with the notifier, so check for it.
          if (not past.trace.starts_with("substitute-for-whole-trajectory/")) {
            past.trace = "substitute-for-whole-trajectory/" + past.trace;
          }
          eventsToKeep.push_back(past);
        }

        it++;
      }

      logDebug(
        "addEvent(...) ",
        "shortening history of " << identifier.toString(
        ) << " to " << eventsToKeep.size()
      );

      if (eventsToKeep.size() >= _maxParticleSnapshotsToKeepTrackOf) {
        logError(
          "addEvent(...)",
          "There are more trees than events the database is allowed to keep track of."
        );
      }

      history.clear();
      for (Event keep : eventsToKeep) {
        history.push_back(keep);
      }
    }

    // Get previous event
    Event previousEventOnThisTree = Event(Event::Type::NotFound);

    auto it = history.crbegin();
    while (it != history.crend()) {
      if (it->treeId == treeId) {
        previousEventOnThisTree = *it;
        break;
      }
      it++;
    }

    // Add the new event now.
    history.push_back(event);
    logDebug(
      "addEvent(...) ",
      "added event " + event.toString() + " "
      + identifier.toString()
    );

if (identifier.particleID == 2325473) {
  lock.~MultiReadSingleWriteLock();
  std::cout << "====================== In AddEvent: FOUND PARTICLE ID=" << identifier.particleID << "\n" << toString() << std::endl;
}
    return previousEventOnThisTree;
  }
}


void toolbox::particles::assignmentchecks::internal::Database::replaceEvent(
  ParticleSearchIdentifier identifier,
  Event&                   oldEvent,
  Event&                   newEvent
) {

std::cerr << "DATABASE_TRACE:" << "replaceEvent," << identifier.toString() << "," << oldEvent.toString() << "," << newEvent.toString() << "\n";

  tarch::multicore::MultiReadSingleWriteLock
    lock(_semaphore, tarch::multicore::MultiReadSingleWriteLock::Write);

  // Take note of the current mesh sweep index.
  newEvent.meshSweepIndex = getCurrentMeshSweepIndex();

  auto search = _data.find(identifier);

  assertion1(
    search != _data.end(),
    "Trying to replace event of particle with no history"
  );

  assertion1(search->first.particleID == identifier.particleID, "Particle identifier ID not identical");

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
  shiftIdentifierCoordinates(
    toolbox::particles::assignmentchecks::internal::ParticleSearchIdentifier
                                          identifier,
    tarch::la::Vector<Dimensions, double> newParticleX
  ) {

std::cerr << "DATABASE_TRACE:" << "shiftIdentifierCoordinates," << identifier.toString() << "," << newParticleX << "\n";

  tarch::multicore::MultiReadSingleWriteLock
    lock(_semaphore, tarch::multicore::MultiReadSingleWriteLock::Write);

  std::vector<tarch::la::Vector<Dimensions, double>> positions;
  int count = 0;
  for (auto e = _data.cbegin(); e!= _data.cend(); e++){
    if (e->first.particleID == 2325473){
      count += 1;
      positions.push_back(e->first.particleX);
    }
  }

  std::cout << "==================================== BEFORE SHIFT: COUNT ELEMENTS="<< count << "\n";
  for (auto p: positions){
    std::cout << p << ",";
  }
  std::cout << std::endl;







  auto search = _data.find(identifier);

  assertion1(
    search != _data.end(),
    "Particle not found through its identifier"
  );

  assertion1(search->first.particleID == identifier.particleID, "Identifier IDs not equal");

  // Do we need to shift the coordinates of the identifier?  Use the one the
  // search gives you back, because we allow for a fuzzy search.  You need to
  // compare to the actually stored coordinates, not to whatever you think they
  // currently may be.
  ParticleSearchIdentifier key     = search->first;
  ParticleEvents&    history = search->second;

// if (identifier.particleID == 2325473) {
  std::cout << "========================== In SHIFT: ID=" << identifier.particleID <<
    " x=" << identifier.particleX << " x2=" << newParticleX << " x_key=" << key.particleX <<
    " dx=" << key.particleX - newParticleX <<
    " tol:" << ParticleSearchIdentifier::shiftTolerance * identifier.positionTolerance << std::endl;
// }

  bool shift = tarch::la::oneGreater(
    tarch::la::abs(key.particleX - newParticleX),
    // ParticleSearchIdentifier::shiftTolerance * identifier.positionTolerance
    0.
  );

  if (shift) {
// if (identifier.particleID == 2325473) {
//   std::cout << "========================== SHIFTING" << std::endl;
// }
    // delete old entry in database and add the new one.
    logDebug(
      "shiftIdentifierCoordinates(...) ",
      "shifting particle identifier " <<
      key.toString() << " from " << key.particleX << " to " << newParticleX
    );

    ParticleEvents historyCopy = history;
    std::cout << "erase" << std::endl;
    _data.erase(identifier);
    ParticleSearchIdentifier newIdentifier = ParticleSearchIdentifier(
      identifier.particleName,
      newParticleX,
      identifier.particleID,
      identifier.positionTolerance
    );
    std::cout << "insert" << std::endl;
    _data.insert(
      std::pair<ParticleSearchIdentifier, ParticleEvents>(newIdentifier, historyCopy)
    );



if (identifier.particleID == 2325473) {
  lock.~MultiReadSingleWriteLock();
  std::cout << "========================== AFTER SHIFTING\n" << toString() << std::endl;
}


  std::vector<tarch::la::Vector<Dimensions, double>> positions2;
  count = 0;
  for (auto e = _data.cbegin(); e!= _data.cend(); e++){
    if (e->first.particleID == 2325473){
      count += 1;
      positions2.push_back(e->first.particleX);
    }
  }

  std::cout << "==================================== AFTER SHIFT: COUNT ELEMENTS="<< count << "\n";
  for (auto p: positions2){
    std::cout << p << ",";
  }
  std::cout << std::endl;




  }
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
