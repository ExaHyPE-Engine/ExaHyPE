#include "InterpolationRestrictionTest.h"



exahype2::fv::tests::InterpolationRestrictionTest::InterpolationRestrictionTest():
  TestCase ("exahype2::fv::tests::InterpolationRestrictionTest") {
}


void exahype2::fv::tests::InterpolationRestrictionTest::run() {
  testMethod (testPiecewiseConstantInterpolation);
}


void exahype2::fv::tests::InterpolationRestrictionTest::testPiecewiseConstantInterpolation() {
  validate(false);
}
