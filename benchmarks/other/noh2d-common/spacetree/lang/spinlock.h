#pragma once

#include <atomic>
#include <emmintrin.h>

// https://rigtorp.se/spinlock/
class Spinlock {
    std::atomic<bool> lock_ = {false};

public:

    void lock() {
        for (;;) {
            // Optimistically assume the lock is free on the first try
            if (!this->lock_.exchange(true, std::memory_order_acquire)) {
                return;
            }
            // Wait for lock to be released without generating cache misses
            while (this->lock_.load(std::memory_order_relaxed)) {
                // Issue X86 PAUSE or ARM YIELD instruction to reduce contention between
                // hyper-threads
                _mm_pause();
            }
        }
    }

    void unlock() {
        lock_.store(false, std::memory_order_release);
    }
};
