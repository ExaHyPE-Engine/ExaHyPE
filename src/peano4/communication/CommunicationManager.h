// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include <mpi.h>

namespace peano4::communication {
  class CommunicationManager;
} // namespace peano4::communication

/**
 * Owns the exchange communicator of the rank-boundary and migration messages, hands out the
 * message tags on it and drives the transfers a phase posts. init() creates the communicator
 * and is collective.
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
   * Holds the poll of one posted transfer until the poll reports completion.
   */
  struct InFlightTransfer final {
    std::function<bool()> poll{};
    InFlightTransfer*     next{nullptr};
  };

  static peano4::communication::CommunicationManager _singleton;

  /**
   * Carries every message of the exchanges.
   */
  MPI_Comm _exchangeCommunicator{MPI_COMM_NULL};

  /**
   * Largest tag every rank accepts on the exchange communicator.
   */
  int _tagUpperBound{0};

  int _nextTag{0};

  /**
   * Collects freshly registered transfers pushed lock-free by the posting threads. The poller
   * is the single consumer and takes the whole list into its own polling set, so no lock sits
   * on the polling loop.
   */
  std::atomic<InFlightTransfer*> _newInFlightTransfers{nullptr};

  /**
   * Belongs exclusively to the polling thread, so its accesses need no synchronisation.
   */
  std::vector<std::unique_ptr<InFlightTransfer>> _transfersBeingPolled{};
};
