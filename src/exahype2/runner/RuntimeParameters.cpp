// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "RuntimeParameters.h"

#include "peano4/Partition.h"
#include "tarch/Assertions.h"
#include "tarch/Termination.h"
#include "tarch/la/VectorOperations.h"
#include "tarch/reader/JSONReader.h"

namespace {
  template <typename Scalar>
  std::string getTypeName() {
    if constexpr (std::is_same_v<Scalar, double>) {
      return "double";
    } else if constexpr (std::is_same_v<Scalar, int>) {
      return "int";
    } else if constexpr (std::is_same_v<Scalar, bool>) {
      return "bool";
    } else {
      return "string";
    }
  }

  template <typename Scalar>
  std::string format(const Scalar& value) {
    if constexpr (std::is_same_v<Scalar, bool>) {
      return value ? "true" : "false";
    } else {
      return exahype2::runner::RuntimeParameters::formatDefault(value);
    }
  }

  template <typename Scalar>
  std::string format(const Scalar* const values, const int size) {
    std::string result;
    for (int index = 0; index < size; index++) {
      result += (index > 0 ? "," : "") + format(values[index]);
    }
    return result;
  }

  bool parseBoolean(std::string value, const std::string& key) {
    std::transform(value.begin(), value.end(), value.begin(), [](const unsigned char character) {
      return static_cast<char>(std::tolower(character));
    });
    if (value == "true" or value == "1" or value == "yes" or value == "on") {
      return true;
    }
    if (value == "false" or value == "0" or value == "no" or value == "off") {
      return false;
    }
    tarch::terminateWithError("Runtime parameter \"" + key + "\" expects a boolean (got \"" + value + "\").");
  }

  template <typename Scalar>
  Scalar parse(const std::string& value, const std::string& key) {
    if constexpr (std::is_same_v<Scalar, double>) {
      return exahype2::runner::RuntimeParameters::parseDouble(value, key);
    } else if constexpr (std::is_same_v<Scalar, int>) {
      return exahype2::runner::RuntimeParameters::parseInteger(value, key);
    } else if constexpr (std::is_same_v<Scalar, bool>) {
      return parseBoolean(value, key);
    } else {
      return value;
    }
  }

  template <typename Scalar>
  Scalar readScalar(const tarch::reader::JSONReader& reader, const std::string& name) {
    if constexpr (std::is_same_v<Scalar, double>) {
      return reader.getDouble(name);
    } else if constexpr (std::is_same_v<Scalar, int>) {
      return reader.getInteger(name);
    } else if constexpr (std::is_same_v<Scalar, bool>) {
      return reader.getBoolean(name);
    } else {
      return reader.getString(name);
    }
  }

  template <typename Scalar>
  std::vector<Scalar> readArray(const tarch::reader::JSONReader& reader, const std::string& name) {
    if constexpr (std::is_same_v<Scalar, double>) {
      return reader.getDoubleArray(name);
    } else if constexpr (std::is_same_v<Scalar, int>) {
      return reader.getIntegerArray(name);
    } else if constexpr (std::is_same_v<Scalar, bool>) {
      return reader.getBooleanArray(name);
    } else {
      return reader.getStringArray(name);
    }
  }

  template <typename Scalar>
  void assign(const std::vector<Scalar>& source, Scalar* const values, const int size, const std::string& name) {
    if (source.size() < static_cast<std::size_t>(size)) {
      tarch::terminateWithError(
        "Runtime parameter \"" + name + "\" expects " + std::to_string(size) + " entries (got "
        + std::to_string(source.size()) + ")."
      );
    }
    std::copy_n(source.begin(), size, values);
  }

  template <typename Scalar>
  void readFromConfiguration(
    const tarch::reader::JSONReader& reader,
    const std::string&               name,
    Scalar* const                    values,
    const int                        size,
    const bool                       isArray
  ) {
    if (isArray) {
      assign(readArray<Scalar>(reader, name), values, size, name);
    } else {
      *values = readScalar<Scalar>(reader, name);
    }
  }

