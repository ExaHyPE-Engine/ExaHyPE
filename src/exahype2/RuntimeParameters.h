// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/la/Vector.h"

#include <string>

namespace exahype2 {
  class RuntimeParameters;
} // namespace exahype2

class exahype2::RuntimeParameters {
private:
  std::string _configFilePath;
  double      _minTerminalTime;
  double      _maxTerminalTime;
  double      _firstPlotTimeStamp;
  int         _numberOfSnapshots;
  std::string _plotOutputPath;
  bool        _plotOutputPathConfigured;
  double      _logIntervalInSeconds;
  int         _gridLevels;
  int         _maxGridLevels;
  bool        _gridLevelsConfigured;
  bool        _maxGridLevelsConfigured;
  double      _loadBalancingQuality;
  int         _minSizeOfTree;
  int         _maxNumberOfTreesPerRank;
  int         _targetDevice;

  RuntimeParameters();

  static std::string formatDefault(double value);
  static std::string formatDefault(int value);
  static std::string formatDefault(const std::string& value);
  static std::string formatTerminalTime(double value);
  static double      divideByThreePower(double value, int exponent);
  static int         parseInteger(const std::string& value, const std::string& key);
  static double      parseDouble(const std::string& value, const std::string& key);
  static std::string normaliseOutputPath(const std::string& path);
  static std::string consumeValue(int& index, int argc, char** argv, const std::string& argument);
  void               normaliseGridLevels();
  void               normalisePlotOutputPath();

  void extractConfigFileFromCommandLine(int argc, char** argv);
  void readJsonConfiguration();
  void applyCommandLineConfiguration(int& argc, char** argv);

public:
  RuntimeParameters(const RuntimeParameters&)            = delete;
  RuntimeParameters& operator=(const RuntimeParameters&) = delete;

  static RuntimeParameters& getInstance();

  void setGeneratedDefaults(double minTerminalTime, double maxTerminalTime, int gridLevels, int maxGridLevels);
  void printUsage(const std::string& executableName = "ExaHyPE") const;
  void configure(int& argc, char** argv);

  const std::string& getConfigFilePath() const;
  double             getMinTerminalTime() const;
  double             getMaxTerminalTime() const;
  double             getFirstPlotTimeStamp() const;
  int                getNumberOfSnapshots() const;
  const std::string& getPlotOutputPath() const;
  double             getLogIntervalInSeconds() const;
  int                getGridLevels() const;
  double             getMaxAdmissiblePatchSize(const tarch::la::Vector<DIMENSIONS, double>& domainSize) const;
  double             getMinAdmissiblePatchSize(const tarch::la::Vector<DIMENSIONS, double>& domainSize) const;
  double             getMaxAdmissibleCellSize(const tarch::la::Vector<DIMENSIONS, double>& domainSize,
                                              int                                          cellsPerPatchPerAxis) const;
  double             getMinAdmissibleCellSize(const tarch::la::Vector<DIMENSIONS, double>& domainSize,
                                              int                                          cellsPerPatchPerAxis) const;
  double             getLoadBalancingQuality() const;
  int                getMinSizeOfTree() const;
  int                getMaxNumberOfTreesPerRank() const;
  int                getTargetDevice() const;
};
