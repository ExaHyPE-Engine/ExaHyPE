// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

/**
 * Calls the test method name, given without parentheses, and reports on std::cerr when a validate
 * macro failed inside it.
 */
#define testMethod(name) \
  { \
    _error = false; \
    name(); \
    if (_error) { \
      std::cerr << "Test case " << _testCaseName << "::" << #name << "() failed" << std::endl << std::endl; \
    } \
  };

/**
 * Is true when lhs and rhs differ by at most eps. A NaN on either side compares unequal.
 */
#define testNumericalEqualsWithEps(lhs, rhs, eps) (std::abs((rhs) - (lhs)) <= (eps))
#define testNumericalEquals(lhs, rhs) testNumericalEqualsWithEps(lhs, rhs, 1.0e-10)

#define validate(booleanExpr) \
  if (not static_cast<bool>(booleanExpr)) { \
    _errors++; \
    _error = true; \
    std::cerr << "  Boolean test failed " << std::endl \
              << "  File: " << __FILE__ << " \t line: " << __LINE__ << std::endl \
              << "  Statement: " << #booleanExpr << std::endl; \
    return; \
  }

#define validateTrue(booleanExpr) validate(booleanExpr)
#define validateFalse(booleanExpr) validate(not(booleanExpr))

#define validateWithParams1(booleanExpr, param0) \
  if (not static_cast<bool>(booleanExpr)) { \
    _errors++; \
    _error = true; \
    std::cerr << "  Boolean test failed " << std::endl \
              << "  File: " << __FILE__ << " \t line: " << __LINE__ << std::endl \
              << "  Statement: " << #booleanExpr << std::endl \
              << "  Parameter " << #param0 << "=" << (param0) << std::endl; \
    return; \
  }

#define validateWithParams2(booleanExpr, param0, param1) \
  if (not static_cast<bool>(booleanExpr)) { \
    _errors++; \
    _error = true; \
    std::cerr << "  Boolean test failed " << std::endl \
              << "  File: " << __FILE__ << " \t line: " << __LINE__ << std::endl \
              << "  Statement: " << #booleanExpr << std::endl \
              << "  Parameter " << #param0 << "=" << (param0) << std::endl \
              << "  Parameter " << #param1 << "=" << (param1) << std::endl; \
    return; \
  }

#define validateWithParams3(booleanExpr, param0, param1, param2) \
  if (not static_cast<bool>(booleanExpr)) { \
    _errors++; \
    _error = true; \
    std::cerr << "  Boolean test failed " << std::endl \
              << "  File: " << __FILE__ << " \t line: " << __LINE__ << std::endl \
              << "  Statement: " << #booleanExpr << std::endl \
              << "  Parameter " << #param0 << "=" << (param0) << std::endl \
              << "  Parameter " << #param1 << "=" << (param1) << std::endl \
              << "  Parameter " << #param2 << "=" << (param2) << std::endl; \
    return; \
  }

#define validateEquals(actualValue, validValue) \
  if (not((actualValue) == (validValue))) { \
    _errors++; \
    _error = true; \
    std::cerr << "  Equality test failed: " << (actualValue) << ", expected " << (validValue) << std::endl \
              << "  File: " << __FILE__ << " \t line: " << __LINE__ << std::endl \
              << "  Statement: " << #actualValue << "==" << #validValue << std::endl; \
    return; \
  }

#define validateEqualsWithParams1(actualValue, validValue, param0) \
  if (not((actualValue) == (validValue))) { \
    _errors++; \
    _error = true; \
    std::cerr << "  Equality test failed: " << (actualValue) << ", expected " << (validValue) << std::endl \
              << "  File: " << __FILE__ << " \t line: " << __LINE__ << std::endl \
              << "  Statement: " << #actualValue << "==" << #validValue << std::endl \
              << "  Parameter " << #param0 << "=" << (param0) << std::endl; \
    return; \
  }

#define validateEqualsWithParams2(actualValue, validValue, param0, param1) \
  if (not((actualValue) == (validValue))) { \
    _errors++; \
    _error = true; \
    std::cerr << "  Equality test failed: " << (actualValue) << ", expected " << (validValue) << std::endl \
              << "  File: " << __FILE__ << " \t line: " << __LINE__ << std::endl \
              << "  Statement: " << #actualValue << "==" << #validValue << std::endl \
              << "  Parameter " << #param0 << "=" << (param0) << ", parameter " << #param1 << "=" << (param1) \
              << std::endl; \
    return; \
  }

