// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/la/Vector.h"
#include "tarch/plotter/Format.h"

namespace tarch::reader {
  class JSONReader;
} // namespace tarch::reader

namespace exahype2::runner {
  class ParameterTable;
  class RuntimeParameters;
} // namespace exahype2::runner

/**
 * Runtime parameters of an application, stored in variables the caller owns. load() reads the
 * configuration file first and the command line second. It removes the consumed arguments from
 * argv.
 */
class exahype2::runner::ParameterTable final {
public:
  using Values = std::variant<double*, int*, bool*, std::string*>;

  ParameterTable()                                 = default;
  ParameterTable(const ParameterTable&)            = default;
  ParameterTable(ParameterTable&&)                 = default;
  ParameterTable& operator=(const ParameterTable&) = default;
  ParameterTable& operator=(ParameterTable&&)      = default;
  ~ParameterTable()                                = default;

  void add(const std::string& name, Values values);
  void addArray(const std::string& name, Values values, int size);

  void load(const tarch::reader::JSONReader* reader) const;
  void load(const tarch::reader::JSONReader* reader, int& argc, char** argv) const;

  [[nodiscard]] std::string getUsage() const;
  void printUsage() const;

private:
  struct Entry final {
    std::string name{};
    Values      values{};
    int         size{1};
    bool        isArray{false};
    std::string defaultText{};
  };

  std::vector<Entry> _entries{};
};

/**
 * Runtime parameters of the engine, read from a JSON file and the command line.
 */
class exahype2::runner::RuntimeParameters final {
public:
  RuntimeParameters(const RuntimeParameters&)            = delete;
  RuntimeParameters(RuntimeParameters&&)                 = delete;
  RuntimeParameters& operator=(const RuntimeParameters&) = delete;
  RuntimeParameters& operator=(RuntimeParameters&&)      = delete;
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
  [[nodiscard]] bool isMPIRequestTrackingEnabled() const;
  [[nodiscard]] bool isCommunicationThreadEnabled() const;
  [[nodiscard]] bool isLoadBalancingDisabled() const;
  [[nodiscard]] bool isVerbose() const;
  [[nodiscard]] double getTimeStepLogInterval() const;
  [[nodiscard]] double getMinTerminalTime() const;
  [[nodiscard]] double getMaxTerminalTime() const;
  [[nodiscard]] double getSolverTerminalTime() const;
  [[nodiscard]] int getNumberOfTimeSteps() const;
  [[nodiscard]] double getFirstPlotTimeStamp() const;
  [[nodiscard]] int getNumberOfSnapshots() const;
  [[nodiscard]] bool getPlotDoublePrecision() const;
  [[nodiscard]] tarch::plotter::Format getPlotFormat() const;
  [[nodiscard]] int getGridLevels() const;
  [[nodiscard]] int getMaxGridLevels() const;
  [[nodiscard]] const std::string& getConfigFilePath() const;
  [[nodiscard]] const std::string& getOutputPath() const;
  [[nodiscard]] const std::string& getProjectName() const;
  [[nodiscard]] const std::string& getExecutableName() const;

  static std::string formatTerminalTime(double value);

  [[nodiscard]] double getMaxAdmissiblePatchSize(const tarch::la::Vector<DIMENSIONS, double>& domainSize) const;
  [[nodiscard]] double getMinAdmissiblePatchSize(const tarch::la::Vector<DIMENSIONS, double>& domainSize) const;

private:
  static std::string normaliseOutputPath(const std::string& path);

  double _MPITimeoutWarning{120.0};
  double _MPIDeadlockTimeout{600.0};
  bool   _MPITimeoutConfigured{false};

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
  std::string _plotFormat{tarch::plotter::toString(tarch::plotter::getDefaultFormat())};
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

  RuntimeParameters();

  void normaliseGridLevels();
  void boundTerminalTimes();
  void validateParameterRanges() const;
  void extractConfigFileFromCommandLine(int argc, char** argv);
  void readJSONConfiguration();
  void applyCommandLineConfiguration(int& argc, char** argv);
};
