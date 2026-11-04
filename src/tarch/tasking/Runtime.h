// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

namespace tarch::tasking {
  class Runtime;

  int getNumberOfUnmaskedThreads();
  bool oversubscribesNode();
  std::string printUnmaskedThreads();
} // namespace tarch::tasking

/**
 * Stores the thread count of the process. init() fixes it for every caller.
 */
class tarch::tasking::Runtime final {
public:
  Runtime(const Runtime& other)            = delete;
  Runtime(Runtime&& other)                 = delete;
  Runtime& operator=(const Runtime& other) = delete;
  Runtime& operator=(Runtime&& other)      = delete;
  ~Runtime()                               = default;

  static tarch::tasking::Runtime& getInstance();

  bool init();
  void shutdown();
  bool isInitialised() const;
  int getNumberOfThreads() const;
  int getCoreNumber() const;
  int getThreadNumber() const;

private:
  Runtime() = default;

  int _numberOfThreads{0};
};
