// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

namespace tarch::tasking {
  class Runtime;

  int getNumberOfUnmaskedThreads();

  std::string printUnmaskedThreads();
} // namespace tarch::tasking

/**
 * One process-wide instance keeps the backend's thread-count decision
 * consistent across task producers.
 */
class tarch::tasking::Runtime final {
public:
  Runtime(const Runtime&)            = delete;
  Runtime(Runtime&&)                 = delete;
  Runtime& operator=(const Runtime&) = delete;
  Runtime& operator=(Runtime&&)      = delete;
  ~Runtime()                         = default;

  static tarch::tasking::Runtime& getInstance();

  bool init();
  void shutdown();
  bool isInitialised() const;
  int  getNumberOfThreads() const;
  int  getCoreNumber() const;
  int  getThreadNumber() const;

private:
  Runtime() = default;

  int _numberOfThreads{0};
};
