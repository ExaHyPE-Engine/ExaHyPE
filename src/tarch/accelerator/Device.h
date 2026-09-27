// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/accelerator/CompileTarget.h"

namespace tarch::accelerator {
  class Device;

#if not defined(PEANO_WITH_GPU_BACKEND)
  struct CpuTeam {};
  using TeamHandle = CpuTeam;
#endif
} // namespace tarch::accelerator

/**
 * Singleton that every accelerator backend provides. A caller addresses the GPUs of a rank with
 * the logical numbers 0 to getNumberOfDevices() - 1. A build without a GPU backend provides a
 * Device without GPUs.
 *
 * @author Tobias Weinzierl
 */
class tarch::accelerator::Device {
private:
  Device() = default;

public:
  TARGET_DECLARE
  static constexpr int DefaultDevice = 0;
  TARGET_DECLARE_END

  ~Device() = default;

  static Device& getInstance();

  bool init();

  void shutdown();

  bool isInitialised() const;

  int getNumberOfDevices() const;
};
