#pragma once

#include <concepts>

#include <lang/assert.h>
#include <lang/type.h>
#include <lang/numvec.h>
#include <lang/math.h>

template<Dimensionality dim, Precision fp>
class Point : public NumVec<fp, dim::N> {
public:
    using D = dim;
    using P = fp;

    Point() = default;

    explicit Point(fp val) : NumVec<fp, dim::N>(val) {}

    Point(std::initializer_list<fp> list) : NumVec<fp, dim::N>(list) {}

    explicit Point(const fp(&list)[dim::N]) : NumVec<fp, dim::N>(list) {}

    Point(const Point &other) : NumVec<fp, dim::N>(other) {}

    Point(const NumVec<fp, dim::N> &other) : NumVec<fp, dim::N>(other) {}

    fp distance(const Point &p) const {
        auto diff = p - *this;
        auto diff2 = diff * diff;
        auto dist = std::sqrt(diff2.sum());
        return dist;
    }
};

template<Dimensionality dim, Precision fp, bool topBoundaryInclusive = false>
struct Region {
private:
    bool _betweenIn(const fp min, const fp val, const fp max) const {
        return (min <= val) & (val <= max);
    }

    bool _betweenEx(const fp min, const fp val, const fp max) const {
        return (min <= val) & (val < max);
    }

    bool between(const fp min, const fp val, const fp max) const {
        if (topBoundaryInclusive) return this->_betweenIn(min, val, max);
        else return this->_betweenEx(min, val, max);
    }

public:
    using D = dim;
    using P = fp;

    using Point = Point<D, P>;

    Point low;
    Point high;

    static Region fromInclusiveBounds(const Point &low, const Point &high) {
        auto highEx = high;
        for (i32 axis = 0; axis < dim::N; axis++) {
            highEx[axis] = std::nexttoward(highEx[axis], highEx[axis] + 1);
        }
        return Region(low, highEx);
    }

    bool operator==(const Region &r) const {
        return this->low == r.low & this->high == r.high;
    }

    bool operator!=(const Region &r) const {
        return this->low != r.low | this->high != r.high;
    }

    P size() const {
        auto delta = this->high - this->low;

        P size = 1;

        for (i32 i = 0; i < dim::N; i++) {
            size *= delta[i];
        }

        return size;
    }

    bool contains(const Point &p) const {
        bool result = true;
        for (i32 i = 0; i < dim::N; i++) {
            result &= between(this->low[i], p[i], this->high[i]);
        }
        return result;
    }

    bool containsEx(const Point &p) const {
        bool result = true;
        for (i32 i = 0; i < dim::N; i++) {
            result &= _betweenIn(this->low[i], p[i], this->high[i]);
        }
        return result;
    }

    NumVec<i32, dim::N> containsAxis(const Point &p) const {
        auto result = NumVec<i32, dim::N>(0);

        for (i32 i = 0; i < dim::N; i++) {
            result[i] = -1 * !between(this->low[i], p[i], this->high[i]);
        }

        return result;
    }

    // https://silentmatt.com/rectangle-intersection/
    bool overlaps(const Region &r) const {
        bool res = true;
        for (i32 i = 0; i < dim::N; i++) {
            res &= this->low[i] < r.high[i] & this->high[i] > r.low[i];
        }
        return res;
    }

    bool contains(const Region &r) const {
        bool result = true;

        for (i32 i = 0; i < dim::N; i++) {
            auto lmin = this->low[i];
            auto lmax = this->high[i];

            auto rmin = r.low[i];
            auto rmax = r.high[i];

            if (topBoundaryInclusive) {
                result &= between(lmin, rmin, lmax) & between(lmin, rmax, lmax);
            } else {
                result &= between(lmin, rmin, lmax) & ((lmin <= rmax) & (rmax <= lmax));
            }
        }

        return result;
    }

    bool sharesBoundary(const Region &r) const {
        auto lowDelta = this->low - r.low;
        bool isLowZero = false;

        for (u32 i = 0; i < D::N; i++) isLowZero |= lowDelta[i] == 0;

        if (isLowZero) return true;

        auto highDelta = this->high - r.high;

        bool isHighZero = false;
        for (u32 i = 0; i < D::N; i++) isHighZero |= highDelta[i] == 0;

        if (isHighZero) return true;

        return false;
    }

    Region intersection(const Region &r) const {
        Point iLow, iHigh;

        for (i32 i = 0; i < dim::N; i++) {
            iLow[i] = std::max(this->low[i], r.low[i]);
            iHigh[i] = std::min(this->high[i], r.high[i]);
        }

        bool valid = true;

        for (int i = 0; i < dim::N; i++) {
            valid &= iLow[i] < iHigh[i];
        }

        if (valid) return Region{iLow, iHigh};
        else return Region{Point{}, Point{}};
    }

    Point centre() const {
        return (this->low + this->high) / 2;
    }

    void split(u32 n, Region *regions) const {
        Point inc;

        for (i32 i = 0; i < dim::N; i++) {
            inc[i] = (this->high[i] - this->low[i]) / n;
        }

        for (u32 i = 0; i < (u32) std::pow(n, dim::N); i++) {
            Point pLow = this->low;
            Point pHigh = this->low;

            for (u32 d = 0; d < dim::N; d++) {
                auto didx = (i / (u32) std::pow(n, d)) % n;
                pLow[d] += inc[d] * didx;
                pHigh[d] += inc[d] * (didx + 1);
            }

            regions[i] = Region{pLow, pHigh};
        }
    }

    u32 splitIdx(u32 n, const Point &p) const {
        if (n == 2) {
            auto center = this->low + (this->high - this->low) / 2;

            u32 idx = 0;

            for (u32 d = 0; d < dim::N; d++) {
                idx += (center[d] <= p[d]) * std::pow(n, d);
            }

            return idx;
        }

        auto pnorm = (p - this->low) / (this->high - this->low);

        auto pidx = (pnorm * n).floor();

        u32 idx = 0;
        for (u32 d = 0; d < dim::N; d++) {
            idx += pidx[d] * std::pow(n, d);
        }

        assert(idx < std::pow(n, dim::N))
        return idx;
    }

    friend std::ostream &operator<<(std::ostream &stream, const Region &value) {
        stream << "{ L:" << value.low << " H:" << value.high << " }";
        return stream;
    }
};

template<typename dim, typename fp, bool topBoundaryInclusive = false> requires Dimensionality<dim> and Precision<fp>
struct Geometry {
    using D = dim;
    using P = fp;

    using Point = Point<dim, fp>;
    using Region = Region<dim, fp, topBoundaryInclusive>;

    using RowMajorIdx = NumVec<u64, D::N>;

    using Vec = NumVec<P, D::N>;

    static constexpr u32 TwoPowerN() { return PowerN<2>(); }

    template<u32 N>
    static constexpr u32 PowerN() { return pow_constexpr(N, D::N); }
};
