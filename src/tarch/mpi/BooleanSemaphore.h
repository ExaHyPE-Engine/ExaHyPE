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
 * Each boolean semaphore constructs a unique global number which is used to
 * identify the semaphore within the whole system. When we request a lock, we
 * send an integer message with this number to the global master through the
 * semaphore tag. If we we release it, we send the negative number back.
 *
 * As an mpi semaphore includes all ranks and all threads, it is something that
 * is more general than a "normal" tarch::multicore::BooleanSemaphore.
 * Therefore, the mpi semaphore hosts a multicore semaphore. As
 * long as a thread on the local rank has blocked a critical section, we don't
 * even have to go to the global master.
 *
 * However, this is not the only semaphore: On the global master, we need a
 * second semaphore to protect the global semaphore map. This map might be
 * changed even if we are not in a critical section.
 *
 * ## Implementation
 *
 * The implementation is straightforward:
 *
 * 1. Lock the local rank.
 * 2. Send the integer tied to this boolean semaphore to the global master. Wait
 *    for the go ahead message to return.
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
 * the same time, i.e. reside on the global master's call stack. They will be
 * served one by one, although implicitly in reverse order as we work with the
 * call stack. It is important to poll receiveDanglingMessages() all the time,
 * as we might wait for a lock to be freed, but for the lock to be freed, we
 * have to check unexpected aka dangling messages.
 *
 *
 * @author Tobias Weinzierl
 */
class tarch::mpi::BooleanSemaphore {
  public:
    class BooleanSemaphoreService: public tarch::services::Service {
      private:
        static BooleanSemaphoreService  _singleton;

        /**
         * Tag used to exchange locking MPI messages
         */
        int                  _semaphoreTag;

        /**
         * Semaphore for the global master's map of locks
         *
         * On the master, I have to be sure that no two threads do
         * access the map of semaphores concurrently. Therefore, I
         * need a shared memory semaphore.
         */
        tarch::multicore::BooleanSemaphore  _mapAccessSemaphore;
        tarch::multicore::BooleanSemaphore  _reserverationRequestsSemaphore;

        struct SemaphoreMapEntry {
          /**
           * Flag indicating if a semaphore is currently locked
           */
          bool   locked;

          /**
           * Bookkeeping of who has locked a semaphore last. I originally
           * wanted to use this one for some fairness checks, but this all
           * became too complicated and did not pay off. Therefore, this
           * flag is solely there for keeping track of historic data.
           */
          int    rankThatLastLocked;

          /**
           * Create new semaphore entry
           *
           * The default is that a semaphore is not locked. New entries should
           * only be inserted by addMapEntryLazily() which in turn is triggered
           * by tryLockSemaphoreOnGlobalMaster().
           */
          SemaphoreMapEntry();
          std::string toString() const;
        };

        /**
         * Map of semaphores
         *
         * This is the actual map which stores per semaphore number
         * a bool that says whether it is currently taken. So false
         * means noone's requested it, true means someone holds it.
         */
        std::map<int,SemaphoreMapEntry>                  _map;

        /**
         * List of pending lock requests
         *
         * If another rank requests a semaphore, I store this request
         * temporarily here. Each entry is a map from a rank that
         * requests access to the number of the semaphore it is asking
         * for.
         */
        std::vector< std::pair<int, int> >   _pendingLockRequests;

        BooleanSemaphoreService() = default;

        BooleanSemaphoreService( const BooleanSemaphoreService& ) = delete;

        /**
         * Serve pending lock requests
         *
         * If a remote rank does request a lock, I don't reserve it
         * straightway. Instead, I buffer it in _pendingLockRequests.
         * This buffering is realised within receiveDanglingMessages().
         *
         * From time to time, I now have to check whether I can
         * serve the request. I do so in receiveDanglingMessages() whenever
         * there are no more messages to be appended to the list of locks,
         * and I also try to serve requests directly after someone
         * has released a lock, hoping that this is the perfect timing.
         *
         * The routine serves delegates the actual decision whether to lock
         * or not to tryLockSemaphoreOnGlobalMaster(), but it can serve
         * multiple lock requests. This makes sense, as the
         * global master manages all sempahores and not only one, i.e.
         * it might be able to serve multiple requests. Actually, to avoid
         * deadlock scenarios, it is furthermore important that the routine
         * runs through all requests.
         */
        void serveLockRequests();

