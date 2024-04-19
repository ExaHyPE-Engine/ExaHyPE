#pragma once

#include <optional>

#include <lang/assert.h>
#include <lang/spinlock.h>
#include <lang/type.h>

enum Direction {
    Enter, Exit,
};

enum EventEmitFlag {
    EmitEnter = 1 << 0,
    EmitExit = 1 << 1,
};

template<typename Spacetree>
struct Item {
    Direction direction;
    Spacetree *cell;
};

template<typename TGrid>
struct IterationOrder {
    using Grid = TGrid;

    Grid *root = nullptr;

    Grid *getFirst(Grid *parent) = delete;

    Grid *getNext(Grid *sibling) = delete;

    explicit IterationOrder(Grid *top) : root(top) {}
};

template<typename TGrid>
struct NaturalOrder : public IterationOrder<TGrid> {
    using Base = IterationOrder<TGrid>;
    using Grid = Base::Grid;

    Grid *getFirst(Grid *parent) {
        return &parent->subtrees[0];
    }

    Grid *getNext(Grid *node) {
        return node->getNextSibling();
    }

    explicit NaturalOrder(Grid *top) : Base(top) {}
};

template<typename TGrid, typename Derived, typename TIterationOrder = NaturalOrder<TGrid>>
class DefaultIteratorCRTP {
protected:
    TIterationOrder iterationOrder;

    TGrid *cell = nullptr;

    Direction state = Direction::Enter;

    bool hasEmittedLast = false;

    constexpr EventEmitFlag emitFlags() {
        return EventEmitFlag(EventEmitFlag::EmitEnter | EventEmitFlag::EmitExit);
    }

    bool enter(TGrid *) {
        return true;
    }

    bool exit(TGrid *) {
        return true;
    }

    bool hasNext() {
        auto *SELF = (Derived *) this;
        
        if (SELF->hasEmittedLast) return false;

        return true;
    }

public:
    using Grid = TGrid;
    using Item = Item<TGrid>;

    DefaultIteratorCRTP() = default;

    explicit DefaultIteratorCRTP(TIterationOrder order) : iterationOrder(order), cell(order.root) {}

    explicit DefaultIteratorCRTP(Grid *top) : iterationOrder(TIterationOrder(top)), cell(top) {}

    std::optional<Item> next() {
        auto *SELF = (Derived *) this;

        if (!SELF->hasNext()) return std::nullopt;

        RESTART:
        if (SELF->state == Direction::Enter) {
            auto event = Item(Direction::Enter, SELF->cell);
            auto shouldRecurse = SELF->enter(SELF->cell);
            if (shouldRecurse & (SELF->cell->subtrees != nullptr)) {
                SELF->cell = SELF->iterationOrder.getFirst(SELF->cell);
                SELF->state = Direction::Enter;
            } else SELF->state = Direction::Exit;
            if (SELF->emitFlags() & EventEmitFlag::EmitEnter) {
                return event;
            } else goto RESTART;
        } else {
            auto event = Item(Direction::Exit, SELF->cell);
            SELF->exit(SELF->cell);
            if (SELF->cell == SELF->iterationOrder.root) [[unlikely]] {
                SELF->hasEmittedLast = true;
                if (SELF->emitFlags() & EventEmitFlag::EmitExit) {
                    return event;
                } else return std::nullopt;
            }
            auto *nextSibling = SELF->iterationOrder.getNext(SELF->cell);
            if (nextSibling) {
                SELF->cell = nextSibling;
                SELF->state = Direction::Enter;
            } else {
                SELF->cell = SELF->cell->parent;
                SELF->state = Direction::Exit;
            }
            if (SELF->emitFlags() & EventEmitFlag::EmitExit) {
                return event;
            } else goto RESTART;
        }
    }
};

template<typename Spacetree, typename TIterationOrder = NaturalOrder<Spacetree>>
class DefaultIterator : public DefaultIteratorCRTP<Spacetree, DefaultIterator<Spacetree, TIterationOrder>, TIterationOrder> {
    using Base = DefaultIteratorCRTP<Spacetree, DefaultIterator<Spacetree, TIterationOrder>, TIterationOrder>;
public:
    using Base::Base;
};

