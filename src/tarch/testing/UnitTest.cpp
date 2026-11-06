// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "UnitTest.h"

#include "tarch/Assertions.h"
#include "tarch/mpi/Collectives.h"
#include "tarch/mpi/Diagnostics.h"
#include "tarch/mpi/Rank.h"
#include "tarch/tasking/Runtime.h"

#include <unistd.h>

namespace {
  /**
   * One unit test and the path of its file below src/unittests without the extension. The report
   * groups the unit tests of a file by this path.
   */
  struct RegisteredUnitTest final {
    std::string file{};
    std::string description{};
    int         line{0};
    void (*run)(){nullptr};
  };

  /**
   * Counts the checks of one unit test and stores the descriptions of its failed checks. The threads
   * that work for the unit test write into the same record.
   */
  struct CheckRecord final {
    std::atomic<unsigned long long> checks{0};
    std::mutex                      failuresMutex{};
    std::vector<std::string>        failures{};
  };

  /**
   * One failure description and the ranks that report it.
   */
  struct Failure final {
    std::string      description{};
    std::vector<int> ranks{};
  };

  /**
   * Result of one unit test summed over all ranks.
   */
  struct UnitTestResult final {
    std::string          description{};
    double               seconds{0.0};
    unsigned long long   checks{0};
    unsigned long long   failedChecks{0};
    std::vector<Failure> failures{};
  };

  /**
   * Results of the unit tests of one file.
   */
  struct TestFileResult final {
    std::string_view            file{};
    double                      seconds{0.0};
    std::vector<UnitTestResult> unitTests{};
  };

  struct CommandLine final {
    std::vector<std::string> filters{};
    bool                     listUnitTests{false};
    std::string              junitFile{};
  };

  std::vector<RegisteredUnitTest>& getRegisteredUnitTests() {
    static std::vector<RegisteredUnitTest> registeredUnitTests;
    return registeredUnitTests;
  }

  std::atomic<CheckRecord*> activeCheckRecord{nullptr};

  /**
   * Chooses for every run of a unit test the first incomplete block of each nesting level. Only the
   * thread that runs the unit test enters and leaves blocks. Every thread may record a failure.
   */
  class SectionTracker final {
  public:
    void startRun();

    [[nodiscard]] bool enter(std::string_view kind, std::string_view description, int line);

    void leave(bool unwinding);

    void recordFailure();

    [[nodiscard]] int getFailureLevel() const;

    [[nodiscard]] std::string getLabels() const;

    [[nodiscard]] std::string getLabelsAtFailure() const;

    [[nodiscard]] bool finishRun(int failureLevel);

  private:
    std::set<std::string>    _completedPaths{};
    std::vector<std::string> _enteredPath{};
    std::vector<std::string> _labels{};
    std::string              _labelsAtFailure{};
    std::vector<bool>        _childEntered{};
    std::vector<bool>        _childSkipped{};
    std::size_t              _openBlocks{0};
    int                      _failureLevel{-1};
    mutable std::mutex       _mutex{};

    [[nodiscard]] std::string getPathKey(std::size_t length) const;

    [[nodiscard]] std::string getJoinedLabels() const;
  };

  std::atomic<SectionTracker*> activeSectionTracker{nullptr};

  SectionTracker& getActiveSectionTracker() {
    SectionTracker* const tracker = activeSectionTracker.load(std::memory_order_acquire);
    ASSERTION(tracker != nullptr);
    return *tracker;
  }

  void SectionTracker::startRun() {
    const std::scoped_lock lock(_mutex);
    _enteredPath.clear();
    _labels.clear();
    _labelsAtFailure.clear();
    _childEntered.assign(1, false);
    _childSkipped.assign(1, false);
    _openBlocks   = 0;
    _failureLevel = -1;
  }

  /**
   * Enters the block unless the path to it is complete or the run entered another block of the
   * same level. A block skipped for another one leaves its enclosing block incomplete.
   */
  bool SectionTracker::enter(const std::string_view kind, const std::string_view description, const int line) {
    const std::scoped_lock lock(_mutex);
    const std::size_t      level = _openBlocks;
    ASSERTION(level < _childEntered.size() and level < _childSkipped.size(), level, _childEntered.size());
    const std::string identity = "\n" + std::to_string(line) + " " + std::string(kind) + " " + std::string(description);
    if (_completedPaths.contains(getPathKey(level) + identity)) {
      return false;
    }
    if (_childEntered[level]) {
      _childSkipped[level] = true;
      return false;
    }
    ASSERTION(level == _enteredPath.size(), level, _enteredPath.size());
    _childEntered[level] = true;
    _enteredPath.push_back(identity);
    _labels.push_back(std::string(kind) + ": " + std::string(description));
    _childEntered.resize(level + 2);
    _childSkipped.resize(level + 2);
    _childEntered[level + 1] = false;
    _childSkipped[level + 1] = false;
    _openBlocks++;
    return true;
  }

