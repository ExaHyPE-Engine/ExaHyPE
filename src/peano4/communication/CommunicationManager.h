// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include <mpi.h>

namespace peano4::communication {
  class CommunicationManager;
} // namespace peano4::communication

/**
 * Owns the communicator of the halo and migration messages, reserves their tags and polls the
 * posted transfers. Every rank calls init() at the same time.
 */
class peano4::communication::CommunicationManager final {
public:
  CommunicationManager(const CommunicationManager&)            = delete;
  CommunicationManager(CommunicationManager&&)                 = delete;
  CommunicationManager& operator=(const CommunicationManager&) = delete;
  CommunicationManager& operator=(CommunicationManager&&)      = delete;
  ~CommunicationManager()                                      = default;

  static peano4::communication::CommunicationManager& getInstance();

  void init();

  void shutdown();

  [[nodiscard]] MPI_Comm getCommunicator() const;

  [[nodiscard]] int reserveTag();

  void registerInFlightTransfer(std::function<bool()> pollTransfer);

  bool pollInFlightTransfers();

private:
  CommunicationManager() = default;

  /**
   * The poll function of one posted transfer, kept until it reports completion.
   */
  struct InFlightTransfer final {
    std::function<bool()> poll{};
    InFlightTransfer*     next{nullptr};
  };

  static peano4::communication::CommunicationManager _singleton;

  MPI_Comm _exchangeCommunicator{MPI_COMM_NULL};

  /**
   * Largest tag every rank accepts on the exchange communicator.
   */
  int _tagUpperBound{0};

  int _nextTag{0};

  /**
   * Lock-free stack of the transfers the posting threads register. The polling thread is its only
   * consumer and moves the whole stack into its polling set.
   */
  std::atomic<InFlightTransfer*> _newInFlightTransfers{nullptr};

  /**
   * Only the polling thread accesses this list, so it needs no synchronisation.
   */
  std::vector<std::unique_ptr<InFlightTransfer>> _transfersBeingPolled{};
};
