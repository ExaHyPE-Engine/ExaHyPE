#pragma once

#include <memory>
#include <cstring>

#include <lang/assert.h>
#include <lang/lambda.h>
#include <lang/vec.h>

#include <geometry/geometry.h>
#include <grid/linked_grid.h>
#include <grid/linked_grid_iterator.h>

#include <lang/task.h>

template<typename T, typename Spacetree>
concept CtxUnawareStatelessOnNodeCallback = requires (T t, Spacetree *node) {
    { t(node) } -> std::same_as<void>;
};

template<typename T, typename Spacetree>
concept CtxAwareStatelessOnNodeCallback = requires (T t, Spacetree *node, TaskCtx &ctx) {
    { t(node, ctx) } -> std::same_as<void>;
};

template<typename T, typename Spacetree>
concept StatelessOnNodeCallback = CtxUnawareStatelessOnNodeCallback<T, Spacetree> || CtxAwareStatelessOnNodeCallback<T, Spacetree>;

template<typename T, typename Spacetree, typename Val>
concept CtxUnawareStatefulOnNodeCallback = requires (T t, Spacetree *node, Val &val) {
    { t(node, val) } -> std::same_as<void>;
};

template<typename T, typename Spacetree, typename Val>
concept CtxAwareStatefulOnNodeCallback = requires (T t, Spacetree *node, Val &val, TaskCtx &ctx) {
    { t(node, val, ctx) } -> std::same_as<void>;
};

template<typename T, typename Spacetree, typename Val>
concept StatefulOnNodeCallback = CtxUnawareStatefulOnNodeCallback<T, Spacetree, Val> || CtxAwareStatefulOnNodeCallback<T, Spacetree, Val>;

// https://stackoverflow.com/questions/37486137/how-can-i-create-a-constexpr-function-that-returns-a-type-to-be-used-in-a-templ
template<typename Spacetree, typename = void>
struct IterationOrderSelector;

template<typename Spacetree>
struct IterationOrderSelector<Spacetree> {
    using type = NaturalOrder<Spacetree>;
};

template<typename _Geometry, typename _Item, int THRESHOLD = 64, int SPLIT_FACTOR = 2, typename _TaskBackend = ThreadPoolTaskBackend>
struct Spacetree : public LinkedGrid<_Geometry, SPLIT_FACTOR, Spacetree<_Geometry, _Item, THRESHOLD, SPLIT_FACTOR, _TaskBackend>> {
    using Base = LinkedGrid<_Geometry, SPLIT_FACTOR, Spacetree>;
    
    using Geo = _Geometry;
    using Item = _Item;
    using TaskBackend = _TaskBackend;

    static constexpr int CAPACITY = THRESHOLD;

    using IterOrder = IterationOrderSelector<Spacetree>::type;

    using LeafsIterator = LeafsIterator<Spacetree, IterOrder>;
    using BoundaryLeafsIterator = BoundaryLeafsIterator<Spacetree, IterOrder>;
    using InsideRegionLeafsIterator = InsideRegionLeafsIterator<Spacetree, IterOrder>;
    using OutsideRegionLeafsIterator = OutsideRegionLeafsIterator<Spacetree, IterOrder>;

    template<int LEVEL = 4>
    using LeafOrLevelIterator = LeafOrLevelIterator<Spacetree, LEVEL, IterOrder>;

    template<int LEVEL = 4>
    using BottomUpLeafOrLevelIterator = BottomUpLeafOrLevelIterator<Spacetree, LEVEL, IterOrder>;

    Vector<Item> items;

    explicit Spacetree(Geo::Region range) : Base(range) {}

    Spacetree() = default;

    Spacetree(Geo::Region range, Spacetree *parent, Geo::RowMajorIdx idx, int level) : Base(range, parent, idx, level) {}

    Spacetree(Spacetree&& other) noexcept {
        this->operator=(std::move(other));
    }

    Spacetree& operator=(Spacetree&& other) noexcept {
        Base::operator=(std::move(other));
        this->items = std::move(other.items);

        return *this;
    }

    Spacetree* findLeaf(const Geo::Point &p) {
        auto *node = this;

        while (node->subtrees) {
            auto subtreeIdx = node->range.splitIdx(SPLIT_FACTOR, p);
            node = node->subtrees + subtreeIdx;
        }

        return node;
    }

    Spacetree* findChildNode(const Geo::Region &region) {
        auto *node = this;

        while (node->subtrees) {
            auto subtreeIdx = node->range.splitIdx(SPLIT_FACTOR, region.low);
            auto *nodeCandidate = node->subtrees + subtreeIdx;
            if (nodeCandidate->range.containsEx(region.high)) node = nodeCandidate;
            else break;
        }

        return node;
    }

    Spacetree* insert(Item item) {
        auto itemPos = item._x;

        auto *inRangeNode = this;
        while (!inRangeNode->range.contains(itemPos)) {
            inRangeNode = inRangeNode->parent;
            if (!inRangeNode) [[unlikely]] {
                return nullptr;
            }
        }

        auto *leafNode = inRangeNode->findLeaf(itemPos);

        if (leafNode->items.size() == THRESHOLD) [[unlikely]] {
            leafNode->splitIntoSubtrees();
            leafNode = leafNode->findLeaf(itemPos);
        }

        leafNode->pushItem(item);

        return leafNode;
    }

