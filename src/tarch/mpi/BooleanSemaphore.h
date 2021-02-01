// Copyright (C) 2009 Technische Universitaet Muenchen
// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#ifndef _TARCH_MPI_BOOLEAN_SEMAPHORE_H_
#define _TARCH_MPI_BOOLEAN_SEMAPHORE_H_


#include "tarch/mpi/mpi.h"
#include "tarch/logging/Log.h"
#include "tarch/services/Service.h"
#include "tarch/multicore/BooleanSemaphore.h"

#include <string>
#include <map>
#include <vector>


namespace tarch {
  namespace mpi {
    class BooleanSemaphore;
    class Lock;
    class Rank;
  }
}


/**
 * Boolean Semaphore
 *
 * @see Constructor for details
 *
 * @author Tobias Weinzierl
 */
class tarch::mpi::BooleanSemaphore {
  public:
    class BooleanSemaphoreService: public tarch::services::Service {
      private:
        static BooleanSemaphoreService  _singleton;

        int                  _semaphoreTag;

        /**
         * On the master, I have to be sure that no two threads do
         * access the map of semaphores concurrently. Therefore, I
         * need a shared memory semaphore.
         */
        tarch::multicore::BooleanSemaphore  _mapAccessSemaphore;

        /**
         * This is the actual map which stores per semaphore number
         * a bool that says whether it is currently taken. So false
         * means noone's requested it, true means someone holds it.
         */
        std::map<int,bool>                  _map;

        /**
         * If another rank requests a semaphore, I store this request
         * temporarily here. Each entry is a map from a rank that
         * requests access to the number of the semaphore it is asking
         * for.
         */
        std::vector< std::pair<int,int> >   _pendingLockRequests;

        BooleanSemaphoreService();

        BooleanSemaphoreService( const BooleanSemaphoreService& copy ) = delete;

        /**
         * If a remote rank does request a lock, I don't reserve it
         * straightaway. Instead, I buffer it in _pendingLockRequests.
         * From time to time, I now have to check whether I can
         * serve the request. I do so in receiveDanglingMessages()
         * and I also try to serve requests directly after someone
         * has released a lock, as this is the perfect timing.
         *
         * The routine serves multiple lock requests. This seems to be
         * strange as it solely locks. But it makes sense, as the
         * global master manages all sempahores and not only one, i.e.
         * it might be able to serve multiple requests. Please note
         * that this operation is invokved by receiveDanglingMessages().
         * It is thus implicitly called iteratively. To avoid deadlock
         * scenarios, it is furthermore important that the routine
         * runs through all requests.
         */
        void serveLockRequests();

        /**
         * @number Every boolean semaphore has a globally unique positive
         *    number. The MPI messages we send around carry this number if we
         *    want to acquire a lock. They carry the negative number if we
         *    want to release a lock. To allow for these sign flips, number
         *    may not equal zero.
         */
        void addMapEntryLazily(int number);
      public:
        void init();
        void shutdown() override;

        /**
         * Destructor of the service. As the service is a singleton and a service
         * (hence the name), it has to deregister itself. Otherwise, the overall
         * service landscape still might try to call receiveDanglingMessages().
         */
        ~BooleanSemaphoreService();

        /**
         * We rely on the fact that no multiple threads can access the
         * receiveDangling thing at the same time.
         */
        void receiveDanglingMessages() override;

        /**
         * Don't use this routine. It returns the global semaphore and is only used
         * on the master/by the core.
         */
        static BooleanSemaphoreService& getInstance();

        void acquireLock( int number );
        void releaseLock( int number );

        int getNumberOfLockedSemaphores();

        std::string toString() const;
    };
  private:
    friend class Rank;
    friend class tarch::mpi::Lock;

    static tarch::logging::Log  _log;

    static int                  _semaphoreCounter;

    const int  _semaphoreNumber;

    /**
     * Waits until I can enter the critical section.
     */
    void enterCriticalSection();

    /**
     * Tells the semaphore that it is about to leave.
     */
    void leaveCriticalSection();

    /**
     * You may not copy a semaphore
     */
    BooleanSemaphore( const BooleanSemaphore& semaphore ) = delete;

    /**
     * You may not copy a semaphore
     */
    BooleanSemaphore& operator=( const BooleanSemaphore& semaphore ) = delete;

  public:
    /**
     * Register a new global semaphore. Please note that global semaphores
     * have to be static!
     *
     * @identifier Unique identifier. I usually use the fully qualified classname.
     *   The string however is only used for bookkeeping reasons. There's no
     *   semantics behind the string (it even doesn't have to be unique though I
     *   do not encourage passing non-meaningful arguments).
     */
    BooleanSemaphore( const std::string& identifier );

    ~BooleanSemaphore();
};


#endif
