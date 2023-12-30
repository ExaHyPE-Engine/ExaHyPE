// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "MemoryManagerTest.h"

#include "tarch/memory/MemoryManager.h"
#include "tarch/tests/TestMacros.h"


tarch::memory::tests::MemoryManagerTest::MemoryManagerTest():
  TestCase("tarch::memory::tests::MemoryManagerTest") {}


void tarch::memory::tests::MemoryManagerTest::run() {
  testMethod(testMemoryAllocationsForOneDevice);
  testMethod(testMemoryAllocationsForMultipleDevices);
}


void tarch::memory::tests::MemoryManagerTest::testMemoryAllocationsForOneDevice() {
  static constexpr int NumberOfDataElements = 10000;

  // As the MemoryManager is a singleton we need to clear it here before doing any tests with
  // it as it holds already memory pages from previous unit tests.
  MemoryManager::getInstance().clear();

  // Sanity check to make sure we begin with a clean state (everything should be 0).
  validateEquals(MemoryManager::getInstance().getNumberOfUsedPages(), 0);
  validateEquals(MemoryManager::getInstance().getNumberOfFreePages(), 0);
  validateEquals(MemoryManager::getInstance().getUsedAllocatedMemoryInBytes(), 0);
  validateEquals(MemoryManager::getInstance().getFreeAllocatedMemoryInBytes(), 0);

  // Now, start to allocate some data on the emulated "device 0".
  // We differentiate between the type of memory we allocate to check that
  // The MemoryManager is able to correctly track the allocated bytes stored.
  int    device    = 0;
  float* floatData = static_cast<float*>(MemoryManager::getInstance().allocate(NumberOfDataElements * sizeof(float), tarch::memory::MemoryLocation::Heap, device));
  validate(floatData != nullptr);

  // Check the contents of the MemoryManager over all devices.
  validateEquals(MemoryManager::getInstance().getNumberOfUsedPages(), 1);
  validateEquals(MemoryManager::getInstance().getNumberOfFreePages(), 0);
  validateEquals(MemoryManager::getInstance().getUsedAllocatedMemoryInBytes(), NumberOfDataElements * sizeof(float));
  validateEquals(MemoryManager::getInstance().getFreeAllocatedMemoryInBytes(), NumberOfDataElements * sizeof(float));

  // Check the contents of the MemoryManager for one device.
  validateEquals(MemoryManager::getInstance().getNumberOfUsedPages(device), 1);
  validateEquals(MemoryManager::getInstance().getNumberOfFreePages(device), 0);
  validateEquals(MemoryManager::getInstance().getUsedAllocatedMemoryInBytes(device), NumberOfDataElements * sizeof(float));
  validateEquals(MemoryManager::getInstance().getFreeAllocatedMemoryInBytes(device), NumberOfDataElements * sizeof(float));

  // We free our data, which does not deallocate the memory but just frees the handle.
  MemoryManager::getInstance().free(floatData, device);

  // Check the contents of the MemoryManager over all devices.
  validateEquals(MemoryManager::getInstance().getNumberOfUsedPages(), 0);
  validateEquals(MemoryManager::getInstance().getNumberOfFreePages(), 1);
  validateEquals(MemoryManager::getInstance().getUsedAllocatedMemoryInBytes(), 0);
  validateEquals(MemoryManager::getInstance().getFreeAllocatedMemoryInBytes(), NumberOfDataElements * sizeof(float));

  // Check the contents of the MemoryManager for one device.
  validateEquals(MemoryManager::getInstance().getNumberOfUsedPages(device), 0);
  validateEquals(MemoryManager::getInstance().getNumberOfFreePages(device), 1);
  validateEquals(MemoryManager::getInstance().getUsedAllocatedMemoryInBytes(device), 0);
  validateEquals(MemoryManager::getInstance().getFreeAllocatedMemoryInBytes(device), NumberOfDataElements * sizeof(float));

  // Now, we allocate more different data types and see how the MemoryManager handle those.
  // Float data does not need to be allocated again, as we already have a free handle to this data.
  floatData = static_cast<float*>(MemoryManager::getInstance().allocate(NumberOfDataElements * sizeof(float), tarch::memory::MemoryLocation::Heap, device));
  validate(floatData != nullptr);

  double* doubleData = static_cast<double*>(MemoryManager::getInstance().allocate(NumberOfDataElements * sizeof(double), tarch::memory::MemoryLocation::Heap, device));
  validate(doubleData != nullptr);

  int* integerData = static_cast<int*>(MemoryManager::getInstance().allocate(NumberOfDataElements * sizeof(int), tarch::memory::MemoryLocation::Heap, device));
  validate(integerData != nullptr);

  // Check the contents of the MemoryManager over all devices.
  validateEquals(MemoryManager::getInstance().getNumberOfUsedPages(), 3);
  validateEquals(MemoryManager::getInstance().getNumberOfFreePages(), 0);
  validateEquals(
    MemoryManager::getInstance().getUsedAllocatedMemoryInBytes(), NumberOfDataElements * sizeof(float) + NumberOfDataElements * sizeof(double) + NumberOfDataElements * sizeof(int)
  );
  validateEquals(MemoryManager::getInstance().getFreeAllocatedMemoryInBytes(), NumberOfDataElements * sizeof(double) + NumberOfDataElements * sizeof(int));

  // Check the contents of the MemoryManager for one device.
  validateEquals(MemoryManager::getInstance().getNumberOfUsedPages(device), 3);
  validateEquals(MemoryManager::getInstance().getNumberOfFreePages(device), 0);
  validateEquals(
    MemoryManager::getInstance().getUsedAllocatedMemoryInBytes(device),
    NumberOfDataElements * sizeof(float) + NumberOfDataElements * sizeof(double) + NumberOfDataElements * sizeof(int)
  );
  validateEquals(MemoryManager::getInstance().getFreeAllocatedMemoryInBytes(device), NumberOfDataElements * sizeof(double) + NumberOfDataElements * sizeof(int));

  MemoryManager::getInstance().free(floatData, device);
  MemoryManager::getInstance().free(doubleData, device);
  MemoryManager::getInstance().free(integerData, device);

  // Check the contents of the MemoryManager over all devices.
  validateEquals(MemoryManager::getInstance().getNumberOfUsedPages(), 0);
  validateEquals(MemoryManager::getInstance().getNumberOfFreePages(), 3);
  validateEquals(MemoryManager::getInstance().getUsedAllocatedMemoryInBytes(), 0);
  validateEquals(MemoryManager::getInstance().getFreeAllocatedMemoryInBytes(device), NumberOfDataElements * sizeof(double) + NumberOfDataElements * sizeof(int));

  // Check the contents of the MemoryManager for one device.
  validateEquals(MemoryManager::getInstance().getNumberOfUsedPages(device), 0);
  validateEquals(MemoryManager::getInstance().getNumberOfFreePages(device), 3);
  validateEquals(MemoryManager::getInstance().getUsedAllocatedMemoryInBytes(device), 0);
  validateEquals(MemoryManager::getInstance().getFreeAllocatedMemoryInBytes(device), NumberOfDataElements * sizeof(double) + NumberOfDataElements * sizeof(int));

  floatData = static_cast<float*>(MemoryManager::getInstance().allocate(NumberOfDataElements * sizeof(float), tarch::memory::MemoryLocation::Heap, device));
  validate(floatData != nullptr);

  float* floatData1 = static_cast<float*>(MemoryManager::getInstance().allocate(NumberOfDataElements * sizeof(float), tarch::memory::MemoryLocation::Heap, device));
  validate(floatData1 != nullptr);

  float* floatData2 = static_cast<float*>(MemoryManager::getInstance().allocate(NumberOfDataElements * sizeof(float), tarch::memory::MemoryLocation::Heap, device));
  validate(floatData2 != nullptr);

  float* floatData3 = static_cast<float*>(MemoryManager::getInstance().allocate(NumberOfDataElements * sizeof(float), tarch::memory::MemoryLocation::Heap, device));
  validate(floatData3 != nullptr);

  validate(floatData != floatData1);
  validate(floatData != floatData2);
  validate(floatData != floatData3);

  // Check the contents of the MemoryManager over all devices.
  validateEquals(MemoryManager::getInstance().getNumberOfUsedPages(), 4);
  validateEquals(MemoryManager::getInstance().getNumberOfFreePages(), 1);
  validateEquals(
    MemoryManager::getInstance().getUsedAllocatedMemoryInBytes(), NumberOfDataElements * sizeof(float) + NumberOfDataElements * sizeof(double) + NumberOfDataElements * sizeof(int)
  );
  validateEquals(MemoryManager::getInstance().getFreeAllocatedMemoryInBytes(), NumberOfDataElements * sizeof(double) + NumberOfDataElements * sizeof(int));

  // Check the contents of the MemoryManager for one device.
  validateEquals(MemoryManager::getInstance().getNumberOfUsedPages(device), 4);
  validateEquals(MemoryManager::getInstance().getNumberOfFreePages(device), 1);
  validateEquals(
    MemoryManager::getInstance().getUsedAllocatedMemoryInBytes(device),
    NumberOfDataElements * sizeof(float) + NumberOfDataElements * sizeof(double) + NumberOfDataElements * sizeof(int)
  );
  validateEquals(MemoryManager::getInstance().getFreeAllocatedMemoryInBytes(device), NumberOfDataElements * sizeof(double) + NumberOfDataElements * sizeof(int));

  MemoryManager::getInstance().free(floatData, device);
  MemoryManager::getInstance().free(floatData1, device);
  MemoryManager::getInstance().free(floatData2, device);
  MemoryManager::getInstance().free(floatData3, device);

  // Check the contents of the MemoryManager over all devices.
  validateEquals(MemoryManager::getInstance().getNumberOfUsedPages(), 0);
  validateEquals(MemoryManager::getInstance().getNumberOfFreePages(), 5);
  validateEquals(MemoryManager::getInstance().getUsedAllocatedMemoryInBytes(), 0);
  validateEquals(MemoryManager::getInstance().getFreeAllocatedMemoryInBytes(device), NumberOfDataElements * sizeof(double) + NumberOfDataElements * sizeof(int));

  // Check the contents of the MemoryManager for one device.
  validateEquals(MemoryManager::getInstance().getNumberOfUsedPages(device), 0);
  validateEquals(MemoryManager::getInstance().getNumberOfFreePages(device), 5);
  validateEquals(MemoryManager::getInstance().getUsedAllocatedMemoryInBytes(device), 0);
  validateEquals(MemoryManager::getInstance().getFreeAllocatedMemoryInBytes(device), NumberOfDataElements * sizeof(double) + NumberOfDataElements * sizeof(int));

  // Clear all data and make sure we again have a clean state.
  MemoryManager::getInstance().clear();

  validateEquals(MemoryManager::getInstance().getNumberOfUsedPages(), 0);
  validateEquals(MemoryManager::getInstance().getNumberOfFreePages(), 0);
  validateEquals(MemoryManager::getInstance().getUsedAllocatedMemoryInBytes(), 0);
  validateEquals(MemoryManager::getInstance().getFreeAllocatedMemoryInBytes(), 0);

  // TODO: Test also memory locations lookups.
}

