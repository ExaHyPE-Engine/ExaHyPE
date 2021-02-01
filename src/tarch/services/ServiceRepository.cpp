#include "tarch/services/ServiceRepository.h"
#include "tarch/Assertions.h"
#include "tarch/tarch.h"
#include "tarch/multicore/RecursiveLock.h"


#include <sstream>


tarch::services::ServiceRepository  tarch::services::ServiceRepository::_singleton;


tarch::services::ServiceRepository::ServiceRepository():
  _services() {
}


tarch::services::ServiceRepository::~ServiceRepository() {
  _services.clear();
}


void tarch::services::ServiceRepository::init() {
}


void tarch::services::ServiceRepository::shutdown() {
}


tarch::services::ServiceRepository& tarch::services::ServiceRepository::getInstance() {
  return _singleton;
}


void tarch::services::ServiceRepository::addService( Service* const service, const std::string& name ) {
  assertion2( service!=0, name, getListOfRegisteredServices() );
  assertion2( !name.empty(), name, getListOfRegisteredServices() );
  assertion2( !hasService(service), name, getListOfRegisteredServices() );

  ServiceEntry entry;
  entry._name    = name;
  entry._service = service;
  _services.push_back( entry );
}


void tarch::services::ServiceRepository::removeService( Service* const service ) {
  for (std::vector<ServiceEntry>::iterator p=_services.begin(); p!=_services.end(); ) {
	if ( p->_service==service) {
	  p = _services.erase(p);
	}
	else p++;
  }
}


bool tarch::services::ServiceRepository::hasService( Service* service ) const {
  for (
      ServiceContainer::const_iterator p = _services.begin();
      p != _services.end();
      p++
  ) {
    if (p->_service==service) {
      return true;
    }
  }
  return false;
}


void tarch::services::ServiceRepository::receiveDanglingMessages() {
  tarch::multicore::RecursiveLock  lock(_receiveDanglingMessagesSemaphore,false);
  if ( lock.tryLock() ) {
    for (
      ServiceContainer::iterator p = _services.begin();
      p != _services.end();
      p++
    ) {
        p->_service->receiveDanglingMessages();
    }
  }
} 


std::string tarch::services::ServiceRepository::getListOfRegisteredServices() const {
  std::ostringstream result;
  for (
      ServiceContainer::const_iterator p = _services.begin();
      p != _services.end();
      p++
  ) {
    result << " " << p->_name;
  }
  return result.str();
}

