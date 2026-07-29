// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "ExchangeChannelsTest.h"

#include "peano4/parallel/ExchangeChannels.h"
#include "peano4/parallel/TreeIdMapping.h"
#include "tarch/mpi/Rank.h"

#include <mpi.h>

peano4::parallel::tests::ExchangeChannelsTest::ExchangeChannelsTest() :
  TestCase("peano4::parallel::tests::ExchangeChannelsTest") {}

void peano4::parallel::tests::ExchangeChannelsTest::testTagCalculation() {
  constexpr int          MinimumMpiTagUpperBound  = 32767;
  constexpr std::int64_t TagsPerSendingTree       = static_cast<std::int64_t>(4) * MAX_SPACETREES_PER_RANK;
  const int              expectedMinimumGroupSize = static_cast<int>(std::min<std::int64_t>(
    MAX_SPACETREES_PER_RANK,
    (static_cast<std::int64_t>(MinimumMpiTagUpperBound) + 1) / TagsPerSendingTree
  ));
  const int              minimumGroupSize =
    peano4::parallel::ExchangeChannels::getNumberOfSendingTreesPerDataExchangeCommunicator(MinimumMpiTagUpperBound);
  validateEquals(minimumGroupSize, expectedMinimumGroupSize);
  validate(minimumGroupSize > 0);

  const int tagUpperBoundBelowOneSender = static_cast<int>(TagsPerSendingTree - 2);
  validateEquals(
    peano4::parallel::ExchangeChannels::getNumberOfSendingTreesPerDataExchangeCommunicator(tagUpperBoundBelowOneSender),
    0
  );

  constexpr int LargeTagUpperBound     = std::numeric_limits<int>::max();
  const int     expectedLargeGroupSize = static_cast<int>(std::min<std::int64_t>(
    MAX_SPACETREES_PER_RANK,
    (static_cast<std::int64_t>(LargeTagUpperBound) + 1) / TagsPerSendingTree
  ));
  validateEquals(
    peano4::parallel::ExchangeChannels::getNumberOfSendingTreesPerDataExchangeCommunicator(LargeTagUpperBound),
    expectedLargeGroupSize
  );

  if (minimumGroupSize <= 0) {
    return;
  }

  validateEquals(peano4::parallel::ExchangeChannels::getDataExchangeCommunicatorIndex(0, minimumGroupSize), 0);
  validateEquals(
    peano4::parallel::ExchangeChannels::getDataExchangeCommunicatorIndex(minimumGroupSize - 1, minimumGroupSize),
    0
  );

  const int lastTagInFirstGroup = peano4::parallel::ExchangeChannels::getDataExchangeTag(
    minimumGroupSize - 1,
    MAX_SPACETREES_PER_RANK - 1,
    peano4::parallel::ExchangeChannels::ExchangeMode::PeriodicData,
    minimumGroupSize
  );
  validateEquals(lastTagInFirstGroup, 4 * minimumGroupSize * MAX_SPACETREES_PER_RANK - 1);
  validate(lastTagInFirstGroup <= MinimumMpiTagUpperBound);
  if (minimumGroupSize < MAX_SPACETREES_PER_RANK) {
    validateEquals(
      peano4::parallel::ExchangeChannels::getDataExchangeCommunicatorIndex(minimumGroupSize, minimumGroupSize),
      1
    );
    validateEquals(
      peano4::parallel::ExchangeChannels::getDataExchangeTag(
        minimumGroupSize,
        0,
        peano4::parallel::ExchangeChannels::ExchangeMode::HorizontalData,
        minimumGroupSize
      ),
      0
    );
  }

  constexpr std::array<peano4::parallel::ExchangeChannels::ExchangeMode, 4> ExchangeModes{
    peano4::parallel::ExchangeChannels::ExchangeMode::HorizontalData,
    peano4::parallel::ExchangeChannels::ExchangeMode::VerticalData,
    peano4::parallel::ExchangeChannels::ExchangeMode::PeriodicMetadata,
    peano4::parallel::ExchangeChannels::ExchangeMode::PeriodicData
  };
  std::set<int> tags;
  bool          tagsAreUnique = true;
  bool          tagsFit       = true;
  for (int localSendingTreeId = 0; localSendingTreeId < minimumGroupSize; localSendingTreeId++) {
    for (int localReceivingTreeId = 0; localReceivingTreeId < MAX_SPACETREES_PER_RANK; localReceivingTreeId++) {
      for (const auto exchangeMode : ExchangeModes) {
        const int tag = peano4::parallel::ExchangeChannels::getDataExchangeTag(
          localSendingTreeId,
          localReceivingTreeId,
          exchangeMode,
          minimumGroupSize
        );
        tagsAreUnique = tags.insert(tag).second and tagsAreUnique;
        tagsFit       = tag <= MinimumMpiTagUpperBound and tagsFit;
      }
    }
  }
  validate(tagsAreUnique);
  validate(tagsFit);
  validateEquals(
    tags.size(),
    static_cast<std::size_t>(minimumGroupSize) * MAX_SPACETREES_PER_RANK * ExchangeModes.size()
  );

  const int numberOfRanks    = tarch::mpi::Rank::getInstance().getNumberOfRanks();
  const int senderTree       = 0;
  const int receiverTree0    = 0;
  const int receiverTree1    = numberOfRanks;
  const int receiverTreeLast = (MAX_SPACETREES_PER_RANK - 1) * numberOfRanks;

  const auto horizontal0 = peano4::parallel::ExchangeChannels::getInstance().getGridDataExchangeMetaInformation(
    senderTree,
    receiverTree0,
    peano4::parallel::ExchangeChannels::ExchangeMode::HorizontalData
  );
  const auto vertical0 = peano4::parallel::ExchangeChannels::getInstance().getGridDataExchangeMetaInformation(
    senderTree,
    receiverTree0,
    peano4::parallel::ExchangeChannels::ExchangeMode::VerticalData
  );
  const auto horizontal1 = peano4::parallel::ExchangeChannels::getInstance().getGridDataExchangeMetaInformation(
    senderTree,
    receiverTree1,
    peano4::parallel::ExchangeChannels::ExchangeMode::HorizontalData
  );
  const auto vertical1 = peano4::parallel::ExchangeChannels::getInstance().getGridDataExchangeMetaInformation(
    senderTree,
    receiverTree1,
    peano4::parallel::ExchangeChannels::ExchangeMode::VerticalData
  );
  const auto periodicLast = peano4::parallel::ExchangeChannels::getInstance().getGridDataExchangeMetaInformation(
    senderTree,
    receiverTreeLast,
    peano4::parallel::ExchangeChannels::ExchangeMode::PeriodicData
  );

  validateWithParams4(
    horizontal0.first != vertical0.first,
    horizontal0.first,
    vertical0.first,
    receiverTree0,
    numberOfRanks
  );
  validateWithParams4(
    vertical0.first != horizontal1.first,
    vertical0.first,
    horizontal1.first,
    receiverTree1,
    numberOfRanks
  );
  validateWithParams4(
    horizontal1.first != vertical1.first,
    horizontal1.first,
    vertical1.first,
    receiverTree1,
    numberOfRanks
  );
  validate(periodicLast.first >= 0);
}