  template <typename Scalar>
  void readFromCommandLine(
    const std::string& value,
    const std::string& argument,
    Scalar* const      values,
    const int          size,
    const bool         isArray
  ) {
    if (not isArray) {
      *values = parse<Scalar>(value, argument);
      return;
    }
    std::vector<Scalar> parsed;
    std::string         entry;
    std::stringstream   stream(value);
    while (std::getline(stream, entry, ',')) {
      parsed.push_back(parse<Scalar>(entry, argument));
    }
    assign(parsed, values, size, argument);
  }
} // namespace

/**
 * The value in the storage at registration becomes the default of the usage text.
 */
void
exahype2::runner::ParameterTable::add(const std::string& name, const Values values) {
  assertion(not name.empty());
  _entries.push_back(
    {name, values, 1, false, std::visit([](const auto* const pointer) { return format(*pointer); }, values)}
  );
}

void
exahype2::runner::ParameterTable::addArray(const std::string& name, const Values values, const int size) {
  assertion(not name.empty());
  assertion(size > 0, name, size);
  _entries.push_back(
    {name, values, size, true, std::visit([size](const auto* const pointer) { return format(pointer, size); }, values)}
  );
}

void
exahype2::runner::ParameterTable::load(const tarch::reader::JSONReader* const reader) const {
  if (reader == nullptr) {
    return;
  }
  for (const auto& entry : _entries) {
    if (reader->hasKey(entry.name)) {
      std::visit(
        [&](auto* const values) { readFromConfiguration(*reader, entry.name, values, entry.size, entry.isArray); },
        entry.values
      );
    }
  }
}

/**
 * Arguments that match no entry stay in argv in their order, so the parser of the caller reads them
 * afterwards.
 */
void
exahype2::runner::ParameterTable::load(
  const tarch::reader::JSONReader* const reader,
  int&                                   argc,
  char** const                           argv
) const {
  assertion(argc >= 1 and argv != nullptr, argc);
  load(reader);
  int writeIndex = 1;
  for (int readIndex = 1; readIndex < argc; readIndex++) {
    const std::string argument = argv[readIndex];
    const auto        entry    = std::find_if(_entries.begin(), _entries.end(), [&argument](const Entry& candidate) {
      return argument == "--" + candidate.name;
    });
    if (entry == _entries.end()) {
      argv[writeIndex] = argv[readIndex];
      writeIndex++;
      continue;
    }
    const std::string value = RuntimeParameters::consumeValue(readIndex, argc, argv, argument);
    std::visit(
      [&](auto* const values) { readFromCommandLine(value, argument, values, entry->size, entry->isArray); },
      entry->values
    );
  }
  argc       = writeIndex;
  argv[argc] = nullptr;
}

std::string
exahype2::runner::ParameterTable::getUsage() const {
  std::string result = "\nApplication-specific runtime configuration:\n";
  if (_entries.empty()) {
    return result + "  <none>\n";
  }
  bool hasArrays = false;
  for (const auto& entry : _entries) {
    const auto type = std::visit(
      [](const auto* const pointer) { return getTypeName<std::remove_cvref_t<decltype(*pointer)>>(); },
      entry.values
    );
    result += "  --" + entry.name + " <" + type + (entry.isArray ? "[" + std::to_string(entry.size) + "]" : "") + ">"
      + " (default: " + entry.defaultText + ").\n";
    hasArrays = hasArrays or entry.isArray;
  }
  if (hasArrays) {
    result += "  Array values are comma-separated.\n";
  }
  return result;
}

void
exahype2::runner::ParameterTable::printUsage() const {
  std::cout << getUsage();
}

// The out-of-line constructor and destructor keep JSONReader forward-declared in the header.
exahype2::runner::RuntimeParameters::RuntimeParameters()  = default;
exahype2::runner::RuntimeParameters::~RuntimeParameters() = default;

