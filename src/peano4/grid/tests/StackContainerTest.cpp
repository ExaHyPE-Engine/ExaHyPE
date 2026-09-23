// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "StackContainerTest.h"

#include "peano4/grid/Stack.h"
#include "peano4/grid/StackContainer.h"

namespace {
  /** Counts the bind calls the container issues for its slots. */
  struct PooledProbe {
    inline static int         bindCalls{0};
    inline static std::size_t lastBoundSlotCount{0};

    static void bindPoolStorage(std::vector<PooledProbe>& slots) {
      bindCalls++;
      lastBoundSlotCount = slots.size();
    }

    int value{0};
  };
} // namespace

peano4::grid::tests::StackContainerTest::StackContainerTest() :
  TestCase("peano4::grid::tests::StackContainerTest") {}

void peano4::grid::tests::StackContainerTest::run() { testMethod(testPooledStorageBindsOnlyOnRequest); }

/**
 * A resize or migration binds the pooled storage only when the caller asks;
 * bindPool() binds the current slots.
 */
void peano4::grid::tests::StackContainerTest::testPooledStorageBindsOnlyOnRequest() {
  peano4::grid::StackContainer<peano4::grid::Stack<PooledProbe>> container(0);
  PooledProbe::bindCalls          = 0;
  PooledProbe::lastBoundSlotCount = 0;

  container.resizePool(5, false);
  validateEquals(PooledProbe::bindCalls, 0);
  validateEquals(container.getPoolSize(), std::size_t{6});

  container.bindPool();
  validateEquals(PooledProbe::bindCalls, 1);
  validateEquals(PooledProbe::lastBoundSlotCount, std::size_t{6});

  container.resizePool(7, true);
  validateEquals(PooledProbe::bindCalls, 2);
  validateEquals(PooledProbe::lastBoundSlotCount, std::size_t{8});

  container.migratePoolSlots({}, 9, false);
  validateEquals(PooledProbe::bindCalls, 2);

  container.migratePoolSlots({}, 11, true);
  validateEquals(PooledProbe::bindCalls, 3);
  validateEquals(PooledProbe::lastBoundSlotCount, std::size_t{12});
}
