// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "RuntimeParameters.h"

#include "peano4/sfc/ActiveCurve.h"
#include "tarch/Assertions.h"
#include "tarch/Termination.h"
#include "tarch/reader/JSONReader.h"

runner::RuntimeParameters::RuntimeParameters()  = default;
runner::RuntimeParameters::~RuntimeParameters() = default;

runner::RuntimeParameters& runner::RuntimeParameters::getInstance() {
  static runner::RuntimeParameters result;
  return result;
}

std::string runner::RuntimeParameters::formatDefault(const double value) {
  std::ostringstream result;
  result << value;
  return result.str();
}

std::string runner::RuntimeParameters::formatDefault(const int value) { return std::to_string(value); }

std::string runner::RuntimeParameters::formatDefault(const std::string& value) {
  return value.empty() ? "<none>" : value;
}

std::string runner::RuntimeParameters::formatTerminalTime(const double value) {
  return value < std::numeric_limits<double>::max() / 10.0 ? formatDefault(value) : "unbounded";
}

/**
 * Divides a length by the subdivision factor of the active curve once per level, so the
 * result is the width of a cell exponent levels below a cell of the given width.
 */
double runner::RuntimeParameters::divideBySubdivisionFactorPower(double value, const int exponent) {
  assertion(exponent >= 0, exponent);
  for (int level = 0; level < exponent; ++level) {
    value /= peano4::sfc::ActiveCurve::SubdivisionFactor;
  }
  return value;
}

int runner::RuntimeParameters::parseInteger(const std::string& value, const std::string& key) {
  const char* const end    = value.data() + value.size();
  auto              result = 0;
  const auto        parsed = std::from_chars(value.data(), end, result);
  if (parsed.ec != std::errc{} or parsed.ptr != end) {
    tarch::terminateWithError("Value \"" + value + "\" of runtime parameter \"" + key + "\" is not an integer.");
  }
  return result;
}

double runner::RuntimeParameters::parseDouble(const std::string& value, const std::string& key) {
  const char* const end    = value.data() + value.size();
  auto              result = 0.0;
  const auto        parsed = std::from_chars(value.data(), end, result);
  if (parsed.ec != std::errc{} or parsed.ptr != end) {
    tarch::terminateWithError(
      "Value \"" + value + "\" of runtime parameter \"" + key + "\" is not a floating point number."
    );
  }
  return result;
}

std::string runner::RuntimeParameters::normaliseOutputPath(const std::string& path) {
  if (path.empty()) {
    return "./";
  }
  return path.back() == '/' ? path : path + "/";
}