exahype2::runner::RuntimeParameters&
exahype2::runner::RuntimeParameters::getInstance() {
  static exahype2::runner::RuntimeParameters result;
  return result;
}

std::string
exahype2::runner::RuntimeParameters::formatDefault(const double value) {
  std::ostringstream result;
  result << value;
  return result.str();
}

std::string
exahype2::runner::RuntimeParameters::formatDefault(const int value) {
  return std::to_string(value);
}

std::string
exahype2::runner::RuntimeParameters::formatDefault(const std::string& value) {
  return value.empty() ? "<none>" : value;
}

std::string
exahype2::runner::RuntimeParameters::formatTerminalTime(const double value) {
  return value < std::numeric_limits<double>::max() / 10.0 ? formatDefault(value) : "Unbounded";
}

/**
 * Returns the width of a cell exponent levels below a cell of width value.
 */
double
exahype2::runner::RuntimeParameters::divideBySubdivisionFactorPower(double value, const int exponent) {
  assertion(exponent >= 0, exponent);
  for (int level = 0; level < exponent; ++level) {
    value /= peano4::SubdivisionFactor;
  }
  return value;
}

int
exahype2::runner::RuntimeParameters::parseInteger(const std::string& value, const std::string& key) {
  const char* const end    = value.data() + value.size();
  auto              result = 0;
  const auto        parsed = std::from_chars(value.data(), end, result);
  if (parsed.ec != std::errc{} or parsed.ptr != end) {
    tarch::terminateWithError("Runtime parameter \"" + key + "\" expects an integer (got \"" + value + "\").");
  }
  return result;
}

double
exahype2::runner::RuntimeParameters::parseDouble(const std::string& value, const std::string& key) {
  const char* const end    = value.data() + value.size();
  auto              result = 0.0;
  const auto        parsed = std::from_chars(value.data(), end, result);
  if (parsed.ec != std::errc{} or parsed.ptr != end) {
    tarch::terminateWithError(
      "Runtime parameter \"" + key + "\" expects a floating-point number (got \"" + value + "\")."
    );
  }
  return result;
}

std::string
exahype2::runner::RuntimeParameters::normaliseOutputPath(const std::string& path) {
  if (path.empty()) {
    return "./";
  }
  return path.back() == '/' ? path : path + "/";
}

std::string
exahype2::runner::RuntimeParameters::consumeValue(
  int&               index,
  const int          argc,
  char**             argv,
  const std::string& argument
) {
  if (index + 1 >= argc) {
    tarch::terminateWithError("Command line argument \"" + argument + "\" expects a value.");
  }
  ++index;
  return argv[index];
}

