// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "tarch/accelerator/Buffer.h"

#include "tarch/accelerator/ParallelFor.h"
#include "tarch/testing/UnitTest.h"

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

UNIT_TEST("A default buffer is an empty heap buffer") {
  GIVEN("A default buffer") {
    const tarch::accelerator::Buffer<double> buffer;

    THEN("The heap buffer counts zero entries and zero bytes and returns a null pointer and an empty value list") {
      VALIDATE(buffer.isEmpty());
      VALIDATE_EQUALS(buffer.getSize(), 0);
      VALIDATE_EQUALS(buffer.getSizeInBytes(), 0);
      VALIDATE(buffer.getData() == nullptr);
      VALIDATE(buffer.getLocation() == tarch::MemoryLocation::Heap);
      VALIDATE(buffer.getValues().empty());
    }
  }
}

UNIT_TEST("The host writes the entries of a heap buffer through its array") {
  GIVEN("A heap buffer of 37 integers") {
    tarch::accelerator::Buffer<int> buffer(NumberOfEntries, tarch::MemoryLocation::Heap);

    WHEN("The host writes three times the index into every entry of the array") {
      for (std::size_t entry = 0; entry < NumberOfEntries; entry++) {
        buffer.getData()[entry] = static_cast<int>(3 * entry);
      }
      const auto values = buffer.getValues();

      THEN("The buffer allocates the bytes of 37 integers and returns three times the index of every entry") {
        VALIDATE_EQUALS(buffer.getSize(), NumberOfEntries);
        VALIDATE_EQUALS(buffer.getSizeInBytes(), NumberOfEntries * sizeof(int));
        VALIDATE_EQUALS(values.size(), NumberOfEntries);
        for (std::size_t entry = 0; entry < NumberOfEntries; entry++) {
          VALIDATE_EQUALS(values[entry], static_cast<int>(3 * entry), entry);
        }
      }
    }
  }
}

UNIT_TEST("A kernel writes a device buffer and the test reads it through the host copy") {
  GIVEN("A device buffer of 37 values rising by one from one half") {
    tarch::accelerator::Buffer<double> buffer(tarch::MemoryLocation::Device);
    buffer.setValues(getRampOf(NumberOfEntries, 0.5));

    THEN("The buffer is a device buffer of 37 entries") {
      VALIDATE(buffer.getLocation() == tarch::MemoryLocation::Device);
      VALIDATE_EQUALS(buffer.getSize(), NumberOfEntries);
    }

    WHEN("A kernel doubles every entry and the host copies every entry and the first four entries") {
      doubleEveryEntry(buffer);
      const auto          values = buffer.getValues();
      std::vector<double> partial(4, 0.0);
      buffer.copyToHost(partial.data(), partial.size());

      THEN("Both host copies store the doubled values") {
        for (std::size_t entry = 0; entry < NumberOfEntries; entry++) {
          VALIDATE_EQUALS(values[entry], 2.0 * (0.5 + static_cast<double>(entry)), entry);
        }
        VALIDATE_EQUALS(partial[3], 7.0);
      }
    }
  }
}

UNIT_TEST("An allocation replaces the array of a buffer") {
  GIVEN("A heap buffer of four values") {
    tarch::accelerator::Buffer<double> buffer(tarch::MemoryLocation::Heap);
    buffer.setValues(getRampOf(4, 1.0));

    THEN("The buffer stores four entries") { VALIDATE_EQUALS(buffer.getSize(), 4); }

    WHEN("The buffer allocates nine entries") {
      buffer.allocate(9);

      THEN("The buffer stores nine entries") { VALIDATE_EQUALS(buffer.getSize(), 9); }

      AND_WHEN("The buffer allocates zero entries") {
        buffer.allocate(0);

        THEN("The buffer is empty and returns a null data pointer") {
          VALIDATE(buffer.isEmpty());
          VALIDATE(buffer.getData() == nullptr);
        }

        AND_WHEN("The buffer releases its array") {
          buffer.release();

          THEN("The buffer stays empty") { VALIDATE(buffer.isEmpty()); }
        }
      }
    }
  }
}

UNIT_TEST("A move transfers the array of a buffer") {
  GIVEN("A heap buffer of five values rising by one from ten") {
    tarch::accelerator::Buffer<double> source(tarch::MemoryLocation::Heap);
    source.setValues(getRampOf(5, 10.0));
    const double* const array = source.getData();

    WHEN("The test constructs a new buffer from the moved buffer") {
      tarch::accelerator::Buffer<double> moved(std::move(source));

      THEN("The new buffer stores the five values in the array of the moved buffer") {
        VALIDATE_EQUALS(moved.getSize(), 5);
        VALIDATE(moved.getData() == array);
        VALIDATE_EQUALS(moved.getValues()[4], 14.0);
      }

      AND_WHEN("The test moves the new buffer into a heap buffer of two entries") {
        tarch::accelerator::Buffer<double> target(2, tarch::MemoryLocation::Heap);
        target = std::move(moved);

        THEN("The target stores the five values in the array of the first buffer") {
          VALIDATE_EQUALS(target.getSize(), 5);
          VALIDATE(target.getData() == array);
          VALIDATE_EQUALS(target.getValues()[0], 10.0);
        }
      }
    }
  }
}

