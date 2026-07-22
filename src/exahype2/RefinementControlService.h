// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "exahype2/RefinementControl.h"
#include "peano4/grid/GridControlEvent.h"
#include "tarch/services/Service.h"

namespace exahype2 {
  class RefinementControl;
  class RefinementControlService;
} // namespace exahype2

/**
 * Rank-wide assembly of all refine and erase requests
 *
 * ## MPI
 *
 * After each mesh traversal, we expect the code to invoke finishStep().
 * Here, we exchange all local events with the other ranks. This exchange
 * serves two purposes:
 *
 * - MPI ranks might fork trees which end up on other ranks. This other
 *   rank has to know if there had been refinement events.
 * - If domain boundaries between ranks are ragged, we might end up with
 *   many small refinement events which cannot be merged into one large
 *   one. By exchanging them, we can merge the events on each and every
 *   rank and come up with one large region which has to be refined.
 *
 * Coarsening commands are finalised before they are handed to Peano. ExaHyPE
 * users issue one erase request per fine patch; the service promotes these
 * requests to parent-sized Peano events only if the complete 3^d sibling
 * group has requested coarsening. Generic GridControlEvent merging is then
 * only a reduction of the number of events to test during the traversal.
 *
 * ## AMR realisation
 *
 * Ordinary ExaHyPE AMR events are independent of the selected solver and its
 * time-stepping scheme. A command is re-delivered once to trigger the topology
 * transition and once more to realise the created and destroyed cells and
 * faces. This is Peano-internal grid-control bookkeeping and is not exposed as
 * a solver option.
 *
 * A commit that exchanges its raw requests collectively already holds every rank's
 * requests, so it discards the point-to-point ones it happens to hold. Only a commit
 * without that exchange folds them in, and a rank that hears about a remote request too
 * late for the running traversal then acts on it one sweep later.
 */
class exahype2::RefinementControlService final : public tarch::services::Service {
public:
  static RefinementControlService& getInstance();

  ~RefinementControlService() override;

  RefinementControlService(const RefinementControlService&)            = delete;
  RefinementControlService(RefinementControlService&&)                 = delete;
  RefinementControlService& operator=(const RefinementControlService&) = delete;
  RefinementControlService& operator=(RefinementControlService&&)      = delete;

  /**
   * Receive refinement control messages from other ranks
   */
  void receiveDanglingMessages() override;

  /**
   * The events active for the next mesh traversal.
   */
  std::vector<peano4::grid::GridControlEvent> getGridControlEvents() const;

  /**
   * Is there at least one committed grid-control event for the next mesh traversal?
   */
  bool hasGridControlEvents() const;

  /**
   * Has a topology change been realised since the last time step?
   */
  bool hasRealisedTopologyChangesSinceLastTimeStep() const;

  /**
   * Anchor used to identify Peano sibling groups when patch-local erase
   * requests are promoted to parent erase events.
   */
  void setDomainOffset(const tarch::la::Vector<DIMENSIONS, double>& domainOffset);

  /**
   * Set the number of solvers whose consent a coarsening requires.
   */
  void setNumberOfSolvers(int numberOfSolvers);

  void shutdown() override;

  /**
   * Reduce the requests of one completed traversal into the committed event set.
   */
  void finishStep(bool useCollectiveExchange = false, int numberOfDeferredTopologyChanges = 0);

  /**
   * Commit the events collected by a refinement criterion traversal.
   */
  void finishRefinementCriterionStep(bool useCollectiveExchange = false);

  /**
   * Finish a mesh refinement realisation traversal.
   */
  void finishDynamicMeshRefinementStep(bool useCollectiveExchange = false, int numberOfDeferredTopologyChanges = 0);

  std::string toString() const;

  void merge(const RefinementControl& control);

private:
  RefinementControlService();

  /**
   * A raw request as it travels between ranks: the geometric event, the number of the
   * solver that issued it (so the per-solver coarsening consensus also works for sibling
   * groups that are split across ranks), and whether this is the request's first
   * delivery. The flag travels with the request so every rank derives the same
   * suppression and redelivery decisions. Trivially copyable, exchanged as bytes.
   */
  struct RawRequest {
    peano4::grid::GridControlEvent event;
    std::int32_t                   solverNumber    = 0;
    std::int32_t                   isFirstDelivery = 1;
  };
  static_assert(std::is_trivially_copyable_v<RawRequest>, "Raw requests travel between ranks as bytes");

