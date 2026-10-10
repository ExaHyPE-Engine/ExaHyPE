// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "RefinementVotesTest.h"

#include "exahype2/Limits.h"
#include "exahype2/RefinementCommand.h"
#include "exahype2/RefinementRequests.h"
#include "exahype2/RefinementVotes.h"
#include "peano4/CellConnectivity.h"
#include "peano4/Spacetree.h"
#include "peano4/tests/TestGrids.h"
#include "tarch/CompilerSpecificSettings.h"
#include "tarch/accelerator/ChunkedMemoryPool.h"
#include "tarch/accelerator/MemoryLocation.h"
#include "tarch/accelerator/ParallelFor.h"
#include "tarch/accelerator/Range.h"
#include "tarch/accelerator/accelerator.h"
#include "tarch/la/Vector.h"

namespace {
  constexpr double MaxAdmissibleSize = 1.0;
  constexpr double MinAdmissibleSize = 0.1;
  constexpr double TimeStamp         = 0.5;

  /**
   * Solver whose refinement criterion returns the command encoded in the first value of the state.
   */
  struct StubSolver final {
    static GPU_CALLABLE_METHOD exahype2::RefinementCommand refinementCriterion(
      const double* const                                           Q,
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& centre,
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& width,
      [[maybe_unused]] const double                                 timeStamp
    ) {
      return Q == nullptr ? exahype2::RefinementCommand::Keep
                          : static_cast<exahype2::RefinementCommand>(static_cast<std::uint8_t>(Q[0]));
    }
  };

  int getNumberOf(const exahype2::RefinementCommand command) { return static_cast<int>(command); }

  constexpr int Depth = 1;

  using Pool = tarch::accelerator::ChunkedMemoryPool<double>;

  const peano4::CellConnectivity& buildGrid() {
    peano4::tests::buildRegularGrid(Depth);
    return peano4::Spacetree::getInstance().getCellConnectivity();
  }

  TARGET_DECLARE
  struct WriteCommandIntoEveryCell final {
    peano4::CellConnectivity cellConnectivity{};
    Pool::View               solution{};
    double                   code{0.0};

    INLINE_METHOD GPU_CALLABLE_METHOD void operator()(const int entry) const {
      solution.getChunk(cellConnectivity.getCells()[entry])[0] = code;
    }
  };
  TARGET_DECLARE_END

  void writeCommandIntoEveryCell(
    Pool&                             pool,
    const peano4::CellConnectivity&   cellConnectivity,
    const exahype2::RefinementCommand command
  ) {
    tarch::accelerator::parallelFor(
      tarch::accelerator::Range(cellConnectivity.getCells().size),
      WriteCommandIntoEveryCell{
        cellConnectivity.onDevice(),
        pool.getView(),
        static_cast<double>(static_cast<std::uint8_t>(command))
      }
    );
  }

  /**
   * Returns limits that leave every vote on the regular grid of depth Depth to the criterion.
   */
  exahype2::Limits getLimitsAroundTheGrid() {
    exahype2::Limits limits;
    limits.minAdmissiblePatchSize = 0.0;
    limits.maxAdmissiblePatchSize = 1.0;
    return limits;
  }

  exahype2::RefinementCommand getVoteOf(const exahype2::RefinementCommand criterion, const double width) {
    const auto state = static_cast<double>(static_cast<std::uint8_t>(criterion));
    return exahype2::voteWithinAdmissibleSizes<StubSolver>(
      &state,
      tarch::la::Vector<DIMENSIONS, double>(0.5),
      tarch::la::Vector<DIMENSIONS, double>(width),
      MaxAdmissibleSize,
      MinAdmissibleSize,
      TimeStamp
    );
  }
} // namespace

exahype2::tests::RefinementVotesTest::RefinementVotesTest() :
  tarch::tests::TestCase("exahype2::tests::RefinementVotesTest") {}

