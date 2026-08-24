// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "exahype2/RefinementControl.h"
#include "peano4/grid/GridControlEvent.h"
#include "peano4/parallel/SpacetreeSet.h"

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
 * The exchange is one collective gather, so every rank leaves it holding the same
 * request set and derives the same committed events from it. A sibling group whose
 * children are spread over several ranks is therefore assembled on all of them, and
 * the suppression and redelivery decisions are rank-uniform.
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
 */
class exahype2::RefinementControlService final {
public:
  static RefinementControlService& getInstance();

  ~RefinementControlService() = default;

  RefinementControlService(const RefinementControlService&)            = delete;
  RefinementControlService(RefinementControlService&&)                 = delete;
  RefinementControlService& operator=(const RefinementControlService&) = delete;
  RefinementControlService& operator=(RefinementControlService&&)      = delete;

  /**
   * The events active for the next mesh traversal, delivered against the trees
   * this rank hosts.
   */
  std::vector<peano4::grid::GridControlEvent> getGridControlEvents() const;

  /**
   * The events active for the next mesh traversal within the given tree extents.
   */
  std::vector<peano4::grid::GridControlEvent>
  getGridControlEvents(const std::vector<peano4::parallel::SpacetreeSet::TreeExtent>& localTreeExtents) const;

  /**
   * Is there at least one committed grid-control event for the next mesh traversal?
   */
  bool hasGridControlEvents() const;

  /**
   * Has a topology change been realised since the last time step?
   */
  bool hasRealisedTopologyChangesSinceLastTimeStep() const;

  /**
   * Did the criterion traversal that just finished collect nothing because the mesh had changed?
   */
  bool lastCriterionStepWasSuppressed() const;

  /**
   * Anchor used to identify Peano sibling groups when patch-local erase
   * requests are promoted to parent erase events.
   */
  void setDomainOffset(const tarch::la::Vector<DIMENSIONS, double>& domainOffset);

  /**
   * Set the number of solvers whose consent a coarsening requires.
   */
  void setNumberOfSolvers(int numberOfSolvers);

  /**
   * Reduce the requests of one completed traversal into the committed event set.
   */
  void finishStep(int numberOfDeferredTopologyChanges = 0);

  /**
   * Commit the events collected by a refinement criterion traversal.
   */
  void finishRefinementCriterionStep();

  /**
   * Finish a mesh refinement realisation traversal.
   */
  void finishDynamicMeshRefinementStep(int numberOfDeferredTopologyChanges = 0);

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

  /**
   * True while the criterion traversal that finished last was the one silenced by
   * _topologyChangedSinceLastTimeStep. Its empty request set says nothing about
   * whether the mesh has settled, so a caller that drains mesh updates has to run
   * one more round before it may conclude that the criterion wants nothing.
   */
  bool _lastCriterionStepWasSuppressed;

  tarch::la::Vector<DIMENSIONS, double> _domainOffset;

  int _numberOfSolvers;

  void finishStep(bool suppressEventsIfUnchanged, bool recordRealisedEvents, int numberOfDeferredTopologyChanges = 0);

  /**
   * Promote patch-local erase requests to parent-sized erase events.
   */
  std::vector<peano4::grid::GridControlEvent> finaliseCoarseningEvents(
    const std::vector<RawRequest>&               requests,
    std::vector<peano4::grid::GridControlEvent>& parentEventsRequiringRealisationDelivery
  ) const;

  /**
   * Replace the local raw requests by the union over all ranks.
   */
  static void exchangeRawRequests(std::vector<RawRequest>& requests);
};
