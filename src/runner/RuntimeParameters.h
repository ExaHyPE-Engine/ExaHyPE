// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/accelerator/Device.h"
#include "tarch/la/Vector.h"

namespace tarch::reader {
  class JSONReader;
} // namespace tarch::reader

namespace runner {
  class RuntimeParameters;
} // namespace runner

namespace runner::tests {
  class RuntimeParametersTest;
} // namespace runner::tests

class runner::RuntimeParameters final {
public:
  RuntimeParameters(const RuntimeParameters&)            = delete;
  RuntimeParameters(RuntimeParameters&&)                 = delete;
  RuntimeParameters& operator=(const RuntimeParameters&) = delete;
  RuntimeParameters& operator=(RuntimeParameters&&)      = delete;
  // The out-of-line destructor permits JSONReader to remain forward-declared.
  ~RuntimeParameters();

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
  bool   isCommunicationThreadEnabled() const;
  bool   isLoadBalancingDisabled() const;
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

  double getFirstCheckpointTimeStamp() const;
  int    getNumberOfCheckpoints() const;
  bool   hasCheckpointLoadPath() const;

  const std::string& getCheckpointOutputPath() const;
  const std::string& getCheckpointLoadPath() const;
  const std::string& getConfigFilePath() const;
  const std::string& getOutputPath() const;
  const std::string& getProjectName() const;
  const std::string& getExecutableName() const;

  static std::string formatDefault(double value);
  static std::string formatDefault(int value);
  static std::string formatDefault(const std::string& value);
  static std::string formatTerminalTime(double value);
  static double      divideByThreePower(double value, int exponent);
  static int         parseInteger(const std::string& value, const std::string& key);
  static double      parseDouble(const std::string& value, const std::string& key);
  static std::string normaliseOutputPath(const std::string& path);

  double getMaxAdmissiblePatchSize(const tarch::la::Vector<DIMENSIONS, double>& domainSize) const;
  double getMinAdmissiblePatchSize(const tarch::la::Vector<DIMENSIONS, double>& domainSize) const;
  double getMaxAdmissibleCellSize(
    const tarch::la::Vector<DIMENSIONS, double>& domainSize,
    int                                          cellsPerPatchPerAxis
  ) const;

private:
  friend class runner::tests::RuntimeParametersTest;

  /**
   * Seconds one MPI wait may take before the run warns and before it aborts. No rank makes progress
   * on another rank's behalf, so a mismatched send, receive or collective waits forever. The option
   * --deadlockTimeout raises or disables the abort bound.
   */
  double _mpiTimeoutWarning{120.0};
  double _mpiDeadlockTimeout{600.0};

  bool _mpiTimeoutConfigured{false};

  int  _targetDevice{tarch::accelerator::Device::DefaultDevice};
  bool _verbose{false};

  std::string _configFilePath{};
  double      _minTerminalTime{0.1};
  double      _maxTerminalTime{std::numeric_limits<double>::max()};
  int         _numberOfTimeSteps{-1};
  double      _firstPlotTimeStamp{0.0};
  int         _numberOfSnapshots{10};
  std::string _outputPath{normaliseOutputPath("output")};
  bool        _outputPathConfigured{false};
  bool        _plotDoublePrecision{false};
  std::string _projectName{};
  std::string _executableName{};
  double      _firstCheckpointTimeStamp{0.0};
  int         _numberOfCheckpoints{0};
  std::string _checkpointOutputPath{"checkpoints"};
  std::string _checkpointLoadPath{};
  double      _timeStepLogInterval{5.0};
  int         _gridLevels{3};
  int         _maxGridLevels{3};
  bool        _gridLevelsConfigured{false};
  bool        _maxGridLevelsConfigured{false};
  bool        _communicationThread{false};
  bool        _disableLoadBalancing{false};

  std::unique_ptr<tarch::reader::JSONReader> _configReader{};

  // The out-of-line constructor permits JSONReader to remain forward-declared:
  // A constructor defined here would need the type to destroy _configReader on
  // an exception.
  RuntimeParameters();

  static std::string consumeValue(int& index, int argc, char** argv, const std::string& argument);
  void               normaliseGridLevels();
  void               validateParameterRanges() const;

  void extractConfigFileFromCommandLine(int argc, char** argv);
  void readJsonConfiguration();
  void applyCommandLineConfiguration(int& argc, char** argv);
};
