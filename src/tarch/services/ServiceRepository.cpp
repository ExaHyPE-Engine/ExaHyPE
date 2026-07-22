#include "ServiceRepository.h"

#include "tarch/Assertions.h"
#include "tarch/multicore/RecursiveLock.h"
#include "tarch/multicore/Task.h"
#include "tarch/MemoryUsage.h"

tarch::services::ServiceRepository tarch::services::ServiceRepository::_singleton;

tarch::services::ServiceRepository::ServiceRepository() :
  _services() {}

tarch::services::ServiceRepository::~ServiceRepository() { _services.clear(); }

void tarch::services::ServiceRepository::init() {}

void tarch::services::ServiceRepository::shutdown() {
  tarch::multicore::RecursiveLock lock(_receiveDanglingMessagesSemaphore);

  std::vector<Service*> services;
  services.reserve(_services.size());
  for (const auto& entry : _services) {
    services.push_back(entry._service);
  }

  for (Service* service : services) {
    if (hasService(service)) {
      service->shutdown();
    }
  }

  _services.clear();
}

tarch::services::ServiceRepository& tarch::services::ServiceRepository::getInstance() { return _singleton; }

/**
 * Add a new service
 *
 * This routine is thread-safe, i.e. it blocks other addService() calls as
 * well as receiveDanglingMessages().
 *
 * ## Implementation
 *
 * The recursive semaphore is skipped for TBB builds. addService() runs very
 * early in the start-up, and taking the lock at that point yields
 * segmentation faults with TBB.
 *
 * @param service Pointer to service
 * @param name    Name of service (mandatory, not empty)
 */
void tarch::services::ServiceRepository::addService(Service* const service, const std::string& name) {
  assertion(service != 0, name, getListOfRegisteredServices());
  assertion(!name.empty(), name, getListOfRegisteredServices());
  assertion(!hasService(service), name, getListOfRegisteredServices());

  ServiceEntry entry;
  entry._name    = name;
  entry._service = service;

#if !defined(SharedTBB)
  tarch::multicore::RecursiveLock lock(_receiveDanglingMessagesSemaphore);
#endif

  _services.push_back(entry);
}

/**
 * Remove a service from the registry
 *
 * This routine is thread-safe, i.e. it blocks out receiveDanglingMessages()
 * and every other routine which registers or de-registers services.
 */
void tarch::services::ServiceRepository::removeService(Service* const service) {
  tarch::multicore::RecursiveLock lock(_receiveDanglingMessagesSemaphore);
  for (std::vector<ServiceEntry>::iterator p = _services.begin(); p != _services.end();) {
    if (p->_service == service) {
      p = _services.erase(p);
    } else
      p++;
  }
}

bool tarch::services::ServiceRepository::hasService(Service* service) const {
  for (ServiceContainer::const_iterator p = _services.begin(); p != _services.end(); p++) {
    if (p->_service == service) {
      return true;
    }
  }
  return false;
}

/**
 * Answer to MPI messages
 *
 * Tell all registered services to answer to MPI messages that are still
 * pending in the MPI queues.
 *
 * ## MPI tuning
 *
 * The registered services are polled without an Iprobe guard in front of
 * them. A service therefore has to cope with being asked when it has nothing
 * to receive.
 *
 * ## Starvation
 *
 * A call to this routine signals a lack of MPI progression or poor load
 * balancing, so it is tempting to let the calling thread process a task
 * meanwhile. That backfires once there are many tasks: up to p-1 threads then
 * hit the task queue, block each other, and no MPI progress is made at all,
 * as very few tasks issue MPI calls. The routine hence confines itself to
 * polling the services.
 *
 * ## Receive dangling messages in a multithreaded environment
 *
 * Never call receiveDanglingMessages() of a service directly, always go
 * through the repository. Most services do not protect their
 * receiveDanglingMessages() with an additional semaphore, so two threads may
 * both issue an Iprobe. With one message in the queue, both probes return
 * true and both threads issue a receive, yet only one of them succeeds. The
 * repository blocks all other threads with a semaphore before it invokes the
 * services, and thus avoids the race.
 *
 * This routine is thread-safe compared to addService() and removeService().
 */
void tarch::services::ServiceRepository::receiveDanglingMessages() {
  tarch::multicore::RecursiveLock lock(_receiveDanglingMessagesSemaphore, false);
  if (lock.tryLock()) {
    for (ServiceContainer::iterator p = _services.begin(); p != _services.end(); p++) {
      p->_service->receiveDanglingMessages();
    }
  }
}

std::string tarch::services::ServiceRepository::getListOfRegisteredServices() const {
  std::ostringstream result;
  for (ServiceContainer::const_iterator p = _services.begin(); p != _services.end(); p++) {
    result << " " << p->_name;
  }
  return result.str();
}