        /**
         * Try to lock a semaphore
         *
         * This routine is only called on the global master. We try to lock the
         * global semaphore number for forRank. If that works, we make a note
         * for which rank we have locked it. Before we however check if we can
         * serve a request, we invoke addMapEntryLazily(), i.e. we ensure that
         * there is an entry in the database for forRank.
         *
         * @return Lock request has been served successfully
         */
        bool tryLockSemaphoreOnGlobalMaster( int number, int forRank );

        /**
         *
         * ## Global master and deadlocks
         *
         * We always have to call receiveDanglingMessages() quite aggressively.
         * On the one hand, we might try to lock a semaphore which is already
         * locked by another rank. We have to give that other rank the
         * opportunity to free the lock again. If we don't allow this to happen,
         * we will never get the opportunity to lock this semaphore for our own
         * rank, i.e. the global master.
s         *
         * There is another reason for this polling, too: It is
         * important that we don't prematurely lock the semaphore if we are
         * called on the master. We should always try to serve other ranks
         * first to avoid massive MPI rank divergence. Further to that, we take
         * the flag _localRankLockRequestSemaphore into account. If it is set,
         * we know that each rank will request the semaphore at least once.
         * Consequently, we do acquire it if and only if another rank has
         * grabbed it before. Basically, we try to prioritise all other ranks
         * and let locks by rank 0 pass through last.
         * Please consult the class documentation on potential deadlocks.
         */
        void lockSemaphoreOnGlobalMaster( int number, int forRank );

        /**
         * Unlock semaphore on global master
         *
         * Very simple implementation:
         *
         * - Acquire a lock on the global semaphore map
         * - Check that all data is consistent, i.e. someone has actually
         *   requested this lock before
         * - Free the lock, i.e. set the flag to "not locked"
         */
        void unlockSemaphoreOnGlobalMaster( int number, int forRank );

        /**
         * Add map entry
         *
         * Only defined/used on global master. We do not maintain a map of all
         * existing semaphores, but build up a dictionary of these guys at
         * runtime in a lazy fashion.
         *
         * This routine is not thread-safe, i.e. I assume that somebody
         * calling this routine has ensured that no data races occur and
         * notably that noone uses _map meanwhile.
         *
         * @param number Every boolean semaphore has a globally unique positive
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
        ~BooleanSemaphoreService() = default;

        /**
         * Receive any lock-related dangling messages
         *
         * This routine polls and if there's a release or request message, it
         * tries to answer it.
         *
         * ## Usage within multithreaded environment
         *
         * We rely on the fact that no multiple threads can access the
         * receiveDangling thing at the same time if you go through the
         * services. In return, please do never invoke this routine directly:
         * If two threads jump into receiveDangingMessages, they might issue
         * the Iprobe one after another and get a "yes" - even though there is
         * only one message in the queue. Consequently, both threads bump into
         * the receive operation. Now, only one message is there and,
         * consequently, the second thread deadlocks.
         *
         * ## Priorities
         *
         * The message polls for messages. Messages with a positive value mean
         * the system tries to acquire an MPI lock. Messages with a negative
         * value mean that the corresponding ranks wants to release the lock
         * with the corresponding positive value.
         *
         * Release requests are handled immediately by calling releaseLock().
         * Lock requests are not handled direclty. Instead, we enqueue them
         * into _pendingLockRequests. Eventually, I serve them by invoking
         * serveLockRequests(). This is a function I call if and only if no
         * further messages are pending. Therefore, this code fragments
         * prioritises frees over locks.
         */
        virtual void receiveDanglingMessages() override;

        /**
         * Don't use this routine. It returns the global semaphore and is only used
         * on the master/by the core.
         */
        static BooleanSemaphoreService& getInstance();

        /**
         * Acquire the lock
         *
         * If this routine is called on the global master, we redirect the call
         * to lockSemaphoreOnGlobalMaster(). If it is called on another rank, we
         * send a lock message to the global master.
         */
        void acquireLock( int number);

        /**
         * Release a lock
         *
         * This routine delegates the call to unlockSemaphoreOnGlobalMaster() if
         * we run it on the global master. To avoid rank divergence, we immediately
         * serve follow-up lock requests after we've freed the lock.
         *
         * If we are not on the global master, we send an unlock message to this
         * master and return. There will be no confirmation or similar.
         */
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
     * @todo explain why we lock locally first
     */
    tarch::multicore::BooleanSemaphore  _localRankLockRequestSemaphore;

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
