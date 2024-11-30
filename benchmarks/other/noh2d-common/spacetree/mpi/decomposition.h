#pragma once

#include <lang/assert.h>
#include <lang/type.h>

#include <grid/linked_grid.h>
#include <grid/linked_grid_iterator.h>
#include <grid/__include_mortonND_BMI2.h>

#include <vector>

template<typename Geo>
class DomainDecomposition {
    Geo::Region range;
    u32 localRankId;
    u32 rankCount;
    Geo::P haloThickness;

    class Grid : public LinkedGrid<Geo, 2, Grid> {
        using Base = LinkedGrid<Geo, 2, Grid>;
        using Base::Base; // inherit all constructors
    };

    using LeafsIter = LeafsIterator<Grid>;
    using InsideRegionIter = InsideRegionLeafsIterator<Grid>;

    using Morton = mortonnd::MortonNDBmi<Geo::D::N, u64>;

    Grid grid;

    u64 getLeafRankId(Grid *leaf) {
        assert(!leaf->subtrees)

        auto leafRankId = Morton::Encode((u64*) &leaf->idx);
        return leafRankId;
    }

    Geo::Region getLocalAreaForRankId(u64 rankId) {
        auto *leaf = (Grid*) nullptr;

        auto leafIter = LeafsIter(&this->grid);

        while ((leaf = leafIter.next().value_or(nullptr))) {
            if (this->getLeafRankId(leaf) != rankId) continue;

            auto localArea = leaf->range;
            return localArea;
        }

        assert(false)
    }

public:
    DomainDecomposition(Geo::Region range, u64 localRankId, u32 rankCount, Geo::P haloThickness)
        : range(range), localRankId(localRankId), rankCount(rankCount), haloThickness(haloThickness) {
        this->grid = Grid(range);

        auto maxChildren = Grid::getMaxChildren();
        auto splitLevel = 1;

        while (maxChildren < rankCount) {
            maxChildren *= Grid::getMaxChildren();
            splitLevel++;
        }
        assert(maxChildren == rankCount)

        this->grid.split(splitLevel);
    }

    Geo::Region getLocalArea() {
        auto localArea = this->getLocalAreaForRankId(this->localRankId);
        return localArea;
    }

    Geo::Region getLocalHaloArea() {
        auto localArea = this->getLocalArea();
        auto localHalo = typename Geo::Region(localArea.low - this->haloThickness, localArea.high + this->haloThickness);
        localHalo = this->range.intersection(localHalo);
        return localHalo;
    }

    std::vector<u64> getNeighbourRankIds() {
        auto neighbourRankIds = std::vector<u64>();

        auto localHalo = this->getLocalHaloArea();

        auto haloIter = InsideRegionIter(&this->grid, localHalo);
        auto *neighbour = (Grid*) nullptr;

        while ((neighbour = haloIter.next().value_or(nullptr))) {
            auto neighbourRankId = this->getLeafRankId(neighbour);
            if (neighbourRankId == this->localRankId) continue;
            neighbourRankIds.push_back(neighbourRankId);
        }

        return neighbourRankIds;
    }

    Geo::Region getLocalHaloForNeighbour(u64 neighbourRankId) {
        assert(this->localRankId != neighbourRankId)

        auto neighbourLocalArea = this->getLocalAreaForRankId(neighbourRankId);
        auto neighbourHaloRegion = typename Geo::Region(neighbourLocalArea.low - this->haloThickness, neighbourLocalArea.high + this->haloThickness);

        auto intersection = this->getLocalArea().intersection(neighbourHaloRegion);
        return intersection;
    }

    Geo::Region getRemoteHaloForNeighbour(u64 neighbourRankId) {
        assert(this->localRankId != neighbourRankId)

        auto localHalo = this->getLocalHaloArea();

        auto intersection = this->getLocalAreaForRankId(neighbourRankId).intersection(localHalo);
        return intersection;
    }

};
