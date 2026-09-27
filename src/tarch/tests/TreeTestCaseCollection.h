// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/tests/TestCase.h"

namespace tarch::tests {
  class TreeTestCaseCollection;
} // namespace tarch::tests

/**
 * Holds test cases and further collections in a tree that follows the ::
 * separated names of the test cases.
 *
 * @author Tobias Weinzierl
 * @author Wolfgang Eckhardt
 */
class tarch::tests::TreeTestCaseCollection final : public tarch::tests::TestCase {
public:
  /**
   * @param treeTestCaseCollectionName Name without ::. The empty string makes
   *   the collection a container that runs its sub collections without writing
   *   a progress line of its own.
   */
  TreeTestCaseCollection(
    const std::string& treeTestCaseCollectionName = "",
    bool               deleteTestCases            = true,
    bool               writeToLog                 = true
  );

  ~TreeTestCaseCollection() override;

  void run() override;

  /**
   * Runs every test case of this collection and puts the prefix in front of
   * each name.
   */
  void run(const std::string& prefix);

  /**
   * Adds the test case under its fully qualified path. The collection deletes
   * it in the destructor when the constructor was called with
   * deleteTestCases set.
   */
  void addTestCase(const std::string& fullQualifiedPath, TestCase* testCase);

  /**
   * Adds the test case under the path that getTestCaseName() returns.
   */
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