void
exahype2::runner::RuntimeParameters::printUsage(const std::string& executableName) const {
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
      + (_numberOfTimeSteps > 0 ? formatDefault(_numberOfTimeSteps) : std::string("Unbounded")) + ")."
  );
  addOption(
    "-ns, --numberOfSnapshots <n>",
    "Number of plot snapshots; use a non-positive value to disable plotting (default: "
      + formatDefault(_numberOfSnapshots) + ")."
  );
  addOption("-o, --outputPath <path>", "Output path (default: " + formatDefault(_outputPath) + ").");
  addOption("-pdp, --plotDoublePrecision", "Write plot files in double precision (default: Off, single precision).");
  addOption(
    "-pf, --plotFormat <vtu|hdf5|netcdf|vtkhdf>",
    "File format of the plot files (default: " + _plotFormat + ")."
  );
  addOption(
    "-li, --timeStepLogInterval <seconds>",
    "Minimum wall-clock seconds between time step reports (default: " + formatDefault(_timeStepLogInterval) + ")."
  );
  addOption(
    "-l, --gridLevels <n>",
    "Set coarsest and finest grid level to n (default: " + formatDefault(_gridLevels) + ")."
  );
  addOption("-lmin, --minGridLevels <n>", "Coarsest grid level (default: " + formatDefault(_gridLevels) + ").");
  addOption("-lmax, --maxGridLevels <n>", "Finest grid level (default: " + formatDefault(_maxGridLevels) + ").");
  addOption(
    "--timeoutWarning <seconds>",
    "Warn after this many seconds waiting for MPI progress; fractional values such as 0.1 are "
    "allowed, 0 disables. Setting it also records the origin, partner and tag of every request, so "
    "the warning names the one that hangs (default: "
      + formatDefault(_MPITimeoutWarning) + ")."
  );
  addOption(
    "--deadlockTimeout <seconds>",
    "Terminate after this many seconds waiting for MPI progress; fractional values such as 0.1 are "
    "allowed, 0 disables. Records requests in the same detail as --timeoutWarning (default: "
      + formatDefault(_MPIDeadlockTimeout) + ")."
  );
  addOption(
    "-ct, --communicationThread",
    "Drive the outstanding MPI communication on a thread of its own. The thread reserves a core and "
    "only runs where a job has more than one rank (default: Off)."
  );
  addOption(
    "-dlb, --disableLoadBalancing",
    "Keep the rank partition of the grid construction across regrids. A regrid still repartitions "
    "when the kept partition would spread a coarse cell's children, one of them unrefined, over "
    "several ranks; those children have to stay on one rank (default: Off)."
  );
  addOption("-v, --verbose", "Show the full log line prefix (timestamp, core, scope) (default: Off).");
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

void
exahype2::runner::RuntimeParameters::normaliseGridLevels() {
  if (_gridLevelsConfigured and not _maxGridLevelsConfigured) {
    _maxGridLevels = _gridLevels;
  } else if (_maxGridLevelsConfigured and not _gridLevelsConfigured) {
    _gridLevels = _maxGridLevels;
  }
  if (_gridLevels < 1) {
    tarch::terminateWithError(
      "Runtime parameter \"minGridLevels\" expects at least 1 (got " + std::to_string(_gridLevels) + ")."
    );
  }
  if (_maxGridLevels < 1) {
    tarch::terminateWithError(
      "Runtime parameter \"maxGridLevels\" expects at least 1 (got " + std::to_string(_maxGridLevels) + ")."
    );
  }
  if (_maxGridLevels < _gridLevels) {
    tarch::terminateWithError("Runtime parameter \"maxGridLevels\" expects at least the value of \"minGridLevels\".");
  }
}

/**
 * Lowers the maximum terminal time to PEANO_MAX_TERMINAL_TIME if it is set, and the minimum
 * terminal time to the maximum one.
 */
void
exahype2::runner::RuntimeParameters::boundTerminalTimes() {
  const char* const environmentBound = std::getenv("PEANO_MAX_TERMINAL_TIME"); // NOLINT(concurrency-mt-unsafe)
  if (environmentBound != nullptr) {
    _maxTerminalTime = std::min(_maxTerminalTime, parseDouble(environmentBound, "PEANO_MAX_TERMINAL_TIME"));
  }
  _minTerminalTime = std::min(_minTerminalTime, _maxTerminalTime);
}