void
exahype2::tests::RefinementVotesTest::run() {
  testMethod(testACellAboveTheLargestSizeRefines);
  testMethod(testTheCriterionDecidesWithinTheAdmissibleSizes);
  testMethod(testARefineVoteAtTheSmallestSizeTurnsIntoKeep);
  testMethod(testAnEraseVoteThatWouldPassTheLargestSizeTurnsIntoKeep);
  testMethod(testTheVotesOnEveryCellReachTheRequests);
  testMethod(testGridConstructionRefinesEveryCellAboveTheLargestSize);
  testMethod(testOnlyTheListedCellsVote);
  testMethod(testLimitsThatFixTheMeshSkipTheVotes);
}

void
exahype2::tests::RefinementVotesTest::testACellAboveTheLargestSizeRefines() {
  const double width = 2.0 * MaxAdmissibleSize;
  validateEquals(
    getNumberOf(getVoteOf(exahype2::RefinementCommand::Keep, width)),
    getNumberOf(exahype2::RefinementCommand::Refine)
  );
  validateEquals(
    getNumberOf(getVoteOf(exahype2::RefinementCommand::Erase, width)),
    getNumberOf(exahype2::RefinementCommand::Refine)
  );
}

void
exahype2::tests::RefinementVotesTest::testTheCriterionDecidesWithinTheAdmissibleSizes() {
  const double width = MaxAdmissibleSize / peano4::SubdivisionFactor;
  validateEquals(
    getNumberOf(getVoteOf(exahype2::RefinementCommand::Refine, width)),
    getNumberOf(exahype2::RefinementCommand::Refine)
  );
  validateEquals(
    getNumberOf(getVoteOf(exahype2::RefinementCommand::Keep, width)),
    getNumberOf(exahype2::RefinementCommand::Keep)
  );
  validateEquals(
    getNumberOf(getVoteOf(exahype2::RefinementCommand::Erase, width)),
    getNumberOf(exahype2::RefinementCommand::Erase)
  );
}

void
exahype2::tests::RefinementVotesTest::testARefineVoteAtTheSmallestSizeTurnsIntoKeep() {
  validateEquals(
    getNumberOf(getVoteOf(exahype2::RefinementCommand::Refine, MinAdmissibleSize)),
    getNumberOf(exahype2::RefinementCommand::Keep)
  );
}

void
exahype2::tests::RefinementVotesTest::testAnEraseVoteThatWouldPassTheLargestSizeTurnsIntoKeep() {
  const double width = MaxAdmissibleSize;
  validateEquals(
    getNumberOf(getVoteOf(exahype2::RefinementCommand::Erase, width)),
    getNumberOf(exahype2::RefinementCommand::Keep)
  );
}

void
exahype2::tests::RefinementVotesTest::testTheVotesOnEveryCellReachTheRequests() {
  const auto& cellConnectivity = buildGrid();
  const int   numberOfSlots    = cellConnectivity.getNumberOfSlots();
  Pool        pool(1, tarch::MemoryLocation::Device);
  pool.reserve(numberOfSlots);
  exahype2::RefinementVotes    votes;
  exahype2::RefinementRequests requests;

  writeCommandIntoEveryCell(pool, cellConnectivity, exahype2::RefinementCommand::Refine);
  requests.clear(numberOfSlots);
  votes.voteOnEveryCell<StubSolver>(
    cellConnectivity,
    std::as_const(pool).getView(),
    getLimitsAroundTheGrid(),
    TimeStamp,
    requests
  );
  validate(not requests.getRefinementRequests(cellConnectivity).empty());
  validate(votes.getAllocatedBytes().getTotal() >= static_cast<std::size_t>(numberOfSlots));

  writeCommandIntoEveryCell(pool, cellConnectivity, exahype2::RefinementCommand::Keep);
  requests.clear(numberOfSlots);
  votes.voteOnEveryCell<StubSolver>(
    cellConnectivity,
    std::as_const(pool).getView(),
    getLimitsAroundTheGrid(),
    TimeStamp,
    requests
  );
  validate(requests.getRefinementRequests(cellConnectivity).empty());

  votes.release();
  validateEquals(votes.getAllocatedBytes().getTotal(), std::size_t{0});
}

