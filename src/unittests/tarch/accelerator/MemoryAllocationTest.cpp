// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "tarch/accelerator/MemoryBlocks.h"
#include "tarch/accelerator/MemoryLocation.h"
#include "tarch/accelerator/accelerator.h"
#include "tarch/testing/UnitTest.h"

namespace {
  constexpr std::size_t                          ElementCount = 4;
  constexpr std::size_t                          ByteCount    = ElementCount * sizeof(int);
  constexpr std::array<tarch::MemoryLocation, 3> MemoryLocations{
    tarch::MemoryLocation::Heap,
    tarch::MemoryLocation::Device,
    tarch::MemoryLocation::Pinned
  };
} // namespace

UNIT_TEST("Raw data of every memory location returns the bytes copied into it") {
  GIVEN("The current device and the integers 2, 4, 6 and 8") {
    const int                           device = tarch::accelerator::getCurrentDevice();
    const std::array<int, ElementCount> source{2, 4, 6, 8};

    WHEN("Every memory location copies the integers into raw data and back and frees it and a null pointer") {
      std::array<bool, MemoryLocations.size()> allocationWorked{};
      std::array<bool, MemoryLocations.size()> copiedCorrectly{};
#if PEANO_DEBUG
      std::array<std::size_t, MemoryLocations.size()> trackedSizes{};
#endif
      for (std::size_t entry = 0; entry < MemoryLocations.size(); entry++) {
        const tarch::MemoryLocation   location = MemoryLocations[entry];
        std::array<int, ElementCount> target{0, 0, 0, 0};

        void* const rawData = tarch::accelerator::allocateRawData(ByteCount, location, device);

        tarch::accelerator::copyTo(rawData, source.data(), ByteCount, location, device);
        tarch::accelerator::copyFrom(rawData, target.data(), ByteCount, location, device);

#if PEANO_DEBUG
        trackedSizes[entry] = tarch::accelerator::MemoryBlocks::getInstance().getSizeOfMemoryBlock(rawData);
#endif
        copiedCorrectly[entry]  = target == source;
        allocationWorked[entry] = rawData != nullptr;

        tarch::accelerator::freeRawData(rawData, location, device);
        tarch::accelerator::freeRawData(nullptr, location, device);
      }

      THEN("Every allocation succeeds and returns the integers and a debug build tracks its size") {
        for (std::size_t entry = 0; entry < MemoryLocations.size(); entry++) {
          VALIDATE(allocationWorked[entry], tarch::toString(MemoryLocations[entry]));
          VALIDATE(copiedCorrectly[entry], tarch::toString(MemoryLocations[entry]));
#if PEANO_DEBUG
          VALIDATE(trackedSizes[entry] == ByteCount, tarch::toString(MemoryLocations[entry]), trackedSizes[entry]);
#endif
        }
      }
    }
  }
}

UNIT_TEST("Typed memory of every memory location returns the elements copied into it") {
  GIVEN("The integers 11, 13, 17 and 19") {
    const std::array<int, ElementCount> source{11, 13, 17, 19};

    WHEN("Every memory location copies the integers into typed memory and back and frees it and a null pointer") {
      std::array<bool, MemoryLocations.size()> allocationWorked{};
      std::array<bool, MemoryLocations.size()> copiedCorrectly{};
#if PEANO_DEBUG
      std::array<std::size_t, MemoryLocations.size()> trackedSizes{};
#endif
      for (std::size_t entry = 0; entry < MemoryLocations.size(); entry++) {
        const tarch::MemoryLocation   location = MemoryLocations[entry];
        std::array<int, ElementCount> target{0, 0, 0, 0};

        int* const data = tarch::allocateMemory<int>(ElementCount, location);

        tarch::copyTo(data, source.data(), ElementCount, location);
        tarch::copyFrom(data, target.data(), ElementCount, location);

#if PEANO_DEBUG
        trackedSizes[entry] = tarch::accelerator::MemoryBlocks::getInstance().getSizeOfMemoryBlock(data);
#endif
        allocationWorked[entry] = data != nullptr;
        copiedCorrectly[entry]  = target == source;

        tarch::freeMemory(data, location);
        tarch::freeMemory(nullptr, location);
      }

      THEN("Every allocation succeeds and returns the integers and a debug build tracks its size") {
        for (std::size_t entry = 0; entry < MemoryLocations.size(); entry++) {
          VALIDATE(allocationWorked[entry], tarch::toString(MemoryLocations[entry]));
          VALIDATE(copiedCorrectly[entry], tarch::toString(MemoryLocations[entry]));
#if PEANO_DEBUG
          VALIDATE(trackedSizes[entry] == ByteCount, tarch::toString(MemoryLocations[entry]), trackedSizes[entry]);
#endif
        }
      }
    }
  }
}

UNIT_TEST("The allocator of every memory location returns memory") {
  GIVEN("The current device") {
    const int device = tarch::accelerator::getCurrentDevice();

    WHEN("Every memory location allocates the bytes of four integers with its allocator and frees them") {
      std::array<bool, MemoryLocations.size()> allocationWorked{};
      for (std::size_t entry = 0; entry < MemoryLocations.size(); entry++) {
        const tarch::MemoryLocation location = MemoryLocations[entry];
        void*                       data     = nullptr;

        switch (location) {
        case tarch::MemoryLocation::Heap:
          data = tarch::accelerator::allocateHeapMemory(ByteCount);
          break;
        case tarch::MemoryLocation::Device:
          data = tarch::accelerator::allocateDeviceMemory(ByteCount, device);
          break;
        case tarch::MemoryLocation::Pinned:
          data = tarch::accelerator::allocatePinnedMemory(ByteCount, device);
          break;
        }

        allocationWorked[entry] = data != nullptr;

        if (data != nullptr) {
          switch (location) {
          case tarch::MemoryLocation::Heap:
            tarch::accelerator::freeHeapMemory(data);
            break;
          case tarch::MemoryLocation::Device:
            tarch::accelerator::freeDeviceMemory(data, device);
            break;
          case tarch::MemoryLocation::Pinned:
            tarch::accelerator::freePinnedMemory(data, device);
            break;
          }
        }
      }

      THEN("Every allocator returns memory") {
        for (std::size_t entry = 0; entry < MemoryLocations.size(); entry++) {
          VALIDATE(allocationWorked[entry], tarch::toString(MemoryLocations[entry]));
        }
      }
    }
  }
}

