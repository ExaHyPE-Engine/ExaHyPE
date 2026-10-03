// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "LevelJumpsTest.h"

#include "exahype2/LevelJumps.h"
#include "peano4/Constants.h"
#include "peano4/Loop.h"
#include "peano4/Partition.h"
#include "peano4/RefinementRequest.h"
#include "peano4/Spacetree.h"
#include "peano4/tests/TestGrids.h"
#include "tarch/Assertions.h"
#include "tarch/CompilerSpecificSettings.h"
#include "tarch/Span.h"
#include "tarch/accelerator/Buffer.h"
#include "tarch/accelerator/MemoryLocation.h"
#include "tarch/accelerator/accelerator.h"
#include "tarch/la/Vector.h"
#include "tarch/mpi/Rank.h"

namespace {
  /**
   * Smallest depth at which the first cell of level one covers at least three cells per axis.
   */
  constexpr int Depth = peano4::SubdivisionFactor >= 3 ? 2 : 3;

  /**
   * Returns the partition of a regular grid whose first cell of level one is refined, so level
   * jumps surround it. The curve starts in that cell, so on more than one rank the first rank owns
   * only refined cells beside coarse cells of another rank.
   */
  const peano4::Partition& buildPartitionWithLevelJumps() {
    peano4::tests::buildRegularGrid(Depth);
    // A request refines the cells up to the ring radius around its cell, so the requested cells
    // stay one ring radius inside the far side of the first cell of level one.
    const int ringRadius            = (peano4::tests::getNumberOfCellsPerAxisRefinedAroundTheCentre() - 1) / 2;
    const int requestedCellsPerAxis = peano4::power(peano4::SubdivisionFactor, Depth - 1) - ringRadius;
    assertion(requestedCellsPerAxis >= 1, requestedCellsPerAxis);
    std::vector<peano4::RefinementRequest> requests;
    peano4::dForEach(requestedCellsPerAxis, [&requests](const tarch::la::Vector<DIMENSIONS, int>& latticeIndex) {
      peano4::RefinementRequest request;
      request.setRefinementControl(peano4::RefinementRequest::RefinementControl::Refine);
      request.level        = Depth;
      request.latticeIndex = latticeIndex;
      requests.push_back(request);
    });
    peano4::tests::refineGrid(requests);
    return peano4::Spacetree::getInstance().getPartition();
  }

  int getNumberOfLevelJumpsCoarserThanEveryCell(const peano4::Partition& partition) {
    int result = 0;
    for (const auto adjacency : {peano4::Adjacency::Local, peano4::Adjacency::Remote}) {
      for (const auto& levelJump : partition.getLevelJumps(adjacency)) {
        result += levelJump.level < partition.getMinimumLevel() ? 1 : 0;
      }
    }
    return result;
  }

  /**
   * Returns every level jump of the partition, which stores the remote level jumps right after the
   * local ones.
   */
  tarch::Span<const peano4::LevelJump> getEveryLevelJump(const peano4::Partition& partition) {
    const auto local  = partition.getLevelJumps(peano4::Adjacency::Local);
    const auto remote = partition.getLevelJumps(peano4::Adjacency::Remote);
    return {local.data, local.size + remote.size};
  }

  TARGET_DECLARE
  /**
   * Counts the visits of every level jump and, per visit, the launched level jumps that have to run
   * earlier and have no visit yet.
   */
  struct RecordVisit final {
    tarch::Span<const peano4::LevelJump> levelJumps{};
    const int*                           launched{nullptr};
    int*                                 visits{nullptr};
    int*                                 missedEarlierVisits{nullptr};
    bool                                 coarsestLevelFirst{true};

    INLINE_METHOD GPU_CALLABLE_METHOD void operator()(const peano4::LevelJump& levelJump) const {
      const auto visited = static_cast<int>(&levelJump - levelJumps.data);
      for (int other = 0; other < levelJumps.size; other++) {
        const int  otherLevel  = levelJumps[other].level;
        const bool runsEarlier = coarsestLevelFirst ? otherLevel < levelJump.level : otherLevel > levelJump.level;
        if (launched[other] != 0 and runsEarlier and visits[other] == 0) {
          missedEarlierVisits[visited]++;
        }
      }
      visits[visited]++;
    }
  };
  TARGET_DECLARE_END

  std::vector<int> getRemoteMarks(const peano4::Partition& partition) {
    const auto       local  = partition.getLevelJumps(peano4::Adjacency::Local);
    const auto       remote = partition.getLevelJumps(peano4::Adjacency::Remote);
    std::vector<int> marks(static_cast<std::size_t>(local.size), 0);
    marks.insert(marks.end(), static_cast<std::size_t>(remote.size), 1);
    return marks;
  }

  tarch::accelerator::Buffer<int> getDeviceCopyOf(const std::vector<int>& values) {
    tarch::accelerator::Buffer<int> buffer(tarch::MemoryLocation::Device);
    buffer.setValues(values);
    return buffer;
  }
} // namespace

exahype2::tests::LevelJumpsTest::LevelJumpsTest() :
  tarch::tests::TestCase("exahype2::tests::LevelJumpsTest") {}

void
exahype2::tests::LevelJumpsTest::run() {
  testMethod(testTheInterpolationsVisitEveryLevelJumpOnceCoarsestLevelFirst);
  testMethod(testTheRestrictionVisitsEveryLevelJumpOnceFinestLevelFirst);
  testMethod(testTheGroupedRestrictionsVisitEveryLevelJumpOnceFinestLevelFirst);
}

