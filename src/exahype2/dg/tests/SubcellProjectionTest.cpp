// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "SubcellProjectionTest.h"

#include "exahype2/dg/SubcellProjection.h"

exahype2::dg::tests::SubcellProjectionTest::SubcellProjectionTest() :
  tarch::tests::TestCase("exahype2::dg::tests::SubcellProjectionTest") {}

void exahype2::dg::tests::SubcellProjectionTest::run() {
  testMethod(testOneDimensionalIdentityLeavesDataUnchanged);
  testMethod(testTwoDimensionalIdentityLeavesDataUnchanged);
  testMethod(testThreeDimensionalIdentityLeavesDataUnchanged);
  testMethod(testProjectionPreservesAConstantField);
}

void exahype2::dg::tests::SubcellProjectionTest::testOneDimensionalIdentityLeavesDataUnchanged() {
  constexpr int                                                       NumberOfDofsPerAxis = 2;
  constexpr int                                                       ValuesPerDof        = 2;
  const std::array<double, NumberOfDofsPerAxis * NumberOfDofsPerAxis> identity{1.0, 0.0, 0.0, 1.0};
  const std::array<double, NumberOfDofsPerAxis * ValuesPerDof>        input{1.0, 2.0, 3.0, 4.0};
  std::array<double, NumberOfDofsPerAxis * ValuesPerDof>              output{};

  exahype2::dg::projectTensorProduct<1, NumberOfDofsPerAxis, NumberOfDofsPerAxis, ValuesPerDof>(
    input.data(),
    output.data(),
    identity.data()
  );

  for (int index = 0; index < NumberOfDofsPerAxis * ValuesPerDof; index++) {
    validateNumericalEquals(output[index], input[index]);
  }
}

void exahype2::dg::tests::SubcellProjectionTest::testTwoDimensionalIdentityLeavesDataUnchanged() {
  constexpr int                                                       NumberOfDofsPerAxis = 2;
  const std::array<double, NumberOfDofsPerAxis * NumberOfDofsPerAxis> identity{1.0, 0.0, 0.0, 1.0};
  const std::array<double, NumberOfDofsPerAxis * NumberOfDofsPerAxis> input{1.0, 2.0, 3.0, 4.0};
  std::array<double, NumberOfDofsPerAxis * NumberOfDofsPerAxis>       output{};

  exahype2::dg::projectTensorProduct<2, NumberOfDofsPerAxis, NumberOfDofsPerAxis, 1>(
    input.data(),
    output.data(),
    identity.data()
  );

  for (int index = 0; index < NumberOfDofsPerAxis * NumberOfDofsPerAxis; index++) {
    validateNumericalEquals(output[index], input[index]);
  }
}

void exahype2::dg::tests::SubcellProjectionTest::testThreeDimensionalIdentityLeavesDataUnchanged() {
  constexpr int                                                       NumberOfDofsPerAxis = 2;
  const std::array<double, NumberOfDofsPerAxis * NumberOfDofsPerAxis> identity{1.0, 0.0, 0.0, 1.0};
  const std::array<double, NumberOfDofsPerAxis * NumberOfDofsPerAxis * NumberOfDofsPerAxis>
    input{1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0};
  std::array<double, NumberOfDofsPerAxis * NumberOfDofsPerAxis * NumberOfDofsPerAxis> output{};

  exahype2::dg::projectTensorProduct<3, NumberOfDofsPerAxis, NumberOfDofsPerAxis, 1>(
    input.data(),
    output.data(),
    identity.data()
  );

  for (int index = 0; index < NumberOfDofsPerAxis * NumberOfDofsPerAxis * NumberOfDofsPerAxis; index++) {
    validateNumericalEquals(output[index], input[index]);
  }
}

void exahype2::dg::tests::SubcellProjectionTest::testProjectionPreservesAConstantField() {
  constexpr int    NumberOfDofsPerAxis = 2;
  constexpr double Constant            = 7.0;
  // Each column sums to one, so the projection reproduces a constant field exactly.
  const std::array<double, NumberOfDofsPerAxis * NumberOfDofsPerAxis> columnStochasticMatrix{0.25, 0.75, 0.75, 0.25};
  const std::array<double, NumberOfDofsPerAxis * NumberOfDofsPerAxis> input{Constant, Constant, Constant, Constant};
  std::array<double, NumberOfDofsPerAxis * NumberOfDofsPerAxis>       output{};

  exahype2::dg::projectTensorProduct<2, NumberOfDofsPerAxis, NumberOfDofsPerAxis, 1>(
    input.data(),
    output.data(),
    columnStochasticMatrix.data()
  );

  for (int index = 0; index < NumberOfDofsPerAxis * NumberOfDofsPerAxis; index++) {
    validateNumericalEquals(output[index], Constant);
  }
}
