// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "TaskGraphTest.h"

#include "tarch/tasking/FunctorTask.h"
#include "tarch/tasking/Task.h"
#include "tarch/tasking/TaskGraph.h"

namespace {
  constexpr int NumberOfTasksToExecute = 64;
  constexpr int ManyDependencies       = 32;
  constexpr int ShortDelayMilliseconds = 1;
  constexpr int LongDelayMilliseconds  = 5;

  class CountingTask final : public tarch::tasking::Task {
  public:
    explicit CountingTask(std::atomic<int>& executionCount) :
      _executionCount(executionCount) {}
    CountingTask(const CountingTask&)            = delete;
    CountingTask(CountingTask&&)                 = delete;
    CountingTask& operator=(const CountingTask&) = delete;
    CountingTask& operator=(CountingTask&&)      = delete;
    ~CountingTask() override                     = default;

    void run() override { _executionCount.fetch_add(1, std::memory_order_relaxed); }

  private:
    std::atomic<int>& _executionCount;
  };

  class FlagSettingTask final : public tarch::tasking::Task {
  public:
    FlagSettingTask(
      std::atomic<int>&        executionCount,
      std::atomic<bool>* const flagToSet,
      const int                sleepMilliseconds = 0
    ) :
      _executionCount(executionCount),
      _flagToSet(flagToSet),
      _sleepMilliseconds(sleepMilliseconds) {}
    FlagSettingTask(const FlagSettingTask&)            = delete;
    FlagSettingTask(FlagSettingTask&&)                 = delete;
    FlagSettingTask& operator=(const FlagSettingTask&) = delete;
    FlagSettingTask& operator=(FlagSettingTask&&)      = delete;
    ~FlagSettingTask() override                        = default;

    void run() override {
      _executionCount.fetch_add(1, std::memory_order_relaxed);
      if (_sleepMilliseconds > 0) {
        std::this_thread::sleep_for(std::chrono::milliseconds(_sleepMilliseconds));
      }
      if (_flagToSet != nullptr) {
        _flagToSet->store(true, std::memory_order_release);
      }
    }

  private:
    std::atomic<int>&        _executionCount;
    std::atomic<bool>* const _flagToSet;
    const int                _sleepMilliseconds;
  };

  class FlagCheckingTask final : public tarch::tasking::Task {
  public:
    FlagCheckingTask(
      std::atomic<int>&              executionCount,
      std::atomic<int>&              errorCount,
      const std::atomic<bool>* const flags,
      const int                      numberOfFlags,
      std::atomic<bool>* const       flagToSet         = nullptr,
      const int                      sleepMilliseconds = 0
    ) :
      _executionCount(executionCount),
      _errorCount(errorCount),
      _flags(flags),
      _numberOfFlags(numberOfFlags),
      _flagToSet(flagToSet),
      _sleepMilliseconds(sleepMilliseconds) {}
    FlagCheckingTask(const FlagCheckingTask&)            = delete;
    FlagCheckingTask(FlagCheckingTask&&)                 = delete;
    FlagCheckingTask& operator=(const FlagCheckingTask&) = delete;
    FlagCheckingTask& operator=(FlagCheckingTask&&)      = delete;
    ~FlagCheckingTask() override                         = default;

    void run() override {
      for (int flagIndex = 0; flagIndex < _numberOfFlags; ++flagIndex) {
        if (not _flags[flagIndex].load(std::memory_order_acquire)) {
          _errorCount.fetch_add(1, std::memory_order_relaxed);
        }
      }
      if (_sleepMilliseconds > 0) {
        std::this_thread::sleep_for(std::chrono::milliseconds(_sleepMilliseconds));
      }
      _executionCount.fetch_add(1, std::memory_order_relaxed);
      if (_flagToSet != nullptr) {
        _flagToSet->store(true, std::memory_order_release);
      }
    }

  private:
    std::atomic<int>&              _executionCount;
    std::atomic<int>&              _errorCount;
    const std::atomic<bool>* const _flags;
    const int                      _numberOfFlags;
    std::atomic<bool>* const       _flagToSet;
    const int                      _sleepMilliseconds;
  };
} // namespace

