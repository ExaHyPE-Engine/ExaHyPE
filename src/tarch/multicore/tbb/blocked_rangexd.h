// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org. While it is
// a genuine Peano file, it is strongly inspired by Intel's oneAPI
// implementation. Therefore, also their Apache License, Version 2.0, holds.
#if !defined(__TBB_blocked_rangexd_H) && defined(SharedTBB)
#define __TBB_blocked_rangexd_H


#include <cstddef>
#include <vector>

//#include <detail/_config.h"
//#include <detail/_namespace_injection.h"
//#include <>detail/_range_common.h"

#include <oneapi/tbb/blocked_range.h>

namespace tbb {
  namespace detail {
    namespace d1 {
      /**
       * Declaration as I need this typename further down.
       */
      template<typename Value, int RangeDimensions>
          __TBB_requires(blocked_range_value<Value>)
      class blocked_rangexd;


      /**
       * Declaration as I need this type within blocked_rangexd
       */
      template<typename Value>
      class redefined_blocked_range_with_friend;
    }
  }
}



namespace tbb {
namespace detail {
namespace d1 {


/**
 * Hack class
 *
 * The TBB design is not particularly clean when it comes to ranges (at least
 * in my opinion): The tbb::blocked_range class is heavily used by
 * blocked_range2d and blocked_range3d. However, it has a few private fields
 * which the 2d and 3d versions have to manipulate. Therefore, blocked_range
 * makes both blocked_range2d and blocked_range3d friends. The implementation
 * disables the variable visibility checks for those two classes.
 *
 * As a consequence, we cannot use the tbb::blocked_range in our d-dimensional
 * implementation. We would have to alter the core TBB code and make our new
 * class a friend, too. As this is not convenient - we want people to work with
 * their own TBB installation, we create an "artificial" subclass of
 * tbb:blocked_range which we call overwritten_blocked_range. This artificial
 * subclass has no added value besides the fact that it declares a new friend.
 *
 * Such a hack would break down if blocked_range were a proper class: You
 * cannot inherit from a class and then allow the child to make the parent's
 * fields visible to a friend. However, the hack works with templates
 * apparently.
 *
 * There is one more unclean thing in this design: The code makes all
 * blocked_rangexd classes a friend of the blocked_range. This is not what we
 * really would like to have. We would prefer to have those guys as friend
 * which share Value. However, the current C++ compiler generation does not
 * allow us to use partial specialiation within a friend.
 */
template<typename Value>
class redefined_blocked_range_with_friend {
  public:
    /**
     * This is the only missing ingredient
     */
    template <typename OtherValue,int RangeDimensions>
    friend class blocked_rangexd;


    //! Type of a value
    /** Called a const_iterator for sake of algorithms that need to treat a blocked_range
        as an STL container. */
    using const_iterator = Value;

    //! Type for size of a range
    using size_type = std::size_t;

    //! Construct range over half-open interval [begin,end), with the given grainsize.
    redefined_blocked_range_with_friend( Value begin_, Value end_, size_type grainsize_=1 ) :
        my_end(end_), my_begin(begin_), my_grainsize(grainsize_)
    {
        __TBB_ASSERT( my_grainsize>0, "grainsize must be positive" );
    }

    //! Beginning of range.
    const_iterator begin() const { return my_begin; }

    //! One past last value in range.
    const_iterator end() const { return my_end; }

    //! Size of the range
    /** Unspecified if end()<begin(). */
    size_type size() const {
        __TBB_ASSERT( !(end()<begin()), "size() unspecified if end()<begin()" );
        return size_type(my_end-my_begin);
    }

    //! The grain size for this range.
    size_type grainsize() const { return my_grainsize; }

    //------------------------------------------------------------------------
    // Methods that implement Range concept
    //------------------------------------------------------------------------

    //! True if range is empty.
    bool empty() const { return !(my_begin<my_end); }

    //! True if range is divisible.
    /** Unspecified if end()<begin(). */
    bool is_divisible() const { return my_grainsize<size(); }

    //! Split range.
    /** The new Range *this has the second part, the old range r has the first part.
        Unspecified if end()<begin() or !is_divisible(). */
    redefined_blocked_range_with_friend( redefined_blocked_range_with_friend& r, split ) :
        my_end(r.my_end),
        my_begin(do_split(r, split())),
        my_grainsize(r.my_grainsize)
    {
        // only comparison 'less than' is required from values of blocked_range objects
        __TBB_ASSERT( !(my_begin < r.my_end) && !(r.my_end < my_begin), "blocked_range has been split incorrectly" );
    }

    //! Split range.
    /** The new Range *this has the second part split according to specified proportion, the old range r has the first part.
        Unspecified if end()<begin() or !is_divisible(). */
    redefined_blocked_range_with_friend( redefined_blocked_range_with_friend& r, proportional_split& proportion ) :
        my_end(r.my_end),
        my_begin(do_split(r, proportion)),
        my_grainsize(r.my_grainsize)
    {
        // only comparison 'less than' is required from values of blocked_range objects
        __TBB_ASSERT( !(my_begin < r.my_end) && !(r.my_end < my_begin), "blocked_range has been split incorrectly" );
    }

  private:
    Value my_end;
    Value my_begin;
    size_type my_grainsize;

    //! Auxiliary function used by the splitting constructor.
    static Value do_split( redefined_blocked_range_with_friend& r, split )
    {
        __TBB_ASSERT( r.is_divisible(), "cannot split blocked_range that is not divisible" );
        Value middle = r.my_begin + (r.my_end - r.my_begin) / 2u;
        r.my_end = middle;
        return middle;
    }

