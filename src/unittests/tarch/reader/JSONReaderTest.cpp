// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "tarch/reader/JSONReader.h"

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
      / std::filesystem::path(stem + "-" + testRunIdentifier + "-" + std::to_string(counter++) + ".json");

    std::ofstream output(path);
    if (not output.is_open()) {
      std::stringstream message;
      message << "The test cannot create the temporary file '" << path.string() << "'.";
      tarch::terminateWithError(message.str());
    }
    output << content;
    return path.string();
  }
} // namespace

UNIT_TEST("The reader finds the keys of a file and returns its primitives, arrays and nested values") {
  GIVEN("A reader of a file with a string, an integer, a double, a boolean, four arrays and a nested object") {
    const std::string path = writeTempFile(
      "peano-json-reader-valid",
      R"({
        "name": "demo",
        "count": 7,
        "ratio": 2.5,
        "enabled": true,
        "labels": ["alpha", "beta"],
        "steps": [1, 2, 3],
        "weights": [1, 2.5, 3.75],
        "flags": [true, false, true],
        "nested": { "inner": 1, "weights": [4, 5.5] }
      })"
    );
    const tarch::reader::JSONReader reader(path);

    WHEN("The reader reads every value, the integer count also as a double") {
      const std::string name          = reader.getString("name");
      const int         count         = reader.getInteger("count");
      const int         nestedInner   = reader.getInteger("nested.inner");
      const double      ratio         = reader.getDouble("ratio");
      const double      countAsDouble = reader.getDouble("count");
      const bool        enabled       = reader.getBoolean("enabled");
      const auto        labels        = reader.getStringArray("labels");
      const auto        steps         = reader.getIntegerArray("steps");
      const auto        weights       = reader.getDoubleArray("weights");
      const auto        nestedWeights = reader.getDoubleArray("nested.weights");
      const auto        flags         = reader.getBooleanArray("flags");

      THEN("The reader finds name and nested.inner, misses an absent key and returns the values of the file") {
        VALIDATE(reader.hasKey("name"));
        VALIDATE(reader.hasKey("nested.inner"));
        VALIDATE(not reader.hasKey("missing"));
        VALIDATE_EQUALS(name, std::string("demo"));
        VALIDATE_EQUALS(count, 7);
        VALIDATE_EQUALS(nestedInner, 1);
        VALIDATE_NUMERICAL_EQUALS(ratio, 2.5);
        VALIDATE_NUMERICAL_EQUALS(countAsDouble, 7.0);
        VALIDATE(enabled);

        VALIDATE_EQUALS(labels.size(), static_cast<std::size_t>(2));
        VALIDATE_EQUALS(labels[0], std::string("alpha"));
        VALIDATE_EQUALS(labels[1], std::string("beta"));

        VALIDATE_EQUALS(steps.size(), static_cast<std::size_t>(3));
        VALIDATE_EQUALS(steps[0], 1);
        VALIDATE_EQUALS(steps[2], 3);

        VALIDATE_EQUALS(weights.size(), static_cast<std::size_t>(3));
        VALIDATE_NUMERICAL_EQUALS(weights[0], 1.0);
        VALIDATE_NUMERICAL_EQUALS(weights[1], 2.5);
        VALIDATE_NUMERICAL_EQUALS(weights[2], 3.75);

        VALIDATE_EQUALS(nestedWeights.size(), static_cast<std::size_t>(2));
        VALIDATE_NUMERICAL_EQUALS(nestedWeights[0], 4.0);
        VALIDATE_NUMERICAL_EQUALS(nestedWeights[1], 5.5);

        VALIDATE_EQUALS(flags.size(), static_cast<std::size_t>(3));
        VALIDATE(flags[0]);
        VALIDATE(not flags[1]);
        VALIDATE(flags[2]);
      }
    }
    std::filesystem::remove(path);
  }
}

UNIT_TEST("The unused keys are the top-level keys that the getters skipped") {
  GIVEN("A reader of a file with the top-level integers alpha, beta and gamma") {
    const std::string path = writeTempFile("peano-json-reader-unused", R"({ "alpha": 1, "beta": 2, "gamma": 3 })");
    const tarch::reader::JSONReader reader(path);

    WHEN("The reader reads zero keys") {
      const auto unused = reader.getUnusedKeys();

      THEN("All three keys are unused") { VALIDATE_EQUALS(unused.size(), static_cast<std::size_t>(3)); }
    }

    WHEN("The reader reads alpha and beta and looks up gamma") {
      static_cast<void>(reader.getInteger("alpha"));
      static_cast<void>(reader.getInteger("beta"));
      static_cast<void>(reader.hasKey("gamma"));
      const auto unused = reader.getUnusedKeys();

      THEN("The lookup leaves gamma the one unused key") {
        VALIDATE_EQUALS(unused.size(), static_cast<std::size_t>(1));
        VALIDATE_EQUALS(unused[0], std::string("gamma"));
      }
    }
    std::filesystem::remove(path);
  }

  GIVEN("A reader of a file with the integer entry inside the object section and the top-level integer stray") {
    const std::string nestedPath =
      writeTempFile("peano-json-reader-unused-nested", R"({ "section": { "entry": 1 }, "stray": 2 })");
    const tarch::reader::JSONReader reader(nestedPath);

    WHEN("The reader reads the member entry of section") {
      static_cast<void>(reader.getInteger("section.entry"));
      const auto unused = reader.getUnusedKeys();

      THEN("Reading a member marks the whole top-level object used and leaves stray the one unused key") {
        VALIDATE_EQUALS(unused.size(), static_cast<std::size_t>(1));
        VALIDATE_EQUALS(unused[0], std::string("stray"));
      }
    }
    std::filesystem::remove(nestedPath);
  }
}
