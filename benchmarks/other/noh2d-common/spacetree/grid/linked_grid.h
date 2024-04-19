#pragma once

#include <lang/type.h>
#include <lang/numvec.h>
#include <geometry/geometry.h>

template<typename _Geo, u32 SPLIT_FACTOR, typename Derived> requires (SPLIT_FACTOR > 1)
class LinkedGrid {
    static _Geo::RowMajorIdx getIdxForLinearSpaceIdx(u32 linearSpaceIdx) {
        auto idx = typename Geo::RowMajorIdx(0);

        for (int d = 0; d < Geo::D::N; d++) {
            idx[d] = (linearSpaceIdx / (u32) std::pow(SPLIT_FACTOR, d)) % SPLIT_FACTOR;
        }

        return idx;
    }

    _Geo::RowMajorIdx getChildIdx(int linearSpaceChildIdx) const {
        auto childIdx = getIdxForLinearSpaceIdx(linearSpaceChildIdx);
        childIdx = this->idx * SPLIT_FACTOR + childIdx;
        return childIdx;
    }

    void moveOp(LinkedGrid &&other) {
        assert(this->subtrees == nullptr)
        assert(other.parent == nullptr)

        this->range = other.range;
        this->idx = other.idx;
        this->level = other.level;
        this->subtrees = other.subtrees;

        other.subtrees = nullptr;

        if (!this->subtrees) return;

        for (u32 i = 0; i < LinkedGrid::getMaxChildren(); i++) {
            this->subtrees[i].parent = (Derived*) this;
        }
    }

public:
    using Geo = _Geo;

    static constexpr u32 SplitFactor = SPLIT_FACTOR;

    Derived *parent = nullptr;
    Derived *subtrees = nullptr; // TODO find more suitable names for these

    Geo::Region range = typename Geo::Region();

    Geo::RowMajorIdx idx = typename Geo::RowMajorIdx(0);
    unsigned int level = 0;

    LinkedGrid() = default;

    explicit LinkedGrid(Geo::Region range,
                        Derived *parent = nullptr,
                        Geo::RowMajorIdx idx = typename Geo::RowMajorIdx(0),
                        int level = 0) : parent(parent), range(range), idx(idx), level(level) {}

    static constexpr u32 getMaxChildren() {
        return Geo::template PowerN<SPLIT_FACTOR>();
    }

    LinkedGrid(LinkedGrid&& other) noexcept {
        this->moveOp(std::move(other));
    }

    LinkedGrid& operator=(LinkedGrid&& other) noexcept {
        this->moveOp(std::move(other));
        return *this;
    }

    void split(u32 splitLevel = 1) {
        if (splitLevel == 0) return;

        if (!this->subtrees) {
            this->subtrees = (Derived*) malloc(sizeof(Derived) * getMaxChildren());

            typename Geo::Region subranges[getMaxChildren()];
            this->range.split(SPLIT_FACTOR, subranges);

            for (int i = 0; i < getMaxChildren(); i++) {
                auto childIdx = this->getChildIdx(i);
                new (this->subtrees + i) Derived (subranges[i], (Derived*) this, childIdx, this->level + 1);
            }
        }

        for (int i = 0; i < getMaxChildren(); i++) {
            this->subtrees[i].split(splitLevel - 1);
        }
    }

    Derived *getRoot() {
        auto *node = (Derived*) this;
        while (node->parent) node = node->parent;
        return node;
    }

    Derived *getChild(const Geo::Point &p) {
        auto childIdx = this->range.splitIdx(SPLIT_FACTOR, p);
        auto *child = &this->subtrees[childIdx];
        return child;
    }

    u32 getSiblingIdx() const {
        assert(this->parent)
        return (Derived *) this - this->parent->subtrees;
    }

    Derived *getNextSibling() {
        if (!this->parent) return nullptr;

        auto siblingIdx = this->getSiblingIdx();
        if (siblingIdx < getMaxChildren() - 1) {
            return &this->parent->subtrees[siblingIdx + 1];
        }

        return nullptr;
    }
};
