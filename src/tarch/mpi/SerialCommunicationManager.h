// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/mpi/AbstractCommunicationManager.h"

namespace tarch::mpi {
  class SerialCommunicationManager;
} // namespace tarch::mpi

/**
 * Runs the registered pollers inside progress(). A caller that waits for communication keeps
 * calling progress().
 */
class tarch::mpi::SerialCommunicationManager final : public tarch::mpi::AbstractCommunicationManager {
public:
  SerialCommunicationManager()                                             = default;
  SerialCommunicationManager(const SerialCommunicationManager&)            = delete;
  SerialCommunicationManager(SerialCommunicationManager&&)                 = delete;
  SerialCommunicationManager& operator=(const SerialCommunicationManager&) = delete;
  SerialCommunicationManager& operator=(SerialCommunicationManager&&)      = delete;
  ~SerialCommunicationManager() override                                   = default;

  void start() override;
  void stop() override;
  void progress() override;
  void finishPendingWork() override;
};
