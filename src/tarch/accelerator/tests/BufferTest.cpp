// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "BufferTest.h"

#include "tarch/accelerator/Buffer.h"
#include "tarch/accelerator/ParallelFor.h"
#include "tarch/tests/TestMacros.h"

namespace {
  constexpr std::size_t NumberOfEntries = 37;

  std::vector<double> getRampOf(const std::size_t size, const double offset) {
    std::vector<double> result(size);
    for (std::size_t entry = 0; entry < size; entry++) {
      result[entry] = offset + static_cast<double>(entry);
    }
    return result;
  }

  TARGET_DECLARE
  struct DoubleEntry final {
    double* values{nullptr};

    INLINE_METHOD GPU_CALLABLE_METHOD void operator()(const int entry) const { values[entry] = 2.0 * values[entry]; }
  };
  TARGET_DECLARE_END

  void doubleEveryEntry(tarch::accelerator::Buffer<double>& buffer) {
    tarch::accelerator::parallelFor(
      tarch::accelerator::Range(static_cast<int>(buffer.getSize())),
      DoubleEntry{buffer.getData()}
    );
  }
} // namespace

tarch::accelerator::tests::BufferTest::BufferTest() :
  TestCase("tarch::accelerator::tests::BufferTest") {}

void
tarch::accelerator::tests::BufferTest::run() {
  testMethod(testEmptyBuffer);
  testMethod(testHeapBufferIsAddressable);
  testMethod(testDeviceRoundTrip);
  testMethod(testAllocateReplacesTheArray);
  testMethod(testMoveTransfersTheArray);
  testMethod(testPinnedBuffer);
  testMethod(testPinnedCopiesEndBeforeTheyReturn);
  testMethod(testAllocatedBytesFollowTheLocation);
}

void
tarch::accelerator::tests::BufferTest::testEmptyBuffer() {
  const tarch::accelerator::Buffer<double> buffer;

  validate(buffer.isEmpty());
  validateEquals(buffer.getSize(), 0);
  validateEquals(buffer.getSizeInBytes(), 0);
  validate(buffer.getData() == nullptr);
  validate(buffer.getLocation() == tarch::MemoryLocation::Heap);
  validate(buffer.getValues().empty());
}

void
tarch::accelerator::tests::BufferTest::testHeapBufferIsAddressable() {
  tarch::accelerator::Buffer<int> buffer(NumberOfEntries, tarch::MemoryLocation::Heap);

  validateEquals(buffer.getSize(), NumberOfEntries);
  validateEquals(buffer.getSizeInBytes(), NumberOfEntries * sizeof(int));
  for (std::size_t entry = 0; entry < NumberOfEntries; entry++) {
    buffer.getData()[entry] = static_cast<int>(3 * entry);
  }
  const auto values = buffer.getValues();
  validateEquals(values.size(), NumberOfEntries);
  for (std::size_t entry = 0; entry < NumberOfEntries; entry++) {
    validateEqualsWithParams1(values[entry], static_cast<int>(3 * entry), entry);
  }
}

/**
 * A device buffer is written by a kernel and read through the host copy.
 */
void
tarch::accelerator::tests::BufferTest::testDeviceRoundTrip() {
  tarch::accelerator::Buffer<double> buffer(tarch::MemoryLocation::Device);
  buffer.setValues(getRampOf(NumberOfEntries, 0.5));
  validate(buffer.getLocation() == tarch::MemoryLocation::Device);
  validateEquals(buffer.getSize(), NumberOfEntries);

  doubleEveryEntry(buffer);

  const auto values = buffer.getValues();
  for (std::size_t entry = 0; entry < NumberOfEntries; entry++) {
    validateEqualsWithParams1(values[entry], 2.0 * (0.5 + static_cast<double>(entry)), entry);
  }

  std::vector<double> partial(4, 0.0);
  buffer.copyToHost(partial.data(), partial.size());
  validateEquals(partial[3], 7.0);
}

void
tarch::accelerator::tests::BufferTest::testAllocateReplacesTheArray() {
  tarch::accelerator::Buffer<double> buffer(tarch::MemoryLocation::Heap);
  buffer.setValues(getRampOf(4, 1.0));
  validateEquals(buffer.getSize(), 4);

  buffer.allocate(9);
  validateEquals(buffer.getSize(), 9);

  buffer.allocate(0);
  validate(buffer.isEmpty());
  validate(buffer.getData() == nullptr);

  buffer.release();
  validate(buffer.isEmpty());
}

