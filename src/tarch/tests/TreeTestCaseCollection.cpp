// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "TreeTestCaseCollection.h"

#include "tarch/Assertions.h"

/**
 * The name has no ::, and an empty name makes a root collection that writes no progress line of its
 * own.
 */
tarch::tests::TreeTestCaseCollection::TreeTestCaseCollection(
  const std::string& treeTestCaseCollectionName,
  bool               deleteTestCases,
  bool               writeToLog
) :
  tarch::tests::TestCase(treeTestCaseCollectionName),
  _writeToLog(writeToLog),
  _deleteTestCases(deleteTestCases) {
  assertion(
    isNameWithoutHierarchy(treeTestCaseCollectionName),
    treeTestCaseCollectionName,
    deleteTestCases,
    writeToLog
  );
}

tarch::tests::TreeTestCaseCollection::~TreeTestCaseCollection() {
  if (_deleteTestCases) {
    for (tarch::tests::TestCase* const testCase : _testCases) {
      delete testCase;
    }
    for (const auto& subTest : _subTests) {
      delete subTest.second;
    }
  }
}

bool
tarch::tests::TreeTestCaseCollection::isNameWithoutHierarchy(const std::string& testCaseName) {
  return testCaseName.rfind("::") == std::string::npos;
}

std::string
tarch::tests::TreeTestCaseCollection::getFirstIdentifierInHierarchy(const std::string& testCaseName) {
  std::string result = testCaseName.substr(0, testCaseName.find("::"));
  assertion(isNameWithoutHierarchy(result), result, testCaseName);
  return result;
}

std::string
tarch::tests::TreeTestCaseCollection::getRemainingPathWithoutIdentifier(const std::string& testCaseName) {
  assertion(not isNameWithoutHierarchy(testCaseName));
  std::string result = testCaseName.substr(testCaseName.find("::") + 2);
  return result;
}

void
tarch::tests::TreeTestCaseCollection::run() {
  run("");
}

/**
 * Runs every test case of the collection with prefix in front of its name.
 */
void
tarch::tests::TreeTestCaseCollection::run(const std::string& prefix) {
  std::string progressMessage;
  if ((prefix + _testCaseName).length() > 0) {
    progressMessage = "Running test case collection \"" + prefix + _testCaseName + "\" ...";
  } else {
    progressMessage = "Running test case collection ...";
  }

  const std::string fullQualifiedName = _testCaseName.empty() ? prefix : prefix + _testCaseName + ".";
  for (const auto& subTest : _subTests) {
    subTest.second->run(fullQualifiedName);
    _errors += subTest.second->getNumberOfErrors();
  }

  for (tarch::tests::TestCase* const testCase : _testCases) {
    progressMessage += ".";
    testCase->run();
    const int additionalErrors = testCase->getNumberOfErrors();
    if (additionalErrors > 0) {
      _errors += additionalErrors;
    } else {
      progressMessage += ".";
    }
  }

  if (_errors == 0) {
    progressMessage += " ok (" + std::to_string(_testCases.size()) + " tests)";
  } else {
    progressMessage += " failed";
  }

  if (_writeToLog and not _testCaseName.empty() and not _testCases.empty()) {
    std::cout << progressMessage << std::endl;
  }
}

void
tarch::tests::TreeTestCaseCollection::addTestCase(TestCase* testCase) {
  addTestCase(testCase->getTestCaseName(), testCase);
}

/**
 * Adds testCase under fullQualifiedPath. The collection takes ownership if deleteTestCases is set.
 */
void
tarch::tests::TreeTestCaseCollection::addTestCase(const std::string& fullQualifiedPath, TestCase* testCase) {
  if (isNameWithoutHierarchy(fullQualifiedPath)) {
    _testCases.push_back(testCase);
  } else {
    const std::string firstPartOfIdentifier  = getFirstIdentifierInHierarchy(fullQualifiedPath);
    const std::string secondPartOfIdentifier = getRemainingPathWithoutIdentifier(fullQualifiedPath);
    if (_subTests.find(firstPartOfIdentifier) == _subTests.end()) {
      _subTests.insert(
        std::pair<std::string, TreeTestCaseCollection*>(
          firstPartOfIdentifier,
          new TreeTestCaseCollection(firstPartOfIdentifier, _deleteTestCases, _writeToLog)
        )
      );
    }
    _subTests[firstPartOfIdentifier]->addTestCase(secondPartOfIdentifier, testCase);
  }
}
