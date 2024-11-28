#pragma once

#include <omp.h>

#include <atomic>
#include <condition_variable>

#include <lang/assert.h>
#include <lang/channel.h>
#include <lang/thread_pool.h>
#include <lang/type.h>

struct TaskCtx {
    u32 workerId;
};

template<typename T>
concept CtxUnawareStatelessTask = requires(T t) {
    { t() } -> std::same_as<void>;
};

template<typename T>
concept CtxAwareStatelessTask = requires(T t, TaskCtx &ctx) {
    { t(ctx) } -> std::same_as<void>;
};

template<typename T>
concept StatelessTask = CtxUnawareStatelessTask<T> || CtxAwareStatelessTask<T>;

template<typename T>
concept OptionalStatelessTask = Optional<T> && StatelessTask<typename T::value_type>;

template<typename T>
concept StatelessTaskGenerator = requires(T t) {
    { t() } -> OptionalStatelessTask;
};

template<typename T, typename State>
concept CtxUnawareStatefulTask = requires(T t, State &s) {
    { t(s) } -> std::same_as<void>;
};

template<typename T, typename State>
concept CtxAwareStatefulTask = requires(T t, State &s, TaskCtx &ctx) {
    { t(s, ctx) } -> std::same_as<void>;
};

template<typename T, typename State>
concept StatefulTask = CtxUnawareStatefulTask<T, State> || CtxAwareStatefulTask<T, State>;

template<typename T, typename State>
concept OptionalStatefulTask = Optional<T> && StatefulTask<typename T::value_type, State>;

template<typename T, typename State>
concept StatefulTaskGenerator = requires(T t) {
    { t() } -> OptionalStatefulTask<State>;
};

StatefulTaskGenerator<Any> auto ToStatefulGenerator(StatelessTaskGenerator auto generator) {
    return Lambda([](auto &generator) {
        auto taskOpt = generator();

        using TaskType = std::remove_reference_t<decltype(taskOpt.value())>;

        if constexpr (CtxUnawareStatelessTask<TaskType>) {
            return taskOpt ? std::optional(Lambda([](auto &task, auto &acc) { task(); }, taskOpt.value())) : std::nullopt;
        } else if constexpr (CtxAwareStatelessTask<TaskType>) {
            return taskOpt ? std::optional(Lambda([](auto &task, auto &acc, auto &ctx) { task(ctx); }, taskOpt.value())) : std::nullopt;
        } else static_assert(false);

    }, generator);
}

class OmpTaskBackend {
    template<typename Val, StatefulTask<Val> Task>
    static void invokeTask(Task &task, Val &val, i32 threadId) {
        if constexpr (CtxAwareStatefulTask<Task, Val>) {
            auto ctx = TaskCtx{.workerId = (u32) threadId};
            task(val, ctx);
        } else if constexpr (CtxUnawareStatefulTask<Task, Val>) {
            task(val);
        } else static_assert(false);
    }

public:
    template<StatelessTaskGenerator T>
    static void runSerialGenerator(T generator, u32 numThreads = 0) {
        numThreads = OmpTaskBackend::THREAD_COUNT(numThreads);
        auto mapReducerGenerator = ToStatefulGenerator(generator);
        OmpTaskBackend::runSerialGenerator(mapReducerGenerator, [](){return Any{};}, numThreads);
    }

    template<typename StateInitF, StatefulTaskGenerator<std::invoke_result_t<StateInitF>> T>
    static auto runSerialGenerator(T generator, StateInitF stateInitF, u32 numThreads = 0) {
        using ValueType = std::invoke_result_t<StateInitF>;

        numThreads = OmpTaskBackend::THREAD_COUNT(numThreads);

        auto accumulators = std::vector<ValueType>(numThreads);

        #pragma omp parallel num_threads(numThreads) default(none) shared(numThreads, generator, accumulators, stateInitF)
        #pragma omp single nowait
        {
            for (int i = 0; i < numThreads; i++) {
                #pragma omp task default(none) shared(stateInitF, accumulators) firstprivate(i)
                accumulators[i] = stateInitF();
            }

            #pragma omp taskwait

            while (true) {
                auto taskOpt = generator();
                if (!taskOpt) [[unlikely]] break;

                auto task = taskOpt.value();
                #pragma omp task default(none) shared(stateInitF, accumulators) firstprivate(task)
                {
                    auto threadNum = omp_get_thread_num();
                    OmpTaskBackend::invokeTask(task, accumulators[threadNum], threadNum);
                }
            }
        }

        return accumulators;
    }

    template<StatelessTaskGenerator T>
    static void runParallelGenerator(T generator, u32 numThreads = 0) {
        numThreads = OmpTaskBackend::THREAD_COUNT(numThreads);
        auto mapReducerGenerator = ToStatefulGenerator(generator);
        OmpTaskBackend::runParallelGenerator(mapReducerGenerator, [](){ return Any{}; }, numThreads);
    }