void
tarch::accelerator::tests::BufferTest::testMoveTransfersTheArray() {
  tarch::accelerator::Buffer<double> source(tarch::MemoryLocation::Heap);
  source.setValues(getRampOf(5, 10.0));
  const double* const array = source.getData();

  tarch::accelerator::Buffer<double> moved(std::move(source));
  validateEquals(moved.getSize(), 5);
  validate(moved.getData() == array);
  validateEquals(moved.getValues()[4], 14.0);

  tarch::accelerator::Buffer<double> target(2, tarch::MemoryLocation::Heap);
  target = std::move(moved);
  validateEquals(target.getSize(), 5);
  validate(target.getData() == array);
  validateEquals(target.getValues()[0], 10.0);
}

void
tarch::accelerator::tests::BufferTest::testPinnedBuffer() {
  tarch::accelerator::Buffer<float> buffer(tarch::MemoryLocation::Pinned);
  const std::vector<float>          values{1.0F, 2.0F, 3.0F};
  buffer.setValues(values);

  validate(buffer.getLocation() == tarch::MemoryLocation::Pinned);
  validateEquals(buffer.getSize(), 3);
  validateEquals(buffer.getValues()[2], 3.0F);
}

void
tarch::accelerator::tests::BufferTest::testPinnedCopiesEndBeforeTheyReturn() {
  constexpr std::size_t              LargeNumberOfEntries = std::size_t{1} << 22;
  tarch::accelerator::Buffer<double> pinned(LargeNumberOfEntries, tarch::MemoryLocation::Pinned);
  tarch::accelerator::Buffer<double> device(LargeNumberOfEntries, tarch::MemoryLocation::Device);
  double* const                      values = pinned.getData();
  for (std::size_t entry = 0; entry < LargeNumberOfEntries; entry++) {
    values[entry] = static_cast<double>(entry);
  }
  device.copyFromHost(values, LargeNumberOfEntries);
  std::fill_n(values, LargeNumberOfEntries, -1.0);
  doubleEveryEntry(device);
  device.copyToHost(values, LargeNumberOfEntries);
  int mismatches = 0;
  for (std::size_t entry = 0; entry < LargeNumberOfEntries; entry++) {
    if (values[entry] != 2.0 * static_cast<double>(entry)) {
      mismatches++;
    }
  }
  validateEquals(mismatches, 0);
}

/**
 * Heap and pinned buffers count as host bytes, and a device buffer counts as device bytes in a
 * GPU build only.
 */
void
tarch::accelerator::tests::BufferTest::testAllocatedBytesFollowTheLocation() {
  const tarch::accelerator::Buffer<int> heap(NumberOfEntries, tarch::MemoryLocation::Heap);
  const tarch::accelerator::Buffer<int> pinned(NumberOfEntries, tarch::MemoryLocation::Pinned);
  const tarch::accelerator::Buffer<int> device(NumberOfEntries, tarch::MemoryLocation::Device);
  constexpr std::size_t                 Bytes = NumberOfEntries * sizeof(int);

  validateEquals(heap.getAllocatedBytes().host, Bytes);
  validateEquals(heap.getAllocatedBytes().device, 0);
  validateEquals(pinned.getAllocatedBytes().host, Bytes);
  validateEquals(pinned.getAllocatedBytes().device, 0);
#if defined(PEANO_WITH_GPU_BACKEND)
  validateEquals(device.getAllocatedBytes().host, 0);
  validateEquals(device.getAllocatedBytes().device, Bytes);
#else
  validateEquals(device.getAllocatedBytes().host, Bytes);
  validateEquals(device.getAllocatedBytes().device, 0);
#endif

  auto sum = heap.getAllocatedBytes() + device.getAllocatedBytes();
  sum += tarch::AllocatedBytes::onHost(5);
  validateEquals(sum.getTotal(), 2 * Bytes + 5);
  validateEquals(sum.host + sum.device, sum.getTotal());
  validateEquals(tarch::AllocatedBytes::onHost(5).host, 5);
}