  /**
   * Leaves the innermost open block. An exception that leaves the block marks the failure of the
   * run at this block unless a failure came first.
   */
  void SectionTracker::leave(const bool unwinding) {
    const std::scoped_lock lock(_mutex);
    ASSERTION(_openBlocks > 0 and _openBlocks == _labels.size(), _openBlocks, _labels.size());
    if (unwinding and _failureLevel < 0) {
      _failureLevel    = static_cast<int>(_openBlocks);
      _labelsAtFailure = getJoinedLabels();
    }
    _openBlocks--;
    _labels.pop_back();
  }

  void SectionTracker::recordFailure() {
    const std::scoped_lock lock(_mutex);
    if (_failureLevel < 0) {
      _failureLevel    = static_cast<int>(_openBlocks);
      _labelsAtFailure = getJoinedLabels();
    }
  }

  int SectionTracker::getFailureLevel() const {
    const std::scoped_lock lock(_mutex);
    return _failureLevel;
  }

  std::string SectionTracker::getLabels() const {
    const std::scoped_lock lock(_mutex);
    return getJoinedLabels();
  }

  std::string SectionTracker::getLabelsAtFailure() const {
    const std::scoped_lock lock(_mutex);
    return _labelsAtFailure;
  }

  /**
   * Marks the blocks that the run completes and returns whether the unit test is complete.
   * failureLevel is the smallest failure level of all ranks. This minimum makes every rank enter the
   * same blocks in the next run. failureLevel equals std::numeric_limits<int>::max() when every rank
   * passed the run. After such a run, the entered blocks complete from the innermost outwards until
   * the first block that skipped an inner block. A failure completes the innermost block open at the
   * failure. A failure outside every block completes the unit test.
   */
  bool SectionTracker::finishRun(const int failureLevel) {
    const std::scoped_lock lock(_mutex);
    ASSERTION(_openBlocks == 0, _openBlocks);
    ASSERTION(failureLevel >= 0, failureLevel);
    [[maybe_unused]] const std::size_t completedPathsBefore = _completedPaths.size();
    bool                               complete             = false;
    if (failureLevel == 0) {
      complete = true;
    } else if (failureLevel != std::numeric_limits<int>::max()) {
      ASSERTION(std::cmp_less_equal(failureLevel, _enteredPath.size()), failureLevel, _enteredPath.size());
      _completedPaths.insert(getPathKey(static_cast<std::size_t>(failureLevel)));
    } else {
      std::size_t length = _enteredPath.size();
      ASSERTION(length < _childSkipped.size(), length, _childSkipped.size());
      while (length > 0 and not _childSkipped[length]) {
        _completedPaths.insert(getPathKey(length));
        length--;
      }
      complete = length == 0 and not _childSkipped[0];
    }
    ASSERTION(complete or _completedPaths.size() > completedPathsBefore, failureLevel, _enteredPath.size());
    return complete;
  }

  std::string SectionTracker::getPathKey(const std::size_t length) const {
    ASSERTION(length <= _enteredPath.size(), length, _enteredPath.size());
    std::string key;
    for (std::size_t index = 0; index < length; index++) {
      key += _enteredPath[index];
    }
    return key;
  }

  std::string SectionTracker::getJoinedLabels() const {
    std::string joinedLabels;
    for (const std::string& label : _labels) {
      joinedLabels += joinedLabels.empty() ? label : "\n" + label;
    }
    return joinedLabels;
  }

  /**
   * Returns whether text matches pattern. A * in pattern matches any sequence of characters and a ?
   * one character.
   */
  bool matchesPattern(const std::string_view text, const std::string_view pattern) {
    std::size_t textIndex     = 0;
    std::size_t patternIndex  = 0;
    std::size_t starIndex     = std::string_view::npos;
    std::size_t starTextIndex = 0;
    while (textIndex < text.size()) {
      if (patternIndex < pattern.size() and pattern[patternIndex] == '*') {
        starIndex     = patternIndex;
        starTextIndex = textIndex;
        patternIndex++;
      } else if (
        patternIndex < pattern.size() and (pattern[patternIndex] == '?' or pattern[patternIndex] == text[textIndex])
      ) {
        textIndex++;
        patternIndex++;
      } else if (starIndex != std::string_view::npos) {
        starTextIndex++;
        textIndex    = starTextIndex;
        patternIndex = starIndex + 1;
      } else {
        return false;
      }
    }
    while (patternIndex < pattern.size() and pattern[patternIndex] == '*') {
      patternIndex++;
    }
    return patternIndex == pattern.size();
  }

  std::string_view trim(const std::string_view text) {
    const std::size_t begin = text.find_first_not_of(' ');
    if (begin == std::string_view::npos) {
      return {};
    }
    return text.substr(begin, text.find_last_not_of(' ') - begin + 1);
  }

