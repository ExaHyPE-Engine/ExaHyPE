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
 * Therefore, the mpi semaphore hosts the multicore semaphore in return. As
 * long as a thread on the local rank has blocked a critical section, we don't
 * even have to go to the global master.
 *
 * ## Implementation
 *
 * The implementation is straightforward:
 *
 * 1. Lock the local region, so no other region on the present rank can request
 *    a semaphore.
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
 * The need to combine the MPI semaphore with a normal multicore semaphore
 * arises from the fact that we use SPMD and therefore run this code on the
 * global master, too. On any rank, we send out a request and wait for an
 * answer. Another thread on this rank could, in theory, submit a lock request
 * as well, but both will wait until an answer arrives. The only thing that
 * could happen now is that the second request snatches the lock away from the
 * first one, i.e. we become unfair. But no real problems should arise.
 *
 * Things are different on the global master, where we don't wait for an
 * incoming answer message (we simply grab the lock and return) and therefore
 * might run into a race if two threads on the global master concurrently
 * acquire a global MPI lock. Here, we have to use the multicore lock, too.
 *
 *
 * ## Deadlock due to rank divergence
 *
 * If you have a lot of point to point messages, the MPI subsystem might
 * suddenly switch to rendezvous. This is problematic whenever rank 0 has
 * already passed the semaphore, while others have not even hit it. What
 * happens now is that rank 0 does something completely different while
 * potentially all other ranks wait for their boolean semaphore to be
 * served. Two things now can happen which are really annoying:
 *
 * 1. Rank 0 runs into a global communication or non-blocking send which
 *    is not served, as the other ranks are not there yet. Unless the user
 *    codes poll the services (which they should), the code now will
 *    deadlock, as rank 0 waits for some input from the other ranks, while
 *    all other ranks wait for rank 0 to serve their boolean semaphore
 *    requests.
 * 2. Rank n waits for 0 to serve its request, but rank 0 is so busy doing
 *    other things that rank n is blocked for quite a while. Only eventually,
 *    we serve rank n. In this case, we introduce almost a serialisation of the
 *    ranks.
 *
 * A straightforward mitigation of this problem is the attempt to try to avoid
 * rank divergence. This term here is used in line with thread divergence in
 * SIMT: All ranks should more or less do the same thing and hence hit the
 * semaphore around the same time.
 *
 *
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
         * Semaphore to serve on the local rank.
         *
         * It is important to tie the semaphore to the service and not
         * to the actual semaphore: Imaging there are two MPI semaphores
         * in your system. If they had multithreading semaphores tied to
         * them, the ran would never issue two request from the same rank at
         * the same time. However, it could happen that two threads on the rank
         * fire two different MPI lock requests.
         *
         * In this case, the global master receives a lock request A from rank
         * x and then immediately a lock request B from rank x. It decides to
         * serve A and sends back a "go ahead". Unfortunately, the thread that
         * had issues request B catches this message, and now goes ahead. The
         * receivers do not check if the "go ahead" is meant to be for them, as
         * they could not re-inject the message into MPI anyway.
         */
// @todo Deprecated and to be removed as soon as we have confirmation from
//        large-scale runs that this has been an issue. In this case, the
//        docu has to be amended
//        tarch::multicore::BooleanSemaphore  _localRankLockRequestSemaphore;

        struct SemaphoreMapEntry {
          bool   locked;
          int    rankThatLastLocked;

          SemaphoreMapEntry();
          std::string toString() const;
        };

        /**
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
         * Add map entry
         *
         * Only defined/used on global master. We do not maintain a map of all
         * existing semaphores, but build up a dictionary of these guys at
         * runtime in a lazy fashion.
         *
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
         * If this routine is called on the global master, we try to acquire a
         * lock on the master directly. If it is called on another rank, we
         * send a lock message to the global master.
         *
         *
         * ## Global master and deadlocks
         *
         * We always have to call receiveDanglingMessages() quite aggressively.
         * On the one hand, we might try to lock a semaphore which is already
         * locked by another rank. We have to give that other rank the
         * opportunity to free the lock again. If we don't allow this to happen,
         * we will never get the opportunity to lock this semaphore for our own
         * rank, i.e. the global master.
         *
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
        void acquireLock( int number, bool allRanksLockThisSemaphoreAtLeastOnce );

        /**
         * To avoid rank divergence, we immediately serve follow-up lock
         * requests after we've free the lock on the global master.
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

    const bool _allRanksLockThisSemaphoreAtLeastOnce;

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
     *
     * @param allRanksLockThisSemaphoreAtLeastOnce This boolean parameter indicates that,
     *   whenever this semaphore is locked, all ranks will at least lock this
     *   one once.
     */
    BooleanSemaphore(bool allRanksLockThisSemaphoreAtLeastOnce);

    ~BooleanSemaphore();
};