    void pushItem(Item p) {
        assert(!this->subtrees)

        this->items.push_back(p);
    }

    void splitIntoSubtrees() {
        auto *fullTree = this;

        do {
            assert(fullTree->items.size() == THRESHOLD)

            fullTree->split();

            char idxs[THRESHOLD];

            for (int i = 0; i < THRESHOLD; i++) {
                auto pos = fullTree->items[i]._x;
                auto quartIdx = fullTree->range.splitIdx(SPLIT_FACTOR, pos);
                idxs[i] = quartIdx;
            }

            for (int i = 0; i < THRESHOLD; i++) {
                auto *subtree = fullTree->subtrees + idxs[i];
                subtree->items.push_back(fullTree->items[i]);
            }
            fullTree->items = Vector<Item>();  // free underlying memory

            // check if all items went into the same subtree
            // if that's the case, we need to split again
            auto *subtrees = fullTree->subtrees;
            fullTree = nullptr;
            for (int i = 0; i < Spacetree::getMaxChildren(); i++) {
                if (subtrees[i].items.size() < THRESHOLD) continue;
                fullTree = subtrees + i;
                break;
            }
        } while (fullTree && fullTree->level < 128);
    }

    Spacetree *getRegionParent(const Geo::Region &range, bool refined = false) {
        auto *currSubtree = this;

        if (refined && this->range.contains(range)) {
            currSubtree = this->findChildNode(range);
            return currSubtree;
        }

        while (!currSubtree->range.contains(range) & (currSubtree->parent != nullptr)) {
            currSubtree = currSubtree->parent;
        }

        return currSubtree;
    }

    template<StatelessOnNodeCallback<Spacetree> Callback, typename Iter = LeafsIterator>
    void onNode(Callback F, u32 numThreads = 0) {
        auto iter = Iter(this);
        this->onNode(F, iter, numThreads);
    }

    template<StatelessOnNodeCallback<Spacetree> Callback, typename Iter>
    void onNode(Callback F, Iter iter, u32 numThreads = 0) {
        auto taskGenerator = Lambda([](auto &iter, auto &callback) {
            auto nodeOpt = iter.next();
            if constexpr (CtxAwareStatelessOnNodeCallback<Callback, Spacetree>) {
                auto task = Lambda([](auto *node, auto *callback, auto &ctx) {
                    (*callback)(node, ctx);
                }, nodeOpt.value_or(nullptr), &callback);
                return (nodeOpt) ? std::optional(task) : std::nullopt;
            } else if constexpr (CtxUnawareStatelessOnNodeCallback<Callback, Spacetree>) {
                auto task = Lambda([](auto *node, auto *callback) {
                    (*callback)(node);
                }, nodeOpt.value_or(nullptr), &callback);
                return (nodeOpt) ? std::optional(task) : std::nullopt;
            } else static_assert(false);
        }, iter, F);

        TaskBackend::runSerialGenerator(taskGenerator, numThreads);
    }

    template<typename StateInitF, StatefulOnNodeCallback<Spacetree, std::invoke_result_t<StateInitF>> Callback, typename Iter = LeafsIterator>
    auto onNode(Callback F, StateInitF stateInitF, u32 numThreads = 0) {
        auto iter = Iter(this);
        return this->onNode(F, stateInitF, iter, numThreads);
    }

    template<typename StateInitF, StatefulOnNodeCallback<Spacetree, std::invoke_result_t<StateInitF>> Callback, typename Iter>
    auto onNode(Callback F, StateInitF stateInitF, Iter iter, u32 numThreads = 0) {
        using ValueType = std::invoke_result_t<StateInitF>;

        auto taskGenerator = Lambda([](auto &iter, auto &callback) {
            auto nodeOpt = iter.next();
            if constexpr (CtxAwareStatefulOnNodeCallback<Callback, Spacetree, ValueType>) {
                auto task = Lambda([](auto *node, auto *callback, auto &acc, auto &ctx) {
                    (*callback)(node, acc, ctx);
                }, nodeOpt.value_or(nullptr), &callback);
                return (nodeOpt) ? std::optional(task) : std::nullopt;
            } else if constexpr (CtxUnawareStatefulOnNodeCallback<Callback, Spacetree, ValueType>) {
                auto task = Lambda([](auto *node, auto *callback, auto &acc) {
                    (*callback)(node, acc);
                }, nodeOpt.value_or(nullptr), &callback);
                return (nodeOpt) ? std::optional(task) : std::nullopt;
            } else static_assert(false);
        }, iter, F);

        return TaskBackend::runSerialGenerator(taskGenerator, stateInitF, numThreads);
    }

    void destroySubtrees() {
        if (!this->subtrees) return;
        for (int i = 0; i < Spacetree::getMaxChildren(); i++) this->subtrees[i].~Spacetree();
        free(this->subtrees);
        this->subtrees = nullptr;
    }

    void clear() {
        if (!this->subtrees) {
            this->items.clear();
            return;
        }

        for (int i = 0; i < Spacetree::getMaxChildren(); i++) this->subtrees[i].clear();
    }

    static constexpr u32 getMaxChildren() {
        return Geo::template PowerN<SPLIT_FACTOR>();
    }

    ~Spacetree() {
        this->destroySubtrees();
    }

};
