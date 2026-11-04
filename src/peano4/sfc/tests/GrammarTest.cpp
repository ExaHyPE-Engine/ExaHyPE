// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "GrammarTest.h"

#include "peano4/sfc/Grammar.h"

namespace {
  enum NonTerminal : std::uint8_t { First, Second, Third };
} // namespace

peano4::sfc::tests::GrammarTest::GrammarTest() :
  TestCase("peano4::sfc::tests::GrammarTest") {}

void
peano4::sfc::tests::GrammarTest::run() {
  testMethod(testStepsNameAxisAndDirection);
  testMethod(testProductionSeparatesChildrenAndSteps);
}

void
peano4::sfc::tests::GrammarTest::testStepsNameAxisAndDirection() {
  using Step = peano4::sfc::Grammar::Step;
  validateEquals(peano4::sfc::Grammar::getAxis(Step::Left), 0);
  validateEquals(peano4::sfc::Grammar::getAxis(Step::Right), 0);
  validateEquals(peano4::sfc::Grammar::getAxis(Step::Down), 1);
  validateEquals(peano4::sfc::Grammar::getAxis(Step::Up), 1);
  validateEquals(peano4::sfc::Grammar::getAxis(Step::Back), 2);
  validateEquals(peano4::sfc::Grammar::getAxis(Step::Front), 2);
  validateEquals(peano4::sfc::Grammar::getDirection(Step::Left), -1);
  validateEquals(peano4::sfc::Grammar::getDirection(Step::Right), 1);
  validateEquals(peano4::sfc::Grammar::getDirection(Step::Down), -1);
  validateEquals(peano4::sfc::Grammar::getDirection(Step::Up), 1);
  validateEquals(peano4::sfc::Grammar::getDirection(Step::Back), -1);
  validateEquals(peano4::sfc::Grammar::getDirection(Step::Front), 1);
}

void
peano4::sfc::tests::GrammarTest::testProductionSeparatesChildrenAndSteps() {
  using Step                    = peano4::sfc::Grammar::Step;
  constexpr auto TestProduction = peano4::sfc::Grammar::production(Third, First, Step::Up, Second, Step::Right, Third);
  static_assert(TestProduction.children.size() == 3);
  static_assert(TestProduction.steps.size() == 2);
  validateEquals(TestProduction.leftHandSide, Third);
  validateEquals(TestProduction.children[0], First);
  validateEquals(TestProduction.children[1], Second);
  validateEquals(TestProduction.children[2], Third);
  validateEquals(TestProduction.steps[0], Step::Up);
  validateEquals(TestProduction.steps[1], Step::Right);
}
