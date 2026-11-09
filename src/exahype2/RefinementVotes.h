// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "exahype2/RefinementCommand.h"
#include "peano4/CellConnectivity.h"
#include "tarch/CompilerSpecificSettings.h"
#include "tarch/Span.h"
#include "tarch/accelerator/Buffer.h"
#include "tarch/accelerator/MemoryLocation.h"
#include "tarch/accelerator/accelerator.h"
#include "tarch/la/Vector.h"

namespace exahype2 {
  struct Limits;
  class RefinementRequests;
  class RefinementVotes;

  template <typename Derived, typename StoragePrecision>
  struct VoteOnGridConstruction;

  template <typename Derived, typename SolutionView>
  struct VoteOnRefinement;
} // namespace exahype2

/**
 * Refinement votes of one solver, one command per slot of the cell connectivity. Every vote method
 * computes the commands on the device and merges them into requests.
 */
class exahype2::RefinementVotes final {
public:
  RefinementVotes()                                        = default;
  RefinementVotes(const RefinementVotes& other)            = delete;
  RefinementVotes(RefinementVotes&& other)                 = delete;
  RefinementVotes& operator=(const RefinementVotes& other) = delete;
  RefinementVotes& operator=(RefinementVotes&& other)      = delete;
  ~RefinementVotes()                                       = default;

  template <typename Derived, typename StoragePrecision>
  INLINE_METHOD void voteOnGridConstruction(
    const peano4::CellConnectivity&            cellConnectivity,
    const exahype2::Limits&                    limits,
    exahype2::RefinementRequests&              requests,
    const typename Derived::RuntimeParameters& parameters
  );

  template <typename Derived, typename SolutionView>
  INLINE_METHOD void voteOnEveryCell(
    const peano4::CellConnectivity&            cellConnectivity,
    const SolutionView&                        solution,
    const exahype2::Limits&                    limits,
    double                                     timeStamp,
    exahype2::RefinementRequests&              requests,
    const typename Derived::RuntimeParameters& parameters
  );

  template <typename Derived, typename SolutionView>
  INLINE_METHOD void voteOnCells(
    const peano4::CellConnectivity&            cellConnectivity,
    const std::vector<int>&                    cells,
    const SolutionView&                        solution,
    const exahype2::Limits&                    limits,
    double                                     timeStamp,
    exahype2::RefinementRequests&              requests,
    const typename Derived::RuntimeParameters& parameters
  );

  INLINE_METHOD void release();
  [[nodiscard]] INLINE_METHOD tarch::AllocatedBytes getAllocatedBytes() const;

private:
  [[nodiscard]] static INLINE_METHOD bool canRefineOrErase(
    const peano4::CellConnectivity& cellConnectivity,
    const exahype2::Limits&         limits
  );

  INLINE_METHOD void reserve(int numberOfSlots);

  INLINE_METHOD void merge(
    const peano4::CellConnectivity& cellConnectivity,
    int                             votes,
    tarch::Span<const int>          cellsOnHost,
    exahype2::RefinementRequests&   requests
  );

  tarch::accelerator::Buffer<exahype2::RefinementCommand> _commands{tarch::MemoryLocation::Device};
  std::vector<exahype2::RefinementCommand>                _commandsOnHost{};
  tarch::accelerator::Buffer<int>                         _listedCells{tarch::MemoryLocation::Device};
};

TARGET_DECLARE
namespace exahype2 {
  template <typename Derived, typename StoragePrecision>
  INLINE_METHOD GPU_CALLABLE_METHOD RefinementCommand voteWithinAdmissibleSizes(
    const StoragePrecision*                      Q,
    const tarch::la::Vector<DIMENSIONS, double>& centre,
    const tarch::la::Vector<DIMENSIONS, double>& width,
    double                                       maxAdmissibleSize,
    double                                       minAdmissibleSize,
    double                                       timeStamp,
    const typename Derived::RuntimeParameters&   parameters
  );
} // namespace exahype2

/**
 * Calls the refinement criterion of Derived on one cell of a grid under construction and adds one
 * to the accumulator when the command differs from Keep. The patch of the cell is still empty.
 */
template <typename Derived, typename StoragePrecision>
struct exahype2::VoteOnGridConstruction final {
  peano4::CellConnectivity            cellConnectivity{};
  exahype2::RefinementCommand*        commands{nullptr};
  double                              maxAdmissibleSize{0.0};
  double                              minAdmissibleSize{0.0};
  typename Derived::RuntimeParameters parameters{};

  INLINE_METHOD GPU_CALLABLE_METHOD void operator()(int entry, int& accumulator) const;
};

/**
 * Calls the refinement criterion of Derived on the patch of one listed cell and adds one to the
 * accumulator when the command differs from Keep.
 */
template <typename Derived, typename SolutionView>
struct exahype2::VoteOnRefinement final {
  peano4::CellConnectivity            cellConnectivity{};
  tarch::Span<const int>              cells{};
  SolutionView                        solution{};
  exahype2::RefinementCommand*        commands{nullptr};
  double                              maxAdmissibleSize{0.0};
  double                              minAdmissibleSize{0.0};
  double                              timeStamp{0.0};
  typename Derived::RuntimeParameters parameters{};

  INLINE_METHOD GPU_CALLABLE_METHOD void operator()(int entry, int& accumulator) const;
};
TARGET_DECLARE_END

#include "exahype2/RefinementVotes.cpph"