    template<typename StateInitF, StatefulTaskGenerator<std::invoke_result_t<StateInitF>> T>
    static auto runParallelGenerator(T generator, StateInitF stateInitF, u32 numThreads = 0) {
        using ValueType = std::invoke_result_t<StateInitF>;

        numThreads = OmpTaskBackend::THREAD_COUNT(numThreads);

        auto accumulators = std::vector<ValueType>(numThreads);

        #pragma omp parallel num_threads(numThreads) default(none) shared(generator, stateInitF, accumulators)
        {
            auto threadId = omp_get_thread_num();
            accumulators[threadId] = stateInitF();

            while (true) {
                auto taskOpt = generator();
                if (!taskOpt) [[unlikely]] break;

                auto task = taskOpt.value();
                OmpTaskBackend::invokeTask(task, accumulators[threadId], threadId);
            }
        }

        return accumulators;
    }

    static u32 MAX_THREADS() {
        auto numThreads = (u32) omp_get_max_threads();
        return numThreads;
    }

    static u32 THREAD_COUNT(u32 numThreads = 0) {
        if (numThreads == 0) numThreads = OmpTaskBackend::MAX_THREADS();
        numThreads = std::clamp(numThreads, (u32) 1, OmpTaskBackend::MAX_THREADS());
        return numThreads;
    }
};

class ThreadPoolTaskBackend {
    template<typename Task, typename TaskGenerator>
    static std::optional<Task> createNewTasks(TaskGenerator *generator,
                                              SPMCChannel<Task> *channel,
                                              std::atomic_int32_t *producerThreadId) {
        if (channel->isClosed()) {
            producerThreadId->store(-1, std::memory_order_relaxed);
            return std::nullopt;
        }

        while (true) {
            auto taskOpt = (*generator)();

            if (!taskOpt) [[unlikely]] {
                channel->close();
                producerThreadId->store(-1, std::memory_order_relaxed);
                return std::nullopt;
            }

            auto task = taskOpt.value();

            auto success = channel->put(task);
            if (!success) [[unlikely]] return task;
        }
    }

    template<typename Val, StatefulTask<Val> Task>
    static void invokeTask(Task &task, Val &val, i32 threadId) {
        if constexpr (CtxAwareStatefulTask<Task, Val>) {
            auto ctx = TaskCtx{.workerId = (u32) threadId};
            task(val, ctx);
        } else if constexpr (CtxUnawareStatefulTask<Task, Val>) {
            task(val);
        } else static_assert(false);
    }

    template<typename ValueType, StatefulTaskGenerator<ValueType> TaskGenerator, StatefulTask<ValueType> Task, typename StateInitF>
    static void producerConsumerLoop(i32 threadId,
                                     TaskGenerator *generator,
                                     SPMCChannel<Task> *channel,
                                     StateInitF stateInitF,
                                     ValueType *retVal,
                                     std::atomic_int32_t *producerThreadId) {
        auto rx = channel->getReader(threadId);
        auto acc = stateInitF();

        if (threadId == producerThreadId->load(std::memory_order_relaxed)) [[unlikely]] goto I_AM_PRODUCER;
        else goto I_AM_CONSUMER;

        I_AM_PRODUCER:
        while (true) {
            auto myNewTaskOpt = createNewTasks(generator, channel, producerThreadId);
            if (!myNewTaskOpt) [[unlikely]] goto I_AM_CONSUMER;

            producerThreadId->store(-1, std::memory_order_relaxed);
            auto myNewTask = myNewTaskOpt.value();
            ThreadPoolTaskBackend::invokeTask(myNewTask, acc, threadId);

            auto currentProducerThreadId = producerThreadId->load(std::memory_order_relaxed);
            while (currentProducerThreadId == -1) {
                producerThreadId->compare_exchange_weak(currentProducerThreadId, threadId);
            }

            if (currentProducerThreadId != threadId) [[unlikely]] goto I_AM_CONSUMER;
        }

        I_AM_CONSUMER:
        while (true) {
            auto taskOpt = rx.getFast();

            if (taskOpt) {
                auto task = taskOpt.value();
                ThreadPoolTaskBackend::invokeTask(task, acc, threadId);
                continue;
            }

            if (channel->isClosed()) [[unlikely]] {
                while (true) {
                    taskOpt = rx.getOther();
                    if (taskOpt) {
                        auto task = taskOpt.value();
                        ThreadPoolTaskBackend::invokeTask(task, acc, threadId);
                        continue;
                    }
                    break;
                }

                *retVal = std::move(acc);
                return;
            }

            auto m1 = -1;
            auto someElseGenerator = producerThreadId->compare_exchange_weak(m1, threadId);
            if (!someElseGenerator) continue;

            goto I_AM_PRODUCER;
        }
    }

    static ThreadPool &WORKER_POOL() {
        static auto value = ThreadPool();
        return value;
    }

    template<typename StateInitF, StatefulTaskGenerator<std::invoke_result_t<StateInitF>> T>
    static auto runSerialGeneratorSerial(T generator, StateInitF stateInitF) {
        using ValueType = std::invoke_result_t<StateInitF>;

        auto accumulators = std::vector<ValueType>(1);
        accumulators[0] = stateInitF();

        while (true) {
            auto taskOpt = generator();
            if (!taskOpt) break;
            auto task = taskOpt.value();
            ThreadPoolTaskBackend::invokeTask(task, accumulators[0], 0);
        }

        return accumulators;
    }

