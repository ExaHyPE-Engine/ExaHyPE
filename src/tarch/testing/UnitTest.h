// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/accelerator/CompileTarget.h"

namespace tarch::testing {
  class Section;

  /**
   * Units in the last place by which two compilers may round one computation apart.
   */
  constexpr int MaxUnitsInTheLastPlace = 16;

  TARGET_DECLARE
  /**
   * Largest absolute difference between the two values of a VALIDATE_NUMERICAL_EQUALS.
   */
  constexpr double NumericalTolerance = 1.0e-10;
  TARGET_DECLARE_END

  /**
   * Accepts the integer types that std::cmp_equal() compares.
   */
  template <typename Value>
  concept StandardInteger = std::integral<Value> and not std::same_as<Value, bool> and not std::same_as<Value, char>
    and not std::same_as<Value, wchar_t> and not std::same_as<Value, char8_t> and not std::same_as<Value, char16_t>
    and not std::same_as<Value, char32_t>;

  [[nodiscard]] std::filesystem::path getWritableTestDirectory();

  [[nodiscard]] double getToleranceInUnitsInTheLastPlace(std::span<const double> reference);

  [[nodiscard]] bool registerUnitTest(
    const std::string&          description,
    const std::source_location& location,
    void (*unitTest)()
  );

  [[nodiscard]] int runTests(int argc, const char* const* argv);

  [[nodiscard]] std::optional<std::string> checkCondition(bool condition);

  template <typename Actual, typename Expected>
  [[nodiscard]] std::optional<std::string> checkEquals(const Actual& actual, const Expected& expected);

  template <typename Actual, typename Expected>
  [[nodiscard]] std::optional<std::string> checkNumericalEquals(const Actual& actual, const Expected& expected);

  [[nodiscard]] std::optional<std::string> checkNumericalBuffersEquals(
    std::span<const double> actual,
    std::span<const double> expected
  );

  void recordFailure(
    const std::source_location&     location,
    std::string_view                statement,
    std::string_view                details,
    std::string_view                names,
    const std::vector<std::string>& values
  );
} // namespace tarch::testing

/**
 * One block of a unit test. The runs of a unit test enter every innermost block once and at most one
 * block per nesting level each. Every innermost block starts from the state of its enclosing blocks.
 * A caller never puts a block inside a branch that depends on the rank.
 */
class tarch::testing::Section final {
public:
  Section(std::string_view kind, std::string_view description, const std::source_location& location);
  Section(const Section& other)            = delete;
  Section(Section&& other)                 = delete;
  Section& operator=(const Section& other) = delete;
  Section& operator=(Section&& other)      = delete;
  ~Section();

  [[nodiscard]] bool isEntered() const;

private:
  bool _entered{false};
  int  _uncaughtExceptions{std::uncaught_exceptions()};
};

#define TARCH_TESTING_CONCATENATE_TOKENS(left, right) left##right

/**
 * Pastes the expanded arguments into one identifier. The line number then becomes part of a name.
 */
#define TARCH_TESTING_CONCATENATE(left, right) TARCH_TESTING_CONCATENATE_TOKENS(left, right)

/**
 * Defines a unit test of the unit test executable with the block that follows as its body. The
 * report lists the unit test with the path of its file below src/unittests and its description.
 */
#define UNIT_TEST(description) \
  namespace { \
    struct TARCH_TESTING_CONCATENATE(TarchTestingUnitTest, __LINE__) final { \
      static void run(); \
    }; \
    [[maybe_unused]] const bool TARCH_TESTING_CONCATENATE(tarchTestingUnitTestRegistration, __LINE__) = \
      tarch::testing::registerUnitTest( \
        description, \
        std::source_location::current(), \
        &TARCH_TESTING_CONCATENATE(TarchTestingUnitTest, __LINE__)::run \
      ); \
  } \
  void TARCH_TESTING_CONCATENATE(TarchTestingUnitTest, __LINE__)::run()

/**
 * Opens a block of a unit test. The current run of the unit test enters or skips the block.
 */
#define TARCH_TESTING_SECTION(kind, description) \
  if ( \
    const tarch::testing::Section \
      TARCH_TESTING_CONCATENATE(tarchTestingSection, __LINE__)(kind, description, std::source_location::current()); \
    TARCH_TESTING_CONCATENATE(tarchTestingSection, __LINE__).isEntered() \
  )

#define GIVEN(description) TARCH_TESTING_SECTION("Given", description)
#define WHEN(description) TARCH_TESTING_SECTION("When", description)
#define THEN(description) TARCH_TESTING_SECTION("Then", description)
#define AND_WHEN(description) TARCH_TESTING_SECTION("And when", description)
#define AND_THEN(description) TARCH_TESTING_SECTION("And then", description)

