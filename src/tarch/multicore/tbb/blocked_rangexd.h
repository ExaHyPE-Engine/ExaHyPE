// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org. While it is
// a genuine Peano file, it is strongly inspired by Intel's oneAPI
// implementation. Therefore, also their Apache License, Version 2.0, holds.
#pragma once

#include <cstddef>

//#include <detail/_config.h"
//#include <detail/_namespace_injection.h"
//#include <>detail/_range_common.h"

#include <oneapi/tbb/blocked_range.h>

namespace tbb {
namespace detail {
namespace d1 {

template<typename Value, int RangeDimensions>
    __TBB_requires(blocked_range_value<Value>)
class blocked_rangexd {
public:
    //! Type for size of an iteration range
    using range_type = blocked_range<Value>;

private:
    range_type range[RangeDimensions];

public:
    /**
     * This constructor is slightly different to TBB's block_range2d and
     * block_range3d, as it uses arrays to indicate start and end indices.
     */
    blocked_rangexd( Value begin[RangeDimensions], Value end[RangeDimensions], typename range_type::size_type grainsize[RangeDimensions] )
/*
    :
        my_rows(row_begin,row_end,row_grainsize),
        my_cols(col_begin,col_end,col_grainsize)
*/
    {
      for (int d=0; d<RangeDimensions; d++) {
        range[d] = range_type( begin[d], end[d], grainsize[d] );
      }
    }

    /**
     * Variant of constructor which doesn't exist for TBB's block_range2d and
     * block_range3d, but is very useful: It uses the same grain size in all
     * directions.
     */
    blocked_rangexd( Value begin[RangeDimensions], Value end[RangeDimensions], typename range_type::size_type grainsize )
/*
    :
        my_rows(row_begin,row_end,row_grainsize),
        my_cols(col_begin,col_end,col_grainsize)
*/
    {
      for (int d=0; d<RangeDimensions; d++) {
        range[d] = range_type( begin[d], end[d], grainsize );
      }
    }

    blocked_rangexd( Value begin[RangeDimensions], Value end[RangeDimensions] )
/*
    :
        my_rows(row_begin,row_end),
        my_cols(col_begin,col_end)
    {}
*/
    {
      for (int d=0; d<RangeDimensions; d++) {
        range[d] = range_type( begin[d], end[d] );
      }
    }

    //! True if range is empty
    bool empty() const {
        // Range is empty if at least one dimension is empty.
      bool result = false;
      for (int d=0; d<RangeDimensions; d++) {
        result |= range[d].empty();
      }
      return result;
    }

    //! True if range is divisible into two pieces.
    bool is_divisible() const {
      bool result = false;
      for (int d=0; d<RangeDimensions; d++) {
        result |= range[d].is_divisible();
      }
      return result;
    }

    blocked_rangexd( blocked_rangexd& r, split ) {
      for (int d=0; d<RangeDimensions; d++) {
        range[d] = r.range[d];
      }
      split split_obj;
      do_split(r, split_obj);
    }

    blocked_rangexd( blocked_rangexd& r, proportional_split& proportion ) {
      for (int d=0; d<RangeDimensions; d++) {
        range[d] = r.range[d];
      }
      do_split(r, proportion);
    }

    const range_type& subrange(int i) const { return range[i]; }

private:
    template <typename Split>
    void do_split( blocked_rangexd& r, Split& split_obj ) {
      int    biggestRelativeRange = -1;
      double biggestRelativeSize  = -1.0;
      for (int d=0; d<RangeDimensions; d++) {
        double currentRelativeSize = range[d].size()*double(range[d].grainsize());
        if ( currentRelativeSize>biggestRelativeSize ) {
          biggestRelativeSize  = currentRelativeSize;
          biggestRelativeRange = d;
        }
      }
      range[biggestRelativeRange].my_begin = range_type::do_split(r.range[biggestRelativeRange], split_obj);
    }
};

} // namespace d1
} // namespace detail

inline namespace v1 {
using detail::d1::blocked_rangexd;
} // namespace v1
} // namespace tbb