  /**
   * Splits the comma-separated expressions of a check. A comma inside brackets or inside a literal
   * belongs to its expression.
   */
  std::vector<std::string_view> splitExpressions(const std::string_view expressions) {
    std::vector<std::string_view> result;
    int                           depth        = 0;
    char                          literalQuote = '\0';
    bool                          escaped      = false;
    std::size_t                   begin        = 0;
    for (std::size_t index = 0; index < expressions.size(); index++) {
      const char character = expressions[index];
      if (literalQuote != '\0') {
        if (escaped) {
          escaped = false;
        } else if (character == '\\') {
          escaped = true;
        } else if (character == literalQuote) {
          literalQuote = '\0';
        }
      } else if (character == '"' or character == '\'') {
        literalQuote = character;
      } else if (character == '(' or character == '[' or character == '{') {
        depth++;
      } else if (character == ')' or character == ']' or character == '}') {
        depth--;
      } else if (character == ',' and depth == 0) {
        result.push_back(trim(expressions.substr(begin, index - begin)));
        begin = index + 1;
      }
    }
    if (not trim(expressions).empty()) {
      result.push_back(trim(expressions.substr(begin)));
    }
    return result;
  }

  /**
   * Returns whether expression is an integer or floating-point literal, such as -1, 0.5F or 1e-10.
   */
  bool isNumericLiteral(const std::string_view expression) {
    const std::string_view number = expression.starts_with('-') ? expression.substr(1) : expression;
    if (number.empty() or std::isdigit(static_cast<unsigned char>(number.front())) == 0) {
      return false;
    }
    for (std::size_t index = 1; index < number.size(); index++) {
      const char character = number[index];
      const bool exponentSign =
        (character == '-' or character == '+') and (number[index - 1] == 'e' or number[index - 1] == 'E');
      if (
        std::isalnum(static_cast<unsigned char>(character)) == 0 and character != '.' and character != '\''
        and not exponentSign
      ) {
        return false;
      }
    }
    return true;
  }

  /**
   * Returns the path of the file of location from its src directory on.
   */
  std::string_view getSourcePath(const std::source_location& location) {
    const std::string_view path     = location.file_name();
    const std::size_t      position = path.rfind("/src/");
    return position == std::string_view::npos ? path : path.substr(position + 1);
  }

  /**
   * Returns the path of the file of location below its unittests directory without the extension.
   */
  std::string getTestFile(const std::source_location& location) {
    constexpr std::string_view Directory = "/unittests/";
    std::string_view           path      = location.file_name();
    const std::size_t          position  = path.rfind(Directory);
    if (position != std::string_view::npos) {
      path.remove_prefix(position + Directory.size());
    }
    return std::filesystem::path(path).replace_extension().string();
  }

  std::string getCountText(
    const unsigned long long count,
    const std::string_view   singular,
    const std::string_view   plural
  ) {
    return std::to_string(count) + " " + std::string(count == 1 ? singular : plural);
  }

  std::string getRankText(const std::vector<int>& ranks, const int numberOfRanks) {
    ASSERTION(not ranks.empty(), numberOfRanks);
    if (numberOfRanks == 1) {
      return "";
    }
    if (std::cmp_equal(ranks.size(), numberOfRanks)) {
      return " on all " + std::to_string(numberOfRanks) + " ranks";
    }
    std::string text = ranks.size() == 1 ? " on rank " : " on ranks ";
    for (std::size_t index = 0; index < ranks.size(); index++) {
      if (index > 0) {
        text += index + 1 == ranks.size() ? " and " : ", ";
      }
      text += std::to_string(ranks[index]);
    }
    return text + " of " + std::to_string(numberOfRanks);
  }

  std::string escapeXML(const std::string_view text) {
    std::string result;
    for (const char character : text) {
      switch (character) {
      case '&':
        result += "&amp;";
        break;
      case '<':
        result += "&lt;";
        break;
      case '>':
        result += "&gt;";
        break;
      case '"':
        result += "&quot;";
        break;
      case '\'':
        result += "&apos;";
        break;
      default:
        if (static_cast<unsigned char>(character) >= ' ' or character == '\n' or character == '\t') {
          result += character;
        }
      }
    }
    return result;
  }

  std::optional<CommandLine> parseCommandLine(const std::span<const char* const> arguments) {
    ASSERTION(not arguments.empty());
    CommandLine commandLine;
    for (std::size_t index = 1; index < arguments.size(); index++) {
      const std::string_view argument = arguments[index];
      const bool             hasValue = index + 1 < arguments.size();
      if (argument == "--list") {
        commandLine.listUnitTests = true;
      } else if (argument == "--filter" and hasValue) {
        index++;
        commandLine.filters.emplace_back(arguments[index]);
      } else if (argument == "--junit" and hasValue) {
        index++;
        commandLine.junitFile = arguments[index];
      } else {
        return std::nullopt;
      }
    }
    return commandLine;
  }