/**
 * The interpolation before the exchange visits the local level jumps and the one after it the
 * remote ones, also on a rank with level jumps coarser than all its cells.
 */
void
exahype2::tests::LevelJumpsTest::testTheInterpolationsVisitEveryLevelJumpOnceCoarsestLevelFirst() {
  const auto& partition  = buildPartitionWithLevelJumps();
  const auto  levelJumps = getEveryLevelJump(partition.onDevice());

  const std::vector<int> remote = getRemoteMarks(partition);
  std::vector<int>       local(remote.size());
  for (std::size_t jump = 0; jump < remote.size(); jump++) {
    local[jump] = 1 - remote[jump];
  }
  const tarch::accelerator::Buffer<int> localOnDevice  = getDeviceCopyOf(local);
  const tarch::accelerator::Buffer<int> remoteOnDevice = getDeviceCopyOf(remote);
  tarch::accelerator::Buffer<int>       visits         = getDeviceCopyOf(std::vector<int>(remote.size(), 0));
  tarch::accelerator::Buffer<int>       missedVisits   = getDeviceCopyOf(std::vector<int>(remote.size(), 0));

  exahype2::interpolateCoarseFaces(
    partition,
    peano4::Adjacency::Local,
    RecordVisit{levelJumps, localOnDevice.getData(), visits.getData(), missedVisits.getData(), true}
  );
  const std::vector<int> visitsBeforeExchange = visits.getValues();
  exahype2::interpolateCoarseFaces(
    partition,
    peano4::Adjacency::Remote,
    RecordVisit{levelJumps, remoteOnDevice.getData(), visits.getData(), missedVisits.getData(), true}
  );
  const std::vector<int> visitsAfterExchange = visits.getValues();
  const std::vector<int> missed              = missedVisits.getValues();

  validate(not remote.empty() or tarch::mpi::Rank::getInstance().getNumberOfRanks() > 1);
  if (tarch::mpi::Rank::getInstance().getNumberOfRanks() > 1) {
    validate(peano4::tests::getGlobalSumOf(getNumberOfLevelJumpsCoarserThanEveryCell(partition)) > 0);
  }
  for (std::size_t jump = 0; jump < remote.size(); jump++) {
    validateEqualsWithParams1(visitsBeforeExchange[jump], local[jump], jump);
    validateEqualsWithParams1(visitsAfterExchange[jump], 1, jump);
    validateEqualsWithParams1(missed[jump], 0, jump);
  }
}

void
exahype2::tests::LevelJumpsTest::testTheRestrictionVisitsEveryLevelJumpOnceFinestLevelFirst() {
  const auto& partition  = buildPartitionWithLevelJumps();
  const auto  levelJumps = getEveryLevelJump(partition.onDevice());
  const auto  count      = static_cast<std::size_t>(levelJumps.size);

  const tarch::accelerator::Buffer<int> everyLevelJump = getDeviceCopyOf(std::vector<int>(count, 1));
  tarch::accelerator::Buffer<int>       visits         = getDeviceCopyOf(std::vector<int>(count, 0));
  tarch::accelerator::Buffer<int>       missedVisits   = getDeviceCopyOf(std::vector<int>(count, 0));

  exahype2::restrictFineFaces(
    partition,
    RecordVisit{levelJumps, everyLevelJump.getData(), visits.getData(), missedVisits.getData(), false}
  );

  const std::vector<int> visitCounts = visits.getValues();
  const std::vector<int> missed      = missedVisits.getValues();
  for (std::size_t jump = 0; jump < count; jump++) {
    validateEqualsWithParams1(visitCounts[jump], 1, jump);
    validateEqualsWithParams1(missed[jump], 0, jump);
  }
}

void
exahype2::tests::LevelJumpsTest::testTheGroupedRestrictionsVisitEveryLevelJumpOnceFinestLevelFirst() {
  const auto& partition  = buildPartitionWithLevelJumps();
  const auto  levelJumps = getEveryLevelJump(partition.onDevice());

  const std::vector<int> remote = getRemoteMarks(partition);
  std::vector<int>       local(remote.size());
  for (std::size_t jump = 0; jump < remote.size(); jump++) {
    local[jump] = 1 - remote[jump];
  }
  const tarch::accelerator::Buffer<int> localOnDevice  = getDeviceCopyOf(local);
  const tarch::accelerator::Buffer<int> remoteOnDevice = getDeviceCopyOf(remote);
  tarch::accelerator::Buffer<int>       visits         = getDeviceCopyOf(std::vector<int>(remote.size(), 0));
  tarch::accelerator::Buffer<int>       missedVisits   = getDeviceCopyOf(std::vector<int>(remote.size(), 0));

  exahype2::restrictFineFaces(
    partition,
    peano4::Adjacency::Remote,
    RecordVisit{levelJumps, remoteOnDevice.getData(), visits.getData(), missedVisits.getData(), false}
  );
  const std::vector<int> visitsBeforeTheMessages = visits.getValues();
  exahype2::restrictFineFaces(
    partition,
    peano4::Adjacency::Local,
    RecordVisit{levelJumps, localOnDevice.getData(), visits.getData(), missedVisits.getData(), false}
  );
  const std::vector<int> visitsAfterTheMessages = visits.getValues();
  const std::vector<int> missed                 = missedVisits.getValues();

  for (std::size_t jump = 0; jump < remote.size(); jump++) {
    validateEqualsWithParams1(visitsBeforeTheMessages[jump], remote[jump], jump);
    validateEqualsWithParams1(visitsAfterTheMessages[jump], 1, jump);
    validateEqualsWithParams1(missed[jump], 0, jump);
  }
}