tarch::tasking::tests::TaskGraphTest::TaskGraphTest() :
  tarch::tests::TestCase("tarch::tasking::tests::TaskGraphTest") {}

void
tarch::tasking::tests::TaskGraphTest::run() {
  testMethod([this]() { testSpawnIndependentTask(); });
  testMethod([this]() { testSpawnFunctorTask(); });
  testMethod([this]() { testSpawnTaskWithOutDependency(); });
  testMethod([this]() { testSpawnTaskWithInAndOutDependency(); });
  testMethod([this]() { testSpawnTaskWithMultipleInAndOutDependencies(); });
  testMethod([this]() { testSpawnTaskIgnoresNullAndDuplicateDependencies(); });
  testMethod([this]() { testSpawnTaskWithManyOutDependencies(); });
  testMethod([this]() { testSpawnTaskWithManyInDependencies(); });
  testMethod([this]() { testSpawnTaskWithManyInAndOutDependencies(); });
  testMethod([this]() { testWaitForDependencies(); });
  testMethod([this]() { testRepeatedSweepsReuseTheSameTokens(); });
  testMethod([this]() { testTasksMaySpawnDependentTasks(); });
}

/**
 * Producer and consumer keep their order in every sweep that reuses the same dependency addresses.
 */
void
tarch::tasking::tests::TaskGraphTest::testRepeatedSweepsReuseTheSameTokens() {
  constexpr int NumberOfSweeps        = 400;
  constexpr int NumberOfTasksPerSweep = 8;

  std::atomic<int> producerRunCount{0};
  std::atomic<int> consumerRunCount{0};
  std::atomic<int> dependencyViolationCount{0};

  std::vector<char>              producerTokens(NumberOfTasksPerSweep);
  char                           consumerToken{0};
  std::vector<std::atomic<bool>> produced(NumberOfTasksPerSweep);
  std::vector<void*>             producerTokenAddresses(NumberOfTasksPerSweep);
  for (int taskIndex = 0; taskIndex < NumberOfTasksPerSweep; taskIndex++) {
    producerTokenAddresses[taskIndex] = &producerTokens[taskIndex];
  }

  for (int sweep = 0; sweep < NumberOfSweeps; sweep++) {
    for (int taskIndex = 0; taskIndex < NumberOfTasksPerSweep; taskIndex++) {
      produced[taskIndex].store(false, std::memory_order_relaxed);
    }

    for (int taskIndex = 0; taskIndex < NumberOfTasksPerSweep; taskIndex++) {
      tarch::tasking::spawnTask(
        new FlagSettingTask(producerRunCount, &produced[taskIndex]),
        {},
        {&producerTokens[taskIndex]}
      );
    }
    const std::array<void*, 1> consumerTokenAddress{&consumerToken};
    tarch::tasking::spawnTask(
      new FlagCheckingTask(consumerRunCount, dependencyViolationCount, produced.data(), NumberOfTasksPerSweep, nullptr),
      producerTokenAddresses,
      consumerTokenAddress
    );

    tarch::tasking::waitForDependencies(producerTokenAddresses);
    tarch::tasking::waitForDependency(&consumerToken);

    validateEquals(producerRunCount.load(), (sweep + 1) * NumberOfTasksPerSweep);
    validateEquals(consumerRunCount.load(), sweep + 1);
    validateEquals(dependencyViolationCount.load(), 0);
  }
}

/**
 * A task waits for the tasks it spawns before it returns.
 */