  /**
   * Runs the selected unit tests file by file in the same order on every rank. After each unit test,
   * every rank receives the failures of all ranks and rank 0 prints them.
   */
  class TestRunner final {
  public:
    TestRunner(CommandLine commandLine, std::string_view programName);
    TestRunner(const TestRunner& other)            = delete;
    TestRunner(TestRunner&& other)                 = delete;
    TestRunner& operator=(const TestRunner& other) = delete;
    TestRunner& operator=(TestRunner&& other)      = delete;
    ~TestRunner();

    void runTestFile(std::span<const RegisteredUnitTest> unitTests);

    [[nodiscard]] int finish() const;

  private:
    CommandLine                           _commandLine{};
    std::string                           _programName{};
    MPI_Comm                              _communicator{MPI_COMM_NULL};
    int                                   _rank{0};
    int                                   _numberOfRanks{1};
    std::size_t                           _nameWidth{0};
    std::chrono::steady_clock::time_point _start{std::chrono::steady_clock::now()};
    std::vector<TestFileResult>           _results{};
    int                                   _listedTestFiles{0};
    int                                   _listedUnitTests{0};

    [[nodiscard]] bool isSelected(std::string_view fullName) const;

    [[nodiscard]] UnitTestResult runUnitTest(const RegisteredUnitTest& unitTest);

    [[nodiscard]] UnitTestResult reduce(std::string_view description, double seconds, CheckRecord& record) const;

    void printTestFile(const TestFileResult& result) const;

    void writeJUnitFile(double seconds) const;
  };

  /**
   * Returns the label of a unit test in the report.
   */
  std::string getUnitTestName(const UnitTestResult& unitTest) { return "Unit test \"" + unitTest.description + "\""; }

  void recordException(CheckRecord& record, const std::string_view origin, const char* const message) {
    const std::scoped_lock lock(record.failuresMutex);
    if (message == nullptr) {
      record.failures.push_back(std::string(origin) + " threw an exception of an unknown type");
    } else {
      record.failures.push_back(std::string(origin) + " threw an exception with the message \"" + message + "\"");
    }
  }

  TestRunner::TestRunner(CommandLine commandLine, const std::string_view programName) :
    _commandLine(std::move(commandLine)),
    _programName(programName) {
    const int result = MPI_Comm_dup(tarch::mpi::Rank::getInstance().getCommunicator(), &_communicator);
    tarch::mpi::checkReturnValue(result, "MPI_Comm_dup", std::source_location::current());
    _rank          = tarch::mpi::Rank::getInstance().getRank();
    _numberOfRanks = tarch::mpi::Rank::getInstance().getNumberOfRanks();
    for (const RegisteredUnitTest& registeredUnitTest : getRegisteredUnitTests()) {
      _nameWidth = std::max(_nameWidth, registeredUnitTest.file.size());
    }
    ASSERTION(_rank >= 0 and _rank < _numberOfRanks, _rank, _numberOfRanks);

    if (_rank == 0 and not _commandLine.listUnitTests) {
      std::cout << "Unit tests of " << _programName << " on " << _numberOfRanks
                << (_numberOfRanks == 1 ? " rank with " : " ranks with ")
                << tarch::tasking::Runtime::getInstance().getNumberOfThreads() << " threads per rank";
      for (std::size_t index = 0; index < _commandLine.filters.size(); index++) {
        std::cout << (index == 0 ? ", filtered by " : " or ") << _commandLine.filters[index];
      }
      std::cout << std::endl;
    }
  }

  TestRunner::~TestRunner() { MPI_Comm_free(&_communicator); }

  bool TestRunner::isSelected(const std::string_view fullName) const {
    return _commandLine.filters.empty()
      or std::ranges::any_of(_commandLine.filters, [fullName](const std::string& filter) {
             return matchesPattern(fullName, filter);
           });
  }

  /**
   * Runs the selected unit tests of one file and prints them as one entry of the report.
   */
  void TestRunner::runTestFile(const std::span<const RegisteredUnitTest> unitTests) {
    ASSERTION(not unitTests.empty());
    TestFileResult result{.file = unitTests.front().file};
    const int      listedUnitTests = _listedUnitTests;
    const auto     start           = std::chrono::steady_clock::now();
    for (const RegisteredUnitTest& unitTest : unitTests) {
      ASSERTION(unitTest.file == unitTests.front().file, unitTest.file, unitTests.front().file);
      const std::string fullName = unitTest.file + "/" + unitTest.description;
      if (not isSelected(fullName)) {
        continue;
      }
      if (_commandLine.listUnitTests) {
        if (_rank == 0) {
          std::cout << fullName << '\n';
        }
        _listedUnitTests++;
        continue;
      }
      result.unitTests.push_back(runUnitTest(unitTest));
    }
    _listedTestFiles += _listedUnitTests > listedUnitTests ? 1 : 0;
    // The filter selects the same unit tests on every rank. Every rank takes this exit.
    if (_commandLine.listUnitTests or result.unitTests.empty()) {
      return;
    }
    result.seconds = std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count();
    if (_rank == 0) {
      printTestFile(result);
    }
    _results.push_back(std::move(result));
  }

