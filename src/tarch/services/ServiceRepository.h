// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/services/Service.h"

namespace tarch {
  namespace services {
    class ServiceRepository;
  } // namespace services
} // namespace tarch

/**
 * Service repository
 *
 * See the interface Service for a detailed description. The registry is a
 * service itself, but it does not register as a service.
 *
 * @author Tobias Weinzierl
 */
class tarch::services::ServiceRepository : public tarch::services::Service {
private:
  struct ServiceEntry {
    std::string _name;
    Service*    _service;
  };

  using ServiceContainer = std::vector<ServiceEntry>;

  ServiceContainer _services;

  tarch::multicore::RecursiveSemaphore _receiveDanglingMessagesSemaphore;

  /**
   * @see tarch::services::Service for a description how to realise singletons.
   */
  static tarch::services::ServiceRepository _singleton;

  ServiceRepository();

public:
  virtual ~ServiceRepository();

  static ServiceRepository& getInstance();

  /**
   * Add a new service
   *
   * @param service Pointer to service
   * @param name    Name of service (mandatory, not empty)
   */
  void addService(Service* const service, const std::string& name);

  /**
   * This routine is thread-safe.
   */
  bool hasService(Service* service) const;

  /**
   * Remove a service from the registry
   */
  void removeService(Service* const service);

  /**
   * Answer to MPI messages
   */
  void receiveDanglingMessages() override;

  /**
   * @return List of registered services separated by whitespace
   */
  std::string getListOfRegisteredServices() const;

  /**
   * The repository holds no state which needs preparing, so this routine is
   * empty.
   */
  void init();

  void shutdown() override;
};
