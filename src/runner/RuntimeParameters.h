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

  [[nodiscard]] tarch::reader::JSONReader* getConfigReader() const;

  [[nodiscard]] double getMPITimeoutWarning() const;
  [[nodiscard]] double getMPIDeadlockTimeout() const;
  [[nodiscard]] bool   isMPIRequestTrackingEnabled() const;
  [[nodiscard]] bool   isCommunicationThreadEnabled() const;
  [[nodiscard]] bool   isLoadBalancingDisabled() const;
  [[nodiscard]] int    getTargetDevice() const;

  [[nodiscard]] bool   isVerbose() const;
  [[nodiscard]] double getTimeStepLogInterval() const;

  [[nodiscard]] double getMinTerminalTime() const;
  [[nodiscard]] double getMaxTerminalTime() const;
  [[nodiscard]] int    getNumberOfTimeSteps() const;
  [[nodiscard]] double getFirstPlotTimeStamp() const;
  [[nodiscard]] int    getNumberOfSnapshots() const;
  [[nodiscard]] bool   getPlotDoublePrecision() const;
  [[nodiscard]] int    getGridLevels() const;
  [[nodiscard]] int    getMaxGridLevels() const;

  [[nodiscard]] const std::string& getConfigFilePath() const;
  [[nodiscard]] const std::string& getOutputPath() const;
  [[nodiscard]] const std::string& getProjectName() const;
  [[nodiscard]] const std::string& getExecutableName() const;

  static std::string formatDefault(double value);
  static std::string formatDefault(int value);
  static std::string formatDefault(const std::string& value);
  static std::string formatTerminalTime(double value);
  static double      divideBySubdivisionFactorPower(double value, int exponent);
  static int         parseInteger(const std::string& value, const std::string& key);
  static double      parseDouble(const std::string& value, const std::string& key);
  static std::string normaliseOutputPath(const std::string& path);
  static std::string consumeValue(int& index, int argc, char** argv, const std::string& argument);

  [[nodiscard]] double getMaxAdmissiblePatchSize(const tarch::la::Vector<DIMENSIONS, double>& domainSize) const;
  [[nodiscard]] double getMinAdmissiblePatchSize(const tarch::la::Vector<DIMENSIONS, double>& domainSize) const;
  [[nodiscard]] double getMaxAdmissibleCellSize(
    const tarch::la::Vector<DIMENSIONS, double>& domainSize,
    int                                          cellsPerPatchPerAxis
  ) const;

private:
  friend class runner::tests::RuntimeParametersTest;

  /**
   * Seconds one MPI wait may take before a warning goes out and before the program aborts. The
   * option --deadlockTimeout sets or disables the abort bound.
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
  double      _timeStepLogInterval{5.0};
  int         _gridLevels{3};
  int         _maxGridLevels{3};
  bool        _gridLevelsConfigured{false};
  bool        _maxGridLevelsConfigured{false};
  bool        _communicationThread{false};
  bool        _disableLoadBalancing{false};

  std::unique_ptr<tarch::reader::JSONReader> _configReader{};

  // The out-of-line constructor permits JSONReader to remain forward-declared.
  RuntimeParameters();

  void normaliseGridLevels();
  void boundTerminalTimes();
  void validateParameterRanges() const;

  void extractConfigFileFromCommandLine(int argc, char** argv);
  void readJsonConfiguration();
  void applyCommandLineConfiguration(int& argc, char** argv);
};
