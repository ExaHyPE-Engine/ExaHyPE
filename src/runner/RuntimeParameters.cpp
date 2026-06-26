// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "RuntimeParameters.h"

#include "tarch/accelerator/Device.h"
#include "tarch/reader/JSONReader.h"

#include <filesystem>
#include <iostream>
#include <limits>
#include <sstream>
#include <stdexcept>

runner::RuntimeParameters::RuntimeParameters() :
  _mpiTimeOutWarning(0.0),
  _mpiDeadlockTimeOut(0.0),
  _mpiDeadlockDetectionEnabled(false),
  _loadBalancingQuality(0.75),
  _minSizeOfTree(1),
  _maxNumberOfTreesPerRank(-1),
  _targetDevice(tarch::accelerator::Device::DefaultDevice),
  _verbose(false),
  _configFilePath(),
  _minTerminalTime(0.1),
  _maxTerminalTime(std::numeric_limits<double>::max()),
  _numberOfTimeSteps(-1),
  _firstPlotTimeStamp(0.0),
  _numberOfSnapshots(10),
  _plotOutputPath(normaliseOutputPath("solutions")),
  _plotOutputPathConfigured(false),
  _logIntervalInSeconds(5.0),
  _gridLevels(3),
  _maxGridLevels(3),
  _gridLevelsConfigured(false),
  _maxGridLevelsConfigured(false) {}

runner::RuntimeParameters& runner::RuntimeParameters::getInstance() {
  static RuntimeParameters result;
  return result;
}

std::string runner::RuntimeParameters::formatDefault(double value) {
  std::ostringstream result;
  result << value;
  return result.str();
}

std::string runner::RuntimeParameters::formatDefault(int value) { return std::to_string(value); }

std::string runner::RuntimeParameters::formatDefault(const std::string& value) {
  return value.empty() ? "<none>" : value;
}

std::string runner::RuntimeParameters::formatTerminalTime(double value) {
  return value < std::numeric_limits<double>::max() / 10.0 ? formatDefault(value) : "unbounded";
}

double runner::RuntimeParameters::divideByThreePower(double value, int exponent) {
  if (exponent < 0) {
    throw std::runtime_error("Grid levels must not be negative.");
  }
  for (int level = 0; level < exponent; ++level) {
    value /= 3.0;
  }
  return value;
}

int runner::RuntimeParameters::parseInteger(const std::string& value, const std::string& key) {
  try {
    std::size_t processed = 0;
    const int   result    = std::stoi(value, &processed);
    if (processed != value.size()) {
      throw std::invalid_argument("Trailing characters");
    }
    return result;
  } catch (const std::exception&) {
    throw std::runtime_error("Invalid integer value for " + key + ": " + value);
  }
}

int runner::RuntimeParameters::parseNonNegativeInteger(const std::string& value, const std::string& key) {
  const int result = parseInteger(value, key);
  if (result < 0) {
    throw std::runtime_error("Invalid non-negative integer value for " + key + ": " + value);
  }
  return result;
}

double runner::RuntimeParameters::parseDouble(const std::string& value, const std::string& key) {
  try {
    std::size_t  processed = 0;
    const double result    = std::stod(value, &processed);
    if (processed != value.size()) {
      throw std::invalid_argument("Trailing characters");
    }
    return result;
  } catch (const std::exception&) {
    throw std::runtime_error("Invalid numeric value for " + key + ": " + value);
  }
}

double runner::RuntimeParameters::parseNonNegativeDouble(const std::string& value, const std::string& key) {
  const double result = parseDouble(value, key);
  if (result < 0.0) {
    throw std::runtime_error("Invalid non-negative numeric value for " + key + ": " + value);
  }
  return result;
}

std::string runner::RuntimeParameters::normaliseOutputPath(const std::string& path) {
  if (path.empty()) {
    return "./";
  }
  return path.back() == '/' ? path : path + "/";
}

std::string runner::RuntimeParameters::consumeValue(int& index, int argc, char** argv, const std::string& argument) {
  if (index + 1 >= argc) {
    throw std::runtime_error("Missing value for " + argument + ".");
  }
  ++index;
  return argv[index];
}

