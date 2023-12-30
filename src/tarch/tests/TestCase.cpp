// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "tarch/tests/TestCase.h"

tarch::tests::TestCase::TestCase( const std::string& testCaseName ):
  _testCaseName(testCaseName),
  _errors(0),
  _error(false) {
}

tarch::tests::TestCase::~TestCase() {}

int tarch::tests::TestCase::getNumberOfErrors() const {
  return _errors;
}

std::string tarch::tests::TestCase::getTestCaseName() const {
  return _testCaseName;
}
