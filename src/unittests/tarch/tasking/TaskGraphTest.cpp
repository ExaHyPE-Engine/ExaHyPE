// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "tarch/tasking/TaskGraph.h"

#include "tarch/tasking/FunctorTask.h"
#include "tarch/tasking/Task.h"
#include "tarch/testing/UnitTest.h"

namespace {
  constexpr int NumberOfTasksToExecute = 64;
  constexpr int ManyDependencies       = 32;
  constexpr int ShortDelayMilliseconds = 1;
  constexpr int LongDelayMilliseconds  = 5;

  class CountingTask final : public tarch::tasking::Task {
  public:
    explicit CountingTask(std::atomic<int>& executionCount) :
      _executionCount(executionCount) {}
    CountingTask(const CountingTask& other)            = delete;
    CountingTask(CountingTask&& other)                 = delete;
    CountingTask& operator=(const CountingTask& other) = delete;
    CountingTask& operator=(CountingTask&& other)      = delete;
    ~CountingTask() override                           = default;

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
    FlagSettingTask(const FlagSettingTask& other)            = delete;
    FlagSettingTask(FlagSettingTask&& other)                 = delete;
    FlagSettingTask& operator=(const FlagSettingTask& other) = delete;
    FlagSettingTask& operator=(FlagSettingTask&& other)      = delete;
    ~FlagSettingTask() override                              = default;

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
    FlagCheckingTask(const FlagCheckingTask& other)            = delete;
    FlagCheckingTask(FlagCheckingTask&& other)                 = delete;
    FlagCheckingTask& operator=(const FlagCheckingTask& other) = delete;
    FlagCheckingTask& operator=(FlagCheckingTask&& other)      = delete;
    ~FlagCheckingTask() override                               = default;

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

UNIT_TEST("Every spawned task without in dependencies runs once") {
  GIVEN("A counter and one completion token for each of 64 counting tasks") {
    std::atomic<int>   executionCount{0};
    std::vector<char>  completionTokens(NumberOfTasksToExecute);
    std::vector<void*> completionDependencies(NumberOfTasksToExecute);
    for (int taskIndex = 0; taskIndex < NumberOfTasksToExecute; taskIndex++) {
      completionDependencies[taskIndex] = &completionTokens[taskIndex];
    }

    WHEN("The test spawns every task with its token as out dependency and waits for all tokens") {
      for (int taskIndex = 0; taskIndex < NumberOfTasksToExecute; taskIndex++) {
        tarch::tasking::spawnTask(new CountingTask(executionCount), {}, {completionDependencies[taskIndex]});
      }
      tarch::tasking::waitForDependencies(completionDependencies);

      THEN("The counter counts one run per task") { VALIDATE_EQUALS(executionCount.load(), NumberOfTasksToExecute); }
    }
  }
}

UNIT_TEST("Every spawned functor task runs its functor once") {
  GIVEN("A counter and one completion token for each of 64 functor tasks") {
    std::atomic<int>   executionCount{0};
    std::vector<char>  completionTokens(NumberOfTasksToExecute);
    std::vector<void*> completionDependencies(NumberOfTasksToExecute);
    for (int taskIndex = 0; taskIndex < NumberOfTasksToExecute; taskIndex++) {
      completionDependencies[taskIndex] = &completionTokens[taskIndex];
    }

    WHEN("The test spawns every task with a functor that raises the counter and waits for all tokens") {
      for (int taskIndex = 0; taskIndex < NumberOfTasksToExecute; taskIndex++) {
        tarch::tasking::spawnTask(
          new tarch::tasking::FunctorTask([&executionCount]() {
            executionCount.fetch_add(1, std::memory_order_relaxed);
          }),
          {},
          {completionDependencies[taskIndex]}
        );
      }
      tarch::tasking::waitForDependencies(completionDependencies);

      THEN("The counter counts one run per task") { VALIDATE_EQUALS(executionCount.load(), NumberOfTasksToExecute); }
    }
  }
}

UNIT_TEST("A wait for the out dependency of a task returns after the task ran") {
  GIVEN("A counter and one completion token for each of 64 counting tasks") {
    std::atomic<int>  executionCount{0};
    std::vector<char> completionTokens(NumberOfTasksToExecute);

    WHEN("The test spawns every task with its token as out dependency and waits for each token on its own") {
      for (int taskIndex = 0; taskIndex < NumberOfTasksToExecute; taskIndex++) {
        tarch::tasking::spawnTask(new CountingTask(executionCount), {}, {&completionTokens[taskIndex]});
      }
      for (int taskIndex = 0; taskIndex < NumberOfTasksToExecute; taskIndex++) {
        tarch::tasking::waitForDependency(&completionTokens[taskIndex]);
      }

      THEN("The counter counts one run per task") { VALIDATE_EQUALS(executionCount.load(), NumberOfTasksToExecute); }
    }
  }
}

UNIT_TEST("A consumer that depends on the out dependency of its producer runs after the producer") {
  GIVEN("Counters and the tokens and the flag of each of 64 producer and consumer pairs") {
    std::atomic<int>               producerRunCount{0};
    std::atomic<int>               consumerRunCount{0};
    std::atomic<int>               dependencyViolationCount{0};
    std::vector<char>              producerDependencies(NumberOfTasksToExecute);
    std::vector<char>              consumerDependencies(NumberOfTasksToExecute);
    std::vector<std::atomic<bool>> produced(NumberOfTasksToExecute);

    WHEN("The test spawns and awaits per pair a delayed producer that sets the flag and a consumer of that flag") {
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

      THEN("Every producer and every consumer ran once and every consumer ran after its producer") {
        VALIDATE_EQUALS(producerRunCount.load(), NumberOfTasksToExecute);
        VALIDATE_EQUALS(consumerRunCount.load(), NumberOfTasksToExecute);
        VALIDATE_EQUALS(dependencyViolationCount.load(), 0);
      }
    }
  }
}

UNIT_TEST("A join with three in dependencies runs after its three producers and before its two sinks") {
  GIVEN("Counters, the flags of three producers and a join and the tokens of these tasks and of two sinks") {
    std::atomic<int>                 producerRunCount{0};
    std::atomic<int>                 joinRunCount{0};
    std::atomic<int>                 sinkRunCount{0};
    std::atomic<int>                 dependencyViolationCount{0};
    std::vector<char>                producerTokens(3);
    char                             joinToken{0};
    std::vector<char>                sinkTokens(2);
    std::array<std::atomic<bool>, 3> produced{};
    std::atomic<bool>                joined{false};
    for (auto& flag : produced) {
      flag.store(false, std::memory_order_relaxed);
    }

    WHEN("The test spawns three delayed producers, a join over them and two sinks of the join and waits") {
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

      THEN("Three producers, one join and two sinks ran and every task ran after the tasks it depends on") {
        VALIDATE_EQUALS(producerRunCount.load(), 3);
        VALIDATE_EQUALS(joinRunCount.load(), 1);
        VALIDATE_EQUALS(sinkRunCount.load(), 2);
        VALIDATE_EQUALS(dependencyViolationCount.load(), 0);
      }
    }
  }
}

UNIT_TEST("A spawn ignores null and duplicate dependencies") {
  GIVEN("Counters, a producer token, a consumer token and the flag of the producer") {
    std::atomic<int>  producerRunCount{0};
    std::atomic<int>  consumerRunCount{0};
    std::atomic<int>  dependencyViolationCount{0};
    char              producerToken{0};
    char              consumerToken{0};
    std::atomic<bool> produced{false};

    WHEN("The test spawns a delayed producer and a consumer with duplicate and null dependencies and waits") {
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

      THEN("The producer and the consumer ran once each, the consumer after the producer") {
        VALIDATE_EQUALS(producerRunCount.load(), 1);
        VALIDATE_EQUALS(consumerRunCount.load(), 1);
        VALIDATE_EQUALS(dependencyViolationCount.load(), 0);
      }
    }
  }
}

UNIT_TEST("A producer with 32 out dependencies runs before every consumer of one of them") {
  GIVEN("Counters, the flag of the producer and 32 out tokens, consumer tokens and consumer flags") {
    std::atomic<int>               producerRunCount{0};
    std::atomic<int>               consumerRunCount{0};
    std::atomic<int>               dependencyViolationCount{0};
    std::atomic<bool>              produced{false};
    std::vector<char>              outTokens(ManyDependencies);
    std::vector<char>              consumerTokens(ManyDependencies);
    std::vector<std::atomic<bool>> consumed(ManyDependencies);
    std::vector<void*>             outDependencies;
    outDependencies.reserve(outTokens.size());
    for (int dependencyIndex = 0; dependencyIndex < ManyDependencies; ++dependencyIndex) {
      consumed[dependencyIndex].store(false, std::memory_order_relaxed);
      outDependencies.push_back(&outTokens[dependencyIndex]);
    }

    WHEN("The test spawns a delayed producer with every out token and one consumer per out token and waits") {
      std::vector<void*> consumerDependencies;
      consumerDependencies.reserve(consumerTokens.size());

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

      THEN("The producer ran once, every consumer ran once after it and every consumer set its flag") {
        for (int dependencyIndex = 0; dependencyIndex < ManyDependencies; ++dependencyIndex) {
          VALIDATE(consumed[dependencyIndex].load(std::memory_order_acquire));
        }
        VALIDATE(produced.load(std::memory_order_acquire));
        VALIDATE_EQUALS(producerRunCount.load(), 1);
        VALIDATE_EQUALS(consumerRunCount.load(), ManyDependencies);
        VALIDATE_EQUALS(dependencyViolationCount.load(), 0);
      }
    }
  }
}

UNIT_TEST("A consumer with 32 in dependencies runs after all its producers whichever producer the test delays") {
  GIVEN("One round per producer, in which this producer runs with the long delay") {
    constexpr int NumberOfRounds = ManyDependencies;

    WHEN("Every round spawns 32 producers and a consumer over all of them and waits for the consumer") {
      std::vector<int> producerRunCountOfRound;
      std::vector<int> consumerRunCountOfRound;
      std::vector<int> dependencyViolationCountOfRound;
      for (int delayedProducerIndex = 0; delayedProducerIndex < NumberOfRounds; ++delayedProducerIndex) {
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

        producerRunCountOfRound.push_back(producerRunCount.load());
        consumerRunCountOfRound.push_back(consumerRunCount.load());
        dependencyViolationCountOfRound.push_back(dependencyViolationCount.load());
      }

      THEN("In every round every producer and the consumer ran once and the consumer ran after all producers") {
        for (int round = 0; round < static_cast<int>(producerRunCountOfRound.size()); round++) {
          VALIDATE_EQUALS(producerRunCountOfRound[round], ManyDependencies, round);
          VALIDATE_EQUALS(consumerRunCountOfRound[round], 1, round);
          VALIDATE_EQUALS(dependencyViolationCountOfRound[round], 0, round);
        }
      }
    }
  }
}

UNIT_TEST("A join with 32 in and 32 out dependencies runs after all its producers and before all its sinks") {
  GIVEN("Counters, the flags of 32 producers, 32 sinks and a join and 32 producer, sink and completion tokens") {
    std::atomic<int>               producerRunCount{0};
    std::atomic<int>               joinRunCount{0};
    std::atomic<int>               sinkRunCount{0};
    std::atomic<int>               dependencyViolationCount{0};
    std::vector<char>              producerTokens(ManyDependencies);
    std::vector<std::atomic<bool>> produced(ManyDependencies);
    std::vector<char>              sinkTokens(ManyDependencies);
    std::vector<std::atomic<bool>> sinkFlags(ManyDependencies);
    std::vector<char>              completionTokens(ManyDependencies);
    std::atomic<bool>              joined{false};

    WHEN("The test spawns and awaits 32 delayed producers, a delayed join over them and one sink per join output") {
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

      THEN("Every producer and every sink ran once and the join ran once after all producers and before all sinks") {
        for (int dependencyIndex = 0; dependencyIndex < ManyDependencies; ++dependencyIndex) {
          VALIDATE(sinkFlags[dependencyIndex].load(std::memory_order_acquire));
        }
        VALIDATE(joined.load(std::memory_order_acquire));
        VALIDATE_EQUALS(producerRunCount.load(), ManyDependencies);
        VALIDATE_EQUALS(joinRunCount.load(), 1);
        VALIDATE_EQUALS(sinkRunCount.load(), ManyDependencies);
        VALIDATE_EQUALS(dependencyViolationCount.load(), 0);
      }
    }
  }
}

UNIT_TEST("A wait for several dependencies returns after every producer of them finished") {
  GIVEN("One token and one produced flag for each of eight producers") {
    constexpr int                                    NumberOfProducers = 8;
    std::array<std::atomic<bool>, NumberOfProducers> produced{};
    std::vector<char>                                tokens(NumberOfProducers);

    WHEN("The test spawns eight delayed producers that set their flag and waits for all tokens at once") {
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

      THEN("Every producer set its flag") {
        for (int producerIndex = 0; producerIndex < NumberOfProducers; producerIndex++) {
          VALIDATE(produced[producerIndex].load(std::memory_order_acquire));
        }
      }
    }
  }
}

UNIT_TEST("Producers and their consumer keep their order in every sweep that reuses the same dependency addresses") {
  GIVEN("Counters, eight producer tokens and flags and a consumer token") {
    constexpr int NumberOfTasksPerSweep = 8;

    std::atomic<int>               producerRunCount{0};
    std::atomic<int>               consumerRunCount{0};
    std::atomic<int>               dependencyViolationCount{0};
    std::vector<char>              producerTokens(NumberOfTasksPerSweep);
    char                           consumerToken{0};
    std::vector<std::atomic<bool>> produced(NumberOfTasksPerSweep);
    std::vector<void*>             producerTokenAddresses(NumberOfTasksPerSweep);
    for (int taskIndex = 0; taskIndex < NumberOfTasksPerSweep; taskIndex++) {
      producerTokenAddresses[taskIndex] = &producerTokens[taskIndex];
    }

    WHEN("400 sweeps each spawn eight producers and a consumer over all of them and wait for every token") {
      constexpr int NumberOfSweeps = 400;

      std::vector<int> producerRunCountAfterSweep;
      std::vector<int> consumerRunCountAfterSweep;
      std::vector<int> dependencyViolationCountAfterSweep;
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
          new FlagCheckingTask(
            consumerRunCount,
            dependencyViolationCount,
            produced.data(),
            NumberOfTasksPerSweep,
            nullptr
          ),
          producerTokenAddresses,
          consumerTokenAddress
        );

        tarch::tasking::waitForDependencies(producerTokenAddresses);
        tarch::tasking::waitForDependency(&consumerToken);

        producerRunCountAfterSweep.push_back(producerRunCount.load());
        consumerRunCountAfterSweep.push_back(consumerRunCount.load());
        dependencyViolationCountAfterSweep.push_back(dependencyViolationCount.load());
      }

      THEN("Every sweep adds one run of every producer and of the consumer and the consumer runs last") {
        for (int sweep = 0; sweep < static_cast<int>(producerRunCountAfterSweep.size()); sweep++) {
          VALIDATE_EQUALS(producerRunCountAfterSweep[sweep], (sweep + 1) * NumberOfTasksPerSweep, sweep);
          VALIDATE_EQUALS(consumerRunCountAfterSweep[sweep], sweep + 1, sweep);
          VALIDATE_EQUALS(dependencyViolationCountAfterSweep[sweep], 0, sweep);
        }
      }
    }
  }
}

UNIT_TEST("A task that spawns tasks waits for them before it returns") {
  GIVEN("Counters of the inner and the outer runs and one token for each of eight outer tasks") {
    constexpr int NumberOfOuterTasks = 8;

    std::atomic<int>   innerRunCount{0};
    std::atomic<int>   outerRunCount{0};
    std::vector<char>  outerTokens(NumberOfOuterTasks);
    std::vector<void*> outerTokenAddresses(NumberOfOuterTasks);
    for (int outerIndex = 0; outerIndex < NumberOfOuterTasks; outerIndex++) {
      outerTokenAddresses[outerIndex] = &outerTokens[outerIndex];
    }

    WHEN("Eight outer tasks each spawn four inner tasks and wait for them and the caller waits for the outer tasks") {
      constexpr int NumberOfInnerTasks = 4;

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

      THEN("Every outer task and every inner task ran once") {
        VALIDATE_EQUALS(outerRunCount.load(), NumberOfOuterTasks);
        VALIDATE_EQUALS(innerRunCount.load(), NumberOfOuterTasks * NumberOfInnerTasks);
      }
    }
  }
}