void
exahype2::runner::RuntimeParameters::validateParameterRanges() const {
  if (not std::isfinite(_minTerminalTime) or _minTerminalTime < 0.0) {
    tarch::terminateWithError(
      "Runtime parameters \"endTime\" and \"minTerminalTime\" expect a finite non-negative time (got "
      + formatDefault(_minTerminalTime) + ")."
    );
  }
  if (not std::isfinite(_maxTerminalTime)) {
    tarch::terminateWithError("Runtime parameter \"maxTerminalTime\" expects a finite time.");
  }
  if (not std::isfinite(_firstPlotTimeStamp) or _firstPlotTimeStamp < 0.0) {
    tarch::terminateWithError(
      "Runtime parameter \"firstPlotTimeStamp\" expects a finite non-negative time (got "
      + formatDefault(_firstPlotTimeStamp) + ")."
    );
  }
  if (not std::isfinite(_timeStepLogInterval) or _timeStepLogInterval <= 0.0) {
    tarch::terminateWithError(
      "Runtime parameter \"timeStepLogInterval\" expects a finite positive number of seconds (got "
      + formatDefault(_timeStepLogInterval) + ")."
    );
  }
  if (not std::isfinite(_MPITimeoutWarning) or _MPITimeoutWarning < 0.0) {
    tarch::terminateWithError(
      "Runtime parameter \"timeoutWarning\" expects a finite non-negative number of seconds (got "
      + formatDefault(_MPITimeoutWarning) + ")."
    );
  }
  if (not std::isfinite(_MPIDeadlockTimeout) or _MPIDeadlockTimeout < 0.0) {
    tarch::terminateWithError(
      "Runtime parameter \"deadlockTimeout\" expects a finite non-negative number of seconds (got "
      + formatDefault(_MPIDeadlockTimeout) + ")."
    );
  }
}

void
exahype2::runner::RuntimeParameters::extractConfigFileFromCommandLine(const int argc, char** argv) {
  for (int index = 1; index < argc; ++index) {
    const auto argument = std::string(argv[index]);
    if (argument == "--configFile" or argument == "-cf") {
      _configFilePath = consumeValue(index, argc, argv, argument);
    }
  }
}