template<typename Spacetree, typename Derived, typename TIterationOrder = NaturalOrder<Spacetree>>
class LeafsIteratorCRTP : public DefaultIteratorCRTP<Spacetree, Derived, TIterationOrder> {
protected:
    using Base = DefaultIteratorCRTP<Spacetree, Derived, TIterationOrder>;
    friend Base;

    constexpr EventEmitFlag emitFlags() {
        return EventEmitFlag::EmitEnter;
    }

    bool enter(Spacetree *spacetree) {
        return true;
    }

    bool exit(Spacetree *spacetree) {
        return true;
    }

public:
    using Grid = Base::Grid;
    using Item = Spacetree*;

    using Base::Base;

    std::optional<Item> next() {
        while (true) {
            auto next = Base::next();
            if (!next) [[unlikely]] return std::nullopt;
            if (next->cell->subtrees) continue;
            return next->cell;
        }
    }
};

template<typename Spacetree, typename TIterationOrder = NaturalOrder<Spacetree>>
class LeafsIterator : public LeafsIteratorCRTP<Spacetree, LeafsIterator<Spacetree, TIterationOrder>, TIterationOrder> {
    using Base = LeafsIteratorCRTP<Spacetree, LeafsIterator<Spacetree, TIterationOrder>, TIterationOrder>;
public:
    using Base::Base;
};

template<typename Spacetree, typename TIterationOrder = NaturalOrder<Spacetree>>
LeafsIterator(Spacetree*) -> LeafsIterator<Spacetree, TIterationOrder>;

template<typename Spacetree, typename Derived, typename TIterationOrder = NaturalOrder<Spacetree>>
class BoundaryLeafsIteratorCRTP : public LeafsIteratorCRTP<Spacetree, Derived, TIterationOrder> {
protected:
    using Base = LeafsIteratorCRTP<Spacetree, Derived, TIterationOrder>;
    friend Base;
    friend Base::Base;

    Spacetree::Geo::Region boundary;

    bool enter(Spacetree *spacetree) {
        auto *SELF = (Derived*) this;
        if (!SELF->boundary.sharesBoundary(spacetree->range)) return false;
        return Base::enter(spacetree);
    }

public:
    using Grid = Base::Grid;
    using Item = Spacetree*;

    explicit BoundaryLeafsIteratorCRTP(TIterationOrder order) : BoundaryLeafsIteratorCRTP(order, order.root->range) {}

    BoundaryLeafsIteratorCRTP(TIterationOrder order, Spacetree::Geo::Region boundary) : Base(order), boundary(boundary) {}

    explicit BoundaryLeafsIteratorCRTP(Spacetree *top) : BoundaryLeafsIteratorCRTP(top, top->range) {}

    BoundaryLeafsIteratorCRTP(Spacetree *top, Spacetree::Geo::Region boundary) : Base(TIterationOrder(top)), boundary(boundary) {}

    std::optional<Item> next() {
        auto *SELF = (Derived*) this;

        while (true) {
            auto next = Base::next();
            if (!next) return std::nullopt;
            if (!SELF->boundary.sharesBoundary(next.value()->range)) continue;
            return next;
        }
    }
};

template<typename Spacetree, typename TIterationOrder = NaturalOrder<Spacetree>>
class BoundaryLeafsIterator : public BoundaryLeafsIteratorCRTP<Spacetree, BoundaryLeafsIterator<Spacetree, TIterationOrder>, TIterationOrder> {
    using Base = BoundaryLeafsIteratorCRTP<Spacetree, BoundaryLeafsIterator<Spacetree, TIterationOrder>, TIterationOrder>;
    using Base::Base;
};

template<typename Spacetree, typename TIterationOrder = NaturalOrder<Spacetree>>
BoundaryLeafsIterator(Spacetree*) -> BoundaryLeafsIterator<Spacetree, TIterationOrder>;

template<typename Spacetree, typename TIterationOrder = NaturalOrder<Spacetree>>
BoundaryLeafsIterator(Spacetree*, typename Spacetree::Geo::Region region) -> BoundaryLeafsIterator<Spacetree, TIterationOrder>;

template<typename Spacetree, typename Derived, typename TIterationOrder = NaturalOrder<Spacetree>>
class InsideRegionLeafsIteratorCRTP : public LeafsIteratorCRTP<Spacetree, Derived, TIterationOrder> {
protected:
    using Base = LeafsIteratorCRTP<Spacetree, Derived, TIterationOrder>;
    friend Base;
    friend Base::Base;

