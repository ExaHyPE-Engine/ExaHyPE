#pragma once

#include <algorithm>
#include <cmath>
#include <functional>

#include <lang/assert.h>
#include <lang/type.h>

template<typename Tree, auto InitialConditionsCallback>
class InitialConditionsTask {

    using fp = Tree::Geo::P;
    static constexpr auto N = Tree::Geo::D::N;
    
    Tree::Geo::Region globalIcArea;

    Tree::Geo::Point insertionLow;
    Tree::Geo::Point insertionHigh;

    u32 npart;
    
    NumVec<fp, N> getItemIdxAfterPoint(const Tree::Geo::Point &point) const {

        NumVec<fp, N> itemLowIdx;

        for (u32 axis = 0; axis < N; axis++) {
            auto a_minus_A = point[axis] - this->insertionLow[axis];
            auto B_minus_A = this->insertionHigh[axis] - this->insertionLow[axis];
            auto n = (i64) this->npart - 1;

            auto ia = n * (a_minus_A / B_minus_A);
            auto i = std::ceil(ia);
            auto iClamped = std::clamp(i, (fp) 0, (fp) this->npart);
            itemLowIdx[axis] = iClamped;
        }

        return itemLowIdx;
    }

public:
    void operator()(Tree *tree) {
        const auto leafWorkArea = this->globalIcArea.intersection(tree->range);
        if (leafWorkArea.size() == 0) return;

        assert(leafWorkArea.size() <= tree->range.size())

        auto itemIdxLow = this->getItemIdxAfterPoint(leafWorkArea.low);
        auto itemIdxHigh = this->getItemIdxAfterPoint(leafWorkArea.high);

        auto axisWiseItemCounts = itemIdxHigh - itemIdxLow;
        decltype(axisWiseItemCounts) scales;

        u32 itemCount = axisWiseItemCounts.prod();

        auto *lastInsertionLeaf = tree;

        for (u32 itemLinearIdx = 0; itemLinearIdx < itemCount; itemLinearIdx++) {
            auto itemIdx = itemIdxLow;

            u32 scale = 1;
            for (u32 axis = 0; axis < N; axis++) {
                itemIdx[axis] += (i64) (itemLinearIdx / scale) % (i64) axisWiseItemCounts[axis];
                scale *= axisWiseItemCounts[axis];
            }

            auto iDivN = itemIdx.template castAs<fp>() / (this->npart - 1);
            auto pos = this->insertionLow + (this->insertionHigh - this->insertionLow) * iDivN;

            auto item = InitialConditionsCallback(pos);
            lastInsertionLeaf = lastInsertionLeaf->insert(item);

            assert(lastInsertionLeaf != nullptr)
        }
    }

    InitialConditionsTask(Tree::Geo::Region area, u32 npart) : globalIcArea(area), npart(npart) {
        auto areaInclusive = area;

        for (i32 axis = 0; axis < Tree::Geo::D::N; axis++) {
            areaInclusive.high[axis] = std::nexttoward(areaInclusive.high[axis], areaInclusive.low[axis]);
        }

        this->insertionLow = areaInclusive.low;
        this->insertionHigh = areaInclusive.high;
    }
};