  std::string getUnitTestOrigin(const SectionTracker& tracker) {
    const std::string labels = tracker.getLabelsAtFailure();
    return labels.empty() ? "The unit test" : labels + "\nThe unit test";
  }

  /**
   * Runs the unit test until its runs have entered every innermost block once. After each run, every
   * rank receives the smallest failure level of all ranks. Every rank then enters the same blocks in
   * the next run.
   */
  UnitTestResult TestRunner::runUnitTest(const RegisteredUnitTest& unitTest) {
    ASSERTION(unitTest.run != nullptr, unitTest.description);
    CheckRecord    record;
    SectionTracker tracker;
    activeCheckRecord.store(&record, std::memory_order_release);
    activeSectionTracker.store(&tracker, std::memory_order_release);
    const auto start    = std::chrono::steady_clock::now();
    bool       complete = false;
    while (not complete) {
      tracker.startRun();
      try {
        unitTest.run();
      } catch (const std::exception& exception) {
        tracker.recordFailure();
        recordException(record, getUnitTestOrigin(tracker), exception.what());
      } catch (...) {
        tracker.recordFailure();
        recordException(record, getUnitTestOrigin(tracker), nullptr);
      }
      const int localFailureLevel  = tracker.getFailureLevel();
      const int failureLevel       = localFailureLevel < 0 ? std::numeric_limits<int>::max() : localFailureLevel;
      int       globalFailureLevel = 0;
      tarch::mpi::allreduce(&failureLevel, &globalFailureLevel, 1, MPI_INT, MPI_MIN, _communicator);
      complete = tracker.finishRun(globalFailureLevel);
    }
    const double seconds = std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count();
    activeSectionTracker.store(nullptr, std::memory_order_release);
    activeCheckRecord.store(nullptr, std::memory_order_release);
    return reduce(unitTest.description, seconds, record);
  }

  /**
   * Sums the checks of record over all ranks. When a check failed on any rank, every rank
   * receives the failure descriptions of all ranks. The result lists each description once with
   * the ranks that report it.
   */
  UnitTestResult TestRunner::reduce(
    const std::string_view description,
    const double           seconds,
    CheckRecord&           record
  ) const {
    std::vector<std::string> localFailures;
    {
      const std::scoped_lock lock(record.failuresMutex);
      localFailures = std::move(record.failures);
    }
    const std::array<unsigned long long, 2> localCounts{
      record.checks.load(std::memory_order_relaxed),
      static_cast<unsigned long long>(localFailures.size())
    };
    std::array<unsigned long long, 2> globalCounts{0, 0};
    tarch::mpi::allreduce(
      localCounts.data(),
      globalCounts.data(),
      static_cast<int>(localCounts.size()),
      MPI_UNSIGNED_LONG_LONG,
      MPI_SUM,
      _communicator
    );
    UnitTestResult result{
      .description  = std::string(description),
      .seconds      = seconds,
      .checks       = globalCounts[0],
      .failedChecks = globalCounts[1]
    };
    ASSERTION(
      result.checks >= localCounts[0] and result.failedChecks >= localCounts[1],
      result.checks,
      result.failedChecks
    );
    if (result.failedChecks == 0) {
      return result;
    }

    std::string localText;
    for (const std::string& failure : localFailures) {
      localText += failure;
      localText += '\0';
    }
    const int        localLength = static_cast<int>(localText.size());
    std::vector<int> lengths(_numberOfRanks, 0);
    tarch::mpi::allgather(&localLength, 1, MPI_INT, lengths.data(), 1, MPI_INT, _communicator);
    std::vector<int> offsets(_numberOfRanks, 0);
    std::exclusive_scan(lengths.begin(), lengths.end(), offsets.begin(), 0);
    std::string allText(static_cast<std::size_t>(offsets.back() + lengths.back()), '\0');
    tarch::mpi::allgatherv(
      localText.data(),
      localLength,
      MPI_CHAR,
      allText.data(),
      lengths.data(),
      offsets.data(),
      MPI_CHAR,
      _communicator
    );

    for (int rank = 0; rank < _numberOfRanks; rank++) {
      ASSERTION(std::cmp_less_equal(offsets[rank] + lengths[rank], allText.size()), rank, offsets[rank], lengths[rank]);
      std::string_view rankText = std::string_view(allText).substr(offsets[rank], lengths[rank]);
      while (not rankText.empty()) {
        const std::size_t end = rankText.find('\0');
        ASSERTION(end != std::string_view::npos, rank);
        const std::string_view description = rankText.substr(0, end);
        rankText.remove_prefix(end + 1);
        const auto failure = std::ranges::find(result.failures, description, &Failure::description);
        if (failure == result.failures.end()) {
          result.failures.push_back(Failure{.description = std::string(description), .ranks = {rank}});
        } else if (failure->ranks.back() != rank) {
          failure->ranks.push_back(rank);
        }
      }
    }
    return result;
  }