    Spacetree::Geo::Region region;

    bool enter(Spacetree *spacetree) {
        auto *SELF = (Derived*) this;

        if (!SELF->region.overlaps(spacetree->range)) return false;
        return Base::enter(spacetree);
    }

public:
    using Grid = Base::Grid;
    using Item = Spacetree*;

    InsideRegionLeafsIteratorCRTP(TIterationOrder order, Spacetree::Geo::Region region) : Base(order), region(region) {}

    InsideRegionLeafsIteratorCRTP(Spacetree *top, Spacetree::Geo::Region region) : Base(TIterationOrder(top)), region(region) {}

    std::optional<Item> next() {
        auto *SELF = (Derived*) this;

        while (true) {
            auto next = Base::next();
            if (!next) [[unlikely]] return std::nullopt;
            if (!SELF->region.overlaps(next.value()->range)) [[unlikely]] continue;
            return next;
        }
    }
};

template<typename Spacetree, typename TIterationOrder = NaturalOrder<Spacetree>>
class InsideRegionLeafsIterator : public InsideRegionLeafsIteratorCRTP<Spacetree, InsideRegionLeafsIterator<Spacetree, TIterationOrder>, TIterationOrder> {
    using Base = InsideRegionLeafsIteratorCRTP<Spacetree, InsideRegionLeafsIterator<Spacetree, TIterationOrder>, TIterationOrder>;
    using Base::Base;
};

template<typename Spacetree, typename TIterationOrder = NaturalOrder<Spacetree>>
InsideRegionLeafsIterator(Spacetree*, typename Spacetree::Geo::Region region) -> InsideRegionLeafsIterator<Spacetree, TIterationOrder>;

template<typename Spacetree, typename Derived, typename TIterationOrder = NaturalOrder<Spacetree>>
class OutsideRegionLeafsIteratorCRTP : public LeafsIteratorCRTP<Spacetree, Derived, TIterationOrder> {
protected:
    using Base = LeafsIteratorCRTP<Spacetree, Derived, TIterationOrder>;
    friend Base;
    friend Base::Base;

    Spacetree::Geo::Region region;

    bool enter(Spacetree *spacetree) {
        auto *SELF = (Derived*) this;

        if (SELF->region.contains(spacetree->range)) return false;
        return Base::enter(spacetree);
    }

public:
    using Grid = Base::Grid;
    using Item = Spacetree*;

    OutsideRegionLeafsIteratorCRTP(TIterationOrder order, Spacetree::Geo::Region region) : Base(order), region(region) {}

    OutsideRegionLeafsIteratorCRTP(Spacetree *top, Spacetree::Geo::Region region) : Base(TIterationOrder(top)), region(region) {}

    std::optional<Item> next() {
        auto *SELF = (Derived*) this;

        while (true) {
            auto next = Base::next();
            if (!next) return std::nullopt;
            if (SELF->region.contains(next.value()->range)) continue;
            return next;
        }
    }
};

template<typename Spacetree, typename TIterationOrder = NaturalOrder<Spacetree>>
class OutsideRegionLeafsIterator : public OutsideRegionLeafsIteratorCRTP<Spacetree, OutsideRegionLeafsIterator<Spacetree, TIterationOrder>, TIterationOrder> {
    using Base = OutsideRegionLeafsIteratorCRTP<Spacetree, OutsideRegionLeafsIterator<Spacetree, TIterationOrder>, TIterationOrder>;
    using Base::Base;
};

template<typename Spacetree, typename TIterationOrder = NaturalOrder<Spacetree>>
OutsideRegionLeafsIterator(Spacetree*, typename Spacetree::Geo::Region region) -> OutsideRegionLeafsIterator<Spacetree, TIterationOrder>;

template<typename Spacetree, int LEVEL = 4, typename TIterationOrder = NaturalOrder<Spacetree>>
class LeafOrLevelIterator : public DefaultIteratorCRTP<Spacetree, LeafOrLevelIterator<Spacetree, LEVEL, TIterationOrder>, TIterationOrder> {
    using Base = DefaultIteratorCRTP<Spacetree, LeafOrLevelIterator<Spacetree, LEVEL, TIterationOrder>, TIterationOrder>;
    friend Base;

