// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "PersistentChannelTest.h"

#include "tarch/mpi/PersistentChannel.h"
#include "tarch/mpi/Rank.h"
#include "tarch/mpi/Request.h"

namespace {
  /** Every test sends to its own rank, so the case runs on any rank count. */
  int channelTestTag() {
    static int tag = tarch::mpi::Rank::reserveFreeTag("tarch::mpi::tests::PersistentChannelTest");
    return tag;
  }

  void completeBoth(tarch::mpi::PersistentChannel& sendChannel, tarch::mpi::PersistentChannel& receiveChannel) {
    tarch::mpi::wait(sendChannel.getRequest());
    tarch::mpi::wait(receiveChannel.getRequest());
  }
} // namespace

tarch::mpi::tests::PersistentChannelTest::PersistentChannelTest() :
  TestCase("tarch::mpi::tests::PersistentChannelTest") {}

void tarch::mpi::tests::PersistentChannelTest::run() {
  testMethod(testRoundTripRepeats);
  testMethod(testTransferChangeRebinds);
  testMethod(testMoveTransfersOwnership);
}

/** An unchanged transfer restarts round after round and still moves the current buffer content. */
void tarch::mpi::tests::PersistentChannelTest::testRoundTripRepeats() {
  const int  rank         = tarch::mpi::Rank::getInstance().getRank();
  const auto communicator = tarch::mpi::Rank::getInstance().getCommunicator();

  std::array<int, 4>            outgoing{};
  std::array<int, 4>            incoming{};
  tarch::mpi::PersistentChannel sendChannel;
  tarch::mpi::PersistentChannel receiveChannel;

  for (int round = 0; round < 50; round++) {
    for (std::size_t i = 0; i < outgoing.size(); i++) {
      outgoing[i] = round * 10 + static_cast<int>(i);
    }
    receiveChannel
      .startReceive(incoming.data(), static_cast<int>(sizeof(incoming)), rank, channelTestTag(), communicator);
    sendChannel.startSend(outgoing.data(), static_cast<int>(sizeof(outgoing)), rank, channelTestTag(), communicator);
    completeBoth(sendChannel, receiveChannel);

    for (std::size_t i = 0; i < incoming.size(); i++) {
      validateEqualsWithParams2(incoming[i], outgoing[i], round, i);
    }
  }
}

/** A changed buffer or count re-creates the request transparently. */
void tarch::mpi::tests::PersistentChannelTest::testTransferChangeRebinds() {
  const int  rank         = tarch::mpi::Rank::getInstance().getRank();
  const auto communicator = tarch::mpi::Rank::getInstance().getCommunicator();

  tarch::mpi::PersistentChannel sendChannel;
  tarch::mpi::PersistentChannel receiveChannel;

  std::array<int, 2> smallOutgoing{11, 22};
  std::array<int, 2> smallIncoming{};
  receiveChannel
    .startReceive(smallIncoming.data(), static_cast<int>(sizeof(smallIncoming)), rank, channelTestTag(), communicator);
  sendChannel
    .startSend(smallOutgoing.data(), static_cast<int>(sizeof(smallOutgoing)), rank, channelTestTag(), communicator);
  completeBoth(sendChannel, receiveChannel);
  validateEquals(smallIncoming[1], 22);

  std::array<int, 6> largeOutgoing{1, 2, 3, 4, 5, 6};
  std::array<int, 6> largeIncoming{};
  receiveChannel
    .startReceive(largeIncoming.data(), static_cast<int>(sizeof(largeIncoming)), rank, channelTestTag(), communicator);
  sendChannel
    .startSend(largeOutgoing.data(), static_cast<int>(sizeof(largeOutgoing)), rank, channelTestTag(), communicator);
  completeBoth(sendChannel, receiveChannel);
  for (std::size_t i = 0; i < largeIncoming.size(); i++) {
    validateEqualsWithParams1(largeIncoming[i], largeOutgoing[i], i);
  }
}

/** A moved channel keeps its bound transfer, and the source is left safely empty. */
void tarch::mpi::tests::PersistentChannelTest::testMoveTransfersOwnership() {
  const int  rank         = tarch::mpi::Rank::getInstance().getRank();
  const auto communicator = tarch::mpi::Rank::getInstance().getCommunicator();

  std::array<int, 3>            outgoing{7, 8, 9};
  std::array<int, 3>            incoming{};
  tarch::mpi::PersistentChannel originalSendChannel;
  tarch::mpi::PersistentChannel receiveChannel;

  receiveChannel
    .startReceive(incoming.data(), static_cast<int>(sizeof(incoming)), rank, channelTestTag(), communicator);
  originalSendChannel
    .startSend(outgoing.data(), static_cast<int>(sizeof(outgoing)), rank, channelTestTag(), communicator);
  completeBoth(originalSendChannel, receiveChannel);

  tarch::mpi::PersistentChannel movedSendChannel(std::move(originalSendChannel));
  outgoing = {70, 80, 90};
  receiveChannel
    .startReceive(incoming.data(), static_cast<int>(sizeof(incoming)), rank, channelTestTag(), communicator);
  movedSendChannel.startSend(outgoing.data(), static_cast<int>(sizeof(outgoing)), rank, channelTestTag(), communicator);
  completeBoth(movedSendChannel, receiveChannel);
  for (std::size_t i = 0; i < incoming.size(); i++) {
    validateEqualsWithParams1(incoming[i], outgoing[i], i);
  }
}