void runner::RuntimeParameters::printUsage(const std::string& executableName) const {
  const auto printOption = [](const std::string& argument, const std::string& description) {
    constexpr std::size_t OptionColumnWidth = 43;

    std::cout << "  " << argument;
    if (argument.size() < OptionColumnWidth) {
      std::cout << std::string(OptionColumnWidth - argument.size(), ' ');
    } else {
      std::cout << ' ';
    }
    std::cout << description << "\n";
  };

  std::cout << "Usage: " << executableName << " [options]\n"
            << "\n"
            << "Runtime configuration:\n";

  printOption("-cf, --config-file <path>", "Read runtime settings from JSON (default: auto-detect ./config.json).");
  printOption("-et, --end-time <t>", "Set min and max terminal time to t (default: not set).");
  printOption(
    "-tmin, --min-terminal-time <t>",
    "Minimum terminal time (default: " + formatDefault(_minTerminalTime) + ")."
  );
  printOption(
    "-tmax, --max-terminal-time <t>",
    "Maximum terminal time (default: " + formatTerminalTime(_maxTerminalTime) + ")."
  );
  printOption(
    "-nt, --number-of-timesteps <n>",
    "Run exactly n time steps then stop, superseding the terminal time (for profiling); use <=0 to "
    "disable (default: " +
      (_numberOfTimeSteps > 0 ? formatDefault(_numberOfTimeSteps) : std::string("disabled")) + ")."
  );
  printOption(
    "-fpt, --first-plot-time-stamp <t>",
    "First plot time stamp (default: " + formatDefault(_firstPlotTimeStamp) + ")."
  );
  printOption(
    "-ns, --number-of-snapshots <n>",
    "Number of plot snapshots; use <=0 to disable plotting (default: " + formatDefault(_numberOfSnapshots) + ")."
  );
  printOption("-o, --output-path <path>", "Plot output path (default: " + formatDefault(_plotOutputPath) + ").");
  printOption(
    "-li, --log-interval <seconds>",
    "Terminal log interval in seconds (default: " + formatDefault(_logIntervalInSeconds) + ")."
  );
  printOption(
    "-l, --grid-levels <n>",
    "Set coarsest and finest grid level to n (default: " + formatDefault(_gridLevels) + ")."
  );
  printOption("-lmin, --min-grid-levels <n>", "Coarsest grid level (default: " + formatDefault(_gridLevels) + ").");
  printOption("-lmax, --max-grid-levels <n>", "Finest grid level (default: " + formatDefault(_maxGridLevels) + ").");
  printOption(
    "-lbq, --load-balancing-quality <q>",
    "Worst-case load-balancing ratio (default: " + formatDefault(_loadBalancingQuality) + ")."
  );
  printOption("-tsmin, --min-size-of-tree <n>", "Minimum tree size (default: " + formatDefault(_minSizeOfTree) + ").");
  printOption(
    "-trees, --max-number-of-trees-per-rank <n>",
    "Maximum trees per rank. Use <=0 for thread count (default: " + formatDefault(_maxNumberOfTreesPerRank) + ")."
  );
  printOption(
    "-td, --target-device <n>",
    "Logical accelerator device id (default: " + formatDefault(_targetDevice) + ")."
  );
  printOption(
    "--mpi-timeout-warning <seconds>",
    "Warn after this many seconds waiting for MPI progress; fractional values such as 0.1 are "
    "allowed, 0 disables (default: " +
      formatDefault(_mpiTimeOutWarning) + ")."
  );
  printOption(
    "--mpi-deadlock-timeout <seconds>",
    "Terminate after this many seconds waiting for MPI progress; fractional values such as 0.1 are "
    "allowed, 0 disables (default: " +
      formatDefault(_mpiDeadlockTimeOut) + ")."
  );
  printOption(
    "--mpi-deadlock-detection",
    "Enable detailed MPI request tracking for deadlock diagnostics (default: off)."
  );
  printOption("-v, --verbose", "Show the full log line prefix (timestamp, core, scope) (default: off).");
  printOption("-h, --help", "Print this help and exit.");
}

void runner::RuntimeParameters::normaliseGridLevels() {
  if (_gridLevelsConfigured and not _maxGridLevelsConfigured) {
    _maxGridLevels = _gridLevels;
  } else if (_maxGridLevelsConfigured and not _gridLevelsConfigured) {
    _gridLevels = _maxGridLevels;
  }

  if (_gridLevels < 0) {
    throw std::runtime_error("Grid levels must not be negative.");
  }
  if (_maxGridLevels < 0) {
    throw std::runtime_error("Maximum grid levels must not be negative.");
  }
  if (_maxGridLevels < _gridLevels) {
    throw std::runtime_error("Maximum grid levels must be greater than or equal to the coarsest grid level.");
  }
}

