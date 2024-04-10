// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
//
// Included by peano4/utils/Loop.h. Do not use directly
//

// holds for_each
#include <algorithm>

// holds execution policies
#include <execution>

// holds cartesian product
#include <ranges>

// holds iota
#include <numeric>


//using std::iota;
//using std::ranges::cartesian_product;
using std::ranges::views::iota;
using std::ranges::views::cartesian_product;


namespace {
  /**
   * As the
   */
/*
  inline constexpr auto translateIntoCPPExecutionPolicyForParallelLoop( peano4::utils::LoopPlacement placement ) {
    return placement==peano4::utils::LoopPlacement::Serial ? std::execution::seq : std::execution::par;
  }
*/
  #define translateIntoCPPExecutionPolicyForParallelLoop( placement ) \
    placement==peano4::utils::LoopPlacement::Serial ? std::execution::seq : std::execution::par

  #define translateIntoCPPExecutionPolicyForSimtLoop( placement ) \
    std::execution::seq;
/*
  inline constexpr auto translateIntoCPPExecutionPolicyForSimtLoop( peano4::utils::LoopPlacement placement ) {
    if (placement==peano4::utils::LoopPlacement::Serial)
      return std::execution::seq;
    else if (placement==peano4::utils::LoopPlacement::Nested)
      return std::execution::unseq;
    else
      return std::execution::par_unseq;
  }
*/
}


#define parallelForWithSchedulerInstructions(counter, max, loopParallelism) \
  { \
    auto counter##Range = iota(0, max); \
    std::for_each( std::execution::par_unseq, counter##Range, [&]( const int counter ) {


/**
 * - the first curly bracket closes the functor
 * - the second round bracket closes the tbb::parallel_for call
 */
#define endParallelFor \
  })); \
  }


