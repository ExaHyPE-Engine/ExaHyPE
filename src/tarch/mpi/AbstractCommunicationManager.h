// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

namespace tarch::mpi {
  class AbstractCommunicationManager;
} // namespace tarch::mpi

/**
 * Owns the progress of a rank's outstanding communication.
 *
 * Deferred tasks carry no progress guarantee: A blocking MPI call is no task
 * scheduling point, so work that a remote rank waits for must never sit in a
 * task queue alone. Communication concerns therefore register a poller here,
 * and the manager keeps every registered poller running: The threaded
 * implementation drives them on a thread of its own, the serial one runs them
 * inline whenever progress() is invoked.
 */
class tarch::mpi::AbstractCommunicationManager {
public:
  /** One progress pass over a communication concern. Returns true when the concern has nothing outstanding. */
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
  /** Off until configure() has run, so an early reader never reserves a core. */
  static bool _dedicatedThreadIsUsed;

  std::vector<Poller> _pollers{};
};