void runner::RuntimeParameters::normalisePlotOutputPath() {
  if (_plotOutputPathConfigured) {
    return;
  }

  _plotOutputPath = _numberOfSnapshots <= 0 ? "./" : normaliseOutputPath("solutions");
}

void runner::RuntimeParameters::extractConfigFileFromCommandLine(int argc, char** argv) {
  for (int index = 1; index < argc; ++index) {
    const std::string argument = argv[index];
    if (argument == "--config-file" or argument == "-cf") {
      _configFilePath = consumeValue(index, argc, argv, argument);
    }
  }
}

void runner::RuntimeParameters::readJsonConfiguration() {
  if (_configFilePath.empty()) {
    return;
  }

  tarch::reader::JSONReader reader(_configFilePath);

  const auto readNonNegativeDouble = [&reader](const std::string& key) {
    const double result = reader.getDouble(key);
    if (result < 0.0) {
      throw std::runtime_error("Invalid non-negative numeric value for " + key + ": " + std::to_string(result));
    }
    return result;
  };

  // Framework parameters
  if (reader.hasKey("mpi-timeout-warning")) {
    _mpiTimeOutWarning = readNonNegativeDouble("mpi-timeout-warning");
  }
  if (reader.hasKey("mpi-deadlock-timeout")) {
    _mpiDeadlockTimeOut = readNonNegativeDouble("mpi-deadlock-timeout");
  }
  if (reader.hasKey("mpi-deadlock-detection")) {
    _mpiDeadlockDetectionEnabled = reader.getBoolean("mpi-deadlock-detection");
  }
  if (reader.hasKey("load-balancing-quality")) {
    _loadBalancingQuality = reader.getDouble("load-balancing-quality");
  }
  if (reader.hasKey("min-size-of-tree")) {
    _minSizeOfTree = reader.getInteger("min-size-of-tree");
  }
  if (reader.hasKey("max-number-of-trees-per-rank")) {
    _maxNumberOfTreesPerRank = reader.getInteger("max-number-of-trees-per-rank");
  }
  if (reader.hasKey("target-device")) {
    _targetDevice = reader.getInteger("target-device");
  }
  if (reader.hasKey("verbose")) {
    _verbose = reader.getBoolean("verbose");
  }

  // Solver/algorithm parameters
  if (reader.hasKey("end-time")) {
    _minTerminalTime = reader.getDouble("end-time");
    _maxTerminalTime = _minTerminalTime;
  }
  if (reader.hasKey("min-terminal-time")) {
    _minTerminalTime = reader.getDouble("min-terminal-time");
  }
  if (reader.hasKey("max-terminal-time")) {
    _maxTerminalTime = reader.getDouble("max-terminal-time");
  }
  if (reader.hasKey("number-of-timesteps")) {
    _numberOfTimeSteps = reader.getInteger("number-of-timesteps");
  }
  if (reader.hasKey("first-plot-time-stamp")) {
    _firstPlotTimeStamp = reader.getDouble("first-plot-time-stamp");
  }
  if (reader.hasKey("number-of-snapshots")) {
    _numberOfSnapshots = reader.getInteger("number-of-snapshots");
  }
  if (reader.hasKey("output-path")) {
    _plotOutputPath           = normaliseOutputPath(reader.getString("output-path"));
    _plotOutputPathConfigured = true;
  }
  if (reader.hasKey("log-interval")) {
    _logIntervalInSeconds = reader.getDouble("log-interval");
  }
  if (reader.hasKey("grid-levels")) {
    _gridLevels              = reader.getInteger("grid-levels");
    _maxGridLevels           = _gridLevels;
    _gridLevelsConfigured    = true;
    _maxGridLevelsConfigured = true;
  }
  if (reader.hasKey("min-grid-levels")) {
    _gridLevels           = reader.getInteger("min-grid-levels");
    _gridLevelsConfigured = true;
  }
  if (reader.hasKey("max-grid-levels")) {
    _maxGridLevels           = reader.getInteger("max-grid-levels");
    _maxGridLevelsConfigured = true;
  }
}

