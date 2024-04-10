#include "tarch/tests/TreeTestCaseCollection.h"
#include "tarch/Assertions.h"


#include <iostream>


tarch::logging::Log tarch::tests::TreeTestCaseCollection::_log("tarch::tests::TreeTestCaseCollection");


tarch::tests::TreeTestCaseCollection::TreeTestCaseCollection(const std::string& treeTestCaseCollectionName, bool deleteTestCases, bool writeToLog):
  TestCase::TestCase( treeTestCaseCollectionName ),
  _writeToLog(writeToLog),
  _deleteTestCases(deleteTestCases) {
  assertion3( isNameWithoutHierarchy( treeTestCaseCollectionName ), treeTestCaseCollectionName, deleteTestCases, writeToLog);
}


tarch::tests::TreeTestCaseCollection::~TreeTestCaseCollection() {
  if (_deleteTestCases) {
    for (std::list<tarch::tests::TestCase*>::iterator p = _testCases.begin(); p!=_testCases.end(); p++ ) {
      tarch::tests::TestCase* currentTestCase = *p;
      delete currentTestCase;
    }
    for (std::map<std::string, TreeTestCaseCollection*>::iterator p = _subTests.begin(); p!=_subTests.end(); p++ ) {
      TreeTestCaseCollection* currentTestCase = (*p).second;
      delete currentTestCase;
    }
  }
}


bool tarch::tests::TreeTestCaseCollection::isNameWithoutHierarchy(const std::string& testCaseName) {
  return testCaseName.rfind("::") == std::string::npos;
}


std::string tarch::tests::TreeTestCaseCollection::getFirstIdentifierInHierarchy(const std::string& testCaseName) {
  std::string result = testCaseName.substr(0, testCaseName.find("::"));

  assertion2( isNameWithoutHierarchy( result), result, testCaseName );

  return result;
}


std::string tarch::tests::TreeTestCaseCollection::getRemainingPathWithoutIdentifier(const std::string& testCaseName) {
  assertion( !isNameWithoutHierarchy( testCaseName) );

  std::string result = testCaseName.substr(testCaseName.find("::")+2);

  return result;
}


void tarch::tests::TreeTestCaseCollection::run() {
  run( "" );
}


void tarch::tests::TreeTestCaseCollection::run( const std::string& prefix ) {
  std::string logInformation;
  if ( (prefix + _testCaseName).length()>0 ) {
    logInformation = "running test case collection \"" + prefix + _testCaseName + "\" ...";
  }
  else {
    logInformation = "running test case collection ...";
  }

  if (
    _subTests.empty()
    and
    _testCases.empty()
  ) {
    logInformation += "... WARNING: no tests defined ... ";
    double x = 3.0/0.0;
    logInformation += std::to_string(x);
  }

  // run nested test cases recursively
  std::string fullQualifiedName = _testCaseName.length()>0 ? prefix  +_testCaseName + "." : prefix;
  for (std::map<std::string, TreeTestCaseCollection*>::iterator p = _subTests.begin(); p!=_subTests.end(); p++ ) {
    (*p).second->run( fullQualifiedName );
    _errors += (*p).second->getNumberOfErrors();
  }

  for (std::list<tarch::tests::TestCase*>::iterator p = _testCases.begin(); p!=_testCases.end(); p++ ) {
    tarch::tests::TestCase* currentTestCase = *p;
    logInformation += ".";
    currentTestCase->run();
    int additionalErrors = currentTestCase->getNumberOfErrors();
    if (additionalErrors>0) {
      logInformation += "x";
      _errors += currentTestCase->getNumberOfErrors();
    }
    else {
      logInformation += ".";
    }
  }

  if ( _errors==0 and _testCases.empty() and _subTests.empty() ) {
    logInformation += " pass (no local tests)";
  }
  else if ( _errors==0 and _testCases.empty() ) {
    logInformation += " pass (no local tests, subtests all successful)";
  }
  else if (_errors==0 and _testCases.size()==1) {
    logInformation += " ok (1 test)";
  }
  else if (_errors==0) {
    logInformation += " ok (" + std::to_string(_testCases.size()) + " tests)";
  }
  else {
    logInformation += " failed";
  }

  if ( _writeToLog and _testCaseName!="" ) {
    logInfo("run()",logInformation );
  }
}


void tarch::tests::TreeTestCaseCollection::addTestCase( TestCase* testCase ) {
  addTestCase( testCase->getTestCaseName(), testCase );
}


void tarch::tests::TreeTestCaseCollection::addTestCase( const std::string& fullQualifiedPath, TestCase* testCase ) {
  if ( isNameWithoutHierarchy(fullQualifiedPath) ) {
    _testCases.push_back(testCase);
  }
  else {
    std::string firstPartOfIdentifier  = getFirstIdentifierInHierarchy( fullQualifiedPath );
    std::string secondPartOfIdentifier = getRemainingPathWithoutIdentifier( fullQualifiedPath );
    if (_subTests.find(firstPartOfIdentifier) == _subTests.end() ) {
      _subTests.insert(
        std::pair<std::string,TreeTestCaseCollection*>(
          firstPartOfIdentifier,
          new TreeTestCaseCollection( firstPartOfIdentifier, _deleteTestCases, _writeToLog )
        )
      );
    }
    _subTests[firstPartOfIdentifier]->addTestCase(secondPartOfIdentifier, testCase);
  }
}
