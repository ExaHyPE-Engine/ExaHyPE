// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "tarch/reader/CSVReader.h"

#include "tarch/Termination.h"
#include "tarch/testing/UnitTest.h"

namespace {
  std::string writeTempFile(const std::string& stem, const std::string& content) {
    // The file name contains a per-run identifier. Two processes never write the same fixture.
    static const std::string testRunIdentifier =
      std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()) + "-"
      + std::to_string(std::random_device{}());
    static std::atomic<int> counter = 0;

    const std::filesystem::path path = tarch::testing::getWritableTestDirectory()
      / std::filesystem::path(stem + "-" + testRunIdentifier + "-" + std::to_string(counter++) + ".csv");

    std::ofstream output(path);
    if (not output.is_open()) {
      tarch::terminateWithError("The test cannot create the temporary file '" + path.string() + "'.");
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

UNIT_TEST("The reader returns the lines behind the skipped header lines as rows") {
  GIVEN("A reader that skips the header line of a file with a header line and two rows") {
    const std::string              path = writeTempFile("peano-csv-reader-rows", "x;y\n1;2.5\n3;4.5;\n");
    const tarch::reader::CSVReader reader(path, 1);
    std::filesystem::remove(path);

    WHEN("The reader returns its rows and a row behind either end") {
      const int         numberOfRows   = reader.getNumberOfRows();
      const std::string firstRow       = reader.getRow(0);
      const std::string secondRow      = reader.getRow(1);
      const bool rowBehindTheEndThrows = throwsRuntimeError([&reader]() { static_cast<void>(reader.getRow(2)); });
      const bool negativeRowThrows     = throwsRuntimeError([&reader]() { static_cast<void>(reader.getRow(-1)); });

      THEN("The reader returns both rows unchanged and a row behind either end throws a runtime error") {
        VALIDATE_EQUALS(numberOfRows, 2);
        VALIDATE_EQUALS(firstRow, std::string("1;2.5"));
        VALIDATE_EQUALS(secondRow, std::string("3;4.5;"));
        VALIDATE(rowBehindTheEndThrows);
        VALIDATE(negativeRowThrows);
      }
    }
  }
}

UNIT_TEST("Every entry of a row parses with and without a separator behind it") {
  GIVEN("The row of the entries 1, 2.5 and 7 and the same row with a separator behind its last entry") {
    using Reader                   = tarch::reader::CSVReader;
    const std::string row          = "1;2.5;7";
    const std::string separatedRow = "1;2.5;7;";

    WHEN("The reader parses every entry of both rows, the entry behind their last one and a letter as a double") {
      const int    first          = Reader::extractFundamentalTypeFromRow<int>(';', 0, row);
      const double second         = Reader::extractFundamentalTypeFromRow<double>(';', 1, row);
      const int    third          = Reader::extractFundamentalTypeFromRow<int>(';', 2, row);
      const int    separatedThird = Reader::extractFundamentalTypeFromRow<int>(';', 2, separatedRow);
      const bool   fourthEntryThrows =
        throwsRuntimeError([&row]() { static_cast<void>(Reader::extractFundamentalTypeFromRow<int>(';', 3, row)); });
      const bool separatedFourthThrows = throwsRuntimeError([&separatedRow]() {
        static_cast<void>(Reader::extractFundamentalTypeFromRow<int>(';', 3, separatedRow));
      });
      const bool letterThrows =
        throwsRuntimeError([]() { static_cast<void>(Reader::extractFundamentalTypeFromRow<double>(';', 0, "a;1")); });

      THEN("The entry behind the last one and the letter throw a runtime error and the entries read as 1, 2.5 and 7") {
        VALIDATE_EQUALS(first, 1);
        VALIDATE_NUMERICAL_EQUALS(second, 2.5);
        VALIDATE_EQUALS(third, 7);
        VALIDATE_EQUALS(separatedThird, 7);
        VALIDATE(fourthEntryThrows);
        VALIDATE(separatedFourthThrows);
        VALIDATE(letterThrows);
      }
    }
  }
}

UNIT_TEST("The reader throws for a file that does not exist") {
  GIVEN("The path of a file that does not exist") {
    const std::string path = (tarch::testing::getWritableTestDirectory() / "peano-csv-reader-missing.csv").string();

    WHEN("A reader opens the file") {
      const bool constructionThrows = throwsRuntimeError([&path]() { const tarch::reader::CSVReader reader(path); });

      THEN("The construction throws a runtime error") { VALIDATE(constructionThrows); }
    }
  }
}