void runner::RuntimeParameters::applyCommandLineConfiguration(int& argc, char** argv) {
  int writeIndex = 1;

  for (int readIndex = 1; readIndex < argc; ++readIndex) {
    const std::string argument = argv[readIndex];
    if (argument == "--config-file" or argument == "-cf") {
      _configFilePath = consumeValue(readIndex, argc, argv, argument);
    } else if (argument == "--end-time" or argument == "-et") {
      _minTerminalTime = parseDouble(consumeValue(readIndex, argc, argv, argument), argument);
      _maxTerminalTime = _minTerminalTime;
    } else if (argument == "--min-terminal-time" or argument == "-tmin") {
      _minTerminalTime = parseDouble(consumeValue(readIndex, argc, argv, argument), argument);
    } else if (argument == "--max-terminal-time" or argument == "-tmax") {
      _maxTerminalTime = parseDouble(consumeValue(readIndex, argc, argv, argument), argument);
    } else if (argument == "--number-of-timesteps" or argument == "-nt") {
      _numberOfTimeSteps = parseInteger(consumeValue(readIndex, argc, argv, argument), argument);
    } else if (argument == "--first-plot-time-stamp" or argument == "-fpt") {
      _firstPlotTimeStamp = parseDouble(consumeValue(readIndex, argc, argv, argument), argument);
    } else if (argument == "--number-of-snapshots" or argument == "-ns") {
      _numberOfSnapshots = parseInteger(consumeValue(readIndex, argc, argv, argument), argument);
    } else if (argument == "--output-path" or argument == "-o") {
      _plotOutputPath           = normaliseOutputPath(consumeValue(readIndex, argc, argv, argument));
      _plotOutputPathConfigured = true;
    } else if (argument == "--log-interval" or argument == "-li") {
      _logIntervalInSeconds = parseDouble(consumeValue(readIndex, argc, argv, argument), argument);
    } else if (argument == "--grid-levels" or argument == "-l") {
      _gridLevels              = parseInteger(consumeValue(readIndex, argc, argv, argument), argument);
      _maxGridLevels           = _gridLevels;
      _gridLevelsConfigured    = true;
      _maxGridLevelsConfigured = true;
    } else if (argument == "--min-grid-levels" or argument == "-lmin") {
      _gridLevels           = parseInteger(consumeValue(readIndex, argc, argv, argument), argument);
      _gridLevelsConfigured = true;
    } else if (argument == "--max-grid-levels" or argument == "-lmax") {
      _maxGridLevels           = parseInteger(consumeValue(readIndex, argc, argv, argument), argument);
      _maxGridLevelsConfigured = true;
    } else if (argument == "--load-balancing-quality" or argument == "-lbq") {
      _loadBalancingQuality = parseDouble(consumeValue(readIndex, argc, argv, argument), argument);
    } else if (argument == "--min-size-of-tree" or argument == "-tsmin") {
      _minSizeOfTree = parseInteger(consumeValue(readIndex, argc, argv, argument), argument);
    } else if (argument == "--max-number-of-trees-per-rank" or argument == "-trees") {
      _maxNumberOfTreesPerRank = parseInteger(consumeValue(readIndex, argc, argv, argument), argument);
    } else if (argument == "--target-device" or argument == "-td") {
      _targetDevice = parseInteger(consumeValue(readIndex, argc, argv, argument), argument);
    } else if (argument == "--verbose" or argument == "-v") {
      _verbose = true;
    } else if (argument == "--mpi-timeout-warning") {
      _mpiTimeOutWarning = parseNonNegativeDouble(consumeValue(readIndex, argc, argv, argument), argument);
    } else if (argument == "--mpi-deadlock-timeout") {
      _mpiDeadlockTimeOut = parseNonNegativeDouble(consumeValue(readIndex, argc, argv, argument), argument);
    } else if (argument == "--mpi-deadlock-detection") {
      _mpiDeadlockDetectionEnabled = true;
    } else if (argument == "--no-mpi-deadlock-detection") {
      _mpiDeadlockDetectionEnabled = false;
    } else {
      argv[writeIndex] = argv[readIndex];
      ++writeIndex;
    }
  }

  argc       = writeIndex;
  argv[argc] = nullptr;
}