/**
 * Streams every argument into a string of its own with the output operators of the checked types at
 * the place of the check.
 */
#define TARCH_TESTING_TO_STRINGS(...) \
  [](const auto&... values) { \
    return std::vector<std::string>{[](const auto& value) { \
      std::ostringstream stream; \
      stream << std::boolalpha << std::setprecision(std::numeric_limits<double>::max_digits10) << value; \
      return stream.str(); \
    }(values)...}; \
  }(__VA_ARGS__)

/**
 * Records a failure and leaves the enclosing function when condition is false. The failure report
 * prints every further argument with its value.
 */
#define VALIDATE(condition, ...) \
  do { \
    if ( \
      const std::optional<std::string> validationFailure = \
        tarch::testing::checkCondition(static_cast<bool>(condition)); \
      validationFailure.has_value() \
    ) { \
      tarch::testing::recordFailure( \
        std::source_location::current(), \
        "VALIDATE(" #condition __VA_OPT__(", " #__VA_ARGS__) ")", \
        *validationFailure, \
        #__VA_ARGS__, \
        TARCH_TESTING_TO_STRINGS(__VA_ARGS__) \
      ); \
      return; \
    } \
  } while (false)

/**
 * Records a failure and leaves the enclosing function when actual differs from expected. The
 * macro evaluates actual and expected once. The failure report prints both values and every
 * further argument with its value.
 */
#define VALIDATE_EQUALS(actual, expected, ...) \
  do { \
    if ([&](const auto& validationActual, const auto& validationExpected) { \
          const std::optional<std::string> validationFailure = \
            tarch::testing::checkEquals(validationActual, validationExpected); \
          if (validationFailure.has_value()) { \
            tarch::testing::recordFailure( \
              std::source_location::current(), \
              "VALIDATE_EQUALS(" #actual ", " #expected __VA_OPT__(", " #__VA_ARGS__) ")", \
              *validationFailure, \
              #actual ", " #expected __VA_OPT__(", " #__VA_ARGS__), \
              TARCH_TESTING_TO_STRINGS(validationActual, validationExpected __VA_OPT__(, ) __VA_ARGS__) \
            ); \
          } \
          return validationFailure.has_value(); \
        }((actual), (expected))) { \
      return; \
    } \
  } while (false)

/**
 * Records a failure and leaves the enclosing function when actual and expected differ by more
 * than NumericalTolerance. The macro evaluates actual and expected once. The failure report
 * prints both values, their difference and every further argument with its value.
 */
#define VALIDATE_NUMERICAL_EQUALS(actual, expected, ...) \
  do { \
    if ([&](const auto& validationActual, const auto& validationExpected) { \
          const std::optional<std::string> validationFailure = \
            tarch::testing::checkNumericalEquals(validationActual, validationExpected); \
          if (validationFailure.has_value()) { \
            tarch::testing::recordFailure( \
              std::source_location::current(), \
              "VALIDATE_NUMERICAL_EQUALS(" #actual ", " #expected __VA_OPT__(", " #__VA_ARGS__) ")", \
              *validationFailure, \
              #actual ", " #expected __VA_OPT__(", " #__VA_ARGS__), \
              TARCH_TESTING_TO_STRINGS(validationActual, validationExpected __VA_OPT__(, ) __VA_ARGS__) \
            ); \
          } \
          return validationFailure.has_value(); \
        }((actual), (expected))) { \
      return; \
    } \
  } while (false)

/**
 * Records a failure and leaves the enclosing function when the buffers differ in size or when an
 * entry of actual differs from its entry of expected by more than MaxUnitsInTheLastPlace units in
 * the last place of the largest magnitude of expected. The failure report prints the first
 * differing entry and every further argument with its value.
 */
#define VALIDATE_NUMERICAL_BUFFERS_EQUALS(actual, expected, ...) \
  do { \
    if ( \
      const std::optional<std::string> validationFailure = \
        tarch::testing::checkNumericalBuffersEquals((actual), (expected)); \
      validationFailure.has_value() \
    ) { \
      tarch::testing::recordFailure( \
        std::source_location::current(), \
        "VALIDATE_NUMERICAL_BUFFERS_EQUALS(" #actual ", " #expected __VA_OPT__(", " #__VA_ARGS__) ")", \
        *validationFailure, \
        #__VA_ARGS__, \
        TARCH_TESTING_TO_STRINGS(__VA_ARGS__) \
      ); \
      return; \
    } \
  } while (false)

#include "tarch/testing/UnitTest.cpph"
