// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include <cstdint>

namespace tarch::timing {
  class TPSCalculator;
} // namespace tarch::timing

/**
 * TPSCalculator
 *
 * This class provides a calculator for "Ticks-Per-Seconds" or
 * "Timesteps-Per-Seconds" (TPS) to give a better insight into performance.
 * `TPS` is the number of steps executed divided by the elapsed
 * walltime in seconds.
 *
 * It does not give any insights about hardware utilisation.
 *
 */
class tarch::timing::TPSCalculator {
public:
  TPSCalculator();
  ~TPSCalculator() = default;

  void update(double currentWalltime);

  //! Get the average TPS
  double getTPS() const;

private:
  double        _currentTPS;      //!< Store the current recorded TPS
  std::uint64_t _startTimeStep;   //!< Store the initial time step (helpful for checkpointing)
  std::uint64_t _currentTimeStep; //!< Store the current time step
};
