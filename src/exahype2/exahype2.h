// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/accelerator/accelerator.h"
#include "tarch/la/Vector.h"

#include <bitset>
#include <string>

namespace exahype2 {
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
    inline double domainOffset[3]               = {0.0, 0.0, 0.0};
    inline double domainSize[3]                 = {1.0, 1.0, 1.0};
    inline double minTerminalTime               = 0.0;
    inline double maxTerminalTime               = 0.0;
    inline double firstPlotTimeStamp            = 0.0;
    inline double timeInBetweenPlots            = 0.0;
    inline int    periodicBoundaryConditions[3] = {0, 0, 0};
    TARGET_DECLARE_END

    extern std::string outputPath;
  } // namespace internal

  void                 setDefaultLogStatements();
  std::string          extractConfigFileFromCommandLine(int& argc, char** argv);
  RuntimeConfiguration loadRuntimeConfiguration(
    const std::string&          configFilePath,
    const RuntimeConfiguration& fallbackConfiguration
  );

  INLINE_METHOD GPU_CALLABLE_METHOD tarch::la::Vector<DIMENSIONS, double> getDomainOffset() {
    tarch::la::Vector<DIMENSIONS, double> result;
    for (int d = 0; d < DIMENSIONS; ++d) {
      result[d] = internal::domainOffset[d];
    }
    return result;
  }

  INLINE_METHOD GPU_CALLABLE_METHOD tarch::la::Vector<DIMENSIONS, double> getDomainSize() {
    tarch::la::Vector<DIMENSIONS, double> result;
    for (int d = 0; d < DIMENSIONS; ++d) {
      result[d] = internal::domainSize[d];
    }
    return result;
  }

  INLINE_METHOD GPU_CALLABLE_METHOD double getMinTerminalTime() { return internal::minTerminalTime; }

  INLINE_METHOD GPU_CALLABLE_METHOD double getMaxTerminalTime() { return internal::maxTerminalTime; }

  INLINE_METHOD GPU_CALLABLE_METHOD double getFirstPlotTimeStamp() { return internal::firstPlotTimeStamp; }

  INLINE_METHOD GPU_CALLABLE_METHOD double getTimeInBetweenPlots() { return internal::timeInBetweenPlots; }

  INLINE_METHOD const std::string& getPlotOutputPath() { return internal::outputPath; }

  INLINE_METHOD GPU_CALLABLE_METHOD bool isPeriodicBoundaryCondition(int dimension) {
    return internal::periodicBoundaryConditions[dimension] != 0;
  }

  INLINE_METHOD std::bitset<DIMENSIONS> getPeriodicBC() {
    std::bitset<DIMENSIONS> result;
    for (int d = 0; d < DIMENSIONS; ++d) {
      result[d] = internal::periodicBoundaryConditions[d] != 0;
    }
    return result;
  }
} // namespace exahype2