void tarch::memory::tests::MemoryManagerTest::testMemoryAllocationsForMultipleDevices() {
  static constexpr int NumberOfDataElements = 10000;

  MemoryManager::getInstance().clear();

  validateEquals(MemoryManager::getInstance().getNumberOfUsedPages(), 0);
  validateEquals(MemoryManager::getInstance().getNumberOfFreePages(), 0);
  validateEquals(MemoryManager::getInstance().getUsedAllocatedMemoryInBytes(), 0);
  validateEquals(MemoryManager::getInstance().getFreeAllocatedMemoryInBytes(), 0);

  int device0 = 0;
  int device1 = 1;
  int device2 = 2;

  float* floatData0 = static_cast<float*>(MemoryManager::getInstance().allocate(NumberOfDataElements * sizeof(float), tarch::memory::MemoryLocation::Heap, device0));
  validate(floatData0 != nullptr);

  double* doubleData1 = static_cast<double*>(MemoryManager::getInstance().allocate(NumberOfDataElements * sizeof(double), tarch::memory::MemoryLocation::Heap, device1));
  validate(doubleData1 != nullptr);

  long double* longData2 = static_cast<long double*>(MemoryManager::getInstance().allocate(NumberOfDataElements * sizeof(long double), tarch::memory::MemoryLocation::Heap, device2));
  validate(longData2 != nullptr);

  // Check the contents of the MemoryManager over all devices.
  validateEquals(MemoryManager::getInstance().getNumberOfUsedPages(), 3);
  validateEquals(MemoryManager::getInstance().getNumberOfFreePages(), 0);
  validateEquals(
    MemoryManager::getInstance().getUsedAllocatedMemoryInBytes(),
    NumberOfDataElements * sizeof(float) + NumberOfDataElements * sizeof(double) + NumberOfDataElements * sizeof(long double)
  );
  validateEquals(
    MemoryManager::getInstance().getFreeAllocatedMemoryInBytes(),
    NumberOfDataElements * sizeof(float) + NumberOfDataElements * sizeof(double) + NumberOfDataElements * sizeof(long double)
  );

  // Check the contents of the MemoryManager for one device.
  validateEquals(MemoryManager::getInstance().getNumberOfUsedPages(device0), 1);
  validateEquals(MemoryManager::getInstance().getNumberOfFreePages(device0), 0);
  validateEquals(MemoryManager::getInstance().getUsedAllocatedMemoryInBytes(device0), NumberOfDataElements * sizeof(float));
  validateEquals(MemoryManager::getInstance().getFreeAllocatedMemoryInBytes(device0), NumberOfDataElements * sizeof(float));

  validateEquals(MemoryManager::getInstance().getNumberOfUsedPages(device1), 1);
  validateEquals(MemoryManager::getInstance().getNumberOfFreePages(device1), 0);
  validateEquals(MemoryManager::getInstance().getUsedAllocatedMemoryInBytes(device1), NumberOfDataElements * sizeof(double));
  validateEquals(MemoryManager::getInstance().getFreeAllocatedMemoryInBytes(device1), NumberOfDataElements * sizeof(double));

  validateEquals(MemoryManager::getInstance().getNumberOfUsedPages(device2), 1);
  validateEquals(MemoryManager::getInstance().getNumberOfFreePages(device2), 0);
  validateEquals(MemoryManager::getInstance().getUsedAllocatedMemoryInBytes(device2), NumberOfDataElements * sizeof(long double));
  validateEquals(MemoryManager::getInstance().getFreeAllocatedMemoryInBytes(device2), NumberOfDataElements * sizeof(long double));

  MemoryManager::getInstance().free(floatData0, device0);
  MemoryManager::getInstance().free(doubleData1, device1);
  MemoryManager::getInstance().free(longData2, device2);

  // Check the contents of the MemoryManager over all devices.
  validateEquals(MemoryManager::getInstance().getNumberOfUsedPages(), 0);
  validateEquals(MemoryManager::getInstance().getNumberOfFreePages(), 3);
  validateEquals(MemoryManager::getInstance().getUsedAllocatedMemoryInBytes(), 0);
  validateEquals(
    MemoryManager::getInstance().getFreeAllocatedMemoryInBytes(),
    NumberOfDataElements * sizeof(float) + NumberOfDataElements * sizeof(double) + NumberOfDataElements * sizeof(long double)
  );

  // Check the contents of the MemoryManager for one device.
  validateEquals(MemoryManager::getInstance().getNumberOfUsedPages(device0), 0);
  validateEquals(MemoryManager::getInstance().getNumberOfFreePages(device0), 1);
  validateEquals(MemoryManager::getInstance().getUsedAllocatedMemoryInBytes(device0), 0);
  validateEquals(MemoryManager::getInstance().getFreeAllocatedMemoryInBytes(device0), NumberOfDataElements * sizeof(float));

  validateEquals(MemoryManager::getInstance().getNumberOfUsedPages(device1), 0);
  validateEquals(MemoryManager::getInstance().getNumberOfFreePages(device1), 1);
  validateEquals(MemoryManager::getInstance().getUsedAllocatedMemoryInBytes(device1), 0);
  validateEquals(MemoryManager::getInstance().getFreeAllocatedMemoryInBytes(device1), NumberOfDataElements * sizeof(double));

  validateEquals(MemoryManager::getInstance().getNumberOfUsedPages(device2), 0);
  validateEquals(MemoryManager::getInstance().getNumberOfFreePages(device2), 1);
  validateEquals(MemoryManager::getInstance().getUsedAllocatedMemoryInBytes(device2), 0);
  validateEquals(MemoryManager::getInstance().getFreeAllocatedMemoryInBytes(device2), NumberOfDataElements * sizeof(long double));

  MemoryManager::getInstance().clear();

  validateEquals(MemoryManager::getInstance().getNumberOfUsedPages(), 0);
  validateEquals(MemoryManager::getInstance().getNumberOfFreePages(), 0);
  validateEquals(MemoryManager::getInstance().getUsedAllocatedMemoryInBytes(), 0);
  validateEquals(MemoryManager::getInstance().getFreeAllocatedMemoryInBytes(), 0);

  // TODO: Test also memory locations lookups.
}
