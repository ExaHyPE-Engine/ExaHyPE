#pragma once

#include <grid/linked_grid_iterator.h>

template<typename Spacetree, u32 SPLIT = 4>
class SpacetreePruner {
    Spacetree *spacetree = nullptr;

    static u64 countChildrenItems(Spacetree *globalLevelBottomUpNode, Spacetree **leafWithMostItems) {
        auto *leafWithMostItemsCandidate = (Spacetree *) nullptr;
        auto mostItemsCount = 0;
        auto totalItemsCount = 0;

        for (u32 childIdx = 0; childIdx < Spacetree::getMaxChildren(); childIdx++) {
            auto *leafCandidate = &globalLevelBottomUpNode->subtrees[childIdx];
            if (leafCandidate->subtrees) {
                return Spacetree::CAPACITY + 1;
            }
            auto leafItemsCount = leafCandidate->items.size();
            totalItemsCount += leafItemsCount;
            if ((leafItemsCount > mostItemsCount) | !leafWithMostItemsCandidate) {
                leafWithMostItemsCandidate = leafCandidate;
                mostItemsCount = leafItemsCount;
            }
        }

        *leafWithMostItems = leafWithMostItemsCandidate;
        return totalItemsCount;
    }

    static void mergeChildren(Spacetree *globalLevelBottomUpNode, Spacetree *leafWithMostItems, u64 totalItemsCount) {
        leafWithMostItems->items.reserve(totalItemsCount);
        globalLevelBottomUpNode->items = std::move(leafWithMostItems->items);

        for (u32 childIdx = 0; childIdx < Spacetree::getMaxChildren(); childIdx++) {
            auto *child = &globalLevelBottomUpNode->subtrees[childIdx];
            globalLevelBottomUpNode->items.insert(globalLevelBottomUpNode->items.end(), child->items.begin(), child->items.end());
        }
        globalLevelBottomUpNode->destroySubtrees();
    }

public:
    explicit SpacetreePruner(Spacetree *spacetree) : spacetree(spacetree) {}

    void run(u32 numThreads = 0) {
        auto innerGlobalLevelIter0 = LeafOrLevelIterator<Spacetree, SPLIT>(this->spacetree);

        this->spacetree->onNode([](auto *node) {
            auto globalLevelBottomUpIter = BottomUpIterator(node);

            auto *globalLevelBottomUpNode = (Spacetree*) nullptr;

            while ((globalLevelBottomUpNode = globalLevelBottomUpIter.next().value_or(nullptr))) {
                if (!globalLevelBottomUpNode->subtrees) continue;

                auto *leafWithMostItems = (Spacetree*) nullptr;
                auto totalItemsCount = (u64) SpacetreePruner::countChildrenItems(globalLevelBottomUpNode, &leafWithMostItems);

                if (totalItemsCount > Spacetree::CAPACITY) continue;

                SpacetreePruner::mergeChildren(globalLevelBottomUpNode, leafWithMostItems, totalItemsCount);
            }
        }, innerGlobalLevelIter0, numThreads);

        auto iter = BottomUpLeafOrLevelIterator<Spacetree, SPLIT>(this->spacetree);
        auto *globalNode = (Spacetree*) nullptr;

        while ((globalNode = iter.next().value_or(nullptr))) {
            if (!globalNode->subtrees) [[unlikely]] continue;

            bool allChildrenAreLeafs = true;
            for (u32 childIdx = 0; childIdx < Spacetree::getMaxChildren(); childIdx++) {
                allChildrenAreLeafs &= globalNode->subtrees[childIdx].subtrees == nullptr;
            }

            if (!allChildrenAreLeafs) continue;

            auto *leafWithMostItems = (Spacetree*) nullptr;
            auto totalItemsCount = (u64) SpacetreePruner::countChildrenItems(globalNode, &leafWithMostItems);

            if (totalItemsCount > Spacetree::CAPACITY) continue;

            SpacetreePruner::mergeChildren(globalNode, leafWithMostItems, totalItemsCount);
        }
    }
};