void
exahype2::tests::RefinementVotesTest::testGridConstructionRefinesEveryCellAboveTheLargestSize() {
  const auto&                  cellConnectivity = buildGrid();
  exahype2::RefinementVotes    votes;
  exahype2::RefinementRequests requests;
  exahype2::Limits             limits = getLimitsAroundTheGrid();

  limits.maxAdmissiblePatchSize = 0.5 / peano4::SubdivisionFactor;
  requests.clear(cellConnectivity.getNumberOfSlots());
  votes.voteOnGridConstruction<StubSolver, double>(cellConnectivity, limits, requests);
  validate(not requests.getRefinementRequests(cellConnectivity).empty());

  limits.maxAdmissiblePatchSize = 1.0;
  requests.clear(cellConnectivity.getNumberOfSlots());
  votes.voteOnGridConstruction<StubSolver, double>(cellConnectivity, limits, requests);
  validate(requests.getRefinementRequests(cellConnectivity).empty());
}

void
exahype2::tests::RefinementVotesTest::testOnlyTheListedCellsVote() {
  const auto& cellConnectivity = buildGrid();
  const int   numberOfSlots    = cellConnectivity.getNumberOfSlots();
  Pool        pool(1, tarch::MemoryLocation::Device);
  pool.reserve(numberOfSlots);
  writeCommandIntoEveryCell(pool, cellConnectivity, exahype2::RefinementCommand::Refine);
  exahype2::RefinementVotes    votes;
  exahype2::RefinementRequests requests;
  std::vector<int>             listed;

  requests.clear(numberOfSlots);
  votes.voteOnCells<StubSolver>(
    cellConnectivity,
    listed,
    std::as_const(pool).getView(),
    getLimitsAroundTheGrid(),
    TimeStamp,
    requests
  );
  validate(requests.getRefinementRequests(cellConnectivity).empty());

  if (not cellConnectivity.getCells().isEmpty()) {
    listed.push_back(cellConnectivity.getCells()[0]);
  }
  requests.clear(numberOfSlots);
  votes.voteOnCells<StubSolver>(
    cellConnectivity,
    listed,
    std::as_const(pool).getView(),
    getLimitsAroundTheGrid(),
    TimeStamp,
    requests
  );
  validate(not requests.getRefinementRequests(cellConnectivity).empty());
}

void
exahype2::tests::RefinementVotesTest::testLimitsThatFixTheMeshSkipTheVotes() {
  const auto&      cellConnectivity = buildGrid();
  const int        numberOfSlots    = cellConnectivity.getNumberOfSlots();
  const double     width            = cellConnectivity.getWidthOfLevel(cellConnectivity.getMaximumLevel(), 0);
  exahype2::Limits fixingLimits;
  fixingLimits.minAdmissiblePatchSize = width;
  fixingLimits.maxAdmissiblePatchSize = width;
  validate(not exahype2::RefinementVotes::canRefineOrErase(cellConnectivity, fixingLimits));
  validate(exahype2::RefinementVotes::canRefineOrErase(cellConnectivity, getLimitsAroundTheGrid()));

  Pool pool(1, tarch::MemoryLocation::Device);
  pool.reserve(numberOfSlots);
  writeCommandIntoEveryCell(pool, cellConnectivity, exahype2::RefinementCommand::Refine);
  exahype2::RefinementVotes    votes;
  exahype2::RefinementRequests requests;
  requests.clear(numberOfSlots);
  votes.voteOnEveryCell<StubSolver>(cellConnectivity, std::as_const(pool).getView(), fixingLimits, TimeStamp, requests);
  validate(requests.getRefinementRequests(cellConnectivity).empty());
  votes.release();
}
