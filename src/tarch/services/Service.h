// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#ifndef _TARCH_SERVICES_SERVICE_H_
#define _TARCH_SERVICES_SERVICE_H_


#include "tarch/multicore/RecursiveSemaphore.h"


namespace tarch {
  namespace services {
    class Service;
    class ServiceFactory;
    class ServiceRepository;
  }
}


/**
 * Service Interface
 *
 * A service is a singleton running the background which is available always.
 * If it is shut down, all calls to the service becomes nop, i.e. it is a
 * robust thing. On a parallel machine, services are also do exist only once,
 * i.e. they are global singletons. Therefore, you shall never access a service
 * directly, but always via a proxy/accessor. This proxy either invokes the
 * service directly (if the services are on the local node) or it creates an
 * MPI call to the real service. Consequently, services have to poll the MPI
 * queue regularly. To enable them to do so, all services register at the
 * service repository, and this service repository has a poll operation.
 * Services also have to be protected by semaphores in a multicore environment.
 *
 * If you wanna write a service yourself, you have to do two things:
 * - Implement this interface.
 * - Register your object through tarch::services::ServiceRepository::getInstance().addService().
 *
 * I usually make services singletons. I can then do the registration in the
 * constructor.
 *
 * <h2> How to implement a singleton </h2>
 *
 * Historically, I preferred the make singletons classes with private
 * constructor that offer a getInstance() routine. The latter is implemented as
 * follows:
 *
 * <pre>
MyClass MyClass::getInstance() {
  static MyClass singleton;
  return singleton;
}
   </pre>
 *
 * But this pattern breaks down with Peano's static libraries. At least one
 * some systems, I've observed that singletons still are created multiple
 * times, i.e. not only when we first call getInstance(). The canonical
 * solution to this dilemma is to make the singleton a static (class)
 * attribute which is private and to make getInstance() hand out this static
 * attribute.
 *
 * While this solves the problem of multiple instantiation, it does not solve
 * another problem: If the constructor of a singleton registers the singleton,
 * it could happen that the linker creates the singletons in the wrong order.
 * There's no way to ensure that the tarch's service is created first. So what
 * can happen is that the memory for the ServiceRepository (where all the
 * services register) is allocated. Then, an arbitrary service is created and
 * registers, i.e. it adds is pointer to the repository. Later, the
 * repository's constructor is invoked and effectively overwrites the registered
 * service. It is hence important that we create the repository first. The only
 * way to ensure this, is to init all services manually.
 *
 * So here's the takeaway:
 *
 * (1) Singletons are always realised via a private constructor.
 * (2) Singletons are always realised via a class attribute.
 * (3) Singletons don't do anything in their constructor. They are not allowed
 *     to.
 * (4) Any singleton has an init() operation, as well as a shutdown
 *     (if required).
 *
 * To enforce this behaviour with init() and shutdown(), I make shutdown an
 * abstract functions. You therefore have to implement it. I refrained from
 * doing the same with init(), as services might want init() to have
 * service-specific arguments.
 *
 *
 * @author Tobias Weinzierl
 */
class tarch::services::Service {
  public:
    friend class ServiceRepository;

    virtual ~Service() {};

    virtual void receiveDanglingMessages() = 0;

  protected:
    /**
     * Recursive semaphores
     *
     * We have ensure that receiveDanglingMessages is not ran in parallel with
     * other operations on one rank. This holds for example for MPI updates or
     * updates of lists of MPIRequests. Yet, there can be cases where
     * receiveDanglingMessages() in turn invokes receiveDanglingMessages()
     * recursively. With a standard semaphore, we would now run into a deadlock
     * as default semaphores are not recursion-safe. Therefore, I provide this
     * recursive semaphore. The ServiceRepository does already lock
     * it, so receiveDanglingMessages() invoked through the factory should be
     * fine. You might however want to lock it yourself, too.
     *
     * I had severe issues when I outsourced the tarch into a library. The
     * static initialisation of the recursive semaphores then led to seg faults
     * from time to time. Therefore, I decided to make this semaphore a pointer.
     * It is initialised lazily tarch::services::ServiceRepository::receiveDanglingMessages().
     */
	  tarch::multicore::RecursiveSemaphore  _receiveDanglingMessagesSemaphore;

	  virtual void shutdown() = 0;
};

#endif
