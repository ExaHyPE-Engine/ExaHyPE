// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

namespace tarch::mpi {
  class HelperThread;
} // namespace tarch::mpi

/**
 * Runs one polling function on a thread of its own. The MPI progress runs on this thread, because a
 * deferred task can wait in the queue while another thread blocks inside MPI.
 */
class tarch::mpi::HelperThread final {
public:
  explicit HelperThread(std::function<bool()> pollFunction);
  HelperThread(const HelperThread&)            = delete;
  HelperThread(HelperThread&&)                 = delete;
  HelperThread& operator=(const HelperThread&) = delete;
  HelperThread& operator=(HelperThread&&)      = delete;
  ~HelperThread();

  void start();
  void stop();
  bool hasFinished() const;

private:
  const std::function<bool()> _pollFunction;
  std::atomic<bool>           _hasFinished{true};
  std::atomic<bool>           _stopRequested{false};
  std::thread                 _thread{};
};
