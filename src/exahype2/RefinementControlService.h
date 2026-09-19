// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "exahype2/RefinementControl.h"
#include "peano4/grid/GridControlEvent.h"
#include "tarch/CompilerSpecificSettings.h"
#include "tarch/la/Vector.h"

namespace exahype2 {
  class RefinementControlService;
} // namespace exahype2

/**
 * Collects all refine and erase requests on one rank.
 *
 * finishStep() exchanges the requests collected during one traversal, so every
 * rank holds the same request set and derives the same refinement plan. The
 * collective carries only the removals and additions against the request list
 * each rank shipped last, and it requires the finish calls to run in lockstep
 * across the ranks.
 */
class exahype2::RefinementControlService final {
public:
  RefinementControlService(const RefinementControlService& copy)            = delete;
  RefinementControlService(RefinementControlService&& move)                 = delete;
  RefinementControlService& operator=(const RefinementControlService& copy) = delete;
  RefinementControlService& operator=(RefinementControlService&& move)      = delete;
  ~RefinementControlService()                                               = default;

  static RefinementControlService& getInstance();

  void shutdown();

  [[nodiscard]] std::vector<peano4::grid::GridControlEvent> getGridControlEvents() const;

  [[nodiscard]] bool hasGridControlEvents() const;

  [[nodiscard]] bool hasRealisedTopologyChangesSinceLastTimeStep() const;

  [[nodiscard]] bool lastCriterionStepWasSuppressed() const;

  [[nodiscard]] bool committedEventsAgreeAcrossRanks() const;

  void setDomain(
    const tarch::la::Vector<DIMENSIONS, double>& domainOffset,
    const tarch::la::Vector<DIMENSIONS, double>& domainWidth
  );

  [[nodiscard]] INLINE_METHOD const tarch::la::Vector<DIMENSIONS, double>& getDomainOffset() const noexcept {
    return _domainOffset;
  }
  [[nodiscard]] INLINE_METHOD const tarch::la::Vector<DIMENSIONS, double>& getDomainWidth() const noexcept {
    return _domainWidth;
  }

  void setNumberOfSolvers(int numberOfSolvers);

  void finishStep();

  void finishRefinementCriterionStep();

  void finishDynamicMeshRefinementStep();

  void realiseCommittedEventsWithoutSweep();

  void realisePlanRejectedEvents();

  [[nodiscard]] std::string toString() const;

  void merge(const RefinementControl& control);

private:
  RefinementControlService();

  /**
   * Carries one cell request and its solver number between ranks, so the
   * per-solver coarsening consensus covers sibling groups split across ranks.
   */
  struct RawRequest {
    peano4::grid::GridControlEvent event{};
    std::int32_t                   solverNumber{0};
  };
  static_assert(std::is_trivially_copyable_v<RawRequest>, "Raw requests travel between ranks as bytes.");

  /**
   * Classifies finish steps. The delta exchange keeps one baseline per kind.
   */
  enum class StepKind { Plain = 0, Criterion = 1, Realise = 2 };
  static constexpr int NumberOfStepKinds{3};

  /**
   * Stores the requests every observer merged during the running traversal,
   * one contribution per merge call, per thread. merge() appends to the
   * calling thread's list, so it needs no lock.
   */
  std::vector<std::vector<RefinementControl::NewEvents>> _contributionsPerThread;

  /**
   * Stores what the last finished step of one kind reduced: Its contributions
   * in canonical order and the committed set they produced. A step whose
   * contributions repeat the stored ones reuses the committed set.
   */
  struct StepOutcome {
    std::vector<RefinementControl::NewEvents>   contributions;
    std::vector<peano4::grid::GridControlEvent> committedEvents;
  };
  std::array<StepOutcome, NumberOfStepKinds> _lastOutcomePerKind{};

  /**
   * Stores requests for the next mesh traversal in canonical form: Coarsening promoted
   * to the parent cell, then sorted and deduplicated. Writers run between traversals only, so
   * traversals read this set without a lock.
   */
  std::vector<peano4::grid::GridControlEvent> _committedEvents;

  /**
   * Stores the event set the last completed mesh refinement cycle realised. A criterion commit
   * that reproduces it exactly cannot change the mesh again and is dropped.
   */
  std::vector<peano4::grid::GridControlEvent> _lastRealisedEvents;

  /**
   * Tracks the interval from the first realise traversal that consumed events until the end of the next
   * time step. Every rank derives the flag from the collectively exchanged set, so the
   * decision is rank-uniform.
   */
  bool _topologyChangedSinceLastTimeStep{false};

  /**
   * True while the criterion traversal that finished last was the one silenced by
   * _topologyChangedSinceLastTimeStep. Its empty request set says nothing about
   * whether the mesh has settled, so a caller that drains mesh updates runs one
   * more round.
   */
  bool _lastCriterionStepWasSuppressed{false};

  tarch::la::Vector<DIMENSIONS, double> _domainOffset{};

  tarch::la::Vector<DIMENSIONS, double> _domainWidth{tarch::la::Vector<DIMENSIONS, double>(1.0)};

  int _numberOfSolvers{1};

  /**
   * Stores the canonical request list every rank shipped last, per step kind and rank.
   * Every rank maintains all ranks' lists from the exchanged deltas, so the
   * table is identical across the ranks and the reconstructed union is too.
   */
  std::array<std::vector<std::vector<RawRequest>>, NumberOfStepKinds> _shippedRequestsPerKindAndRank{};

  void finishStep(bool suppressEventsIfUnchanged, bool recordRealisedEvents);

  [[nodiscard]] std::vector<peano4::grid::GridControlEvent> finaliseCoarseningEvents(
    const std::vector<RawRequest>& requests
  ) const;

  bool exchangeRawRequests(std::vector<RawRequest>& requests, StepKind stepKind, bool localRequestsUnchanged);

  static bool rawRequestPrecedes(const RawRequest& left, const RawRequest& right);
  static bool rawRequestsMatch(const RawRequest& left, const RawRequest& right);
};
