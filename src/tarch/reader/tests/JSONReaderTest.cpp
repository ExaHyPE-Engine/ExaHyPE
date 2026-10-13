// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "JSONReaderTest.h"

#include "tarch/Termination.h"
#include "tarch/reader/JSONReader.h"

namespace {
  std::string writeTempFile(const std::string& stem, const std::string& content) {
    // The file name contains a per-run identifier. Two processes never write the same fixture.
    static const std::string testRunIdentifier =
      std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()) + "-"
      + std::to_string(std::random_device{}());
    static std::atomic<int> counter = 0;

    const std::filesystem::path path = tarch::tests::getWritableTestDirectory()
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

tarch::reader::tests::JSONReaderTest::JSONReaderTest() :
  tarch::tests::TestCase("tarch::reader::tests::JSONReaderTest") {}

void
tarch::reader::tests::JSONReaderTest::run() {
  testMethod(testReadPrimitivesAndArrays);
  testMethod(testUnusedKeys);
}

void
tarch::reader::tests::JSONReaderTest::testReadPrimitivesAndArrays() {
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

  std::filesystem::remove(path);
}

void
tarch::reader::tests::JSONReaderTest::testUnusedKeys() {
  const std::string path = writeTempFile("peano-json-reader-unused", R"({ "alpha": 1, "beta": 2, "gamma": 3 })");

  {
    const tarch::reader::JSONReader reader(path);
    validateEquals(reader.getUnusedKeys().size(), static_cast<std::size_t>(3));

    static_cast<void>(reader.getInteger("alpha"));
    static_cast<void>(reader.getInteger("beta"));
    static_cast<void>(reader.hasKey("gamma"));

    const auto unused = reader.getUnusedKeys();
    validateEquals(unused.size(), static_cast<std::size_t>(1));
    validateEquals(unused[0], std::string("gamma"));
  }

  std::filesystem::remove(path);

  // Reading any member marks the whole top-level object used.
  const std::string nestedPath =
    writeTempFile("peano-json-reader-unused-nested", R"({ "section": { "entry": 1 }, "stray": 2 })");

  {
    const tarch::reader::JSONReader reader(nestedPath);
    static_cast<void>(reader.getInteger("section.entry"));

    const auto unused = reader.getUnusedKeys();
    validateEquals(unused.size(), static_cast<std::size_t>(1));
    validateEquals(unused[0], std::string("stray"));
  }

  std::filesystem::remove(nestedPath);
}