void peano4::parallel::tests::ExchangeChannelsTest::testDataExchangeCommunicatorCreation() {
  auto&         channels                = peano4::parallel::ExchangeChannels::getInstance();
  constexpr int MinimumMpiTagUpperBound = 32767;
  const int     minimumGroupSize =
    peano4::parallel::ExchangeChannels::getNumberOfSendingTreesPerDataExchangeCommunicator(MinimumMpiTagUpperBound);
  validate(minimumGroupSize > 0);
  if (minimumGroupSize <= 0) {
    return;
  }

  const int configuredGroupSize                     = channels._sendingTreesPerDataExchangeCommunicator;
  channels._sendingTreesPerDataExchangeCommunicator = minimumGroupSize;

  validateEquals(channels._needCommunicator[0], tarch::mpi::Rank::getInstance().isGlobalMaster());
  channels.createPendingCommunicators();
  const auto firstGroup = channels.getGridDataExchangeMetaInformation(
    peano4::parallel::TreeIdMapping::getGlobalTreeId(0),
    peano4::parallel::TreeIdMapping::getGlobalTreeId(0),
    peano4::parallel::ExchangeChannels::ExchangeMode::HorizontalData
  );
  validate(firstGroup.second != MPI_COMM_NULL);
  validate(not channels._needCommunicator[0]);

  channels._needCommunicator[minimumGroupSize - 1] = true;
  channels.createPendingCommunicators();
  const auto reusedGroup = channels.getGridDataExchangeMetaInformation(
    peano4::parallel::TreeIdMapping::getGlobalTreeId(minimumGroupSize - 1),
    peano4::parallel::TreeIdMapping::getGlobalTreeId(0),
    peano4::parallel::ExchangeChannels::ExchangeMode::HorizontalData
  );
  int reuseComparison = MPI_UNEQUAL;
  validateEquals(MPI_Comm_compare(firstGroup.second, reusedGroup.second, &reuseComparison), MPI_SUCCESS);
  validateEquals(reuseComparison, MPI_IDENT);
  validate(not channels._needCommunicator[minimumGroupSize - 1]);

  if (minimumGroupSize < MAX_SPACETREES_PER_RANK) {
    channels._needCommunicator[minimumGroupSize] = true;
    channels.createPendingCommunicators();
    const auto secondGroup = channels.getGridDataExchangeMetaInformation(
      peano4::parallel::TreeIdMapping::getGlobalTreeId(minimumGroupSize),
      peano4::parallel::TreeIdMapping::getGlobalTreeId(0),
      peano4::parallel::ExchangeChannels::ExchangeMode::HorizontalData
    );
    validate(secondGroup.second != MPI_COMM_NULL);
    int groupComparison = MPI_UNEQUAL;
    validateEquals(MPI_Comm_compare(firstGroup.second, secondGroup.second, &groupComparison), MPI_SUCCESS);
    validateEquals(groupComparison, MPI_CONGRUENT);
    validate(not channels._needCommunicator[minimumGroupSize]);
  }

  channels._sendingTreesPerDataExchangeCommunicator = configuredGroupSize;
}

void peano4::parallel::tests::ExchangeChannelsTest::run() {
  testMethod(testTagCalculation);
  testMethod(testDataExchangeCommunicatorCreation);
}
