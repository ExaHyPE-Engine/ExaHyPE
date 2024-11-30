#pragma once

#include <atomic>
#include <vector>

#include <lang/assert.h>
#include <lang/type.h>

template<typename Item, u32 CELLS_PER_CONSUMER = 16>
class SPMCChannel {
    struct ItemWrapper {
        Item item = {};
        std::atomic_uint64_t itemVersion = 0;
    };

    std::unique_ptr<ItemWrapper[]> buf;
    u32 bufSize = 0;
    std::atomic_bool closed = false;

    u64 insertionCandidateIdxHint = 0;
    u64 version = 1;

    bool tryPut(ItemWrapper *wrapper, Item item) {
        if (wrapper->itemVersion.load(std::memory_order_relaxed)) return false;
        wrapper->item = item;

        std::atomic_thread_fence(std::memory_order_release);

        wrapper->itemVersion.store(this->version++, std::memory_order_relaxed);
        return true;
    }

    std::optional<Item> tryGet(ItemWrapper *wrapper) {
        auto staleVersion = wrapper->itemVersion.load(std::memory_order_relaxed);

        if (!staleVersion) [[unlikely]] return std::nullopt;

        auto valueCandidate = wrapper->item;

        std::atomic_thread_fence(std::memory_order_acquire);

        auto writeSuccess = wrapper->itemVersion.compare_exchange_weak(staleVersion, 0, std::memory_order_relaxed);

        return writeSuccess ? std::optional(valueCandidate) : std::nullopt;
    }

public:
    class Reader {
        SPMCChannel *channel;
        const u32 firstItemHint;
        u32 nextItemHint;

    public:
        explicit Reader(SPMCChannel *channel, u32 consumerIdHint = 0) : channel(channel), firstItemHint(consumerIdHint * CELLS_PER_CONSUMER), nextItemHint(firstItemHint) {}

        std::optional<Item> get() {
            auto valueOpt = this->getFast();
            if (valueOpt) return valueOpt;

            valueOpt = this->getOther();

            return valueOpt;
        }

        std::optional<Item> getFast() {
            for (u32 itemCandidateIdx = this->nextItemHint; itemCandidateIdx < this->firstItemHint + CELLS_PER_CONSUMER - 1; itemCandidateIdx++) {
                auto valueOpt = this->channel->tryGet(this->channel->buf.get() + itemCandidateIdx);
                if (valueOpt) {
                    this->nextItemHint = itemCandidateIdx + 1;
                    return valueOpt;
                }
            }

            auto valueOpt = this->channel->tryGet(this->channel->buf.get() + this->firstItemHint + CELLS_PER_CONSUMER - 1);
            if (valueOpt) {
                this->nextItemHint = 0;
                return valueOpt;
            }

            this->nextItemHint = this->firstItemHint;
            return std::nullopt;
        }

        std::optional<Item> getOther() {
            auto consumerChunkStartHint = this->firstItemHint + CELLS_PER_CONSUMER;  // start searching at the end of the 'fast' range

            for (u32 itemCandidateIdx = consumerChunkStartHint; itemCandidateIdx < this->channel->bufSize; itemCandidateIdx++) {
                auto valueOpt = this->channel->tryGet(this->channel->buf.get() + itemCandidateIdx);
                if (valueOpt) return valueOpt;
            }

            for (u32 itemCandidateIdx = 0; itemCandidateIdx < this->firstItemHint; itemCandidateIdx++) {
                auto valueOpt = this->channel->tryGet(this->channel->buf.get() + itemCandidateIdx);
                if (valueOpt) return valueOpt;
            }

            return std::nullopt;
        }
    };

    explicit SPMCChannel(u32 consumerCountHint = 1) : bufSize(consumerCountHint * CELLS_PER_CONSUMER) {
        this->buf = std::make_unique<ItemWrapper[]>(this->bufSize);
    }

    Reader getReader(u32 consumerIdHint = 0) {
        return Reader(this, consumerIdHint);
    }

    bool put(Item item) {
        for (u32 insertionIdxCandidate = this->insertionCandidateIdxHint; insertionIdxCandidate < this->bufSize; insertionIdxCandidate++) {
            auto success = this->tryPut(this->buf.get() + insertionIdxCandidate, item);
            if (success) {
                this->insertionCandidateIdxHint = insertionIdxCandidate + 1;
                return true;
            }
        }

        for (u32 insertionIdxCandidate = 0; insertionIdxCandidate < this->insertionCandidateIdxHint; insertionIdxCandidate++) {
            auto success = this->tryPut(this->buf.get() + insertionIdxCandidate, item);
            if (success) {
                this->insertionCandidateIdxHint = insertionIdxCandidate + 1;
                return true;
            }
        }

        return false;
    }

    void close() {
        this->closed.store(true, std::memory_order_relaxed);
    }

    bool isClosed() const {
        return this->closed.load(std::memory_order_relaxed);
    }
};
