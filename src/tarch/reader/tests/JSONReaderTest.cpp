// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "JSONReaderTest.h"

#include "tarch/reader/JSONReader.h"

namespace {
  std::string writeTempFile(const std::string& stem, const std::string& content) {
    // The file name carries a per-run identifier, so two processes that write into the test
    // directory at the same time do not overwrite each other's fixture.
    static const std::string testRunIdentifier =
      std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()) + "-"
      + std::to_string(std::random_device{}());
    static std::atomic<int> counter = 0;

    const std::filesystem::path path = tarch::tests::getWritableTestDirectory()
      / std::filesystem::path(stem + "-" + testRunIdentifier + "-" + std::to_string(counter++) + ".json");

    std::ofstream output(path);
    if (not output.is_open()) {
      std::stringstream message;
      message << "Failed to create temporary file \"" << path.string() << "\".";
      throw std::runtime_error(message.str());
    }
    output << content;
    return path.string();
  }
} // namespace

tarch::reader::tests::JSONReaderTest::JSONReaderTest() :
  tarch::tests::TestCase("tarch::reader::tests::JSONReaderTest") {}

void tarch::reader::tests::JSONReaderTest::run() {
  testMethod(testReadPrimitivesAndArrays);
  testMethod(testFailureModes);
  testMethod(testUnusedKeys);
}

void tarch::reader::tests::JSONReaderTest::testReadPrimitivesAndArrays() {
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

  try {
    tarch::reader::JSONReader reader(path);

    validate(reader.hasKey("name"));
    validate(reader.hasKey("nested.inner"));
    validateFalse(reader.hasKey("missing"));
    validateEquals(reader.getString("name"), std::string("demo"));
    validateEquals(reader.getInteger("count"), 7);
    validateEquals(reader.getInteger("nested.inner"), 1);
    validateNumericalEquals(reader.getDouble("ratio"), 2.5);
    validateNumericalEquals(reader.getDouble("count"), 7.0);
    validate(reader.getBoolean("enabled"));

    const auto labels = reader.getStringArray("labels");
    validateEquals(labels.size(), static_cast<std::size_t>(2));
    validateEquals(labels[0], std::string("alpha"));
    validateEquals(labels[1], std::string("beta"));

    const auto steps = reader.getIntegerArray("steps");
    validateEquals(steps.size(), static_cast<std::size_t>(3));
    validateEquals(steps[0], 1);
    validateEquals(steps[2], 3);

    const auto weights = reader.getDoubleArray("weights");
    validateEquals(weights.size(), static_cast<std::size_t>(3));
    validateNumericalEquals(weights[0], 1.0);
    validateNumericalEquals(weights[1], 2.5);
    validateNumericalEquals(weights[2], 3.75);

    const auto nestedWeights = reader.getDoubleArray("nested.weights");
    validateEquals(nestedWeights.size(), static_cast<std::size_t>(2));
    validateNumericalEquals(nestedWeights[0], 4.0);
    validateNumericalEquals(nestedWeights[1], 5.5);

    const auto flags = reader.getBooleanArray("flags");
    validateEquals(flags.size(), static_cast<std::size_t>(3));
    validate(flags[0]);
    validateFalse(flags[1]);
    validate(flags[2]);
  } catch (...) {
    std::filesystem::remove(path);
    throw;
  }

  std::filesystem::remove(path);
}

void tarch::reader::tests::JSONReaderTest::testUnusedKeys() {
  const std::string path = writeTempFile("peano-json-reader-unused", R"({ "alpha": 1, "beta": 2, "gamma": 3 })");

  try {
    tarch::reader::JSONReader reader(path);
    validateEquals(reader.getUnusedKeys().size(), static_cast<std::size_t>(3));

    reader.getInteger("alpha");
    reader.getInteger("beta");
    reader.hasKey("gamma"); // existence check does not read the value

    const auto unused = reader.getUnusedKeys();
    validateEquals(unused.size(), static_cast<std::size_t>(1));
    validateEquals(unused[0], std::string("gamma"));
  } catch (...) {
    std::filesystem::remove(path);
    throw;
  }

  std::filesystem::remove(path);

  // Reading any member marks the whole top-level object used.
  const std::string nestedPath =
    writeTempFile("peano-json-reader-unused-nested", R"({ "section": { "entry": 1 }, "stray": 2 })");

  try {
    tarch::reader::JSONReader reader(nestedPath);
    reader.getInteger("section.entry");

    const auto unused = reader.getUnusedKeys();
    validateEquals(unused.size(), static_cast<std::size_t>(1));
    validateEquals(unused[0], std::string("stray"));
  } catch (...) {
    std::filesystem::remove(nestedPath);
    throw;
  }

  std::filesystem::remove(nestedPath);
}

void tarch::reader::tests::JSONReaderTest::testFailureModes() {
  const std::string invalidPath = writeTempFile("peano-json-reader-invalid", R"({ "name": "demo", )");

  try {
    tarch::reader::JSONReader reader(invalidPath);
    validate(false);
  } catch (const std::runtime_error&) {
  }
  std::filesystem::remove(invalidPath);

  const std::string validPath = writeTempFile(
    "peano-json-reader-errors",
    R"({
      "count": 7,
      "ratio": 2.5,
      "enabled": false,
      "labels": ["alpha", 2]
    })"
  );

  try {
    tarch::reader::JSONReader reader(validPath);

    try {
      reader.getString("missing");
      validate(false);
    } catch (const std::runtime_error&) {
    }

    try {
      reader.getInteger("ratio");
      validate(false);
    } catch (const std::runtime_error&) {
    }

    try {
      reader.getBoolean("count");
      validate(false);
    } catch (const std::runtime_error&) {
    }

    try {
      reader.getStringArray("labels");
      validate(false);
    } catch (const std::runtime_error&) {
    }
  } catch (...) {
    std::filesystem::remove(validPath);
    throw;
  }

  std::filesystem::remove(validPath);

  try {
    tarch::reader::JSONReader reader("/tmp/this-file-does-not-exist.json");
    validate(false);
  } catch (const std::runtime_error&) {
  }
}
