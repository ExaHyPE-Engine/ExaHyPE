// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/instrumentation/Measurement.h"

namespace tarch::reader {
  class JSONReader;
} // namespace tarch::reader

namespace runner {
  struct SolverStatistic {
    enum class Kind { Value, Count };
    std::string label;
    Kind        kind    = Kind::Value;
    double      minimum = 0.0;
    double      maximum = 0.0;
    long        count   = 0;
  };

  struct RepositoryCallbacks {
    std::function<void()> initDataRepository;
    std::function<void()> shutdownDataRepository;

    std::function<void()> afterStep;

    std::function<std::size_t()> getGlobalCellDataMemoryUsageInBytes;
    std::function<std::size_t()> getGlobalFaceDataMemoryUsageInBytes;
    std::function<std::size_t()> getGlobalVertexDataMemoryUsageInBytes;

    std::function<std::string()> getConfigurationInformation;
    std::function<std::string()> getCopyrightBanner;

    std::function<void()>                                         printApplicationHelp;
    std::function<void(tarch::reader::JSONReader*, int&, char**)> loadApplicationParameters;

    std::function<void()> startSimulation;
    std::function<void()> finishSimulation;

    std::function<bool()>                                                              mayPlot;
    std::function<double()>                                                            getMinTimeStamp;
    std::function<double()>                                                            getMaxTimeStamp;
    std::function<int()>                                                               getNumberOfDoFs;
    std::function<std::vector<std::pair<std::string, std::vector<SolverStatistic>>>()> getSolverStatistics;

    std::function<std::vector<std::pair<std::string, std::vector<std::pair<std::string, std::string>>>>()>
      getConfigurationSummary;
  };

  struct MainMeasurements {
    tarch::instrumentation::Measurement                                      timeStep;
    tarch::instrumentation::Measurement                                      plot;
    std::vector<std::pair<std::string, tarch::instrumentation::Measurement>> stepTimes;
  };
} // namespace runner
