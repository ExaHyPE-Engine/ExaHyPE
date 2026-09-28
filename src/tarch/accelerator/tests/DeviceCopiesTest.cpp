// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "DeviceCopiesTest.h"

#include "tarch/Assertions.h"
#include "tarch/accelerator/Buffer.h"
#include "tarch/accelerator/CompileTarget.h"
#include "tarch/accelerator/DeviceCopies.h"
#include "tarch/accelerator/MemoryLocation.h"
#include "tarch/accelerator/ParallelFor.h"
#include "tarch/accelerator/Range.h"
#include "tarch/la/Vector.h"

namespace {
  TARGET_DECLARE
  double                                                 hostValue{0.0};
  [[maybe_unused]] DEVICE_VARIABLE double                deviceValue;
  DEVICE_VARIABLE constexpr tarch::la::Vector<2, double> ConstantVector{1.5, -2.5};
  DEVICE_VARIABLE constexpr double                       ConstantScalar = 4.0;

  /**
   * Writes value into the device copy of this translation unit.
   */
  struct WriteDeviceValue final {
    double value{0.0};

    INLINE_METHOD GPU_CALLABLE_METHOD void operator()([[maybe_unused]] const int workItem) const {
      COMPILE_IF_ON_DEVICE(deviceValue = value;)
    }
  };

  /**
   * Returns the device copy on the device and the host value on the host.
   */
  INLINE_METHOD GPU_CALLABLE_METHOD double getValue() {
    COMPILE_IF_ON_DEVICE(return deviceValue;)
    COMPILE_IF_ON_HOST(return hostValue;)
  }

  /**
   * Writes the value a kernel reads into target.
   */
  struct ReadValue final {
    double* target{nullptr};

    INLINE_METHOD GPU_CALLABLE_METHOD void operator()([[maybe_unused]] const int workItem) const {
      *target = getValue();
    }
  };

  /**
   * Returns the entry of ConstantVector at index plus the larger of index and ConstantScalar.
   */
  INLINE_METHOD GPU_CALLABLE_METHOD double getConstantValue(const int index) {
    assertion(index >= 0 and index < 2, index);
    return ConstantVector[index] + std::max(static_cast<double>(index), ConstantScalar);
  }

  /**
   * Writes the constant value of every work item into target.
   */
  struct ReadConstants final {
    double* target{nullptr};

    INLINE_METHOD GPU_CALLABLE_METHOD void operator()(const int workItem) const {
      target[workItem] = getConstantValue(workItem);
    }
  };
  TARGET_DECLARE_END

  const bool deviceValueIsAdded = tarch::accelerator::addDeviceCopy<WriteDeviceValue>(hostValue);

  /**
   * Returns the value a kernel reads.
   */
  double getValueOfAKernel() {
    tarch::accelerator::Buffer<double> target(1, tarch::MemoryLocation::Device);
    tarch::accelerator::parallelFor(tarch::accelerator::Range(1), ReadValue{target.getData()});
    double result{0.0};
    target.copyToHost(&result, 1);
    return result;
  }
} // namespace

tarch::accelerator::tests::DeviceCopiesTest::DeviceCopiesTest() :
  tarch::tests::TestCase("tarch::accelerator::tests::DeviceCopiesTest") {}

void
tarch::accelerator::tests::DeviceCopiesTest::run() {
  testMethod(testAKernelReadsTheValueOfTheLastUpload);
  testMethod(testHostCodeAndAKernelReadTheSameConstants);
}

/**
 * A kernel reads the host value of the last upload from the device copy.
 */
void
tarch::accelerator::tests::DeviceCopiesTest::testAKernelReadsTheValueOfTheLastUpload() {
  validate(deviceValueIsAdded);

  hostValue = 42.5;
  tarch::accelerator::uploadDeviceCopies();
  validateNumericalEquals(getValueOfAKernel(), 42.5);

  hostValue = -3.0;
  tarch::accelerator::uploadDeviceCopies();
  validateNumericalEquals(getValueOfAKernel(), -3.0);
}

/**
 * Host code and a kernel read the same values from a constexpr variable that carries DEVICE_VARIABLE.
 */
void
tarch::accelerator::tests::DeviceCopiesTest::testHostCodeAndAKernelReadTheSameConstants() {
  tarch::accelerator::Buffer<double> target(2, tarch::MemoryLocation::Device);
  tarch::accelerator::parallelFor(tarch::accelerator::Range(2), ReadConstants{target.getData()});
  double valuesOfAKernel[2]{0.0, 0.0};
  target.copyToHost(valuesOfAKernel, 2);

  validateNumericalEquals(getConstantValue(0), 5.5);
  validateNumericalEquals(getConstantValue(1), 1.5);
  for (int index = 0; index < 2; index++) {
    validateNumericalEqualsWithParams1(valuesOfAKernel[index], getConstantValue(index), index);
  }
}
