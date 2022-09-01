#include "RiemannTest.h"
#include "../Rusanov.h"


exahype2::dg::rusanov::tests::RiemannTest::RiemannTest():
  TestCase ("exahype2::dg::rusanov::tests::RiemannTest") {

}


void exahype2::dg::rusanov::tests::RiemannTest::run() {
  testMethod( testIntegrateOverRiemannSolutionsAndAddToVolume_GaussLegendre );
}





void exahype2::dg::rusanov::tests::RiemannTest::testIntegrateOverRiemannSolutionsAndAddToVolume_GaussLegendre() {

}


