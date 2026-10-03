// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/tests/TestCase.h"

namespace tarch::tests {
  class TreeTestCaseCollection;
} // namespace tarch::tests

/**
 * Test cases and nested collections in a tree along the ::-separated parts of the test case names.
 *
 * @author Tobias Weinzierl
 * @author Wolfgang Eckhardt
 */
class tarch::tests::TreeTestCaseCollection final : public tarch::tests::TestCase {
public:
  TreeTestCaseCollection(
    const std::string& treeTestCaseCollectionName = "",
    bool               deleteTestCases            = true,
    bool               writeToLog                 = true
  );

  ~TreeTestCaseCollection() override;

  void run() override;

  void run(const std::string& prefix);

  void addTestCase(const std::string& fullQualifiedPath, TestCase* testCase);

  void addTestCase(TestCase* testCase);

private:
  std::list<TestCase*>                           _testCases;
  std::map<std::string, TreeTestCaseCollection*> _subTests;

  /**
   * The run() call writes a progress line if this flag is set.
   */
  bool _writeToLog{true};

  /**
   * The destructor deletes the test cases and sub collections if this flag is
   * set.
   */
  bool _deleteTestCases{true};

  static bool isNameWithoutHierarchy(const std::string& testCaseName);
  static std::string getFirstIdentifierInHierarchy(const std::string& testCaseName);
  static std::string getRemainingPathWithoutIdentifier(const std::string& testCaseName);
};
