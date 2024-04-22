// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "RefinementControl.h"
#include "peano4/grid/GridControlEvent.h"
#include "tarch/multicore/RecursiveSemaphore.h"
#include "tarch/services/Service.h"

namespace exahype2 {
  class RefinementControl;
  class RefinementControlService;
} // namespace exahype2

/**
 *
 * ## MPI
 *
 * After each mesh traversal, we expect the code to invoke finishStep().
 * Here, we exchange all local events with the other ranks. This is an
 * important step for two reasons:
 *
 * - MPI ranks might fork trees which end up on other ranks. This other
 *   rank has to know if there had been refinement events.
 * - If domain boundaries between ranks are ragged, we might end up with
 *   many tiny refinement events which cannot be merged into one large
 *   one. By exchanging these tiny guys, we can merge the events on each
 *   and every rank and come up with one large region which has to be
 *   refined.
 *
 * While merging for refinement controls is only an optimisation, merging
 * coarsening commands is mandatory. Only refined regions completely
 * contained within one huge coarsening instruction are actually coarsened.
 *
 *
 * ## Lifecycle
 *
 * Whenever a code inserts a new event, it has to specify the lifetime.
 * For Runge-Kutta 4, for example, you might want to pass in a four as
 * one step might trigger a refinement, but you want to realise the
 * refinement at the end of the time step, i.e. four steps later.
 *
 * Events received via MPI have no lifecycle. We therefore assign it the
 * maximum lifetime plus an offset.
 *
 *
 */
class exahype2::RefinementControlService: public tarch::services::Service {
public:
  static RefinementControlService& getInstance();

  virtual ~RefinementControlService();

  /**
   * Receive refinement control messages as shared by other ranks
   *
   * Whenever we receive such messages, we know that these stem from partner
   * services. Therefore, we know that incoming messages stem from committed
   * messages that are active right now. Therefore, we add them to our
   * committed set. We also add them to the new set, as they might have
   * dropped in slightly too late, i.e. we might already have handed out our
   * events to "our" trees, i.e. those hosted on this rank. As we also add
   * the messages to the new events, they will be delivered in a subsequent
   * sweep in this case.
   *
   * ## Realisation
   *
   * - We first do a general probe on MPI to see if there are any messages.
   *   If there are no messages at all, it makes no sense to continue.
   * - If the guard check says "yes, there is a message", we next have to
   *   block the threads. From hereon, we work in a strict serial sense.
   * - Now we check again (!) if there are messages. This is important as
   *   receiveDanglingMessages() might be called by multiple threads at the
   *   same time. So we might have entered the routine and see a message.
   *   Now we want to receive it, but some other thread might have checked
   *   as well and might have grabbed "our" message.
   * - If the check is, once again, successful, we know the rank from which
   *   the message comes from. So far, all checks use MPI_ANY_RANK, but the
   *   returned status object now is befilled with the correct rank info.
   *   We use this rank from hereon, as we will receive multiple messages
   *   in a row from the rank.
   * - Next, we work out how many messages we have been sent.
   * - Finally, we receive the messages and add them to _committedEvents
   *   and _remoteNewEvents.
   *
   * We note that most of these complex multithreading operations are rarely
   * necessary, as receiveDanglingMessages() within the tarch::services::ServiceRepository::receiveDanglingMessages()
   * is a prior thread-safe. Nevertheless, I decided to try to be on the safe
   * side in case someone wants to call this receiveDanglingMessages()
   * version explicitly.
   */
  virtual void receiveDanglingMessages() override;

  /**
   * @see RefinementControl::clear()
   */
  std::vector<peano4::grid::GridControlEvent> getGridControlEvents() const;

  virtual void shutdown() override;