#define parallelDforWithSchedulerInstructions2d(counter, max, loopParallelism) \
  { \
    tarch::la::Vector<2, int> counter##Max(max); \
    auto                      counter##Range0 = iota(0, counter##Max(0)); \
    auto                      counter##Range1 = iota(0, counter##Max(1)); \
  std::for_each( std::execution::par, cartesian_product( counter##Range0, counter##Range1 ), [&]( auto counter##Native ) { \
    tarch::la::Vector<2, int> counter = { counter##Native[0], counter##Native[1] };


#define parallelDforWithSchedulerInstructions3d(counter, max, loopParallelism) \
  { \
    tarch::la::Vector<3, int> counter##Max(max); \
    auto                      counter##Range0 = iota(0, counter##Max(0)); \
    auto                      counter##Range1 = iota(0, counter##Max(1)); \
    auto                      counter##Range2 = iota(0, counter##Max(2)); \
  std::for_each( std::execution::par, cartesian_product( counter##Range0, counter##Range1, counter##Range2 ), [&]( auto counter##Native ) { \
    tarch::la::Vector<3, int> counter = { counter##Native[0], counter##Native[1], counter##Native[2] };


#define parallelDforWithSchedulerInstructions4d(counter, max, loopParallelism) \
  { \
    tarch::la::Vector<4, int> counter##Max(max); \
    auto                      counter##Range0 = iota(0, counter##Max(0)); \
    auto                      counter##Range1 = iota(0, counter##Max(1)); \
    auto                      counter##Range2 = iota(0, counter##Max(2)); \
    auto                      counter##Range3 = iota(0, counter##Max(3)); \
    std::for_each( std::execution::par, cartesian_product( counter##Range0, counter##Range1, counter##Range2, counter##Range3 ), [&]( auto counter##Native ) { \
      tarch::la::Vector<4, int> counter = { counter##Native[0], counter##Native[1], counter##Native[2], counter##Native[3] };


#define parallelDforWithSchedulerInstructions5d(counter, max, loopParallelism) \
  { \
    tarch::la::Vector<5, int> counter##Max(max); \
    auto                      counter##Range0 = iota(0, counter##Max(0)); \
    auto                      counter##Range1 = iota(0, counter##Max(1)); \
    auto                      counter##Range2 = iota(0, counter##Max(2)); \
    auto                      counter##Range3 = iota(0, counter##Max(3)); \
    auto                      counter##Range4 = iota(0, counter##Max(4)); \
  std::for_each( std::execution::par, cartesian_product( counter##Range0, counter##Range1, counter##Range2, counter##Range3, counter##Range4 ), [&]( auto counter##Native ) { \
    tarch::la::Vector<5, int> counter = { counter##Native[0], counter##Native[1], counter##Native[2], counter##Native[3], counter##Native[4] };


/**
 * - Close functor body
 * - Close for each and add semicolon
 * - Close helper scope for ranges
 */
#define endParallelDfor \
  }); \
  }


#define simtForWithSchedulerInstructions(counter, max, loopParallelism) \
  { \
    std::for_each( std::execution::par_unseq, iota(0, max), [&]( const int counter ) {


/**
 * - the first curly bracket closes the functor
 * - the second round bracket closes the tbb::parallel_for call
 */
#define endSimtFor \
  }); \
  }


#define simtDforWithSchedulerInstructions2d(counter, max, loopParallelism) \
  { \
    tarch::la::Vector<2, int> counter##Max(max); \
    auto                      counter##Range0 = iota(0, counter##Max(0)); \
    auto                      counter##Range1 = iota(0, counter##Max(1)); \
    std::for_each( std::execution::par_unseq, cartesian_product( counter##Range0, counter##Range1 ), [&]( auto counter##Native ) { \
      tarch::la::Vector<2, int> counter = { counter##Native[0], counter##Native[1] };


#define simtDforWithSchedulerInstructions3d(counter, max, loopParallelism) \
  { \
    tarch::la::Vector<3, int> counter##Max(max); \
    auto                      counter##Range0 = iota(0, counter##Max(0)); \
    auto                      counter##Range1 = iota(0, counter##Max(1)); \
    auto                      counter##Range2 = iota(0, counter##Max(2)); \
  std::for_each( std::execution::par_unseq, cartesian_product( counter##Range0, counter##Range1, counter##Range2 ), [&]( auto counter##Native ) { \
    tarch::la::Vector<3, int> counter = { counter##Native[0], counter##Native[1], counter##Native[2] };


#define simtDforWithSchedulerInstructions4d(counter, max, loopParallelism) \
  { \
    tarch::la::Vector<4, int> counter##Max(max); \
    auto                      counter##Range0 = iota(0, counter##Max(0)); \
    auto                      counter##Range1 = iota(0, counter##Max(1)); \
    auto                      counter##Range2 = iota(0, counter##Max(2)); \
    auto                      counter##Range3 = iota(0, counter##Max(3)); \
  std::for_each( std::execution::par_unseq, cartesian_product( counter##Range0, counter##Range1, counter##Range2, counter##Range3 ), [&]( auto counter##Native ) { \
    tarch::la::Vector<4, int> counter = { counter##Native[0], counter##Native[1], counter##Native[2], counter##Native[3] };


#define simtDforWithSchedulerInstructions5d(counter, max, loopParallelism) \
  { \
    tarch::la::Vector<5, int> counter##Max(max); \
    auto                      counter##Range0 = iota(0, counter##Max(0)); \
    auto                      counter##Range1 = iota(0, counter##Max(1)); \
    auto                      counter##Range2 = iota(0, counter##Max(2)); \
    auto                      counter##Range3 = iota(0, counter##Max(3)); \
    auto                      counter##Range4 = iota(0, counter##Max(4)); \
  std::for_each( std::execution::par_unseq, cartesian_product( counter##Range0, counter##Range1, counter##Range2, counter##Range3, counter##Range4 ), [&]( auto counter##Native ) { \
    tarch::la::Vector<5, int> counter = { counter##Native[0], counter##Native[1], counter##Native[2], counter##Native[3], counter##Native[4] };


/**
 * - Close functor body
 * - Close for each and add semicolon
 * - Close helper scope for ranges
 */
#define endSimtDfor \
  }); \
  }