void
tarch::tasking::tests::TaskGraphTest::testTasksMaySpawnDependentTasks() {
  constexpr int NumberOfOuterTasks = 8;
  constexpr int NumberOfInnerTasks = 4;

  std::atomic<int> innerRunCount{0};
  std::atomic<int> outerRunCount{0};

  std::vector<char>  outerTokens(NumberOfOuterTasks);
  std::vector<void*> outerTokenAddresses(NumberOfOuterTasks);
  for (int outerIndex = 0; outerIndex < NumberOfOuterTasks; outerIndex++) {
    outerTokenAddresses[outerIndex] = &outerTokens[outerIndex];
  }

  for (int outerIndex = 0; outerIndex < NumberOfOuterTasks; outerIndex++) {
    tarch::tasking::spawnTask(
      new tarch::tasking::FunctorTask([&innerRunCount, &outerRunCount]() {
        std::vector<char>  innerTokens(NumberOfInnerTasks);
        std::vector<void*> innerTokenAddresses(NumberOfInnerTasks);
        for (int innerIndex = 0; innerIndex < NumberOfInnerTasks; innerIndex++) {
          innerTokenAddresses[innerIndex] = &innerTokens[innerIndex];
        }
        for (int innerIndex = 0; innerIndex < NumberOfInnerTasks; innerIndex++) {
          tarch::tasking::spawnTask(
            new tarch::tasking::FunctorTask([&innerRunCount]() {
              innerRunCount.fetch_add(1, std::memory_order_relaxed);
            }),
            {},
            {&innerTokens[innerIndex]}
          );
        }
        tarch::tasking::waitForDependencies(innerTokenAddresses);
        outerRunCount.fetch_add(1, std::memory_order_relaxed);
      }),
      {},
      {&outerTokens[outerIndex]}
    );
  }
  tarch::tasking::waitForDependencies(outerTokenAddresses);

  validateEquals(outerRunCount.load(), NumberOfOuterTasks);
  validateEquals(innerRunCount.load(), NumberOfOuterTasks * NumberOfInnerTasks);
}

void
tarch::tasking::tests::TaskGraphTest::testSpawnIndependentTask() {
  std::atomic<int>   executionCount{0};
  std::vector<char>  completionTokens(NumberOfTasksToExecute);
  std::vector<void*> completionDependencies(NumberOfTasksToExecute);
  for (int taskIndex = 0; taskIndex < NumberOfTasksToExecute; taskIndex++) {
    completionDependencies[taskIndex] = &completionTokens[taskIndex];
    tarch::tasking::spawnTask(new CountingTask(executionCount), {}, {completionDependencies[taskIndex]});
  }
  tarch::tasking::waitForDependencies(completionDependencies);
  validateEquals(executionCount.load(), NumberOfTasksToExecute);
}

void
tarch::tasking::tests::TaskGraphTest::testSpawnFunctorTask() {
  std::atomic<int>   executionCount{0};
  std::vector<char>  completionTokens(NumberOfTasksToExecute);
  std::vector<void*> completionDependencies(NumberOfTasksToExecute);

  for (int taskIndex = 0; taskIndex < NumberOfTasksToExecute; taskIndex++) {
    completionDependencies[taskIndex] = &completionTokens[taskIndex];
    tarch::tasking::spawnTask(
      new tarch::tasking::FunctorTask([&executionCount]() { executionCount.fetch_add(1, std::memory_order_relaxed); }),
      {},
      {completionDependencies[taskIndex]}
    );
  }

  tarch::tasking::waitForDependencies(completionDependencies);
  validateEquals(executionCount.load(), NumberOfTasksToExecute);
}

void
tarch::tasking::tests::TaskGraphTest::testSpawnTaskWithOutDependency() {
  std::atomic<int>  executionCount{0};
  std::vector<char> completionTokens(NumberOfTasksToExecute);

  for (int taskIndex = 0; taskIndex < NumberOfTasksToExecute; taskIndex++) {
    tarch::tasking::spawnTask(new CountingTask(executionCount), {}, {&completionTokens[taskIndex]});
  }

  for (int taskIndex = 0; taskIndex < NumberOfTasksToExecute; taskIndex++) {
    tarch::tasking::waitForDependency(&completionTokens[taskIndex]);
  }

  validateEquals(executionCount.load(), NumberOfTasksToExecute);
}

