// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/accelerator/CompileTarget.h"

namespace tarch::accelerator {
  class Device;

#if not defined(PEANO_WITH_GPU_BACKEND)
  // One host thread runs a whole team. The scratch is a buffer of TeamScratchBytes of that thread.
  struct CpuTeam {
    unsigned char* scratch{nullptr};
  };
  using TeamHandle = CpuTeam;

  inline constexpr int ThreadsPerWarp        = 1;
  inline constexpr int DefaultThreadsPerTeam = 1;
  inline constexpr int TeamScratchBytes      = 16384;
#endif
} // namespace tarch::accelerator

/**
 * Singleton that every accelerator backend provides. A caller addresses the GPUs of a rank with
 * the logical numbers 0 to getNumberOfDevices() - 1. A build without a GPU backend provides a
 * Device without GPUs.
 *
 * @author Tobias Weinzierl
 */
class tarch::accelerator::Device final {
public:
  TARGET_DECLARE
  static constexpr int DefaultDevice = 0;
  TARGET_DECLARE_END

  Device(const Device& other)            = delete;
  Device(Device&& other)                 = delete;
  Device& operator=(const Device& other) = delete;
  Device& operator=(Device&& other)      = delete;

  static Device& getInstance();

  bool init();
  void shutdown();
  [[nodiscard]] bool isInitialised() const;

  [[nodiscard]] int getNumberOfDevices() const;
  [[nodiscard]] int getNumberOfConcurrentThreads() const;
  [[nodiscard]] std::optional<bool> isMPIGPUAware() const;
  [[nodiscard]] std::string getIdOfCurrentDevice() const;

private:
  Device()  = default;
  ~Device() = default;
};
