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

  struct InFlightTransfer final {
    std::function<bool()> poll{};
    InFlightTransfer*     next{nullptr};
  };

  static peano4::communication::CommunicationManager _singleton;

  MPI_Comm _exchangeCommunicator{MPI_COMM_NULL};

  int _tagUpperBound{0};

  int _nextTag{0};

  std::atomic<InFlightTransfer*> _newInFlightTransfers{nullptr};

  std::vector<std::unique_ptr<InFlightTransfer>> _transfersBeingPolled{};
};
