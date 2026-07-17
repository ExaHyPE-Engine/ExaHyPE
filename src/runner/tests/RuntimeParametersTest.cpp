// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "RuntimeParametersTest.h"

#include "runner/RuntimeParameters.h"

runner::tests::RuntimeParametersTest::RuntimeParametersTest() :
  tarch::tests::TestCase("runner::tests::RuntimeParametersTest") {}

void runner::tests::RuntimeParametersTest::run() {
  testMethod(testLoadBalancingQualityLimits);
  testMethod(testMaxNumberOfTreesPerRankLimits);
}

void runner::tests::RuntimeParametersTest::testLoadBalancingQualityLimits() {
  auto&      parameters                   = runner::RuntimeParameters::getInstance();
  const auto originalLoadBalancingQuality = parameters.getLoadBalancingQuality();
  const auto configureQuality = [&parameters](const std::string& optionName, const double loadBalancingQuality) {
    auto               executableName = std::string("RuntimeParametersTest");
    auto               option         = optionName;
    std::ostringstream value;
    value << std::setprecision(std::numeric_limits<double>::max_digits10) << loadBalancingQuality;
    auto valueString       = value.str();
    auto arguments         = std::array<char*, 4>{executableName.data(), option.data(), valueString.data(), nullptr};
    int  numberOfArguments = 3;
    parameters.configure(numberOfArguments, arguments.data());
    return numberOfArguments;
  };
  const auto readRejectionMessage = [&configureQuality](const std::string& optionName, const double value) {
    try {
      configureQuality(optionName, value);
    } catch (const std::runtime_error& exception) {
      return std::string(exception.what());
    }
    return std::string();
  };

  const auto shortOptionArgumentCount = configureQuality("-lbq", 0.5);
  const auto shortOptionValue         = parameters.getLoadBalancingQuality();
  const auto longOptionArgumentCount  = configureQuality("--loadBalancingQuality", 1.0);
  const auto longOptionValue          = parameters.getLoadBalancingQuality();
  const auto lowerBoundRejection      = readRejectionMessage("-lbq", 0.0);
  const auto upperBoundRejection      = readRejectionMessage("--loadBalancingQuality", 1.01);
  const auto restoredArgumentCount    = configureQuality("-lbq", originalLoadBalancingQuality);

  validateEquals(shortOptionArgumentCount, 1);
  validateNumericalEquals(shortOptionValue, 0.5);
  validateEquals(longOptionArgumentCount, 1);
  validateNumericalEquals(longOptionValue, 1.0);
  validate(lowerBoundRejection.find("(0, 1]") != std::string::npos);
  validate(upperBoundRejection.find("(0, 1]") != std::string::npos);
  validateEquals(restoredArgumentCount, 1);
}

void runner::tests::RuntimeParametersTest::testMaxNumberOfTreesPerRankLimits() {
  static_assert(MAX_SPACETREES_PER_RANK > 0, "The tree limit must be positive.");
  static_assert(MAX_SPACETREES_PER_RANK < std::numeric_limits<int>::max(), "The tree limit must fit into an int.");

  auto&      parameters                   = runner::RuntimeParameters::getInstance();
  const auto originalNumberOfTreesPerRank = parameters.getMaxNumberOfTreesPerRank();
  const auto configureTrees               = [&parameters](const int numberOfTreesPerRank) {
    auto executableName    = std::string("RuntimeParametersTest");
    auto option            = std::string("-trees");
    auto value             = std::to_string(numberOfTreesPerRank);
    auto arguments         = std::array<char*, 4>{executableName.data(), option.data(), value.data(), nullptr};
    int  numberOfArguments = 3;
    parameters.configure(numberOfArguments, arguments.data());
    return numberOfArguments;
  };

  constexpr auto AlwaysAcceptedNumbersOfTrees = std::array<int, 3>{-1, 0, MAX_SPACETREES_PER_RANK};
  for (const auto numberOfTreesPerRank : AlwaysAcceptedNumbersOfTrees) {
    validateEquals(configureTrees(numberOfTreesPerRank), 1);
    validateEquals(parameters.getMaxNumberOfTreesPerRank(), numberOfTreesPerRank);
  }
  if constexpr (MAX_SPACETREES_PER_RANK >= 128) {
    validateEquals(configureTrees(128), 1);
    validateEquals(parameters.getMaxNumberOfTreesPerRank(), 128);
  }
  if constexpr (MAX_SPACETREES_PER_RANK >= 256) {
    validateEquals(configureTrees(256), 1);
    validateEquals(parameters.getMaxNumberOfTreesPerRank(), 256);
  }

  constexpr int RejectedNumberOfTrees = MAX_SPACETREES_PER_RANK + 1;
  auto          rejectionMessage      = std::string();
  try {
    configureTrees(RejectedNumberOfTrees);
  } catch (const std::runtime_error& exception) {
    rejectionMessage = exception.what();
  }
  const auto restoredNumberOfArguments = configureTrees(originalNumberOfTreesPerRank);

  validate(not rejectionMessage.empty());
  validate(rejectionMessage.find(std::to_string(MAX_SPACETREES_PER_RANK)) != std::string::npos);
  validate(rejectionMessage.find(std::to_string(RejectedNumberOfTrees)) != std::string::npos);
  validateEquals(restoredNumberOfArguments, 1);
}