  void TestRunner::printTestFile(const TestFileResult& result) const {
    ASSERTION(_rank == 0, _rank);
    ASSERTION(not result.unitTests.empty(), result.file);
    unsigned long long checks = 0;
    bool               passed = true;
    for (const UnitTestResult& unitTest : result.unitTests) {
      checks += unitTest.checks;
      passed = passed and unitTest.failedChecks == 0;
    }
    std::cout << (passed ? "Passed" : "Failed") << "  " << std::left << std::setw(static_cast<int>(_nameWidth))
              << result.file << ' ' << std::right << std::setw(4) << result.unitTests.size()
              << (result.unitTests.size() == 1 ? " Unit Test " : " Unit Tests") << ' ' << std::setw(9) << checks
              << (checks == 1 ? " Check " : " Checks") << ' ' << std::setw(9) << std::fixed << std::setprecision(1)
              << result.seconds * 1000.0 << " ms" << std::defaultfloat << '\n';

    if (checks > 0) {
      for (const UnitTestResult& unitTest : result.unitTests) {
        if (unitTest.checks == 0 and unitTest.failedChecks == 0) {
          std::cout << "    " << getUnitTestName(unitTest) << " ran zero checks\n";
        }
      }
    }
    for (const UnitTestResult& unitTest : result.unitTests) {
      for (const Failure& failure : unitTest.failures) {
        std::cout << "    " << getUnitTestName(unitTest) << " failed" << getRankText(failure.ranks, _numberOfRanks)
                  << '\n';
        std::string_view description = failure.description;
        while (not description.empty()) {
          const std::size_t end = std::min(description.find('\n'), description.size());
          std::cout << "      " << description.substr(0, end) << '\n';
          description.remove_prefix(std::min(end + 1, description.size()));
        }
      }
    }
    std::cout << std::flush;
  }

  void TestRunner::writeJUnitFile(const double seconds) const {
    ASSERTION(_rank == 0, _rank);
    ASSERTION(not _commandLine.junitFile.empty());
    std::ofstream file(_commandLine.junitFile);
    if (not file.good()) {
      std::cerr << "Cannot write the JUnit report " << _commandLine.junitFile << std::endl;
      return;
    }
    std::size_t unitTests       = 0;
    std::size_t failedUnitTests = 0;
    for (const TestFileResult& result : _results) {
      unitTests += result.unitTests.size();
      failedUnitTests +=
        static_cast<std::size_t>(std::ranges::count_if(result.unitTests, [](const UnitTestResult& unitTest) {
          return unitTest.failedChecks > 0;
        }));
    }
    const std::string runName =
      _numberOfRanks == 1 ? _programName : _programName + "-" + std::to_string(_numberOfRanks) + "Ranks";
    file << std::fixed << std::setprecision(6) << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
         << "<testsuites name=\"" << escapeXML(runName) << "\" tests=\"" << unitTests << "\" failures=\""
         << failedUnitTests << "\" time=\"" << seconds << "\">\n";
    for (const TestFileResult& result : _results) {
      const auto        failures  = std::ranges::count_if(result.unitTests, [](const UnitTestResult& unitTest) {
        return unitTest.failedChecks > 0;
      });
      const std::string suiteName = escapeXML(runName + "/" + std::string(result.file));
      file << "  <testsuite name=\"" << suiteName << "\" tests=\"" << result.unitTests.size() << "\" failures=\""
           << failures << "\" time=\"" << result.seconds << "\">\n";
      for (const UnitTestResult& unitTest : result.unitTests) {
        file << "    <testcase classname=\"" << suiteName << "\" name=\"" << escapeXML(unitTest.description)
             << "\" time=\"" << unitTest.seconds << "\"";
        if (unitTest.failures.empty()) {
          file << "/>\n";
          continue;
        }
        file << ">\n";
        for (const Failure& failure : unitTest.failures) {
          file << "      <failure message=\""
               << escapeXML(getUnitTestName(unitTest) + " failed" + getRankText(failure.ranks, _numberOfRanks)) << "\">"
               << escapeXML(failure.description) << "</failure>\n";
        }
        file << "    </testcase>\n";
      }
      file << "  </testsuite>\n";
    }
    file << "</testsuites>\n";
  }

  std::string getInventoryText(const std::size_t testFiles, const std::size_t unitTests) {
    return getCountText(testFiles, "test file", "test files") + " with "
      + getCountText(unitTests, "unit test", "unit tests");
  }

