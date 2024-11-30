#pragma once

#include <omp.h>

#include <grid/linked_grid_iterator.h>

template<typename Spacetree>
void EmptySortCallback(Spacetree *node) {}

template<typename Spacetree, auto PreSortCallback = EmptySortCallback<Spacetree>>
class SpacetreeSorter {
    Spacetree *spacetree = nullptr;

    struct SortWorkerState {
        Spacetree localTree;
        std::vector<typename Spacetree::Item> lostItems;
    };

public:
    explicit SpacetreeSorter(Spacetree *spacetree) : spacetree(spacetree) {}

    std::vector<typename Spacetree::Item> run(u32 numThreads = 0) {
        constexpr int SPLIT = 5;

        auto sortStates = this->spacetree->onNode([](Spacetree *node, SortWorkerState &state) {
            node->onNode([supernode = node, state = &state](Spacetree *node) {
                PreSortCallback(node);

                bool mustMove[node->items.size()];

                for (i64 itemIdx = 0; itemIdx < node->items.size(); itemIdx++) {
                    auto &item = node->items[itemIdx];
                    mustMove[itemIdx] = !node->range.contains(item._x);
                }

                auto *lastInsertionNode = supernode;
                for (i64 itemIdx = node->items.size() - 1; itemIdx >= 0; itemIdx--) {
                    if (!mustMove[itemIdx]) continue;

                    auto item = node->items.unstableTake(itemIdx);

                    if (supernode->range.contains(item._x)) {
                        lastInsertionNode = lastInsertionNode->insert(item);
                    } else {
                        auto *insertionNode = state->localTree.insert(item);
                        if (!insertionNode) [[unlikely]] {
                            state->lostItems.push_back(item);
                        }
                    }
                }
            }, 1u);

        }, [range = this->spacetree->range]() {
            auto state = SortWorkerState {
                .localTree = Spacetree(range),
                .lostItems = std::vector<typename Spacetree::Item>()
            };

            return state;
        }, LeafOrLevelIterator<Spacetree, SPLIT>(this->spacetree));

        this->spacetree->onNode([sortStates = &sortStates](Spacetree *node) {
            for (auto &sortState : *sortStates) {
                auto iter = InsideRegionLeafsIterator(&sortState.localTree, node->range);

                auto *localInRangeCell = iter.next().value_or(nullptr);

                while (localInRangeCell) {
                    for (auto &localInRangeItem: localInRangeCell->items) {
                        if (!node->range.contains(localInRangeItem._x)) continue;
                        node->insert(localInRangeItem);
                    }
                    localInRangeCell = iter.next().value_or(nullptr);
                }
            }
        }, LeafOrLevelIterator<Spacetree, SPLIT>(this->spacetree));

        auto lostItemsCount = 0;
        for (auto &sortState : sortStates) {
            lostItemsCount += sortState.lostItems.size();
        }

        std::vector<typename Spacetree::Item> lostItems;
        lostItems.reserve(lostItemsCount);

        for (auto &sortState : sortStates) {
            lostItems.insert(lostItems.end(), sortState.lostItems.begin(), sortState.lostItems.end());
        }

        return lostItems;
    }
};