  /**
   * Should be called after each traversal per rank.
   *
   * This routine rolls over _newEvents into the _committedEvents if they are still
   * alive. That is, _newEvents holds all the events including an "expiry date", while
   * _committedEvents holds only those which are committed for this mesh traversal.
   *
   * We clear the committed events and then copy the new events over one by one unless
   * they have expired. If they have expired, we delete them within _newEvents. In
   * this context, we also reduce the maxLifetime, but this one is solely used for
   * reporting purposes. It is not used. The global maximum lifetime is stored within
   * _maxLifetime. This one is monotonously growing, while the locally reduced value
   * here might decrease over time if no new events are added.
   *
   * Once we have committed all non-expiring events, we exchange them via MPI.
   * For this, we first wait for all previous non-blocking communication to
   * terminate (freeAllPendingSendRequests()), I create a copy of the
   * committed events and then delegate the data exchange to
   * triggerSendOfCopyOfCommittedEvents().
   *
   * Finally, we append the remote events to our set of committed events.
   * This has to happen after the actual MPI exchange, so we can be sure
   * that remote events are not immediately sent back again.
   *
   * I found it to be very important to merge the committed events immediately:
   * Peano's spacetrees will merge, too, but if we work with 100 trees per node,
   * each and every one will merge locally. This is time we better spend on
   * something else.
   */
  void finishStep();

  std::string toString() const;

  void merge(const RefinementControl& control);

private:
  static tarch::logging::Log _log;

  static tarch::multicore::RecursiveSemaphore _semaphore;

  RefinementControlService();

  /**
   * I need a tag of my own to exchange control info after each step.
   */
  static int _reductionTag;

  /**
   * Container to accumulate new events. This is a list as we may assume
   * that a lot of inserts are done per iteration.
   */
  RefinementControl::NewEvents _localNewEvents;

  std::list<peano4::grid::GridControlEvent> _remoteNewEvents;

  /**
   * Container with all the valid events. Is an extract from _newEvents
   * which is built up in finishStep() and then handed out to Peano once
   * it asks.
   *
   * ## Event lifetime
   *
   * The individual events are not only copied over. Each event is annotated
   * with its lifetime. That is, events might remain active over multiple
   * iterations. This operation decrements the lifespan and, after that,
   * copies those events that are still alive over into the result.
   *
   * ## Optimisation
   *
   * The raw events might become really large over time. However, I decided to
   * keep this routine clear of any optimisation. It is the grid which has
   * to clean up the refinement events appropriately.
   *
   * ## MPI data excange
   *
   * I originally wanted to use an MPI reduction to have a consistent view of
   * all refinement controls. However, this seems not to work, as some ranks
   * can be out of sync. So what I do instead now is a delayed broadcast: Every
   * rank sends out its committed events to all others and appends all incoming
   * ones to its own set.
   *
   * This simple implementation also works for our dynamic event sets, where we
   * do not know a priori how many events are triggered by a rank. Still, it can
   * happen that event views are out-of-sync. This is not a problem here:
   *
   * The actual grid changes are all triggered via vertices, so we never obtain
   * an invalid grid. The individual grid events have a lifetime and thus are
   * active over multiple iterations. Hence, they will be merge at one point.
   *
   * We have to split up the send and receive loop such that we first send out
   * all stuff and then receive.
   *
   */
  std::vector<peano4::grid::GridControlEvent> _committedEvents;

#ifdef Parallel
  std::vector<MPI_Request*>                   _sendRequests;
  std::vector<peano4::grid::GridControlEvent> _copyOfCommittedEvents;

  /**
   * Complete pending sends from previous mesh traversal
   *
   * @see triggerSendOfCopyOfCommittedEvents()
   */
  void freeAllPendingSendRequests();

  /**
   *
   * ## MPI handing
   *
   * The MPI data exchange here is non-trivial, as we do not know which rank has how
   * many messages. So we run in three steps: First, we send the committed events out to
   * all partners. These sends are done non-blocking. They logically are a broadcast.
   * Second, we loop over all the arising MPI_Requests and poll until they are finished.
   * So we know all data has gone out. This is a while loop with the predicate
   *
   *       not sendRequests.empty()
   *
   * Third, we hook into this while loop and probe for all the other ranks if they
   * have sent us anything. So while we wait for our sends to go out, we poll the other
   * ranks for their data, integrate this information into our data structures and
   * therefore also free any MPI queues.
   *
   * As we build up the incoming data while we wait for our sends to go out, we have to
   * send from a copy of the actual data (copyOfCommittedEvents): We cannot alter the
   * outgoing data due to polls for incoming messages while we have still pending sends.
   *
   * It is important to work with non-blocking calls here, as the code otherwise tends
   * to deadlock if we have a lot of events. This is due to MPI falling back to
   * rendezvous message passing if the data that is on-the-fly becomes too large.
   */
  void triggerSendOfCopyOfCommittedEvents();
#endif
};
