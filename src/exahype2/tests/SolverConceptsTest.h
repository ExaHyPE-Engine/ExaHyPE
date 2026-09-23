// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/tests/TestCase.h"

namespace exahype2::tests {
  class SolverConceptsTest;
} // namespace exahype2::tests

class exahype2::tests::SolverConceptsTest final : public tarch::tests::TestCase {
public:
  SolverConceptsTest();
  SolverConceptsTest(const SolverConceptsTest&)            = delete;
  SolverConceptsTest(SolverConceptsTest&&)                 = delete;
  SolverConceptsTest& operator=(const SolverConceptsTest&) = delete;
  SolverConceptsTest& operator=(SolverConceptsTest&&)      = delete;
  ~SolverConceptsTest() override                           = default;

  void run() override;

private:
  void testEveryTermOfACompleteSolverSatisfiesItsConcept();
  void testAMissingOrMistypedTermFailsItsConcept();
  void testAFamilySolverSatisfiesTheConceptOfItsFamily();
  void testASolverWithoutTheConstantsOfItsFamilyFailsItsConcept();
};