  int TestRunner::finish() const {
    const double seconds = std::chrono::duration<double>(std::chrono::steady_clock::now() - _start).count();
    if (_commandLine.listUnitTests) {
      if (_rank == 0) {
        std::cout << getInventoryText(
          static_cast<std::size_t>(_listedTestFiles),
          static_cast<std::size_t>(_listedUnitTests)
        ) << std::endl;
      }
      return EXIT_SUCCESS;
    }

    std::size_t              unitTests = 0;
    unsigned long long       checks    = 0;
    std::vector<std::string> failures;
    for (const TestFileResult& result : _results) {
      unitTests += result.unitTests.size();
      for (const UnitTestResult& unitTest : result.unitTests) {
        checks += unitTest.checks;
        if (unitTest.failedChecks > 0) {
          failures.push_back(std::string(result.file) + "/" + unitTest.description);
        }
      }
    }

    if (_rank == 0) {
      std::cout << "Summary: " << getInventoryText(_results.size(), unitTests) << " and "
                << getCountText(checks, "check", "checks") << " in " << std::fixed << std::setprecision(2) << seconds
                << " s, " << std::defaultfloat;
      if (failures.empty()) {
        std::cout << "all passed\n";
      } else {
        std::cout << failures.size() << " failed\n";
        for (const std::string& failure : failures) {
          std::cout << "  " << failure << '\n';
        }
      }
      std::cout << std::flush;
      if (not _commandLine.junitFile.empty()) {
        writeJUnitFile(seconds);
      }
    }
    return failures.empty() ? EXIT_SUCCESS : EXIT_FAILURE;
  }

  std::filesystem::path getFirstWritableDirectory() {
    std::error_code             errorCode;
    const std::filesystem::path candidates[] = {
      std::filesystem::current_path(errorCode),
      std::filesystem::temp_directory_path(errorCode)
    };

    for (const auto& base : candidates) {
      if (base.empty()) {
        continue;
      }
      // Each process probes a file of its own. Concurrent ranks do not race for the probe.
      const std::filesystem::path probe = base / (".peano-write-probe-" + std::to_string(::getpid()));
      std::ofstream               stream(probe);
      if (stream.good()) {
        stream.close();
        std::filesystem::remove(probe, errorCode);
        return base;
      }
    }
    return std::filesystem::current_path(errorCode);
  }
} // namespace

/**
 * Returns the first of the working directory and the system temporary directory in which the
 * process can create a file. Without such a directory the function returns the working directory.
 */
std::filesystem::path
tarch::testing::getWritableTestDirectory() {
  static const std::filesystem::path directory = getFirstWritableDirectory();
  return directory;
}

/**
 * Returns MaxUnitsInTheLastPlace units in the last place of the largest magnitude of reference.
 */
double
tarch::testing::getToleranceInUnitsInTheLastPlace(const std::span<const double> reference) {
  double largestMagnitude = 0.0;
  for (const double value : reference) {
    largestMagnitude = std::max(largestMagnitude, std::abs(value));
  }
  const double unitInTheLastPlace =
    std::nextafter(largestMagnitude, std::numeric_limits<double>::infinity()) - largestMagnitude;
  // A compiler that flushes subnormal results to zero turns a subnormal tolerance into an exact
  // comparison. The smallest normal double is the floor.
  const double tolerance = MaxUnitsInTheLastPlace * std::max(unitInTheLastPlace, std::numeric_limits<double>::min());
  ASSERTION(tolerance > 0.0, tolerance, largestMagnitude);
  return tolerance;
}

/**
 * Adds a unit test to the registry that runTests() works through. The path of the file of location
 * below its unittests directory groups the unit tests of that file.
 */
bool
tarch::testing::registerUnitTest(
  const std::string&          description,
  const std::source_location& location,
  void (*const unitTest)()
) {
  ASSERTION(not description.empty());
  ASSERTION(unitTest != nullptr, description);
  getRegisteredUnitTests().push_back(
    RegisteredUnitTest{
      .file        = getTestFile(location),
      .description = description,
      .line        = static_cast<int>(location.line()),
      .run         = unitTest
    }
  );
  return true;
}

/**
 * Enters the block when the current run of the unit test takes it.
 */
tarch::testing::Section::Section(
  const std::string_view      kind,
  const std::string_view      description,
  const std::source_location& location
) :
  _entered(getActiveSectionTracker().enter(kind, description, static_cast<int>(location.line()))) {}

/**
 * Leaves an entered block. An exception that leaves the block marks the failure of the run.
 */
tarch::testing::Section::~Section() {
  if (_entered) {
    getActiveSectionTracker().leave(std::uncaught_exceptions() > _uncaughtExceptions);
  }
}

bool
tarch::testing::Section::isEntered() const {
  return _entered;
}

/**
 * Runs the unit tests that the command line selects file by file in the order of their lines.
 * Returns EXIT_SUCCESS on every rank when every check passed on every rank. Every rank calls the function.
 */