void runner::RuntimeParameters::setGeneratedDefaults(
  double minTerminalTime,
  double maxTerminalTime,
  int    gridLevels,
  int    maxGridLevels
) {
  if (minTerminalTime >= 0.0) {
    _minTerminalTime = minTerminalTime;
    _maxTerminalTime = maxTerminalTime >= minTerminalTime ? maxTerminalTime : std::numeric_limits<double>::max();
  }
  if (gridLevels >= 0) {
    _gridLevels           = gridLevels;
    _gridLevelsConfigured = true;
    if (maxGridLevels < 0) {
      _maxGridLevels           = gridLevels;
      _maxGridLevelsConfigured = true;
    }
  }
  if (maxGridLevels >= 0) {
    _maxGridLevels           = maxGridLevels;
    _maxGridLevelsConfigured = true;
    if (gridLevels < 0) {
      _gridLevels           = maxGridLevels;
      _gridLevelsConfigured = true;
    }
  }
}

void runner::RuntimeParameters::configure(int& argc, char** argv) {
  extractConfigFileFromCommandLine(argc, argv);

  if (_configFilePath.empty() and std::filesystem::exists("./config.json")) {
    _configFilePath = "./config.json";
  }

  readJsonConfiguration();
  applyCommandLineConfiguration(argc, argv);
  normalisePlotOutputPath();
  normaliseGridLevels();
}

const std::string& runner::RuntimeParameters::getConfigFilePath() const { return _configFilePath; }

double runner::RuntimeParameters::getMPITimeOutWarning() const { return _mpiTimeOutWarning; }
double runner::RuntimeParameters::getMPIDeadlockTimeOut() const { return _mpiDeadlockTimeOut; }
bool   runner::RuntimeParameters::isMPIDeadlockDetectionEnabled() const { return _mpiDeadlockDetectionEnabled; }
double runner::RuntimeParameters::getLoadBalancingQuality() const { return _loadBalancingQuality; }
int    runner::RuntimeParameters::getMinSizeOfTree() const { return _minSizeOfTree; }
int    runner::RuntimeParameters::getMaxNumberOfTreesPerRank() const { return _maxNumberOfTreesPerRank; }
int    runner::RuntimeParameters::getTargetDevice() const { return _targetDevice; }
bool   runner::RuntimeParameters::isVerbose() const { return _verbose; }

double             runner::RuntimeParameters::getMinTerminalTime() const { return _minTerminalTime; }
double             runner::RuntimeParameters::getMaxTerminalTime() const { return _maxTerminalTime; }
int                runner::RuntimeParameters::getNumberOfTimeSteps() const { return _numberOfTimeSteps; }
double             runner::RuntimeParameters::getFirstPlotTimeStamp() const { return _firstPlotTimeStamp; }
int                runner::RuntimeParameters::getNumberOfSnapshots() const { return _numberOfSnapshots; }
const std::string& runner::RuntimeParameters::getPlotOutputPath() const { return _plotOutputPath; }
double             runner::RuntimeParameters::getLogIntervalInSeconds() const { return _logIntervalInSeconds; }
int                runner::RuntimeParameters::getGridLevels() const { return _gridLevels; }
int                runner::RuntimeParameters::getMaxGridLevels() const { return _maxGridLevels; }

double
runner::RuntimeParameters::getMaxAdmissiblePatchSize(const tarch::la::Vector<DIMENSIONS, double>& domainSize) const {
  // A grid level maps to a size via the longest domain edge; the shortest edge would over-refine
  // rectangular domains by up to a level (min == max for cubic domains).
  return divideByThreePower(1.1 * tarch::la::max(domainSize), _gridLevels);
}

double
runner::RuntimeParameters::getMinAdmissiblePatchSize(const tarch::la::Vector<DIMENSIONS, double>& domainSize) const {
  return divideByThreePower(1.1 * tarch::la::max(domainSize), _maxGridLevels);
}

double runner::RuntimeParameters::getMaxAdmissibleCellSize(
  const tarch::la::Vector<DIMENSIONS, double>& domainSize,
  int                                          cellsPerPatchPerAxis
) const {
  if (cellsPerPatchPerAxis <= 0) {
    throw std::runtime_error("cellsPerPatchPerAxis must be positive.");
  }
  return getMaxAdmissiblePatchSize(domainSize) / cellsPerPatchPerAxis;
}

double runner::RuntimeParameters::getMinAdmissibleCellSize(
  const tarch::la::Vector<DIMENSIONS, double>& domainSize,
  int                                          cellsPerPatchPerAxis
) const {
  if (cellsPerPatchPerAxis <= 0) {
    throw std::runtime_error("cellsPerPatchPerAxis must be positive.");
  }
  return getMinAdmissiblePatchSize(domainSize) / cellsPerPatchPerAxis;
}
