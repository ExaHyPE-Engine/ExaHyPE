// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/multicore/RecursiveSemaphore.h"

namespace tarch {
  namespace services {
    class Service;
    class ServiceFactory;
    class ServiceRepository;
  } // namespace services
} // namespace tarch

/**
 * Service interface
 *
 * A service is a singleton which runs in the background and is available at
 * all times. Once it is shut down, every call to it becomes a no-op, so a
 * service is robust against a premature shutdown. On a parallel machine, a
 * service exists exactly once, i.e. it is a global singleton. Never access a
 * service directly therefore, but always through a proxy. The proxy either
 * invokes the service locally or issues an MPI call to the rank which hosts
 * it. A service consequently has to poll the MPI queue regularly: all
 * services register at the service repository, and the repository offers a
 * poll operation. In a multicore environment, a service also has to protect
 * its state with semaphores.
 *
 * To write a service, do two things:
 * - Implement this interface.
 * - Register the object through tarch::services::ServiceRepository::getInstance().addService().
 *
 * ## How to implement a singleton
 *
 * A singleton which lives in one of Peano's static libraries can end up
 * being constructed more than once if getInstance() creates it as a
 * function-local static. A singleton is hence a private static class
 * attribute, and getInstance() hands out this attribute.
 *
 * The order in which the linker creates these static attributes is not
 * defined. If a service registered itself within its constructor, it could
 * add its pointer to the repository before the repository's own constructor
 * runs, and that constructor would then wipe the entry. Services are
 * therefore initialised explicitly, once the repository exists:
 *
 * (1) A singleton has a private constructor.
 * (2) A singleton is a class attribute.
 * (3) A singleton's constructor does nothing.
 * (4) A singleton offers an init() operation, plus a shutdown() if required.
 *
 * shutdown() is abstract to enforce that every service provides it. init() is
 * not, as a service may want service-specific arguments there.
 *
 * @author Tobias Weinzierl
 */
class tarch::services::Service {
public:
  friend class ServiceRepository;

  virtual ~Service() {};

  /**
   * Receive the dangling messages from the MPI subsystem
   *
   * Draining the MPI queues regularly keeps MPI progressing: once the buffers
   * are full of unexpected messages, no eager or unexpected receive succeeds
   * anymore. Always trigger the polling through the repository, never on a
   * service directly.
   *
   * @see ServiceRepository::receiveDanglingMessages()
   */
  virtual void receiveDanglingMessages() = 0;

protected:
  /**
   * Recursive semaphore
   *
   * receiveDanglingMessages() must not run in parallel with other operations
   * on a rank, such as MPI updates or updates of lists of MPI requests. As
   * receiveDanglingMessages() may in turn invoke receiveDanglingMessages()
   * recursively, a plain semaphore would deadlock, so the lock is recursive.
   * The ServiceRepository already holds this lock while it polls, but a
   * service may lock it in addition.
   */
  tarch::multicore::RecursiveSemaphore _receiveDanglingMessagesSemaphore;

  virtual void shutdown() = 0;
};
