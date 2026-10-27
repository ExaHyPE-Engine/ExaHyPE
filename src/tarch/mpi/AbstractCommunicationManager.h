// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

namespace tarch::mpi {
  class AbstractCommunicationManager;
} // namespace tarch::mpi

/**
 * Runs the pollers that complete the outstanding communication of a rank. A blocking MPI call is
 * no task scheduling point. Work that a remote rank waits for registers a poller here.
 */
class tarch::mpi::AbstractCommunicationManager {
public:
  using Poller = std::function<bool()>;

  AbstractCommunicationManager(const AbstractCommunicationManager&)            = delete;
  AbstractCommunicationManager(AbstractCommunicationManager&&)                 = delete;
  AbstractCommunicationManager& operator=(const AbstractCommunicationManager&) = delete;
  AbstractCommunicationManager& operator=(AbstractCommunicationManager&&)      = delete;
  virtual ~AbstractCommunicationManager()                                      = default;

  static tarch::mpi::AbstractCommunicationManager& getInstance();

  static void configure(const bool communicationThreadEnabled);
  static bool usesDedicatedThread();

  void registerPoller(Poller poller);

  virtual void start()             = 0;
  virtual void stop()              = 0;
  virtual void progress()          = 0;
  virtual void finishPendingWork() = 0;

protected:
  AbstractCommunicationManager() = default;

  bool runPollersOnce();

private:
  static bool _dedicatedThreadIsUsed;

  std::vector<Poller> _pollers{};
};