void
exahype2::runner::RuntimeParameters::readJSONConfiguration() {
  if (_configFilePath.empty()) {
    return;
  }
  _configReader = std::make_unique<tarch::reader::JSONReader>(_configFilePath);
  auto& reader  = *_configReader;

  const auto configuredTimeoutWarning  = reader.getDouble("timeoutWarning", _MPITimeoutWarning);
  const auto configuredDeadlockTimeout = reader.getDouble("deadlockTimeout", _MPIDeadlockTimeout);
  if (reader.hasKey("timeoutWarning") or reader.hasKey("deadlockTimeout")) {
    _MPITimeoutConfigured = true;
  }
  _MPITimeoutWarning   = configuredTimeoutWarning;
  _MPIDeadlockTimeout  = configuredDeadlockTimeout;
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
  if (reader.hasKey("plotFormat")) {
    _plotFormat = reader.getString("plotFormat");
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

void
exahype2::runner::RuntimeParameters::applyCommandLineConfiguration(int& argc, char** argv) {
  assertion(argc >= 1 and argv != nullptr, argc);
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
    } else if (argument == "--plotFormat" or argument == "-pf") {
      _plotFormat = consumeValue(readIndex, argc, argv, argument);
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
    } else if (argument == "--verbose" or argument == "-v") {
      _verbose = true;
    } else if (argument == "--timeoutWarning") {
      _MPITimeoutWarning    = parseDouble(consumeValue(readIndex, argc, argv, argument), argument);
      _MPITimeoutConfigured = true;
    } else if (argument == "--deadlockTimeout") {
      _MPIDeadlockTimeout   = parseDouble(consumeValue(readIndex, argc, argv, argument), argument);
      _MPITimeoutConfigured = true;
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

void
exahype2::runner::RuntimeParameters::setGeneratedDefaults(
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

void
exahype2::runner::RuntimeParameters::configure(int& argc, char** argv) {
  if (argc > 0) {
    _executableName = std::filesystem::path(argv[0]).filename().string();
  }
  extractConfigFileFromCommandLine(argc, argv);
  if (_configFilePath.empty() and std::filesystem::exists("./config.json")) {
    _configFilePath = "./config.json";
  }
  readJSONConfiguration();
  applyCommandLineConfiguration(argc, argv);
  normaliseGridLevels();
  boundTerminalTimes();
  validateParameterRanges();
}

/**
 * Returns the reader of the JSON configuration file, or nullptr when no configuration file is read.
 */
tarch::reader::JSONReader*
exahype2::runner::RuntimeParameters::getConfigReader() const {
  return _configReader.get();
}

double
exahype2::runner::RuntimeParameters::getMPITimeoutWarning() const {
  return _MPITimeoutWarning;
}

double
exahype2::runner::RuntimeParameters::getMPIDeadlockTimeout() const {
  return _MPIDeadlockTimeout;
}

/**
 * Returns whether to record the origin, partner and tag of every non-blocking request. Setting a
 * timeout switches it on.
 */
bool
exahype2::runner::RuntimeParameters::isMPIRequestTrackingEnabled() const {
  return _MPITimeoutConfigured;
}

bool
exahype2::runner::RuntimeParameters::isCommunicationThreadEnabled() const {
  return _communicationThread;
}

/**
 * Returns whether a regrid keeps the domain decomposition of the grid construction. A regrid still
 * repartitions when the kept decomposition would place the children of one cell, one of them
 * unrefined, on several ranks.
 */
bool
exahype2::runner::RuntimeParameters::isLoadBalancingDisabled() const {
  return _disableLoadBalancing;
}

bool
exahype2::runner::RuntimeParameters::isVerbose() const {
  return _verbose;
}

double
exahype2::runner::RuntimeParameters::getTimeStepLogInterval() const {
  return _timeStepLogInterval;
}

double
exahype2::runner::RuntimeParameters::getMinTerminalTime() const {
  return _minTerminalTime;
}

double
exahype2::runner::RuntimeParameters::getMaxTerminalTime() const {
  return _maxTerminalTime;
}

int
exahype2::runner::RuntimeParameters::getNumberOfTimeSteps() const {
  return _numberOfTimeSteps;
}

double
exahype2::runner::RuntimeParameters::getFirstPlotTimeStamp() const {
  return _firstPlotTimeStamp;
}

int
exahype2::runner::RuntimeParameters::getNumberOfSnapshots() const {
  return _numberOfSnapshots;
}

bool
exahype2::runner::RuntimeParameters::getPlotDoublePrecision() const {
  return _plotDoublePrecision;
}

tarch::plotter::Format
exahype2::runner::RuntimeParameters::getPlotFormat() const {
  return tarch::plotter::parseFormat(_plotFormat);
}

int
exahype2::runner::RuntimeParameters::getGridLevels() const {
  return _gridLevels;
}

int
exahype2::runner::RuntimeParameters::getMaxGridLevels() const {
  return _maxGridLevels;
}

const std::string&
exahype2::runner::RuntimeParameters::getConfigFilePath() const {
  return _configFilePath;
}

const std::string&
exahype2::runner::RuntimeParameters::getOutputPath() const {
  return _outputPath;
}

const std::string&
exahype2::runner::RuntimeParameters::getProjectName() const {
  return _projectName;
}

const std::string&
exahype2::runner::RuntimeParameters::getExecutableName() const {
  return _executableName;
}

double
exahype2::runner::RuntimeParameters::getMaxAdmissiblePatchSize(
  const tarch::la::Vector<DIMENSIONS, double>& domainSize
) const {
  // The tolerance absorbs round-off, and the longest edge avoids over-refining rectangular domains.
  return divideBySubdivisionFactorPower(1.1 * tarch::la::max(domainSize), _gridLevels);
}

double
exahype2::runner::RuntimeParameters::getMinAdmissiblePatchSize(
  const tarch::la::Vector<DIMENSIONS, double>& domainSize
) const {
  return divideBySubdivisionFactorPower(1.1 * tarch::la::max(domainSize), _maxGridLevels);
}

double
exahype2::runner::RuntimeParameters::getMaxAdmissibleCellSize(
  const tarch::la::Vector<DIMENSIONS, double>& domainSize,
  const int                                    cellsPerPatchPerAxis
) const {
  assertion(cellsPerPatchPerAxis > 0, cellsPerPatchPerAxis);
  return getMaxAdmissiblePatchSize(domainSize) / cellsPerPatchPerAxis;
}
