#include "DGUtilsTest.h"
#include "TestUtils.h"
#include "../DGUtils.h"

/*
 * We use precomputed values for lagrange polynomials as basis function
 *  on Gauss-Legendre nodes
 */
exahype2::dg::tests::DGUtilsTest::DGUtilsTest():
  TestCase ("exahype2::dg::tests::DGUtilsTest") {
}


void exahype2::dg::tests::DGUtilsTest::run(){
  testMethod( testGetIndex );
  testMethod( testComputeGradientOnConstantSolution );
}


void exahype2::dg::tests::DGUtilsTest::testGetIndex() {
  const int order = 3;
  const int nodesPerAxis = order+1;
  const int nodesPerCell = getNodesPerCell(nodesPerAxis);

  validateEquals(nodesPerCell, std::pow(nodesPerAxis,Dimensions));

  tarch::la::Vector<Dimensions,int> strides = getStrides(nodesPerAxis);
  validateEquals(strides[Dimensions-1], std::pow(nodesPerAxis,Dimensions-1));

  tarch::la::Vector<Dimensions,int> firstIndex = getIndex(0, strides);
  for(int i=0; i<Dimensions; i++){
    validateEquals(firstIndex[i], 0);
  }

  tarch::la::Vector<Dimensions,int> lastIndex = getIndex(nodesPerCell-1, strides);
  for(int i=0; i<Dimensions; i++){
    validateEquals(lastIndex[i], nodesPerAxis-1);
  }
}


void exahype2::dg::tests::DGUtilsTest::testComputeGradientOnConstantSolution() {
  #if Dimensions==2
  const int order        = 3;
  const int nodesPerAxis = order+1;
  const int nodesPerCell = getNodesPerCell(nodesPerAxis);

  // 1 unknown, 1 auxiliary variable
  double cellQin[2*nodesPerCell];
  double gradientValues[2*Dimensions];
  for(int i=0; i<2*nodesPerCell; i++){
    cellQin[i] = 1.0;
  }

  for(int node=0; node<nodesPerCell; node++){
    computeGradient(
      cellQin,
      DerivativeOperatorLagrangeP3,
      10, //invDx
      nodesPerAxis,
      2, //strideQ
      node,
      gradientValues
      );
    for(int var=0; var<Dimensions; var++){
      validateNumericalEquals(gradientValues[var], 0.0);
    }
  }
  #endif
}