  /**
   * The service exchanges its control info after each step on a tag of its own,
   * so the traffic cannot be confused with any other reduction.
   */
  static int _reductionTag;

  /**
   * Per-thread accumulation buffers for new events.
   *
   * merge() is called once per tree at end-of-traversal, and trees traverse as
   * concurrent multicore tasks. The inserts need no semaphore, because each
   * thread appends into its own buffer, so no two threads touch the same
   * slot. finishStep() runs after the traversal has joined
   * (single-threaded) and reduces all per-thread buffers into the committed set.
   * The committed set is order-independent (GridControlEvent total order plus
   * canonical merge), so the per-thread reduction order does not matter.
   */
  std::vector<RefinementControl::NewEvents> _localNewEventsPerThread;

  /**
   * Raw requests other ranks sent us. The next finishStep() folds them into the committed
   * set, so they pass through the sibling-group promotion just like the local ones.
   */
  std::list<RawRequest> _remoteNewEvents;

  /**
   * The events active for the next mesh traversal, in canonical form: coarsening promoted to
   * parent-sized events, then merged. finishStep() is the only writer, and it runs between
   * traversals, so the trees can read this set without a lock.
   */
  std::vector<peano4::grid::GridControlEvent> _committedEvents;

  /**
   * The event set the last completed mesh refinement cycle realised. A criterion commit
   * that reproduces it exactly cannot change the mesh again and is dropped.
   */
  std::vector<peano4::grid::GridControlEvent> _lastRealisedEvents;

  /**
   * True from the first realise traversal that consumed events until the end of the next
   * time step. Mesh-update sweeps clear the per-face Updated entries, which cancels the
   * pending halo roll-over of every face their projection does not reach; the first time
   * step after a topology change therefore rolls all face halves that no change
   * invalidated. Every rank derives the flag from the collectively exchanged set, so the
   * decision is rank-uniform.
   */
  bool _topologyChangedSinceLastTimeStep;

  tarch::la::Vector<DIMENSIONS, double> _domainOffset;

  int _numberOfSolvers;

  void finishStep(
    bool useCollectiveExchange,
    bool suppressEventsIfUnchanged,
    bool recordRealisedEvents,
    int  numberOfDeferredTopologyChanges = 0
  );

  /**
   * Promote patch-local erase requests to parent-sized erase events.
   */
  std::vector<peano4::grid::GridControlEvent> finaliseCoarseningEvents(
    const std::vector<RawRequest>&               requests,
    std::vector<peano4::grid::GridControlEvent>& parentEventsRequiringRealisationDelivery
  ) const;

#ifdef PEANO_WITH_MPI
  std::vector<MPI_Request*> _sendRequests;
  std::vector<RawRequest>   _copyOfRawRequests;

  /**
   * One in-flight body receive per source. Bodies are received concurrently and source-specifically.
   */
  struct IncomingBody {
    MPI_Request             request;
    std::vector<RawRequest> buffer;
  };

  MPI_Request             _recvSizeRequest;
  int                     _recvSizeBuffer;
  std::list<IncomingBody> _incomingBodies;
  bool                    _isShuttingDown;

  // Buffer to keep send sizes valid during non-blocking sends
  std::vector<int> _sendSizeBuffers;

  void postReceiveSize();
  void tryToReceiveDanglingMessage();
  void drainDanglingMessages();
  void cancelReceiveSizeRequest();

  /**
   * Synchronise the raw requests across all ranks in one blocking exchange.
   */
  void exchangeRawRequestsCollectively(std::vector<RawRequest>& requests);

  /**
   * Complete the pending sends of the preceding mesh traversal
   *
   * @see triggerSendOfCopyOfRawRequests()
   */
  void freeAllPendingSendRequests();

  /**
   * Send a copy of the raw requests to every other rank.
   */
  void triggerSendOfCopyOfRawRequests();
#endif
};