void
tarch::tasking::tests::TaskGraphTest::testSpawnTaskWithInAndOutDependency() {
  std::atomic<int> producerRunCount{0};
  std::atomic<int> consumerRunCount{0};
  std::atomic<int> dependencyViolationCount{0};

  std::vector<char> producerDependencies(NumberOfTasksToExecute);
  std::vector<char> consumerDependencies(NumberOfTasksToExecute);

  std::vector<std::atomic<bool>> produced(NumberOfTasksToExecute);

  for (int taskIndex = 0; taskIndex < NumberOfTasksToExecute; ++taskIndex) {
    produced[taskIndex].store(false, std::memory_order_relaxed);

    tarch::tasking::spawnTask(
      new FlagSettingTask(producerRunCount, &produced[taskIndex], ShortDelayMilliseconds),
      {},
      {&producerDependencies[taskIndex]}
    );
    tarch::tasking::spawnTask(
      new FlagCheckingTask(consumerRunCount, dependencyViolationCount, &produced[taskIndex], 1),
      {&producerDependencies[taskIndex]},
      {&consumerDependencies[taskIndex]}
    );
  }

  for (int taskIndex = 0; taskIndex < NumberOfTasksToExecute; taskIndex++) {
    tarch::tasking::waitForDependency(&consumerDependencies[taskIndex]);
  }

  validateEquals(producerRunCount.load(), NumberOfTasksToExecute);
  validateEquals(consumerRunCount.load(), NumberOfTasksToExecute);
  validateEquals(dependencyViolationCount.load(), 0);
}

void
tarch::tasking::tests::TaskGraphTest::testSpawnTaskWithMultipleInAndOutDependencies() {
  std::atomic<int> producerRunCount{0};
  std::atomic<int> joinRunCount{0};
  std::atomic<int> sinkRunCount{0};
  std::atomic<int> dependencyViolationCount{0};

  std::vector<char> producerTokens(3);
  char              joinToken{0};
  std::vector<char> sinkTokens(2);

  std::array<std::atomic<bool>, 3> produced{};
  std::atomic<bool>                joined{false};

  for (auto& flag : produced) {
    flag.store(false, std::memory_order_relaxed);
  }

  for (std::size_t producerIndex = 0; producerIndex < produced.size(); ++producerIndex) {
    tarch::tasking::spawnTask(
      new FlagSettingTask(producerRunCount, &produced[producerIndex], ShortDelayMilliseconds),
      {},
      {&producerTokens[producerIndex]}
    );
  }

  tarch::tasking::spawnTask(
    new FlagCheckingTask(
      joinRunCount,
      dependencyViolationCount,
      produced.data(),
      static_cast<int>(produced.size()),
      &joined,
      LongDelayMilliseconds
    ),
    {&producerTokens[0], &producerTokens[1], &producerTokens[2]},
    {&joinToken}
  );

  tarch::tasking::spawnTask(
    new FlagCheckingTask(sinkRunCount, dependencyViolationCount, &joined, 1),
    {&joinToken},
    {&sinkTokens[0]}
  );
  tarch::tasking::spawnTask(
    new FlagCheckingTask(sinkRunCount, dependencyViolationCount, &joined, 1),
    {&joinToken},
    {&sinkTokens[1]}
  );

  tarch::tasking::waitForDependency(&sinkTokens[0]);
  tarch::tasking::waitForDependency(&sinkTokens[1]);

  validateEquals(producerRunCount.load(), 3);
  validateEquals(joinRunCount.load(), 1);
  validateEquals(sinkRunCount.load(), 2);
  validateEquals(dependencyViolationCount.load(), 0);
}

void
tarch::tasking::tests::TaskGraphTest::testSpawnTaskIgnoresNullAndDuplicateDependencies() {
  std::atomic<int> producerRunCount{0};
  std::atomic<int> consumerRunCount{0};
  std::atomic<int> dependencyViolationCount{0};

  char producerToken{0};
  char consumerToken{0};

  std::atomic<bool> produced{false};

  tarch::tasking::spawnTask(
    new FlagSettingTask(producerRunCount, &produced, ShortDelayMilliseconds),
    {},
    {&producerToken}
  );

  tarch::tasking::spawnTask(
    new FlagCheckingTask(consumerRunCount, dependencyViolationCount, &produced, 1),
    {&producerToken, nullptr, &producerToken},
    {&consumerToken, &consumerToken, nullptr}
  );

  tarch::tasking::waitForDependency(&consumerToken);

  validateEquals(producerRunCount.load(), 1);
  validateEquals(consumerRunCount.load(), 1);
  validateEquals(dependencyViolationCount.load(), 0);
}

