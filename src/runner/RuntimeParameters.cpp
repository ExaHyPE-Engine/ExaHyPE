// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "RuntimeParameters.h"

#include "tarch/accelerator/Device.h"
#include "tarch/reader/JSONReader.h"

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
  _outputPath(normaliseOutputPath("output")),
  _outputPathConfigured(false),
  _projectName(),
  _executableName(),
  _timeStepLogInterval(5.0),
  _gridLevels(3),
  _maxGridLevels(3),
  _gridLevelsConfigured(false),
  _maxGridLevelsConfigured(false) {}

runner::RuntimeParameters::~RuntimeParameters() = default;

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
  std::vector<std::pair<std::string, std::string>> options;
  const auto addOption = [&options](const std::string& argument, const std::string& description) {
    options.emplace_back(argument, description);
  };

  addOption("-cf, --configFile <path>", "Read runtime settings from JSON (default: auto-detect ./config.json).");
  addOption("-et, --endTime <t>", "Advance the whole domain to time t; no upper cap (default: not set).");
  addOption(
    "-tmin, --minTerminalTime <t>",
    "Minimum terminal time (default: " + formatDefault(_minTerminalTime) + ")."
  );
  addOption(
    "-tmax, --maxTerminalTime <t>",
    "Maximum terminal time (default: " + formatTerminalTime(_maxTerminalTime) + ")."
  );
  addOption(
    "-fpt, --firstPlotTimeStamp <t>",
    "First plot time stamp (default: " + formatDefault(_firstPlotTimeStamp) + ")."
  );
  addOption(
    "-nt, --numberOfTimeSteps <n>",
    "Maximum number of time steps; use <=0 for unbounded (default: " +
      (_numberOfTimeSteps > 0 ? formatDefault(_numberOfTimeSteps) : std::string("unbounded")) + ")."
  );
  addOption(
    "-ns, --numberOfSnapshots <n>",
    "Number of plot snapshots; use <=0 to disable plotting (default: " + formatDefault(_numberOfSnapshots) + ")."
  );
  addOption("-o, --outputPath <path>", "Output path (default: " + formatDefault(_outputPath) + ").");
  addOption(
    "-li, --timeStepLogInterval <seconds>",
    "Minimum wall-clock seconds between time-step reports (default: " + formatDefault(_timeStepLogInterval) + ")."
  );
  addOption(
    "-l, --gridLevels <n>",
    "Set coarsest and finest grid level to n (default: " + formatDefault(_gridLevels) + ")."
  );
  addOption("-lmin, --minGridLevels <n>", "Coarsest grid level (default: " + formatDefault(_gridLevels) + ").");
  addOption("-lmax, --maxGridLevels <n>", "Finest grid level (default: " + formatDefault(_maxGridLevels) + ").");
  addOption(
    "-lbq, --loadBalancingQuality <q>",
    "Worst-case load-balancing ratio (default: " + formatDefault(_loadBalancingQuality) + ")."
  );
  addOption("-tsmin, --minSizeOfTree <n>", "Minimum tree size (default: " + formatDefault(_minSizeOfTree) + ").");
  addOption(
    "-trees, --maxNumberOfTreesPerRank <n>",
    "Maximum trees per rank. Use <=0 for thread count (default: " + formatDefault(_maxNumberOfTreesPerRank) + ")."
  );
  addOption(
    "-td, --targetDevice <n>",
    "Logical accelerator device id (default: " + formatDefault(_targetDevice) + ")."
  );
  addOption(
    "--timeoutWarning <seconds>",
    "Warn after this many seconds waiting for MPI progress; fractional values such as 0.1 are "
    "allowed, 0 disables (default: " +
      formatDefault(_mpiTimeOutWarning) + ")."
  );
  addOption(
    "--deadlockTimeout <seconds>",
    "Terminate after this many seconds waiting for MPI progress; fractional values such as 0.1 are "
    "allowed, 0 disables (default: " +
      formatDefault(_mpiDeadlockTimeOut) + ")."
  );
  addOption("--deadlockDetection", "Enable detailed MPI request tracking for deadlock diagnostics (default: off).");
  addOption("-v, --verbose", "Show the full log line prefix (timestamp, core, scope) (default: off).");
  addOption("-h, --help", "Print this help and exit.");

  std::size_t argumentColumnWidth = 0;
  for (const auto& option : options) {
    argumentColumnWidth = std::max(argumentColumnWidth, option.first.size());
  }

  std::cout << "Usage: " << executableName << " [options]\n\nRuntime configuration:\n";
  for (const auto& [argument, description] : options) {
    std::cout << "  " << argument << std::string(argumentColumnWidth - argument.size() + 2, ' ') << description << "\n";
  }
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