int
tarch::testing::runTests(const int argc, const char* const* const argv) {
  ASSERTION(argc >= 1, argc);
  ASSERTION(argv != nullptr);
  const std::span<const char* const> arguments(argv, static_cast<std::size_t>(argc));
  const std::string                  programName = std::filesystem::path(arguments.front()).filename().string();
  const std::optional<CommandLine>   commandLine = parseCommandLine(arguments);
  if (not commandLine.has_value()) {
    if (tarch::mpi::Rank::getInstance().getRank() == 0) {
      std::cerr << "Usage: " << programName << " [--filter pattern]... [--list] [--junit file]" << std::endl;
    }
    return EXIT_FAILURE;
  }

  std::vector<RegisteredUnitTest>& registeredUnitTests = getRegisteredUnitTests();
  std::ranges::sort(registeredUnitTests, [](const RegisteredUnitTest& left, const RegisteredUnitTest& right) {
    return std::tie(left.file, left.line) < std::tie(right.file, right.line);
  });

  TestRunner testRunner(*commandLine, programName);
  auto       unitTests = std::span<const RegisteredUnitTest>(registeredUnitTests);
  while (not unitTests.empty()) {
    const auto fileEnd         = std::ranges::find_if(unitTests, [&unitTests](const RegisteredUnitTest& unitTest) {
      return unitTest.file != unitTests.front().file;
    });
    const auto unitTestsOfFile = static_cast<std::size_t>(fileEnd - unitTests.begin());
    testRunner.runTestFile(unitTests.first(unitTestsOfFile));
    unitTests = unitTests.subspan(unitTestsOfFile);
  }
  return testRunner.finish();
}

/**
 * Counts one check of the running unit test and returns an empty description when condition is
 * false.
 */
std::optional<std::string>
tarch::testing::checkCondition(const bool condition) {
  CheckRecord* const record = activeCheckRecord.load(std::memory_order_acquire);
  ASSERTION(record != nullptr, condition);
  record->checks.fetch_add(1, std::memory_order_relaxed);
  if (condition) {
    return std::nullopt;
  }
  return std::string();
}

/**
 * Counts one check of the running unit test and describes the first differing entry when the
 * buffers differ in size or in an entry by more than getToleranceInUnitsInTheLastPlace(expected).
 * A NaN in either buffer fails the check.
 */
std::optional<std::string>
tarch::testing::checkNumericalBuffersEquals(
  const std::span<const double> actual,
  const std::span<const double> expected
) {
  if (actual.size() != expected.size()) {
    static_cast<void>(checkCondition(false));
    return "The buffer actual contains " + std::to_string(actual.size()) + " entries and the buffer expected "
      + std::to_string(expected.size());
  }
  const double tolerance       = getToleranceInUnitsInTheLastPlace(expected);
  std::size_t  differences     = 0;
  std::size_t  firstDifference = 0;
  for (std::size_t entry = 0; entry < actual.size(); entry++) {
    if (not(std::abs(actual[entry] - expected[entry]) <= tolerance)) {
      firstDifference = differences == 0 ? entry : firstDifference;
      differences++;
    }
  }
  if (not checkCondition(differences == 0).has_value()) {
    return std::nullopt;
  }
  std::ostringstream description;
  description << std::setprecision(std::numeric_limits<double>::max_digits10) << differences << " of " << actual.size()
              << " entries differ by more than " << tolerance << "\nEntry " << firstDifference << " of actual is "
              << actual[firstDifference] << " and of expected " << expected[firstDifference];
  return description.str();
}

/**
 * Appends the description of a failed check to the running unit test. The description lists the
 * open blocks, the location, the statement, the details and one line per expression of names with
 * its value, except for an expression that spells its own value, such as a literal.
 */
void
tarch::testing::recordFailure(
  const std::source_location&     location,
  const std::string_view          statement,
  const std::string_view          details,
  const std::string_view          names,
  const std::vector<std::string>& values
) {
  std::string description =
    std::string(getSourcePath(location)) + ":" + std::to_string(location.line()) + ": " + std::string(statement);
  if (not details.empty()) {
    description += "\n" + std::string(details);
  }
  const std::vector<std::string_view> expressions = splitExpressions(names);
  if (expressions.size() == values.size()) {
    for (std::size_t index = 0; index < values.size(); index++) {
      const std::string_view expression = expressions[index];
      if (
        expression != values[index] and expression != "\"" + values[index] + "\"" and not isNumericLiteral(expression)
      ) {
        description += "\n" + std::string(expression) + " = " + values[index];
      }
    }
  } else {
    description += "\n" + std::string(names) + " =";
    for (const std::string& value : values) {
      description += " " + value;
    }
  }
  SectionTracker* const tracker = activeSectionTracker.load(std::memory_order_acquire);
  if (tracker != nullptr) {
    tracker->recordFailure();
    const std::string labels = tracker->getLabels();
    description              = labels.empty() ? description : labels + "\n" + description;
  }

  CheckRecord* const record = activeCheckRecord.load(std::memory_order_acquire);
  ASSERTION(record != nullptr, description);
  const std::scoped_lock lock(record->failuresMutex);
  record->failures.push_back(std::move(description));
}
