// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/accelerator/accelerator.h"
#include "tarch/la/Vector.h"

#include <bitset>
#include <string>

namespace exahype2 {
  struct RuntimeParameters {
    double      domainOffset[3]    = {0.0, 0.0, 0.0};
    double      domainSize[3]      = {1.0, 1.0, 1.0};
    double      minTerminalTime    = 0.0;
    double      maxTerminalTime    = 0.0;
    double      firstPlotTimeStamp = 0.0;
    double      timeInBetweenPlots = 0.0;
    bool        periodicBC[3]      = {false, false, false};
    std::string plotOutputPath     = "./";
  };

  struct RuntimeConfiguration {
    tarch::la::Vector<DIMENSIONS, double> domainOffset;
    tarch::la::Vector<DIMENSIONS, double> domainSize;
    double                                minTerminalTime;
    double                                maxTerminalTime;
    double                                firstPlotTimeStamp;
    double                                timeInBetweenPlots;
    std::bitset<DIMENSIONS>               periodicBC;
    std::string                           plotOutputPath;
  };

  namespace internal {
    TARGET_DECLARE
    extern RuntimeParameters RuntimeParametersStorage;
    TARGET_DECLARE_END
  } // namespace internal

  void                 writeCopyrightMessage();
  void                 setDefaultLogStatements();
  std::string          extractConfigFileFromCommandLine(int& argc, char** argv);
  RuntimeConfiguration loadRuntimeConfiguration(
    const std::string&          configFilePath,
    const RuntimeConfiguration& fallbackConfiguration
  );

  INLINE_METHOD GPU_CALLABLE_METHOD tarch::la::Vector<DIMENSIONS, double> getDomainOffset() {
    tarch::la::Vector<DIMENSIONS, double> result;
    for (int d = 0; d < DIMENSIONS; ++d) {
      result[d] = internal::RuntimeParametersStorage.domainOffset[d];
    }
    return result;
  }

  INLINE_METHOD GPU_CALLABLE_METHOD tarch::la::Vector<DIMENSIONS, double> getDomainSize() {
    tarch::la::Vector<DIMENSIONS, double> result;
    for (int d = 0; d < DIMENSIONS; ++d) {
      result[d] = internal::RuntimeParametersStorage.domainSize[d];
    }
    return result;
  }

  INLINE_METHOD GPU_CALLABLE_METHOD double getMinTerminalTime() {
    return internal::RuntimeParametersStorage.minTerminalTime;
  }

  INLINE_METHOD GPU_CALLABLE_METHOD double getMaxTerminalTime() {
    return internal::RuntimeParametersStorage.maxTerminalTime;
  }

  INLINE_METHOD GPU_CALLABLE_METHOD double getFirstPlotTimeStamp() {
    return internal::RuntimeParametersStorage.firstPlotTimeStamp;
  }

  INLINE_METHOD GPU_CALLABLE_METHOD double getTimeInBetweenPlots() {
    return internal::RuntimeParametersStorage.timeInBetweenPlots;
  }

  INLINE_METHOD const std::string& getPlotOutputPath() { return internal::RuntimeParametersStorage.plotOutputPath; }

  INLINE_METHOD GPU_CALLABLE_METHOD bool isPeriodicBoundaryCondition(int dimension) {
    return internal::RuntimeParametersStorage.periodicBC[dimension];
  }

  INLINE_METHOD std::bitset<DIMENSIONS> getPeriodicBC() {
    std::bitset<DIMENSIONS> result;
    for (int d = 0; d < DIMENSIONS; ++d) {
      result[d] = internal::RuntimeParametersStorage.periodicBC[d];
    }
    return result;
  }
} // namespace exahype2