UNIT_TEST("A pinned buffer stores the values it receives") {
  GIVEN("A pinned buffer and the values one, two and three") {
    tarch::accelerator::Buffer<float> buffer(tarch::MemoryLocation::Pinned);
    const std::vector<float>          values{1.0F, 2.0F, 3.0F};

    WHEN("The buffer receives the values") {
      buffer.setValues(values);

      THEN("The buffer stays pinned and stores the three values") {
        VALIDATE(buffer.getLocation() == tarch::MemoryLocation::Pinned);
        VALIDATE_EQUALS(buffer.getSize(), 3);
        VALIDATE_EQUALS(buffer.getValues()[2], 3.0F);
      }
    }
  }
}

UNIT_TEST("A copy between a device buffer and pinned host memory has ended when it returns") {
  GIVEN("A pinned buffer that stores the index of each of its 2^22 entries and a device buffer of this size") {
    constexpr std::size_t              LargeNumberOfEntries = std::size_t{1} << 22;
    tarch::accelerator::Buffer<double> pinned(LargeNumberOfEntries, tarch::MemoryLocation::Pinned);
    tarch::accelerator::Buffer<double> device(LargeNumberOfEntries, tarch::MemoryLocation::Device);
    double* const                      values = pinned.getData();
    for (std::size_t entry = 0; entry < LargeNumberOfEntries; entry++) {
      values[entry] = static_cast<double>(entry);
    }

    WHEN("The device buffer copies the pinned entries and the host overwrites them with -1") {
      device.copyFromHost(values, LargeNumberOfEntries);
      std::fill_n(values, LargeNumberOfEntries, -1.0);

      AND_WHEN("A kernel doubles every device entry and the device buffer copies its entries into the pinned buffer") {
        doubleEveryEntry(device);
        device.copyToHost(values, LargeNumberOfEntries);
        int mismatches = 0;
        for (std::size_t entry = 0; entry < LargeNumberOfEntries; entry++) {
          if (values[entry] != 2.0 * static_cast<double>(entry)) {
            mismatches++;
          }
        }

        THEN("Every pinned entry equals twice its index") { VALIDATE_EQUALS(mismatches, 0); }
      }
    }
  }
}

UNIT_TEST("Heap and pinned buffers count host bytes and a device buffer counts device bytes in a GPU build only") {
  GIVEN("A heap, a pinned and a device buffer of 37 integers each") {
    const tarch::accelerator::Buffer<int> heap(NumberOfEntries, tarch::MemoryLocation::Heap);
    const tarch::accelerator::Buffer<int> pinned(NumberOfEntries, tarch::MemoryLocation::Pinned);
    const tarch::accelerator::Buffer<int> device(NumberOfEntries, tarch::MemoryLocation::Device);
    constexpr std::size_t                 Bytes = NumberOfEntries * sizeof(int);

    THEN("The heap and the pinned buffer count host bytes and the device buffer counts device bytes in a GPU build") {
      VALIDATE_EQUALS(heap.getAllocatedBytes().host, Bytes);
      VALIDATE_EQUALS(heap.getAllocatedBytes().device, 0);
      VALIDATE_EQUALS(pinned.getAllocatedBytes().host, Bytes);
      VALIDATE_EQUALS(pinned.getAllocatedBytes().device, 0);
#if defined(PEANO_WITH_GPU_BACKEND)
      VALIDATE_EQUALS(device.getAllocatedBytes().host, 0);
      VALIDATE_EQUALS(device.getAllocatedBytes().device, Bytes);
#else
      VALIDATE_EQUALS(device.getAllocatedBytes().host, Bytes);
      VALIDATE_EQUALS(device.getAllocatedBytes().device, 0);
#endif
    }

    WHEN("The test sums the bytes of the heap and the device buffer and five host bytes") {
      const tarch::AllocatedBytes fiveHostBytes = tarch::AllocatedBytes::onHost(5);
      auto                        sum           = heap.getAllocatedBytes() + device.getAllocatedBytes();
      sum += fiveHostBytes;

      THEN("The five host bytes count as host bytes") { VALIDATE_EQUALS(fiveHostBytes.host, 5); }

      THEN("The total is the bytes of both buffers plus five and the host and the device bytes add to the total") {
        VALIDATE_EQUALS(sum.getTotal(), 2 * Bytes + 5);
        VALIDATE_EQUALS(sum.host + sum.device, sum.getTotal());
      }
    }
  }
}
