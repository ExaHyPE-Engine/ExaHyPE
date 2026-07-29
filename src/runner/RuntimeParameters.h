// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/la/Vector.h"

namespace tarch::reader {
  class JSONReader;
} // namespace tarch::reader

namespace runner {
  class RuntimeParameters;
} // namespace runner

class runner::RuntimeParameters final {
public:
  // The out-of-line destructor permits JSONReader to remain forward-declared.
  ~RuntimeParameters();
  RuntimeParameters(const RuntimeParameters&)            = delete;
  RuntimeParameters(RuntimeParameters&&)                 = delete;
  RuntimeParameters& operator=(const RuntimeParameters&) = delete;
  RuntimeParameters& operator=(RuntimeParameters&&)      = delete;

  static RuntimeParameters& getInstance();

  void setGeneratedDefaults(
    double             minTerminalTime,
    double             maxTerminalTime,
    int                gridLevels,
    int                maxGridLevels,
    const std::string& projectName
  );
  void configure(int& argc, char** argv);
  void printUsage(const std::string& executableName) const;

  tarch::reader::JSONReader* getConfigReader() const;

  double getMPITimeoutWarning() const;
  double getMPIDeadlockTimeout() const;
  bool   isMPIRequestTrackingEnabled() const;
  double getLoadBalancingQuality() const;
  int    getMaxNumberOfTreesPerRank() const;
  int    getTargetDevice() const;

  bool   isVerbose() const;
  double getTimeStepLogInterval() const;

  double getMinTerminalTime() const;
  double getMaxTerminalTime() const;
  int    getNumberOfTimeSteps() const;
  double getFirstPlotTimeStamp() const;
  int    getNumberOfSnapshots() const;
  bool   getPlotDoublePrecision() const;
  int    getGridLevels() const;
  int    getMaxGridLevels() const;

  const std::string& getConfigFilePath() const;
  const std::string& getOutputPath() const;
  const std::string& getProjectName() const;
  const std::string& getExecutableName() const;

  double getMaxAdmissiblePatchSize(const tarch::la::Vector<DIMENSIONS, double>& domainSize) const;
  double getMinAdmissiblePatchSize(const tarch::la::Vector<DIMENSIONS, double>& domainSize) const;
  double
  getMaxAdmissibleCellSize(const tarch::la::Vector<DIMENSIONS, double>& domainSize, int cellsPerPatchPerAxis) const;

private:
  double _mpiTimeoutWarning;
  double _mpiDeadlockTimeout;

  /**
   * Whether a run asked for an MPI timeout of its own rather than taking the default.
   */
  bool _mpiTimeoutConfigured;

  double _loadBalancingQuality;
  int    _maxNumberOfTreesPerRank;
  int    _targetDevice;
  bool   _verbose;

  std::string _configFilePath;
  double      _minTerminalTime;
  double      _maxTerminalTime;
  int         _numberOfTimeSteps;
  double      _firstPlotTimeStamp;
  int         _numberOfSnapshots;
  std::string _outputPath;
  bool        _outputPathConfigured;
  bool        _plotDoublePrecision;
  std::string _projectName;
  std::string _executableName;
  double      _timeStepLogInterval;
  int         _gridLevels;
  int         _maxGridLevels;
  bool        _gridLevelsConfigured;
  bool        _maxGridLevelsConfigured;

  std::unique_ptr<tarch::reader::JSONReader> _configReader;

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
  void               validateParameterRanges() const;

  void extractConfigFileFromCommandLine(int argc, char** argv);
  void readJsonConfiguration();
  void applyCommandLineConfiguration(int& argc, char** argv);
};
