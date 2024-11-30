#pragma once

#include <atomic>
#include <condition_variable>
#include <functional>
#include <thread>

#include "assert.h"
#include "type.h"

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

    static u32 getPoolSize(u32 poolSize = 0) {
        if (poolSize > 0) return poolSize;

        auto *poolSizeStr = std::getenv("THREAD_POOL_SIZE");
        if (poolSizeStr) {
            poolSize = std::stoi(poolSizeStr) - 1;
            return poolSize;
        }

        poolSize = std::thread::hardware_concurrency() - 1;
        return poolSize;
    }

public:
    ThreadPool(const ThreadPool &copy) = delete;

    ThreadPool(ThreadPool &&move) = default;

    ThreadPool &operator=(ThreadPool &&move) = default;

    explicit ThreadPool(u32 poolSize = 0) {
        poolSize = ThreadPool::getPoolSize(poolSize);

        printf("Starting thread pool with 1 main thread + %u worker(s)\n", poolSize);

        this->threadPool = std::make_unique<PooledThread[]>(poolSize);
        this->poolSize = poolSize;

        for (u32 i = 0; i < poolSize; i++) {
            auto *pooledThread = &this->threadPool[i];
            pooledThread->runnable = nullptr;
            pooledThread->shutdown = false;
            pooledThread->thread = std::thread(PooledThread::threadLoop, pooledThread);
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