void runner::RuntimeParameters::validateParameterRanges() const {
  if (_minTerminalTime < 0.0) {
    throw std::runtime_error("Terminal time must not be negative (got " + formatDefault(_minTerminalTime) + ").");
  }
  if (_maxTerminalTime < _minTerminalTime) {
    throw std::runtime_error("Maximum terminal time must be greater than or equal to the minimum terminal time.");
  }
  if (_firstPlotTimeStamp < 0.0) {
    throw std::runtime_error(
      "First plot time stamp must not be negative (got " + formatDefault(_firstPlotTimeStamp) + ")."
    );
  }
  if (_timeStepLogInterval <= 0.0) {
    throw std::runtime_error(
      "Time-step log interval must be positive (got " + formatDefault(_timeStepLogInterval) + " s)."
    );
  }
  if (_loadBalancingQuality <= 0.0 or _loadBalancingQuality > 1.0) {
    throw std::runtime_error(
      "Load-balancing quality must lie in (0, 1] (got " + formatDefault(_loadBalancingQuality) + ")."
    );
  }
  if (_minSizeOfTree < 1) {
    throw std::runtime_error("Minimum tree size must be at least 1 (got " + formatDefault(_minSizeOfTree) + ").");
  }
  if (_targetDevice < 0) {
    throw std::runtime_error("Target device id must not be negative (got " + formatDefault(_targetDevice) + ").");
  }
  if (_mpiTimeOutWarning < 0.0) {
    throw std::runtime_error(
      "MPI timeout warning must not be negative (got " + formatDefault(_mpiTimeOutWarning) + " s)."
    );
  }
  if (_mpiDeadlockTimeOut < 0.0) {
    throw std::runtime_error(
      "MPI deadlock timeout must not be negative (got " + formatDefault(_mpiDeadlockTimeOut) + " s)."
    );
  }
}

void runner::RuntimeParameters::extractConfigFileFromCommandLine(int argc, char** argv) {
  for (int index = 1; index < argc; ++index) {
    const std::string argument = argv[index];
    if (argument == "--configFile" or argument == "-cf") {
      _configFilePath = consumeValue(index, argc, argv, argument);
    }
  }
}

void runner::RuntimeParameters::readJsonConfiguration() {
  if (_configFilePath.empty()) {
    return;
  }

  _configReader = std::make_unique<tarch::reader::JSONReader>(_configFilePath);
  auto& reader  = *_configReader;

  _mpiTimeOutWarning           = reader.getDouble("timeoutWarning", _mpiTimeOutWarning);
  _mpiDeadlockTimeOut          = reader.getDouble("deadlockTimeout", _mpiDeadlockTimeOut);
  _mpiDeadlockDetectionEnabled = reader.getBoolean("deadlockDetection", _mpiDeadlockDetectionEnabled);
  _loadBalancingQuality        = reader.getDouble("loadBalancingQuality", _loadBalancingQuality);
  _minSizeOfTree               = reader.getInteger("minSizeOfTree", _minSizeOfTree);
  _maxNumberOfTreesPerRank     = reader.getInteger("maxNumberOfTreesPerRank", _maxNumberOfTreesPerRank);
  _targetDevice                = reader.getInteger("targetDevice", _targetDevice);
  _verbose                     = reader.getBoolean("verbose", _verbose);
  _numberOfTimeSteps           = reader.getInteger("numberOfTimeSteps", _numberOfTimeSteps);
  _firstPlotTimeStamp          = reader.getDouble("firstPlotTimeStamp", _firstPlotTimeStamp);
  _numberOfSnapshots           = reader.getInteger("numberOfSnapshots", _numberOfSnapshots);
  _timeStepLogInterval         = reader.getDouble("timeStepLogInterval", _timeStepLogInterval);

  // endTime advances the whole domain to that time (min stamp drives completion, no upper cap); an
  // explicit min/max window overrides afterwards.
  if (reader.hasKey("endTime")) {
    _minTerminalTime = reader.getDouble("endTime");
    _maxTerminalTime = std::numeric_limits<double>::max();
  }
  _minTerminalTime = reader.getDouble("minTerminalTime", _minTerminalTime);
  _maxTerminalTime = reader.getDouble("maxTerminalTime", _maxTerminalTime);

  if (reader.hasKey("outputPath")) {
    _outputPath           = normaliseOutputPath(reader.getString("outputPath"));
    _outputPathConfigured = true;
  }
  if (reader.hasKey("gridLevels")) {
    _gridLevels              = reader.getInteger("gridLevels");
    _maxGridLevels           = _gridLevels;
    _gridLevelsConfigured    = true;
    _maxGridLevelsConfigured = true;
  }
  if (reader.hasKey("minGridLevels")) {
    _gridLevels           = reader.getInteger("minGridLevels");
    _gridLevelsConfigured = true;
  }
  if (reader.hasKey("maxGridLevels")) {
    _maxGridLevels           = reader.getInteger("maxGridLevels");
    _maxGridLevelsConfigured = true;
  }
}