    template<typename StateInitF, StatefulTaskGenerator<std::invoke_result_t<StateInitF>> T>
    static auto runSerialGeneratorParallel(T generator, StateInitF stateInitF, u32 numThreads) {
        using ValueType = std::invoke_result_t<StateInitF>;
        using Task = std::invoke_result_t<T>::value_type;

        auto channel = SPMCChannel<Task>(numThreads);
        auto accumulators = std::vector<ValueType>(numThreads);

        std::atomic_int32_t producerThreadId = 0;

        for (i32 localHelperId = 1; localHelperId < numThreads; localHelperId++) {
            auto runnable = Lambda([](auto localHelperId, auto *generator, auto *channel, auto stateInitF, auto *accumulator, auto *producerThreadId) {
                producerConsumerLoop<ValueType, T, Task, StateInitF>(localHelperId, generator, channel, stateInitF, accumulator, producerThreadId);
            }, localHelperId, &generator, &channel, stateInitF, &accumulators[localHelperId], &producerThreadId);

            ThreadPoolTaskBackend::WORKER_POOL().dispatch(runnable, 1);
        }

        producerConsumerLoop<ValueType, T, Task, StateInitF>(0, &generator, &channel, stateInitF, &accumulators[0], &producerThreadId);

        ThreadPoolTaskBackend::WORKER_POOL().wait();

        return accumulators;
    }

    template<typename StateInitF, StatefulTaskGenerator<std::invoke_result_t<StateInitF>> T>
    static auto runParallelGeneratorParallel(T &generator, StateInitF stateInitF, u32 numThreads) {
        using ValueType = std::invoke_result_t<StateInitF>;

        auto accumulators = std::vector<ValueType>(numThreads);

        for (u32 localHelperId = 1; localHelperId < numThreads; localHelperId++) {
            ThreadPoolTaskBackend::WORKER_POOL().dispatch([localHelperId, &accumulators, &generator, stateInitF]() {
                auto acc = stateInitF();
                while (true) {
                    auto taskOpt = generator();
                    if (!taskOpt) break;

                    auto task = taskOpt.value();
                    ThreadPoolTaskBackend::invokeTask(task, acc, localHelperId);
                }
                accumulators[localHelperId] = acc;
            }, 1);
        }

        auto &acc = accumulators[0];
        acc = stateInitF();
        while (true) {
            auto taskOpt = generator();
            if (!taskOpt) break;

            auto task = taskOpt.value();
            ThreadPoolTaskBackend::invokeTask(task, acc, 0);
        }

        ThreadPoolTaskBackend::WORKER_POOL().wait();

        return accumulators;
    }

public:
    template<StatelessTaskGenerator T>
    static void runSerialGenerator(T generator, u32 numThreads = 0) {
        numThreads = ThreadPoolTaskBackend::THREAD_COUNT(numThreads);
        auto mapReducerGenerator = ToStatefulGenerator(generator);
        ThreadPoolTaskBackend::runSerialGenerator(mapReducerGenerator, [](){return Any{};}, numThreads);
    }

    template<typename StateInitF, StatefulTaskGenerator<std::invoke_result_t<StateInitF>> T>
    static auto runSerialGenerator(T generator, StateInitF stateInitF, u32 numThreads = 0) {
        numThreads = ThreadPoolTaskBackend::THREAD_COUNT(numThreads);
        if (numThreads == 1) [[unlikely]] return ThreadPoolTaskBackend::runSerialGeneratorSerial(generator, stateInitF);
        else return ThreadPoolTaskBackend::runSerialGeneratorParallel(generator, stateInitF, numThreads);
    }

    template<StatelessTaskGenerator T>
    static void runParallelGenerator(T generator, u32 numThreads = 0) {
        numThreads = ThreadPoolTaskBackend::THREAD_COUNT(numThreads);
        auto mapReducerGenerator = ToStatefulGenerator(generator);
        ThreadPoolTaskBackend::runParallelGenerator(mapReducerGenerator, [](){return Any{};}, numThreads);
    }

    template<typename StateInitF, StatefulTaskGenerator<std::invoke_result_t<StateInitF>> T>
    static auto runParallelGenerator(T generator, StateInitF stateInitF, u32 numThreads = 0) {
        numThreads = ThreadPoolTaskBackend::THREAD_COUNT(numThreads);
        if (numThreads == 1) [[unlikely]] return ThreadPoolTaskBackend::runSerialGeneratorSerial(generator, stateInitF);
        else return ThreadPoolTaskBackend::runParallelGeneratorParallel(generator, stateInitF, numThreads);
    }

    static u32 THREAD_COUNT(u32 numThreads = 0) {
        if (numThreads == 0) numThreads = ThreadPoolTaskBackend::MAX_THREADS();
        numThreads = std::clamp(numThreads, (u32) 1, ThreadPoolTaskBackend::MAX_THREADS());
        return numThreads;
    }

    static u32 MAX_THREADS() {
        auto size = 1 + ThreadPoolTaskBackend::WORKER_POOL().size();
        return size;
    }
};
