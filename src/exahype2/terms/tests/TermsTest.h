// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/tests/TestCase.h"

namespace exahype2::terms::tests {
  class TermsTest;
} // namespace exahype2::terms::tests

class exahype2::terms::tests::TermsTest final : public tarch::tests::TestCase {
public:
  TermsTest();
  TermsTest(const TermsTest& other)            = delete;
  TermsTest(TermsTest&& other)                 = delete;
  TermsTest& operator=(const TermsTest& other) = delete;
  TermsTest& operator=(TermsTest&& other)      = delete;
  ~TermsTest() override                        = default;

  void run() override;

private:
  struct Tracer;
  struct Complete;
  struct FiniteVolumeSolver;
  struct ADERDGSolver;
  struct RungeKuttaDGSolver;
  template <unsigned long long PeriodicAxes>
  struct FiniteVolumeSolverWithoutBoundaryConditions;
  struct Defective;

  void testEveryTermOfACompleteSolverSatisfiesItsConcept();
  void testAMissingOrMistypedTermFailsItsConcept();
  void testASolverSatisfiesTheConceptOfItsSolverFamily();
  void testASolverWithoutTheConstantsOfItsSolverFamilyFailsItsConcept();
  void testOnlyADomainPeriodicAlongEveryAxisOmitsTheBoundaryConditions();
};