std::string runner::RuntimeParameters::consumeValue(
  int&               index,
  const int          argc,
  char**             argv,
  const std::string& argument
) {
  if (index + 1 >= argc) {
    tarch::terminateWithError("Command line argument \"" + argument + "\" carries no value.");
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
    "Maximum terminal time; it lowers a larger minimum terminal time (default: " + formatTerminalTime(_maxTerminalTime)
      + ")."
  );
  addOption(
    "-fpt, --firstPlotTimeStamp <t>",
    "First plot time stamp (default: " + formatDefault(_firstPlotTimeStamp) + ")."
  );
  addOption(
    "-nt, --numberOfTimeSteps <n>",
    "Maximum number of time steps; use a non-positive value for unbounded (default: "
      + (_numberOfTimeSteps > 0 ? formatDefault(_numberOfTimeSteps) : std::string("unbounded")) + ")."
  );
  addOption(
    "-ns, --numberOfSnapshots <n>",
    "Number of plot snapshots; use a non-positive value to disable plotting (default: "
      + formatDefault(_numberOfSnapshots) + ")."
  );
  addOption("-o, --outputPath <path>", "Output path (default: " + formatDefault(_outputPath) + ").");
  addOption("-pdp, --plotDoublePrecision", "Write plot files in double precision (default: off, single precision).");
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
    "-td, --targetDevice <n>",
    "Logical accelerator device id (default: " + formatDefault(_targetDevice) + ")."
  );
  addOption(
    "--timeoutWarning <seconds>",
    "Warn after this many seconds waiting for MPI progress; fractional values such as 0.1 are "
    "allowed, 0 disables. Setting it also records the origin, partner and tag of every request, so "
    "the warning names the one that hangs (default: "
      + formatDefault(_mpiTimeoutWarning) + ")."
  );
  addOption(
    "--deadlockTimeout <seconds>",
    "Terminate after this many seconds waiting for MPI progress; fractional values such as 0.1 are "
    "allowed, 0 disables. Records requests in the same detail as --timeoutWarning (default: "
      + formatDefault(_mpiDeadlockTimeout) + ")."
  );
  addOption(
    "-ct, --communicationThread",
    "Drive the outstanding MPI communication on a thread of its own. The thread reserves a core and "
    "only runs where a job has more than one rank (default: off)."
  );
  addOption(
    "-dlb, --disableLoadBalancing",
    "Keep the rank partition of the grid construction across regrids. A regrid still repartitions "
    "when the kept partition would spread a coarse cell's children, one of them a leaf, over "
    "several ranks; those children have to stay on one rank (default: off)."
  );
  addOption("-v, --verbose", "Show the full log line prefix (timestamp, core, scope) (default: off).");
  addOption("-h, --help", "Print this help and exit.");

  auto argumentColumnWidth = std::size_t{0};
  for (const auto& option : options) {
    argumentColumnWidth = std::max(argumentColumnWidth, option.first.size());
  }

  std::cout << "Usage: " << executableName << " [options]\n\nRuntime configuration:\n";
  for (const auto& [argument, description] : options) {
    std::cout << "  " << argument << std::string(argumentColumnWidth - argument.size() + 2, ' ') << description << "\n";
  }
  std::cout << "\nThe environment variable PEANO_MAX_TERMINAL_TIME bounds the maximum terminal time.\n";
}

void runner::RuntimeParameters::normaliseGridLevels() {
  if (_gridLevelsConfigured and not _maxGridLevelsConfigured) {
    _maxGridLevels = _gridLevels;
  } else if (_maxGridLevelsConfigured and not _gridLevelsConfigured) {
    _gridLevels = _maxGridLevels;
  }

  if (_gridLevels < 1) {
    tarch::terminateWithError("Grid levels must be at least one.");
  }
  if (_maxGridLevels < 1) {
    tarch::terminateWithError("Maximum grid levels must be at least one.");
  }
  if (_maxGridLevels < _gridLevels) {
    tarch::terminateWithError("Maximum grid levels must be greater than or equal to the coarsest grid level.");
  }
}

/**
 * Lowers the maximum terminal time to the environment variable PEANO_MAX_TERMINAL_TIME when it is
 * set, and the minimum terminal time to the maximum one, so a run never advances beyond the bound.
 */
void runner::RuntimeParameters::boundTerminalTimes() {
  const char* const environmentBound = std::getenv("PEANO_MAX_TERMINAL_TIME"); // NOLINT(concurrency-mt-unsafe)
  if (environmentBound != nullptr) {
    _maxTerminalTime = std::min(_maxTerminalTime, parseDouble(environmentBound, "PEANO_MAX_TERMINAL_TIME"));
  }
  _minTerminalTime = std::min(_minTerminalTime, _maxTerminalTime);
}