#define validateEqualsWithParams3(actualValue, validValue, param0, param1, param2) \
  if (not((actualValue) == (validValue))) { \
    _errors++; \
    _error = true; \
    std::cerr << "  Equality test failed: " << (actualValue) << ", expected " << (validValue) << std::endl \
              << "  File: " << __FILE__ << " \t line: " << __LINE__ << std::endl \
              << "  Statement: " << #actualValue << "==" << #validValue << std::endl \
              << "  Parameter " << #param0 << "=" << (param0) << ", parameter " << #param1 << "=" << (param1) \
              << ", parameter " << #param2 << "=" << (param2) << std::endl; \
    return; \
  }

#define validateEqualsWithParams4(actualValue, validValue, param0, param1, param2, param3) \
  if (not((actualValue) == (validValue))) { \
    _errors++; \
    _error = true; \
    std::cerr << "  Equality test failed: " << (actualValue) << ", expected " << (validValue) << std::endl \
              << "  File: " << __FILE__ << " \t line: " << __LINE__ << std::endl \
              << "  Statement: " << #actualValue << "==" << #validValue << std::endl \
              << "  Parameter " << #param0 << "=" << (param0) << ", parameter " << #param1 << "=" << (param1) \
              << ", parameter " << #param2 << "=" << (param2) << ", parameter " << #param3 << "=" << (param3) \
              << std::endl; \
    return; \
  }

#define validateNumericalEquals(actualValue, validValue) \
  if (not testNumericalEquals(actualValue, validValue)) { \
    _errors++; \
    _error = true; \
    std::cerr << "  Numerical equality test failed: " << (actualValue) << ", expected " << (validValue) << std::endl \
              << "  File: " << __FILE__ << " \t line: " << __LINE__ << std::endl \
              << "  Statement: " << #actualValue << "==" << #validValue << std::endl \
              << "  Difference: " << fabs((actualValue) - (validValue)) << std::endl; \
    return; \
  }

/**
 * Compares two buffers of doubles entry by entry and fails the test when an entry of actual differs
 * from its entry of valid by more than tarch::tests::MaxUnitsInTheLastPlace units in the last place
 * of the largest magnitude of valid.
 */
#define validateNumericalBuffersEquals(actual, valid) \
  { \
    validateEquals((actual).size(), (valid).size()); \
    const double bufferTolerance = tarch::tests::getToleranceInUnitsInTheLastPlace(valid); \
    for (std::size_t bufferEntry = 0; bufferEntry < (actual).size(); bufferEntry++) { \
      validateNumericalEqualsWithEpsWithParams1( \
        (actual)[bufferEntry], \
        (valid)[bufferEntry], \
        bufferTolerance, \
        bufferEntry \
      ); \
    } \
  }

#define validateNumericalEqualsWithEps(actualValue, validValue, eps) \
  if (not testNumericalEqualsWithEps(actualValue, validValue, eps)) { \
    _errors++; \
    _error = true; \
    std::cerr << "  Numerical equality test failed: " << (actualValue) << ", expected " << (validValue) << std::endl \
              << "  File: " << __FILE__ << " \t line: " << __LINE__ << std::endl \
              << "  Statement: " << #actualValue << "==" << #validValue << std::endl \
              << "  Difference: " << fabs((actualValue) - (validValue)) << std::endl; \
    return; \
  }

#define validateNumericalEqualsWithEpsWithParams1(actualValue, validValue, eps, param0) \
  if (not testNumericalEqualsWithEps(actualValue, validValue, eps)) { \
    _errors++; \
    _error = true; \
    std::cerr << "  Numerical equality test failed: " << (actualValue) << ", expected " << (validValue) << std::endl \
              << "  File: " << __FILE__ << " \t line: " << __LINE__ << std::endl \
              << "  Statement: " << #actualValue << "==" << #validValue << std::endl \
              << "  Parameter " << #param0 << "=" << (param0) << std::endl \
              << "  Difference: " << fabs((actualValue) - (validValue)) << std::endl; \
    return; \
  }

#define validateNumericalEqualsWithEpsWithParams2(actualValue, validValue, eps, param0, param1) \
  if (not testNumericalEqualsWithEps(actualValue, validValue, eps)) { \
    _errors++; \
    _error = true; \
    std::cerr << "  Numerical equality test failed: " << (actualValue) << ", expected " << (validValue) << std::endl \
              << "  File: " << __FILE__ << " \t line: " << __LINE__ << std::endl \
              << "  Statement: " << #actualValue << "==" << #validValue << std::endl \
              << "  Parameter " << #param0 << "=" << (param0) << ", parameter " << #param1 << "=" << (param1) \
              << std::endl \
              << "  Difference: " << fabs((actualValue) - (validValue)) << std::endl; \
    return; \
  }

