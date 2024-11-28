#pragma once

#include <vector>
#include <x86intrin.h>

template<typename Tp, typename Alloc = std::allocator<Tp>>
class Vector : public std::vector<Tp, Alloc> {

public:
    Vector() noexcept = default;

    Vector(const std::initializer_list<Tp> &list) {
        this->reserve(list.size());
        for (auto &element : list) this->push_back(element);
    }

    Tp unstableTake(std::size_t idx) {
        auto &idxItem = this->operator[](idx);
        auto &lastItem = this->operator[](this->size() - 1);
        std::swap(idxItem, lastItem);

        auto item = this->back();
        this->pop_back();
        return item;
    }
};

template<typename ITEM, typename ALLOCATOR>
class Vec {

    ITEM* dataPtr = nullptr;

    ALLOCATOR allocator;

    unsigned int capacity = 0;
    unsigned int freeIdx = 0;

public:
    using value_type = ITEM;

    explicit Vec(unsigned int capacity = 0, ALLOCATOR allocator = ALLOCATOR()) : allocator(allocator), capacity(capacity) {
        if (capacity == 0) return;
        this->dataPtr = this->allocator.template alloc<ITEM>(capacity);
    }

    void reserve(unsigned int newCapacity) {
        assert(this->capacity == 0)
        this->capacity = newCapacity;
        this->dataPtr = this->allocator.template alloc<ITEM>(newCapacity);
    }

    void prefetch() const {
        char *data = (char*) this->dataPtr;
        auto laneCount = (sizeof(ITEM) * this->size() + 63) / 64;
        for (int i = 0; i < laneCount; i++) _mm_prefetch(data + i * 64, _MM_HINT_NTA);
    }

    void push_back(ITEM &item) {
        if (this->freeIdx == this->capacity) {
            auto newCapacity = std::max(this->capacity * 2, 4U);  // insures against capacity 0
            auto newData = this->allocator.template alloc<ITEM>(newCapacity);
            memcpy(newData, this->dataPtr, sizeof(ITEM) * this->capacity);
            this->allocator.del(this->dataPtr);
            this->capacity = newCapacity;
            this->dataPtr = newData;
        }

        this->dataPtr[this->freeIdx] = item;
        this->freeIdx++;
    }

    ITEM& operator [](int idx) const {
        return this->dataPtr[idx];
    }

    void clear() {
        this->freeIdx = 0;
    }

    unsigned int size() const {
        return this->freeIdx;
    }

    ITEM* data() const {
        return this->dataPtr;
    }

    Vec(const Vec&) = delete;

    Vec& operator=(Vec&& other) noexcept {
        this->allocator.del(this->dataPtr);

        this->dataPtr = other.dataPtr;
        this->capacity = other.capacity;
        this->freeIdx = other.freeIdx;

        other.dataPtr = nullptr;
        other.freeIdx = 0;
        other.capacity = 0;
        return *this;
    }

    ~Vec() {
        this->allocator.del(this->dataPtr);
    }

    ITEM* begin() {
        return this->dataPtr;
    }

    ITEM* end() {
        return this->dataPtr + this->freeIdx;
    }

    const ITEM* begin() const {
        return this->dataPtr;
    }

    const ITEM* end() const {
        return this->dataPtr + this->freeIdx;
    }
};
