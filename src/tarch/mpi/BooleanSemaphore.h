// Copyright (C) 2009 Technische Universitaet Muenchen
// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

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
 * Boolean semaphore across MPI ranks
 *
 * Only one thread on one rank gets access to a certain code region. This means
 * that this particular semaphore represents a critical section across both
 * threads and ranks in one go. It is inherently multithreading-safe.
 *
 * ## Implementation
 *
 * The implementation is straightforward:
 *
 * 1. Lock the local region, so no other region on the present rank can request
 *    a semaphore.
 * 2. Send the integer tied to this boolean semaphore to the global master. Wait
 *    for the (positive) return.
 * 3. The master receives the lock request in the BooleanSemaphoreService. It
 *    tries to acquire a lock. Until it is successful, it continues to call
 *    receiveDanglingMessages().
 * 4. Once the master is successful in getting the lock, it send an integer
 *    message back to the sender. The sender now is allowed to proceed.
 * 5. Once successful, the rank releases the lock, which materialises in
 *    another message to the global master. This time, we can send logically
 *    unblocking, i.e. we don't have to wait for an answer.
 *
 * Multiple request handling routines (calls of acquireLock()) can be active at
 * the same time, i.e. reside on the call stack. They will be served one by
 * one, although implicitly in reverse order as we work with the call stack.
 * It is important to poll receiveDanglingMessages() all the time, as we might
 * wait for a lock to be freed, but for the lock to be freed, we have to check
 * unexpected aka dangling messages.
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
        std::vector< std::pair<int, int> >   _pendingLockRequests;

        BooleanSemaphoreService();

        BooleanSemaphoreService( const BooleanSemaphoreService& ) = delete;

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
        virtual void shutdown() override;

        /**
         * Destructor of the service. As the service is a singleton and a service
         * (hence the name), it has to deregister itself. Otherwise, the overall
         * service landscape still might try to call receiveDanglingMessages().
         */
        ~BooleanSemaphoreService();

        /**
         * Receive any lock-related dangling messages
         *
         * This routine polls and if there's a release or request message, it
         * tries to answer it.
         *
         * <h2> Usage within multithreaded environment </h2>
         *
         * We rely on the fact that no multiple threads can access the
         * receiveDangling thing at the same time if you go through the services.
         * The other way round, please do never invoke this routine directly: If 
         * two threads jump into receiveDangingMessages, they might issue the Iprobe
         * one after another and get a "yes" - even though there is only one message
         * in the queue. Consequently, both threads bump into the receive operation.
         * Now, only one message is there and, consequently, the second thread 
         * deadlocks.
         */
        virtual void receiveDanglingMessages() override;

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

    /**
     * Semaphore to serve on the local rank.
     */
    static tarch::multicore::BooleanSemaphore  _localRankSemaphore;


    const int  _semaphoreNumber;

    /**
     * Waits until I can enter the critical section.
     *
     * This operation is protected, as you should not invoke it directly. Use
     * tarch::mpi::Lock to acquire it. With such a lock object, you can be sure
     * that the lock is released (the latest when the object is destroyed. This
     * way, your code is a little bit more deadlock-free. You cannot forget to
     * release an MPI semaphore.
     */
    void enterCriticalSection();

    /**
     * Tells the semaphore that it is about to leave.
     *
     * @see enterCriticalSection()
     */
    void leaveCriticalSection();

    /**
     * You may not copy a semaphore
     */
    BooleanSemaphore( const BooleanSemaphore& ) = delete;

    /**
     * You may not copy a semaphore
     */
    BooleanSemaphore& operator = ( const BooleanSemaphore& ) = delete;

  public:
    /**
     * Create new boolean semaphore spanning all MPI ranks
     *
     * Register a new global semaphore. Please note that global semaphores
     * have to be static, and that each rank has to have the same one and
     * create the MPI semaphores in the same order (SPMD programming paradigm).
     *
     * What happens is that the constructor grabs a unique number for this
     * MPI semaphore, and stores it in _semaphoreNumber. This number will
     * later be used globally to identify this semaphore type.
     */
    BooleanSemaphore();

    ~BooleanSemaphore();
};