    int level = -1;
protected:
    constexpr EventEmitFlag emitFlags() {
        return EventEmitFlag::EmitEnter;
    }

    bool enter(Spacetree *spacetree) {
        this->level++;
        if (this->level < LEVEL) {
            return true;
        }
        return false;
    }

    bool exit(Spacetree *spacetree) {
        this->level--;
        return true;
    }

public:
    using Grid = Base::Grid;
    using Item = Spacetree*;

    using Base::Base;

    std::optional<Item> next() {
        while (true) {
            auto next = Base::next();
            if (!next) [[unlikely]] return std::nullopt;
            if (this->level == LEVEL) return next->cell;
            if (next->cell->subtrees) continue;
            return next->cell;
        }
    }
};

template<typename Spacetree, int LEVEL = 4, typename TIterationOrder = NaturalOrder<Spacetree>>
LeafOrLevelIterator(Spacetree*) -> LeafOrLevelIterator<Spacetree, LEVEL, TIterationOrder>;

template<typename Spacetree, typename Derived, typename TIterationOrder = NaturalOrder<Spacetree>>
class BottomUpIteratorCRTP : public DefaultIteratorCRTP<Spacetree, Derived, TIterationOrder> {
protected:
    using Base = DefaultIteratorCRTP<Spacetree, Derived, TIterationOrder>;
    friend Base;

    constexpr EventEmitFlag emitFlags() {
        return EventEmitFlag::EmitExit;
    }

    bool enter(Spacetree *spacetree) {
        return true;
    }

    bool exit(Spacetree *spacetree) {
        return true;
    }

public:
    using Grid = Base::Grid;
    using Item = Spacetree*;

    using Base::Base;

    std::optional<Item> next() {
        while (true) {
            auto next = Base::next();
            if (!next) [[unlikely]] return std::nullopt;
            return next->cell;
        }
    }
};

template<typename Spacetree, typename TIterationOrder = NaturalOrder<Spacetree>>
class BottomUpIterator : public BottomUpIteratorCRTP<Spacetree, BottomUpIterator<Spacetree, TIterationOrder>, TIterationOrder> {
    using Base = BottomUpIteratorCRTP<Spacetree, BottomUpIterator<Spacetree, TIterationOrder>, TIterationOrder>;
public:
    using Base::Base;
};

template<typename Spacetree, typename TIterationOrder = NaturalOrder<Spacetree>>
BottomUpIterator(Spacetree*) -> BottomUpIterator<Spacetree, TIterationOrder>;

template<typename Spacetree, int LEVEL = 4, typename TIterationOrder = NaturalOrder<Spacetree>>
class BottomUpLeafOrLevelIterator : public DefaultIteratorCRTP<Spacetree, BottomUpLeafOrLevelIterator<Spacetree, LEVEL, TIterationOrder>, TIterationOrder> {
    using Base = DefaultIteratorCRTP<Spacetree, BottomUpLeafOrLevelIterator<Spacetree, LEVEL, TIterationOrder>, TIterationOrder>;
    friend Base;

    int level = -1;
protected:
    constexpr EventEmitFlag emitFlags() {
        return EventEmitFlag::EmitExit;
    }

    bool enter(Spacetree *spacetree) {
        this->level++;
        if (this->level < LEVEL) {
            return true;
        }
        return false;
    }

    bool exit(Spacetree *spacetree) {
        this->level--;
        return true;
    }

public:
    using Grid = Base::Grid;
    using Item = Spacetree*;

    using Base::Base;

    std::optional<Item> next() {
        while (true) {
            auto next = Base::next();
            if (!next) return std::nullopt;
            return next->cell;
        }
    }
};

template<typename Spacetree, int LEVEL = 4, typename TIterationOrder = NaturalOrder<Spacetree>>
BottomUpLeafOrLevelIterator(Spacetree*) -> BottomUpLeafOrLevelIterator<Spacetree, LEVEL, TIterationOrder>;

template<typename InnerIterator>
class SpinlockedIterator {
    Spinlock spinLock;
    InnerIterator *iter;

public:
    explicit SpinlockedIterator() = default;

    explicit SpinlockedIterator(InnerIterator *inner) {
        this->iter = inner;
    }

    std::optional<typename InnerIterator::Item> next() {
        this->spinLock.lock();
        auto next = this->iter->next();
        this->spinLock.unlock();
        return next;
    }
};
