#pragma once

#include <atomic>
#include <condition_variable>
#include <functional>
#include <thread>

#include <lang/assert.h>
#include <lang/type.h>

class ThreadPool {
    struct PooledThread {
        std::thread thread;
        std::mutex mutex;
        std::condition_variable cv;
        std::function<void()> runnable = nullptr;
        std::atomic_bool busy = false;
        std::atomic_bool shutdown = false;
        std::atomic_bool dead = false;

        bool isBusy() const {
            return this->busy.load(std::memory_order_relaxed);
        }

        bool isDead() const {
            return this->dead.load(std::memory_order_relaxed);
        }

        void assignWork(std::function<void()> newRunnable) {
            assert(!this->isBusy())
            this->runnable = std::move(newRunnable);
            this->busy.store(true, std::memory_order_release);
            this->cv.notify_one();
        }

        void sendShutdown() {
            this->shutdown.store(true, std::memory_order_relaxed);
            this->cv.notify_one();
        }

        void join() {
            assert(this->isDead())
            this->thread.join();
        }

        static void threadLoop(PooledThread *self) {
            while (true) {
                std::unique_lock<std::mutex> lock(self->mutex);
                self->cv.wait(lock, [=] {
                    return self->busy.load(std::memory_order_relaxed) | self->shutdown.load(std::memory_order_relaxed);
                });

                if (self->shutdown.load(std::memory_order_relaxed)) [[unlikely]] break;

                self->runnable();
                self->busy.store(false, std::memory_order_release);
            }

            self->dead.store(true, std::memory_order_release);
        }
    };

    std::unique_ptr<PooledThread[]> threadPool;
    u32 poolSize = 0;
    u32 freeThreadIdx = 0;

    void setThreadAffinity(auto nativeHandle, u32 coreId) {
        cpu_set_t cpuset;
        CPU_ZERO(&cpuset);
        CPU_SET(coreId, &cpuset);
        int rc = pthread_setaffinity_np(nativeHandle, sizeof(cpu_set_t), &cpuset);
        assert(rc == 0)
    }

    static std::vector<u32> getThreadConfigBindings() {
        auto *poolSizeCharPtr = std::getenv("THREAD_POOL_CONFIG");
        if (!poolSizeCharPtr) return {};

        auto poolSizeStr = std::string(poolSizeCharPtr);

        std::vector<u32> bindings;

        auto lastCommaIdx = 0ul;
        while (lastCommaIdx < poolSizeStr.size()) {
            auto newCommaIdx = poolSizeStr.find(',', lastCommaIdx);
            if (newCommaIdx == -1) newCommaIdx = poolSizeStr.size();
            auto chunk = poolSizeStr.substr(lastCommaIdx, newCommaIdx);

            auto firstColon = chunk.find(':');
            assert(firstColon > 0)

            auto secondColon = chunk.find(':', firstColon + 1);
            assert(secondColon > 0)

            auto start = std::stoi(chunk.substr(0, firstColon));
            auto end = std::stoi(chunk.substr(firstColon + 1, secondColon));
            auto inc = std::stoi(chunk.substr(secondColon + 1));
            for (u32 i = start; i < end; i += inc) bindings.push_back(i);

            lastCommaIdx = newCommaIdx + 1;
        }

        return bindings;
    }

    static std::vector<u32> getThreadBindingsFromSize(u32 threadingLevel = 1) {
        if (threadingLevel == 1) {
            auto hardwareMaxThreads = std::thread::hardware_concurrency();

            auto *poolSizeStr = std::getenv("THREAD_POOL_SIZE");
            if (!poolSizeStr) {
                threadingLevel = hardwareMaxThreads;
            } else {
                auto parsedPoolSize = std::stoi(poolSizeStr);
                if (!parsedPoolSize) {
                    threadingLevel = hardwareMaxThreads;
                } else {
                    threadingLevel = parsedPoolSize;
                }
            }
        }

        std::vector<u32> bindings;
        for (u32 i = 0; i < threadingLevel; i++) bindings.push_back(i);
        return bindings;
    }

public:
    ThreadPool(const ThreadPool &copy) = delete;

    ThreadPool(ThreadPool &&move) = default;

    ThreadPool &operator=(ThreadPool &&move) = default;

    explicit ThreadPool(u32 poolSize = 0) {
        auto threadBindings = ThreadPool::getThreadConfigBindings();
        if (threadBindings.empty()) threadBindings = ThreadPool::getThreadBindingsFromSize(poolSize + 1);
        poolSize = threadBindings.size() - 1;

        printf("Starting thread pool with 1 main thread + %u worker(s)\n", poolSize);
        printf("Thread bindings: [ ");
        for (auto binding : threadBindings) {
            printf("%u ", binding);
        }
        printf("]\n");

        this->threadPool = std::make_unique<PooledThread[]>(poolSize);
        this->poolSize = poolSize;

        this->setThreadAffinity(pthread_self(), threadBindings[0]);

        for (u32 i = 0; i < poolSize; i++) {
            auto *pooledThread = &this->threadPool[i];
            pooledThread->runnable = nullptr;
            pooledThread->shutdown = false;
            pooledThread->thread = std::thread(PooledThread::threadLoop, pooledThread);

            this->setThreadAffinity(pooledThread->thread.native_handle(), threadBindings[1+i]);
        }
    }

    void dispatch(const std::function<void()> &work, u32 numThreads = 0) {
        if (numThreads == 0) numThreads = this->poolSize;
        assert(this->freeThreadIdx + numThreads <= this->poolSize)

        for (u32 i = this->freeThreadIdx; i < this->freeThreadIdx + numThreads; i++) {
            this->threadPool[i].assignWork(work);
        }

        this->freeThreadIdx += numThreads;
    }

    void wait() {
        while (true) {
            auto anyBusy = false;
            for (u32 threadId = 0; threadId < this->freeThreadIdx; threadId++) {
                anyBusy |= this->threadPool[threadId].isBusy();
            }
            if (!anyBusy) break;
            for (u32 threadId = 0; threadId < this->freeThreadIdx; threadId++) {
                this->threadPool[threadId].cv.notify_one();
            }
        }

        std::atomic_thread_fence(std::memory_order_seq_cst);

        this->freeThreadIdx = 0;
    }

    void shutdown() {
        for (u32 threadId = 0; threadId < this->poolSize; threadId++) {
            this->threadPool[threadId].sendShutdown();
        }

        while (true) {
            auto allDead = true;
            for (u32 threadId = 0; threadId < this->poolSize; threadId++) {
                allDead &= this->threadPool[threadId].isDead();
            }

            if (allDead) break;

            for (u32 threadId = 0; threadId < this->poolSize; threadId++) {
                this->threadPool[threadId].cv.notify_one();
            }
        }

        for (u32 threadId = 0; threadId < this->poolSize; threadId++) {
            this->threadPool[threadId].join();
        }
    }

    u32 size() const {
        return this->poolSize;
    }

    ~ThreadPool() {
        this->shutdown();
    }
};