    static Value do_split( redefined_blocked_range_with_friend& r, proportional_split& proportion )
    {
        __TBB_ASSERT( r.is_divisible(), "cannot split blocked_range that is not divisible" );

        // usage of 32-bit floating point arithmetic is not enough to handle ranges of
        // more than 2^24 iterations accurately. However, even on ranges with 2^64
        // iterations the computational error approximately equals to 0.000001% which
        // makes small impact on uniform distribution of such range's iterations (assuming
        // all iterations take equal time to complete). See 'test_partitioner_whitebox'
        // for implementation of an exact split algorithm
        size_type right_part = size_type(float(r.size()) * float(proportion.right())
                                         / float(proportion.left() + proportion.right()) + 0.5f);
        return r.my_end = Value(r.my_end - right_part);
    }
};


/**
 * d-dimensional range object added to TBB
 *
 * TBB supports, at the moment, only 1d, 2d and 3d ranges. That's not enough
 * for our code. Therefore, we artificially introduce a blocked_rangexd which
 * can handle higher dimensions.
 *
 *
 * ## Signature
 *
 * We refrain from speaking of pages, columns and rows, as we want to support
 * arbitrary dimensions. Therefore, we speak of subranges.
 *
 *
 * ## Implementation
 *
 * The implementation mirrors tbb::blocked_range2d directly with a few subtle
 * differences.
 *
 * - The subranges are administered through a std::vector. We should be able to
 *   use a plain array (RangeDimensions is fixed after all), but the range
 *   object does not offer a default constructor. If we administer the array
 *   through a vector object, we can initialise the array elements through a
 *   copy constructor straightforwardly.
 */
template<typename Value, int RangeDimensions>
    __TBB_requires(blocked_range_value<Value>)
class blocked_rangexd {
public:
    //! Type for size of an iteration range
    using range_type = redefined_blocked_range_with_friend<Value>;

private:
    std::vector< range_type > my_subranges;

public:
    /**
     * This constructor is slightly different to TBB's block_range2d and
     * block_range3d, as it uses arrays to indicate start and end indices.
     */
    blocked_rangexd( Value begin[RangeDimensions], Value end[RangeDimensions], typename range_type::size_type grainsize[RangeDimensions] ):
      my_subranges(RangeDimensions, range_type(1,1,1) ) {
      for (int d=0; d<RangeDimensions; d++) {
        my_subranges[d] = range_type( begin[d], end[d], grainsize[d] );
      }
    }

    /**
     * Variant of constructor which doesn't exist for TBB's block_range2d and
     * block_range3d, but is very useful: It uses the same grain size in all
     * directions.
     */
    blocked_rangexd( Value begin[RangeDimensions], Value end[RangeDimensions], typename range_type::size_type grainsize ):
      my_subranges(RangeDimensions, range_type(1,1,1) ) {
      for (int d=0; d<RangeDimensions; d++) {
        my_subranges[d] = range_type( begin[d], end[d], grainsize );
      }
    }

    blocked_rangexd( Value begin[RangeDimensions], Value end[RangeDimensions] ):
      my_subranges(RangeDimensions, range_type(1,1,1) ) {
      for (int d=0; d<RangeDimensions; d++) {
        my_subranges[d] = range_type( begin[d], end[d] );
      }
    }

    //! True if range is empty
    bool empty() const {
        // Range is empty if at least one dimension is empty.
      bool result = false;
      for (int d=0; d<RangeDimensions; d++) {
        result |= my_subranges[d].empty();
      }
      return result;
    }

    //! True if range is divisible into two pieces.
    bool is_divisible() const {
      bool result = false;
      for (int d=0; d<RangeDimensions; d++) {
        result |= my_subranges[d].is_divisible();
      }
      return result;
    }

    blocked_rangexd( blocked_rangexd& r, split ):
      my_subranges(RangeDimensions, range_type(1,1,1) ) {
      for (int d=0; d<RangeDimensions; d++) {
        my_subranges[d] = r.my_subranges[d];
      }
      split split_obj;
      do_split(r, split_obj);
    }

    blocked_rangexd( blocked_rangexd& r, proportional_split& proportion ):
      my_subranges(RangeDimensions, range_type(1,1,1) ) {
      for (int d=0; d<RangeDimensions; d++) {
        my_subranges[d] = r.my_subranges[d];
      }
      do_split(r, proportion);
    }

    const range_type& subrange(int i) const { return my_subranges[i]; }

private:
    template <typename Split>
    void do_split( blocked_rangexd& r, Split& split_obj ) {
      int    biggestRelativeRange = -1;
      double biggestRelativeSize  = -1.0;
      for (int d=0; d<RangeDimensions; d++) {
        double currentRelativeSize = my_subranges[d].size()*double(my_subranges[d].grainsize());
        if ( currentRelativeSize>biggestRelativeSize ) {
          biggestRelativeSize  = currentRelativeSize;
          biggestRelativeRange = d;
        }
      }
      my_subranges[biggestRelativeRange].my_begin = range_type::do_split(r.my_subranges[biggestRelativeRange], split_obj);
    }
};

} // namespace d1
} // namespace detail



  /**
   * Make the type blocked_rangexd visible within tbb namespace.
   */
  inline namespace v1 {
    using detail::d1::blocked_rangexd;
  } // namespace v1
} // namespace tbb

#endif
