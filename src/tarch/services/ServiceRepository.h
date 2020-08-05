// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#ifndef _TARCH_SERVICES_SERVICE_REPOSITORY_H_
#define _TARCH_SERVICES_SERVICE_REPOSITORY_H_


#include <vector>
#include <string>

#include "tarch/services/Service.h"


namespace tarch {
  namespace services {
    class ServiceRepository;
  }
}


/**
 * Service Repository
 *
 * See the interface Service for a detailed description. The registry is a
 * service itself, but it does not register as a service.
 *
 * @author Tobias Weinzierl
 */
class tarch::services::ServiceRepository: public tarch::services::Service {
  private:
    struct ServiceEntry {
      std::string  _name;
      Service*     _service;
    };

    typedef  std::vector<ServiceEntry>      ServiceContainer;

    ServiceContainer                        _services;

    tarch::multicore::RecursiveSemaphore    _receiveDanglingMessagesSemaphore;

    /**
     * @see tarch::services::Serivce for a description how to realise singletons.
     */
    static tarch::services::ServiceRepository _singleton;

    ServiceRepository();
  public:
    virtual ~ServiceRepository();

    static ServiceRepository& getInstance();

    /**
     * @param service Pointer to service
     * @param name    Name of service (mandatory, not empty)
     */
    void addService( Service* const service, const std::string& name );
    bool hasService( Service* service ) const;
    void removeService( Service* const service );

    /**
     * Answer to MPI Messages
     *
     * Tell all registered services to answer to MPI messages that are still
     * pending in the MPI queues.
     *
     * <h2> MPI tuning </h2>
     *
     * I once had an implementation that checks via Iprobe whether to call the
     * receiveDanglingMessages() of all registered services. Without that probe,
     * my ExaHyPE 2 code did crash. Today, I think this has been a bug in the
     * way how parts of the code handled the requests, so I could remove the
     * snippet again. Nevertheless, it makes sense to allow codes to do something
     * else while receiveDanglingMessages() is called. If this routine is called,
     * the system basically flags that there's a lack of MPI progression or poor
     * load balancing, so the routine indeed should give a user code to do
     * something meaningful meanwhile. This can be to check for further messages
     * (unexpected message arrival). But it can also be something else such as
     * the handling of left-ofter tasks.
     */
    void receiveDanglingMessages() override;

    /**
     * @return List of registered services separated by whitespaces
     */
    std::string getListOfRegisteredServices() const;

    /**
     * Maybe the only service that you don't have to init and shutdown.
     */
    void init();
    void shutdown() override;
};


#endif