void runner::RuntimeParameters::applyCommandLineConfiguration(int& argc, char** argv) {
  int writeIndex = 1;

  for (int readIndex = 1; readIndex < argc; ++readIndex) {
    const std::string argument = argv[readIndex];
    if (argument == "--configFile" or argument == "-cf") {
      _configFilePath = consumeValue(readIndex, argc, argv, argument);
    } else if (argument == "--endTime" or argument == "-et") {
      // Advance the whole domain to this time: the minimum stamp drives completion, no upper cap.
      _minTerminalTime = parseDouble(consumeValue(readIndex, argc, argv, argument), argument);
      _maxTerminalTime = std::numeric_limits<double>::max();
    } else if (argument == "--minTerminalTime" or argument == "-tmin") {
      _minTerminalTime = parseDouble(consumeValue(readIndex, argc, argv, argument), argument);
    } else if (argument == "--maxTerminalTime" or argument == "-tmax") {
      _maxTerminalTime = parseDouble(consumeValue(readIndex, argc, argv, argument), argument);
    } else if (argument == "--numberOfTimeSteps" or argument == "-nt") {
      _numberOfTimeSteps = parseInteger(consumeValue(readIndex, argc, argv, argument), argument);
    } else if (argument == "--firstPlotTimeStamp" or argument == "-fpt") {
      _firstPlotTimeStamp = parseDouble(consumeValue(readIndex, argc, argv, argument), argument);
    } else if (argument == "--numberOfSnapshots" or argument == "-ns") {
      _numberOfSnapshots = parseInteger(consumeValue(readIndex, argc, argv, argument), argument);
    } else if (argument == "--outputPath" or argument == "-o") {
      _outputPath           = normaliseOutputPath(consumeValue(readIndex, argc, argv, argument));
      _outputPathConfigured = true;
    } else if (argument == "--timeStepLogInterval" or argument == "-li") {
      _timeStepLogInterval = parseDouble(consumeValue(readIndex, argc, argv, argument), argument);
    } else if (argument == "--gridLevels" or argument == "-l") {
      _gridLevels              = parseInteger(consumeValue(readIndex, argc, argv, argument), argument);
      _maxGridLevels           = _gridLevels;
      _gridLevelsConfigured    = true;
      _maxGridLevelsConfigured = true;
    } else if (argument == "--minGridLevels" or argument == "-lmin") {
      _gridLevels           = parseInteger(consumeValue(readIndex, argc, argv, argument), argument);
      _gridLevelsConfigured = true;
    } else if (argument == "--maxGridLevels" or argument == "-lmax") {
      _maxGridLevels           = parseInteger(consumeValue(readIndex, argc, argv, argument), argument);
      _maxGridLevelsConfigured = true;
    } else if (argument == "--loadBalancingQuality" or argument == "-lbq") {
      _loadBalancingQuality = parseDouble(consumeValue(readIndex, argc, argv, argument), argument);
    } else if (argument == "--minSizeOfTree" or argument == "-tsmin") {
      _minSizeOfTree = parseInteger(consumeValue(readIndex, argc, argv, argument), argument);
    } else if (argument == "--maxNumberOfTreesPerRank" or argument == "-trees") {
      _maxNumberOfTreesPerRank = parseInteger(consumeValue(readIndex, argc, argv, argument), argument);
    } else if (argument == "--targetDevice" or argument == "-td") {
      _targetDevice = parseInteger(consumeValue(readIndex, argc, argv, argument), argument);
    } else if (argument == "--verbose" or argument == "-v") {
      _verbose = true;
    } else if (argument == "--timeoutWarning") {
      _mpiTimeOutWarning = parseDouble(consumeValue(readIndex, argc, argv, argument), argument);
    } else if (argument == "--deadlockTimeout") {
      _mpiDeadlockTimeOut = parseDouble(consumeValue(readIndex, argc, argv, argument), argument);
    } else if (argument == "--deadlockDetection") {
      _mpiDeadlockDetectionEnabled = true;
    } else if (argument == "--noDeadlockDetection") {
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
  double             minTerminalTime,
  double             maxTerminalTime,
  int                gridLevels,
  int                maxGridLevels,
  const std::string& projectName
) {
  _projectName = projectName;

  if (not projectName.empty() and not _outputPathConfigured) {
    _outputPath = normaliseOutputPath(projectName + ".output");
  }

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
  if (argc > 0) {
    _executableName = std::filesystem::path(argv[0]).filename().string();
  }

  extractConfigFileFromCommandLine(argc, argv);

  if (_configFilePath.empty() and std::filesystem::exists("./config.json")) {
    _configFilePath = "./config.json";
  }

  readJsonConfiguration();
  applyCommandLineConfiguration(argc, argv);
  normaliseGridLevels();
  validateParameterRanges();
}

tarch::reader::JSONReader* runner::RuntimeParameters::getConfigReader() { return _configReader.get(); }

double runner::RuntimeParameters::getMPITimeOutWarning() const { return _mpiTimeOutWarning; }
double runner::RuntimeParameters::getMPIDeadlockTimeOut() const { return _mpiDeadlockTimeOut; }
bool   runner::RuntimeParameters::isMPIDeadlockDetectionEnabled() const { return _mpiDeadlockDetectionEnabled; }
double runner::RuntimeParameters::getLoadBalancingQuality() const { return _loadBalancingQuality; }
int    runner::RuntimeParameters::getMinSizeOfTree() const { return _minSizeOfTree; }
int    runner::RuntimeParameters::getMaxNumberOfTreesPerRank() const { return _maxNumberOfTreesPerRank; }
int    runner::RuntimeParameters::getTargetDevice() const { return _targetDevice; }

bool   runner::RuntimeParameters::isVerbose() const { return _verbose; }
double runner::RuntimeParameters::getTimeStepLogInterval() const { return _timeStepLogInterval; }

double runner::RuntimeParameters::getMinTerminalTime() const { return _minTerminalTime; }
double runner::RuntimeParameters::getMaxTerminalTime() const { return _maxTerminalTime; }
int    runner::RuntimeParameters::getNumberOfTimeSteps() const { return _numberOfTimeSteps; }
double runner::RuntimeParameters::getFirstPlotTimeStamp() const { return _firstPlotTimeStamp; }
int    runner::RuntimeParameters::getNumberOfSnapshots() const { return _numberOfSnapshots; }
int    runner::RuntimeParameters::getGridLevels() const { return _gridLevels; }
int    runner::RuntimeParameters::getMaxGridLevels() const { return _maxGridLevels; }

const std::string& runner::RuntimeParameters::getConfigFilePath() const { return _configFilePath; }
const std::string& runner::RuntimeParameters::getOutputPath() const { return _outputPath; }
const std::string& runner::RuntimeParameters::getProjectName() const { return _projectName; }
const std::string& runner::RuntimeParameters::getExecutableName() const { return _executableName; }

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