void runner::RuntimeParameters::validateParameterRanges() const {
  if (not std::isfinite(_minTerminalTime) or _minTerminalTime < 0.0) {
    tarch::terminateWithError(
      "Terminal time must be finite and non-negative (got " + formatDefault(_minTerminalTime) + ")."
    );
  }
  if (not std::isfinite(_maxTerminalTime)) {
    tarch::terminateWithError("Maximum terminal time must be finite.");
  }
  if (not std::isfinite(_firstPlotTimeStamp) or _firstPlotTimeStamp < 0.0) {
    tarch::terminateWithError(
      "First plot time stamp must be finite and non-negative (got " + formatDefault(_firstPlotTimeStamp) + ")."
    );
  }
  if (not std::isfinite(_timeStepLogInterval) or _timeStepLogInterval <= 0.0) {
    tarch::terminateWithError(
      "Time-step log interval must be finite and positive (got " + formatDefault(_timeStepLogInterval) + " s)."
    );
  }
  if (_targetDevice < 0) {
    tarch::terminateWithError("Target device id must not be negative (got " + formatDefault(_targetDevice) + ").");
  }
  if (not std::isfinite(_mpiTimeoutWarning) or _mpiTimeoutWarning < 0.0) {
    tarch::terminateWithError(
      "MPI timeout warning must be finite and non-negative (got " + formatDefault(_mpiTimeoutWarning) + " s)."
    );
  }
  if (not std::isfinite(_mpiDeadlockTimeout) or _mpiDeadlockTimeout < 0.0) {
    tarch::terminateWithError(
      "MPI deadlock timeout must be finite and non-negative (got " + formatDefault(_mpiDeadlockTimeout) + " s)."
    );
  }
}