#define validateNumericalEqualsWithEpsWithParams3(actualValue, validValue, eps, param0, param1, param2) \
  if (not testNumericalEqualsWithEps(actualValue, validValue, eps)) { \
    _errors++; \
    _error = true; \
    std::cerr << "  Numerical equality test failed: " << (actualValue) << ", expected " << (validValue) << std::endl \
              << "  File: " << __FILE__ << " \t line: " << __LINE__ << std::endl \
              << "  Statement: " << #actualValue << "==" << #validValue << std::endl \
              << "  Parameter " << #param0 << "=" << (param0) << ", parameter " << #param1 << "=" << (param1) \
              << ", parameter " << #param2 << "=" << (param2) << std::endl \
              << "  Difference: " << fabs((actualValue) - (validValue)) << std::endl; \
    return; \
  }

#define validateNumericalEqualsWithParams1(actualValue, validValue, param0) \
  if (not testNumericalEquals(actualValue, validValue)) { \
    _errors++; \
    _error = true; \
    std::cerr << "  Numerical equality test failed: " << (actualValue) << ", expected " << (validValue) << std::endl \
              << "  File: " << __FILE__ << " \t line: " << __LINE__ << std::endl \
              << "  Statement: " << #actualValue << "==" << #validValue << std::endl \
              << "  Parameter " << #param0 << "=" << (param0) << std::endl \
              << "  Difference: " << fabs((actualValue) - (validValue)) << std::endl; \
    return; \
  }

#define validateNumericalEqualsWithParams2(actualValue, validValue, param0, param1) \
  if (not testNumericalEquals(actualValue, validValue)) { \
    _errors++; \
    _error = true; \
    std::cerr << "  Numerical equality test failed: " << (actualValue) << ", expected " << (validValue) << std::endl \
              << "  File: " << __FILE__ << " \t line: " << __LINE__ << std::endl \
              << "  Statement: " << #actualValue << "==" << #validValue << std::endl \
              << "  Parameter " << #param0 << "=" << (param0) << ", parameter " << #param1 << "=" << (param1) \
              << std::endl \
              << "  Difference: " << fabs((actualValue) - (validValue)) << std::endl; \
    return; \
  }

#define validateNumericalEqualsWithParams3(actualValue, validValue, param0, param1, param2) \
  if (not testNumericalEquals(actualValue, validValue)) { \
    _errors++; \
    _error = true; \
    std::cerr << "  Numerical equality test failed: " << (actualValue) << ", expected " << (validValue) << std::endl \
              << "  File: " << __FILE__ << " \t line: " << __LINE__ << std::endl \
              << "  Statement: " << #actualValue << "==" << #validValue << std::endl \
              << "  Parameter " << #param0 << "=" << (param0) << ", parameter " << #param1 << "=" << (param1) \
              << ", parameter " << #param2 << "=" << (param2) << std::endl \
              << "  Difference: " << fabs((actualValue) - (validValue)) << std::endl; \
    return; \
  }

#define validateNumericalEqualsWithParams4(actualValue, validValue, param0, param1, param2, param3) \
  if (not testNumericalEquals(actualValue, validValue)) { \
    _errors++; \
    _error = true; \
    std::cerr << "  Numerical equality test failed: " << (actualValue) << ", expected " << (validValue) << std::endl \
              << "  File: " << __FILE__ << " \t line: " << __LINE__ << std::endl \
              << "  Statement: " << #actualValue << "==" << #validValue << std::endl \
              << "  Parameter " << #param0 << "=" << (param0) << ", parameter " << #param1 << "=" << (param1) \
              << ", parameter " << #param2 << "=" << (param2) << ", parameter " << #param3 << "=" << (param3) \
              << std::endl \
              << "  Difference: " << fabs((actualValue) - (validValue)) << std::endl; \
    return; \
  }

/**
 * Compares two vectors entry by entry and fails the test when an entry differs by more than
 * tarch::la::NUMERICAL_ZERO_DIFFERENCE.
 */
#define validateNumericalVectorEquals(actual, valid) \
  if (not tarch::la::equals((actual), (valid))) { \
    _errors++; \
    _error = true; \
    std::cerr << "  Numerical vector equality test failed: " << (actual) << ", expected " << (valid) << std::endl \
              << "  File: " << __FILE__ << " \t line: " << __LINE__ << std::endl \
              << "  Statement: " << #actual << "==" << #valid << std::endl; \
    return; \
  }
