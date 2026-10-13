// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "CSVReaderTest.h"

#include "tarch/Termination.h"
#include "tarch/reader/CSVReader.h"

namespace {
  std::string writeTempFile(const std::string& stem, const std::string& content) {
    // The file name contains a per-run identifier. Two processes never write the same fixture.
    static const std::string testRunIdentifier =
      std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()) + "-"
      + std::to_string(std::random_device{}());
    static std::atomic<int> counter = 0;

    const std::filesystem::path path = tarch::tests::getWritableTestDirectory()
      / std::filesystem::path(stem + "-" + testRunIdentifier + "-" + std::to_string(counter++) + ".csv");

    std::ofstream output(path);
    if (not output.is_open()) {
      tarch::terminateWithError("Failed to create temporary file \"" + path.string() + "\".");
    }
    output << content;
    return path.string();
  }

  bool throwsRuntimeError(const std::function<void()>& call) {
    try {
      call();
    } catch (const std::runtime_error&) {
      return true;
    }
    return false;
  }
} // namespace

tarch::reader::tests::CSVReaderTest::CSVReaderTest() :
  tarch::tests::TestCase("tarch::reader::tests::CSVReaderTest") {}

void
tarch::reader::tests::CSVReaderTest::run() {
  testMethod(testTheReaderKeepsTheRowsAfterTheSkippedLines);
  testMethod(testEveryEntryOfARowParses);
  testMethod(testAMissingFileThrows);
}

void
tarch::reader::tests::CSVReaderTest::testTheReaderKeepsTheRowsAfterTheSkippedLines() {
  const std::string path = writeTempFile("peano-csv-reader-rows", "x;y\n1;2.5\n3;4.5;\n");

  {
    const tarch::reader::CSVReader reader(path, 1);
    validateEquals(reader.getNumberOfRows(), 2);
    validateEquals(reader.getRow(0), std::string("1;2.5"));
    validateEquals(reader.getRow(1), std::string("3;4.5;"));
    validate(throwsRuntimeError([&reader]() { static_cast<void>(reader.getRow(2)); }));
    validate(throwsRuntimeError([&reader]() { static_cast<void>(reader.getRow(-1)); }));
  }

  std::filesystem::remove(path);
}

/**
 * Every entry of a row parses, whether or not a separator follows it.
 */
void
tarch::reader::tests::CSVReaderTest::testEveryEntryOfARowParses() {
  using Reader = tarch::reader::CSVReader;

  validateEquals(Reader::extractFundamentalTypeFromRow<int>(';', 0, "1;2.5;7"), 1);
  validateNumericalEquals(Reader::extractFundamentalTypeFromRow<double>(';', 1, "1;2.5;7"), 2.5);
  validateEquals(Reader::extractFundamentalTypeFromRow<int>(';', 2, "1;2.5;7"), 7);
  validateEquals(Reader::extractFundamentalTypeFromRow<int>(';', 2, "1;2.5;7;"), 7);

  validate(throwsRuntimeError([]() {
    static_cast<void>(Reader::extractFundamentalTypeFromRow<int>(';', 3, "1;2.5;7"));
  }));
  validate(throwsRuntimeError([]() {
    static_cast<void>(Reader::extractFundamentalTypeFromRow<int>(';', 3, "1;2.5;7;"));
  }));
  validate(throwsRuntimeError([]() {
    static_cast<void>(Reader::extractFundamentalTypeFromRow<double>(';', 0, "a;1"));
  }));
}

void
tarch::reader::tests::CSVReaderTest::testAMissingFileThrows() {
  const std::string path = (tarch::tests::getWritableTestDirectory() / "peano-csv-reader-missing.csv").string();
  validate(throwsRuntimeError([&path]() { const tarch::reader::CSVReader reader(path); }));
}