void runner::RuntimeParameters::extractConfigFileFromCommandLine(const int argc, char** argv) {
  for (int index = 1; index < argc; ++index) {
    const auto argument = std::string(argv[index]);
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

  const auto configuredTimeoutWarning  = reader.getDouble("timeoutWarning", _mpiTimeoutWarning);
  const auto configuredDeadlockTimeout = reader.getDouble("deadlockTimeout", _mpiDeadlockTimeout);
  if (reader.hasKey("timeoutWarning") or reader.hasKey("deadlockTimeout")) {
    _mpiTimeoutConfigured = true;
  }
  _mpiTimeoutWarning   = configuredTimeoutWarning;
  _mpiDeadlockTimeout  = configuredDeadlockTimeout;
  _targetDevice        = reader.getInteger("targetDevice", _targetDevice);
  _verbose             = reader.getBoolean("verbose", _verbose);
  _numberOfTimeSteps   = reader.getInteger("numberOfTimeSteps", _numberOfTimeSteps);
  _firstPlotTimeStamp  = reader.getDouble("firstPlotTimeStamp", _firstPlotTimeStamp);
  _numberOfSnapshots   = reader.getInteger("numberOfSnapshots", _numberOfSnapshots);
  _timeStepLogInterval = reader.getDouble("timeStepLogInterval", _timeStepLogInterval);

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
  if (reader.hasKey("plotDoublePrecision")) {
    _plotDoublePrecision = reader.getBoolean("plotDoublePrecision");
  }
  if (reader.hasKey("communicationThread")) {
    _communicationThread = reader.getBoolean("communicationThread");
  }
  if (reader.hasKey("disableLoadBalancing")) {
    _disableLoadBalancing = reader.getBoolean("disableLoadBalancing");
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
    const auto argument = std::string(argv[readIndex]);
    if (argument == "--configFile" or argument == "-cf") {
      _configFilePath = consumeValue(readIndex, argc, argv, argument);
    } else if (argument == "--endTime" or argument == "-et") {
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
    } else if (argument == "--plotDoublePrecision" or argument == "-pdp") {
      _plotDoublePrecision = true;
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
    } else if (argument == "--targetDevice" or argument == "-td") {
      _targetDevice = parseInteger(consumeValue(readIndex, argc, argv, argument), argument);
    } else if (argument == "--verbose" or argument == "-v") {
      _verbose = true;
    } else if (argument == "--timeoutWarning") {
      _mpiTimeoutWarning    = parseDouble(consumeValue(readIndex, argc, argv, argument), argument);
      _mpiTimeoutConfigured = true;
    } else if (argument == "--deadlockTimeout") {
      _mpiDeadlockTimeout   = parseDouble(consumeValue(readIndex, argc, argv, argument), argument);
      _mpiTimeoutConfigured = true;
    } else if (argument == "--communicationThread" or argument == "-ct") {
      _communicationThread = true;
    } else if (argument == "--disableLoadBalancing" or argument == "-dlb") {
      _disableLoadBalancing = true;
    } else {
      argv[writeIndex] = argv[readIndex];
      ++writeIndex;
    }
  }

  argc       = writeIndex;
  argv[argc] = nullptr;
}

void runner::RuntimeParameters::setGeneratedDefaults(
  const double       minTerminalTime,
  const double       maxTerminalTime,
  const int          gridLevels,
  const int          maxGridLevels,
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
  boundTerminalTimes();
  validateParameterRanges();
}

/** Returns the reader of the JSON configuration file, or nullptr when no configuration file is read. */
tarch::reader::JSONReader* runner::RuntimeParameters::getConfigReader() const { return _configReader.get(); }

double runner::RuntimeParameters::getMPITimeoutWarning() const { return _mpiTimeoutWarning; }
double runner::RuntimeParameters::getMPIDeadlockTimeout() const { return _mpiDeadlockTimeout; }

/**
 * Returns whether to record the origin, partner and tag of every non-blocking request. The record
 * costs one table entry per request, so --timeoutWarning or --deadlockTimeout switches it on.
 */
bool runner::RuntimeParameters::isMPIRequestTrackingEnabled() const { return _mpiTimeoutConfigured; }
bool runner::RuntimeParameters::isCommunicationThreadEnabled() const { return _communicationThread; }

/**
 * Returns whether every regrid keeps the current domain decomposition.
 *
 * The grid construction still spreads the blocks over the ranks. A regrid
 * keeps every block on its current rank.
 */
bool runner::RuntimeParameters::isLoadBalancingDisabled() const { return _disableLoadBalancing; }

int runner::RuntimeParameters::getTargetDevice() const { return _targetDevice; }

bool   runner::RuntimeParameters::isVerbose() const { return _verbose; }
double runner::RuntimeParameters::getTimeStepLogInterval() const { return _timeStepLogInterval; }

double runner::RuntimeParameters::getMinTerminalTime() const { return _minTerminalTime; }
double runner::RuntimeParameters::getMaxTerminalTime() const { return _maxTerminalTime; }
int    runner::RuntimeParameters::getNumberOfTimeSteps() const { return _numberOfTimeSteps; }
double runner::RuntimeParameters::getFirstPlotTimeStamp() const { return _firstPlotTimeStamp; }
int    runner::RuntimeParameters::getNumberOfSnapshots() const { return _numberOfSnapshots; }
bool   runner::RuntimeParameters::getPlotDoublePrecision() const { return _plotDoublePrecision; }
int    runner::RuntimeParameters::getGridLevels() const { return _gridLevels; }
int    runner::RuntimeParameters::getMaxGridLevels() const { return _maxGridLevels; }

const std::string& runner::RuntimeParameters::getConfigFilePath() const { return _configFilePath; }
const std::string& runner::RuntimeParameters::getOutputPath() const { return _outputPath; }
const std::string& runner::RuntimeParameters::getProjectName() const { return _projectName; }
const std::string& runner::RuntimeParameters::getExecutableName() const { return _executableName; }

double runner::RuntimeParameters::getMaxAdmissiblePatchSize(
  const tarch::la::Vector<DIMENSIONS, double>& domainSize
) const {
  // The tolerance absorbs round-off, and the longest edge avoids over-refining rectangular domains.
  return divideBySubdivisionFactorPower(1.1 * tarch::la::max(domainSize), _gridLevels);
}

double runner::RuntimeParameters::getMinAdmissiblePatchSize(
  const tarch::la::Vector<DIMENSIONS, double>& domainSize
) const {
  return divideBySubdivisionFactorPower(1.1 * tarch::la::max(domainSize), _maxGridLevels);
}

double runner::RuntimeParameters::getMaxAdmissibleCellSize(
  const tarch::la::Vector<DIMENSIONS, double>& domainSize,
  const int                                    cellsPerPatchPerAxis
) const {
  assertion(cellsPerPatchPerAxis > 0, cellsPerPatchPerAxis);
  return getMaxAdmissiblePatchSize(domainSize) / cellsPerPatchPerAxis;
}
