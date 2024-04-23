// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once


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
     * Add a new service
     *
     * This routine is thread-safe, i.e. it blocks other addService() calls as
     * well as receiveDanglingMessages().
     *
     * ## Implementation
     *
     * I add a recursive semaphore to the addService() routine, as I think this
     * is correct. However, this routine is called very early in the execution.
     * I get seg faults with TBB in this case, and guess this is due to some
     * memory lacking a proper initialisation. So I omit the recursive
     * semaphore whenever I use TBB.
     *
     *
     * @param service Pointer to service
     * @param name    Name of service (mandatory, not empty)
     */
    void addService( Service* const service, const std::string& name );

    /**
     * This routine is thread-safe.
     */
    bool hasService( Service* service ) const;

    /**
     * This routine is thread-safe, i.e. it blocks out
     * receiveDanglingMessages() and all other routine which register or
     * de-register services.
     */
    void removeService( Service* const service );

    /**
     * Answer to MPI Messages
     *
     * Tell all registered services to answer to MPI messages that are still
     * pending in the MPI queues.
     *
     *
     * ## MPI tuning
     *
     * I once had an implementation that checks via Iprobe whether to call the
     * receiveDanglingMessages() of all registered services. Without that probe,
     * my ExaHyPE 2 code did crash. Today, I think this has been a bug in the
     * way how parts of the code handled the requests, so I could remove the
     * snippet again.
     *
     *
     * ## Starvation
     *
     * From a theory point of view, it makes sense nevertheless to allow codes
     * to do something else while receiveDanglingMessages() is called. If this
     * routine is called, the system basically flags that there's a lack of MPI
     * progression or poor load balancing, so the routine indeed should give a
     * user code to do something meaningful meanwhile.
     *
     * I thought it would be an excellent idea to process some tasks therefore.
     * With a lot of tasks, this is a poor idea. Potentially p-1 threads hit
     * the task queue (if one thread is doing I/O for example) and then they
     * all block each other and no MPI progress is made, as very few tasks call
     * the actual MPI routines. So I removed that line to process a task, and
     * this has made the system much more stable.
     *
     *
     * ## Receive dangling message in multithreaded environment
     *
     * It is important that you never ever call receiveDanglingMessages() of a
     * service directly. Instead, you always should go to the repository. The
     * reasons reads as follows: Most services do not protect their receiveDanglingMessages()
     * with an additional semaphore. So two threads might hit it and both thus
     * issue an Iprobe. Let there be one MPI message in the queue. Both probes
     * return a true and, hence, both threads issue a receive. Unfortunately,
     * only one of them succeeds. We could have solved that with new MPI features,
     * but we also solve it in the old-fashioned way if you only issue receives
     * through the repository, as the repository invokes the services, but it
     * first blocks all other threads through a semaphore.
     *
     * This routine is thread-safe compared to addService() and
     * removeService().
     */
    virtual void receiveDanglingMessages() override;

    /**
     * @return List of registered services separated by whitespaces
     */
    std::string getListOfRegisteredServices() const;

    /**
     * Maybe the only service that you don't have to init and shutdown.
     */
    void init();

    virtual void shutdown() override;
};


