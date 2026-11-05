// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "peano4/sfc/Grammar.h"

#include "tarch/testing/UnitTest.h"

namespace {
  enum NonTerminal : std::uint8_t { First, Second, Third };
} // namespace

UNIT_TEST("Every turtle step returns the axis it moves along and the direction of the move") {
  GIVEN("The turtle steps left, right, down, up, back and front") {
    constexpr auto Left  = peano4::sfc::Grammar::Step::Left;
    constexpr auto Right = peano4::sfc::Grammar::Step::Right;
    constexpr auto Down  = peano4::sfc::Grammar::Step::Down;
    constexpr auto Up    = peano4::sfc::Grammar::Step::Up;
    constexpr auto Back  = peano4::sfc::Grammar::Step::Back;
    constexpr auto Front = peano4::sfc::Grammar::Step::Front;

    WHEN("The test reads the axis and the direction of every step") {
      const int axisOfLeft       = peano4::sfc::Grammar::getAxis(Left);
      const int axisOfRight      = peano4::sfc::Grammar::getAxis(Right);
      const int axisOfDown       = peano4::sfc::Grammar::getAxis(Down);
      const int axisOfUp         = peano4::sfc::Grammar::getAxis(Up);
      const int axisOfBack       = peano4::sfc::Grammar::getAxis(Back);
      const int axisOfFront      = peano4::sfc::Grammar::getAxis(Front);
      const int directionOfLeft  = peano4::sfc::Grammar::getDirection(Left);
      const int directionOfRight = peano4::sfc::Grammar::getDirection(Right);
      const int directionOfDown  = peano4::sfc::Grammar::getDirection(Down);
      const int directionOfUp    = peano4::sfc::Grammar::getDirection(Up);
      const int directionOfBack  = peano4::sfc::Grammar::getDirection(Back);
      const int directionOfFront = peano4::sfc::Grammar::getDirection(Front);

      THEN("Left and right move along axis zero, down and up along axis one and back and front along axis two") {
        VALIDATE_EQUALS(axisOfLeft, 0);
        VALIDATE_EQUALS(axisOfRight, 0);
        VALIDATE_EQUALS(axisOfDown, 1);
        VALIDATE_EQUALS(axisOfUp, 1);
        VALIDATE_EQUALS(axisOfBack, 2);
        VALIDATE_EQUALS(axisOfFront, 2);
      }

      THEN("Left, down and back move in the negative direction and right, up and front in the positive one") {
        VALIDATE_EQUALS(directionOfLeft, -1);
        VALIDATE_EQUALS(directionOfRight, 1);
        VALIDATE_EQUALS(directionOfDown, -1);
        VALIDATE_EQUALS(directionOfUp, 1);
        VALIDATE_EQUALS(directionOfBack, -1);
        VALIDATE_EQUALS(directionOfFront, 1);
      }
    }
  }
}

UNIT_TEST("A production separates its child non-terminals from the turtle steps between them") {
  GIVEN("The left-hand side Third, the children First, Second and Third and the turtle steps Up and Right") {
    using Step = peano4::sfc::Grammar::Step;

    constexpr NonTerminal                LeftHandSide = Third;
    constexpr std::array<NonTerminal, 3> Children{First, Second, Third};
    constexpr std::array<Step, 2>        Steps{Step::Up, Step::Right};

    WHEN("The grammar builds the production from the left-hand side and the children with the steps between them") {
      constexpr auto TestProduction =
        peano4::sfc::Grammar::production(LeftHandSide, Children[0], Steps[0], Children[1], Steps[1], Children[2]);
      static_assert(TestProduction.children.size() == 3);
      static_assert(TestProduction.steps.size() == 2);

      THEN("The production stores the left-hand side, the three children and the two steps in their order") {
        VALIDATE_EQUALS(TestProduction.leftHandSide, Third);
        VALIDATE_EQUALS(TestProduction.children[0], First);
        VALIDATE_EQUALS(TestProduction.children[1], Second);
        VALIDATE_EQUALS(TestProduction.children[2], Third);
        VALIDATE_EQUALS(TestProduction.steps[0], Step::Up);
        VALIDATE_EQUALS(TestProduction.steps[1], Step::Right);
      }
    }
  }
}