void
tarch::tasking::tests::TaskGraphTest::testSpawnTaskWithManyOutDependencies() {
  std::atomic<int>  producerRunCount{0};
  std::atomic<int>  consumerRunCount{0};
  std::atomic<int>  dependencyViolationCount{0};
  std::atomic<bool> produced{false};

  std::vector<char>              outTokens(ManyDependencies);
  std::vector<char>              consumerTokens(ManyDependencies);
  std::vector<std::atomic<bool>> consumed(ManyDependencies);

  std::vector<void*> outDependencies;
  std::vector<void*> consumerDependencies;
  outDependencies.reserve(outTokens.size());
  consumerDependencies.reserve(consumerTokens.size());

  for (int dependencyIndex = 0; dependencyIndex < ManyDependencies; ++dependencyIndex) {
    consumed[dependencyIndex].store(false, std::memory_order_relaxed);
    outDependencies.push_back(&outTokens[dependencyIndex]);
  }

  tarch::tasking::spawnTask(
    new FlagSettingTask(producerRunCount, &produced, LongDelayMilliseconds),
    std::span<void* const>(),
    std::span<void* const>(outDependencies.data(), outDependencies.size())
  );

  for (int dependencyIndex = 0; dependencyIndex < ManyDependencies; ++dependencyIndex) {
    tarch::tasking::spawnTask(
      new FlagCheckingTask(consumerRunCount, dependencyViolationCount, &produced, 1, &consumed[dependencyIndex]),
      {&outTokens[dependencyIndex]},
      {&consumerTokens[dependencyIndex]}
    );
    consumerDependencies.push_back(&consumerTokens[dependencyIndex]);
  }

  tarch::tasking::waitForDependencies(consumerDependencies);

  for (int dependencyIndex = 0; dependencyIndex < ManyDependencies; ++dependencyIndex) {
    validate(consumed[dependencyIndex].load(std::memory_order_acquire));
  }

  validate(produced.load(std::memory_order_acquire));
  validateEquals(producerRunCount.load(), 1);
  validateEquals(consumerRunCount.load(), ManyDependencies);
  validateEquals(dependencyViolationCount.load(), 0);
}

void
tarch::tasking::tests::TaskGraphTest::testSpawnTaskWithManyInDependencies() {
  for (int delayedProducerIndex = 0; delayedProducerIndex < ManyDependencies; ++delayedProducerIndex) {
    std::atomic<int> producerRunCount{0};
    std::atomic<int> consumerRunCount{0};
    std::atomic<int> dependencyViolationCount{0};

    std::vector<char>              producerTokens(ManyDependencies);
    std::vector<std::atomic<bool>> produced(ManyDependencies);
    char                           consumerToken{0};

    std::vector<void*> inDependencies;
    void* const        consumerDependency = &consumerToken;
    inDependencies.reserve(producerTokens.size());

    for (int dependencyIndex = 0; dependencyIndex < ManyDependencies; ++dependencyIndex) {
      produced[dependencyIndex].store(false, std::memory_order_relaxed);
      tarch::tasking::spawnTask(
        new FlagSettingTask(
          producerRunCount,
          &produced[dependencyIndex],
          dependencyIndex == delayedProducerIndex ? LongDelayMilliseconds : 0
        ),
        {},
        {&producerTokens[dependencyIndex]}
      );
      inDependencies.push_back(&producerTokens[dependencyIndex]);
    }

    tarch::tasking::spawnTask(
      new FlagCheckingTask(consumerRunCount, dependencyViolationCount, produced.data(), ManyDependencies),
      std::span<void* const>(inDependencies.data(), inDependencies.size()),
      std::span<void* const>(&consumerDependency, 1)
    );

    tarch::tasking::waitForDependency(&consumerToken);

    validateEquals(producerRunCount.load(), ManyDependencies);
    validateEquals(consumerRunCount.load(), 1);
    validateEquals(dependencyViolationCount.load(), 0);
  }
}

