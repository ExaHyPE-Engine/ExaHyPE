// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/instrumentation/Measurement.h"

namespace tarch::reader {
  class JSONReader;
}

namespace runner {
  struct SolverStatistic;
  struct RepositoryCallbacks;
  struct MainMeasurements;
} // namespace runner

struct runner::SolverStatistic {
  enum class Kind { Value, Count };

  std::string label;
  Kind        kind    = Kind::Value;
  double      minimum = 0.0;
  double      maximum = 0.0;
  long        count   = 0;
};

namespace runner {
  using SolverStatistics     = std::vector<std::pair<std::string, std::vector<SolverStatistic>>>;
  using ConfigurationEntries = std::vector<std::pair<std::string, std::string>>;
  using ConfigurationSummary = std::vector<std::pair<std::string, ConfigurationEntries>>;
} // namespace runner

struct runner::RepositoryCallbacks {
  std::function<void()> shutdownDataRepository;
  std::function<void()> afterStep;

  std::function<std::size_t()> getGlobalCellDataMemoryUsageInBytes;
  std::function<std::size_t()> getGlobalFaceDataMemoryUsageInBytes;
  std::function<std::size_t()> getGlobalVertexDataMemoryUsageInBytes;

  std::function<std::string()> getCopyrightBanner;

  std::function<void()>                                         printApplicationHelp;
  std::function<void(tarch::reader::JSONReader*, int&, char**)> loadApplicationParameters;

  std::function<void()> startSimulation;
  std::function<void()> finishSimulation;

  std::function<bool()>             mayPlot;
  std::function<bool()>             hasPendingMeshUpdates;
  std::function<double()>           getMinTimeStamp;
  std::function<double()>           getMaxTimeStamp;
  std::function<int()>              getNumberOfDoFs;
  std::function<SolverStatistics()> getSolverStatistics;

  std::function<ConfigurationSummary()> getConfigurationSummary;
};

struct runner::MainMeasurements {
  tarch::instrumentation::Measurement                                      timeStep;
  tarch::instrumentation::Measurement                                      plot;
  tarch::instrumentation::Measurement                                      dynamicAMR;
  std::vector<std::pair<std::string, tarch::instrumentation::Measurement>> stepTimes;
};