UNIT_TEST("The copy functions of every memory location copy the bytes into the memory and back") {
  GIVEN("The current device and the integers 5, 10, 15 and 20") {
    const int                           device = tarch::accelerator::getCurrentDevice();
    const std::array<int, ElementCount> source{5, 10, 15, 20};

    WHEN("Memory of every memory location receives the integers and returns them through its own copy functions") {
      std::array<bool, MemoryLocations.size()> allocationWorked{};
      std::array<bool, MemoryLocations.size()> copiedCorrectly{};
      for (std::size_t entry = 0; entry < MemoryLocations.size(); entry++) {
        const tarch::MemoryLocation   location = MemoryLocations[entry];
        std::array<int, ElementCount> target{0, 0, 0, 0};
        void*                         data = nullptr;

        switch (location) {
        case tarch::MemoryLocation::Heap:
          data = tarch::accelerator::allocateHeapMemory(ByteCount);
          tarch::accelerator::copyToHeapMemory(data, source.data(), ByteCount);
          tarch::accelerator::copyFromHeapMemory(data, target.data(), ByteCount);
          break;
        case tarch::MemoryLocation::Device:
          data = tarch::accelerator::allocateDeviceMemory(ByteCount, device);
          tarch::accelerator::copyToDeviceMemory(data, source.data(), ByteCount, device);
          tarch::accelerator::copyFromDeviceMemory(data, target.data(), ByteCount, device);
          break;
        case tarch::MemoryLocation::Pinned:
          data = tarch::accelerator::allocatePinnedMemory(ByteCount, device);
          tarch::accelerator::copyToPinnedMemory(data, source.data(), ByteCount, device);
          tarch::accelerator::copyFromPinnedMemory(data, target.data(), ByteCount, device);
          break;
        }

        allocationWorked[entry] = data != nullptr;
        copiedCorrectly[entry]  = target == source;

        if (data != nullptr) {
          switch (location) {
          case tarch::MemoryLocation::Heap:
            tarch::accelerator::freeHeapMemory(data);
            break;
          case tarch::MemoryLocation::Device:
            tarch::accelerator::freeDeviceMemory(data, device);
            break;
          case tarch::MemoryLocation::Pinned:
            tarch::accelerator::freePinnedMemory(data, device);
            break;
          }
        }
      }

      THEN("Every allocation succeeds and returns the integers") {
        for (std::size_t entry = 0; entry < MemoryLocations.size(); entry++) {
          VALIDATE(allocationWorked[entry], tarch::toString(MemoryLocations[entry]));
          VALIDATE(copiedCorrectly[entry], tarch::toString(MemoryLocations[entry]));
        }
      }
    }
  }
}

UNIT_TEST("The device that one host thread selects is the current device of every other host thread") {
  GIVEN("The current device of the rank and the next device") {
    const int deviceOfTheRank = tarch::accelerator::getCurrentDevice();
    const int selectedDevice  = deviceOfTheRank + 1;

    WHEN("This thread selects the next device, another thread reads the current device and this thread resets it") {
      int deviceOfAnotherThread{-1};
      tarch::accelerator::setCurrentDevice(selectedDevice);
      std::thread([&deviceOfAnotherThread]() {
        deviceOfAnotherThread = tarch::accelerator::getCurrentDevice();
      }).join();
      tarch::accelerator::setCurrentDevice(deviceOfTheRank);
      const int deviceAfterTheReset = tarch::accelerator::getCurrentDevice();

      THEN("The other thread reads the next device and the device of the rank is current again") {
        VALIDATE_EQUALS(deviceOfAnotherThread, selectedDevice);
        VALIDATE_EQUALS(deviceAfterTheReset, deviceOfTheRank);
      }
    }
  }
}

UNIT_TEST("Only the device location can be GPU memory and it is GPU memory if its bytes count as device bytes") {
  GIVEN("The heap, the pinned and the device memory location") {
    WHEN("Every location reports whether it is GPU memory and AllocatedBytes counts one byte of the device location") {
      const bool heapIsGPUMemory     = tarch::isGPUMemory(tarch::MemoryLocation::Heap);
      const bool pinnedIsGPUMemory   = tarch::isGPUMemory(tarch::MemoryLocation::Pinned);
      const bool deviceIsGPUMemory   = tarch::isGPUMemory(tarch::MemoryLocation::Device);
      const bool deviceByteIsCounted = tarch::AllocatedBytes::in(tarch::MemoryLocation::Device, 1).device == 1;

      THEN("The heap and the pinned memory are not GPU memory") {
        VALIDATE(not heapIsGPUMemory);
        VALIDATE(not pinnedIsGPUMemory);
      }

      THEN("The device location is GPU memory if AllocatedBytes counts its bytes as device bytes") {
        VALIDATE_EQUALS(deviceIsGPUMemory, deviceByteIsCounted);
      }
    }
  }
}