void
tarch::tasking::tests::TaskGraphTest::testSpawnTaskWithManyInAndOutDependencies() {
  std::atomic<int> producerRunCount{0};
  std::atomic<int> joinRunCount{0};
  std::atomic<int> sinkRunCount{0};
  std::atomic<int> dependencyViolationCount{0};

  std::vector<char>              producerTokens(ManyDependencies);
  std::vector<std::atomic<bool>> produced(ManyDependencies);
  std::vector<char>              sinkTokens(ManyDependencies);
  std::vector<std::atomic<bool>> sinkFlags(ManyDependencies);
  std::vector<char>              completionTokens(ManyDependencies);
  std::atomic<bool>              joined{false};

  std::vector<void*> inDependencies;
  std::vector<void*> outDependencies;
  std::vector<void*> completionDependencies;
  inDependencies.reserve(producerTokens.size());
  outDependencies.reserve(sinkTokens.size());
  completionDependencies.reserve(completionTokens.size());

  for (int dependencyIndex = 0; dependencyIndex < ManyDependencies; ++dependencyIndex) {
    produced[dependencyIndex].store(false, std::memory_order_relaxed);
    sinkFlags[dependencyIndex].store(false, std::memory_order_relaxed);
    tarch::tasking::spawnTask(
      new FlagSettingTask(producerRunCount, &produced[dependencyIndex], ShortDelayMilliseconds),
      {},
      {&producerTokens[dependencyIndex]}
    );
    inDependencies.push_back(&producerTokens[dependencyIndex]);
    outDependencies.push_back(&sinkTokens[dependencyIndex]);
  }

  tarch::tasking::spawnTask(
    new FlagCheckingTask(
      joinRunCount,
      dependencyViolationCount,
      produced.data(),
      ManyDependencies,
      &joined,
      LongDelayMilliseconds
    ),
    std::span<void* const>(inDependencies.data(), inDependencies.size()),
    std::span<void* const>(outDependencies.data(), outDependencies.size())
  );

  for (int dependencyIndex = 0; dependencyIndex < ManyDependencies; ++dependencyIndex) {
    tarch::tasking::spawnTask(
      new FlagCheckingTask(sinkRunCount, dependencyViolationCount, &joined, 1, &sinkFlags[dependencyIndex]),
      {&sinkTokens[dependencyIndex]},
      {&completionTokens[dependencyIndex]}
    );
    completionDependencies.push_back(&completionTokens[dependencyIndex]);
  }

  tarch::tasking::waitForDependencies(completionDependencies);

  for (int dependencyIndex = 0; dependencyIndex < ManyDependencies; ++dependencyIndex) {
    validate(sinkFlags[dependencyIndex].load(std::memory_order_acquire));
  }

  validate(joined.load(std::memory_order_acquire));
  validateEquals(producerRunCount.load(), ManyDependencies);
  validateEquals(joinRunCount.load(), 1);
  validateEquals(sinkRunCount.load(), ManyDependencies);
  validateEquals(dependencyViolationCount.load(), 0);
}

void
tarch::tasking::tests::TaskGraphTest::testWaitForDependencies() {
  constexpr int NumberOfProducers = 8;

  std::array<std::atomic<bool>, NumberOfProducers> produced{};
  std::vector<char>                                tokens(NumberOfProducers);

  for (int producerIndex = 0; producerIndex < NumberOfProducers; producerIndex++) {
    tarch::tasking::spawnTask(
      new tarch::tasking::FunctorTask([&produced, producerIndex]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(ShortDelayMilliseconds));
        produced[producerIndex].store(true, std::memory_order_release);
      }),
      {},
      {&tokens[producerIndex]}
    );
  }

  std::vector<void*> dependencies;
  for (auto& token : tokens) {
    dependencies.push_back(&token);
  }
  tarch::tasking::waitForDependencies(std::span<void* const>(dependencies.data(), dependencies.size()));
  for (int producerIndex = 0; producerIndex < NumberOfProducers; producerIndex++) {
    validate(produced[producerIndex].load(std::memory_order_acquire));
  }
}
