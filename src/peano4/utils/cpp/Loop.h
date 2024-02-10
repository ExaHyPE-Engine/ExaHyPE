// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org

// holds for_each
#include <algorithm>

// holds execution policies
#include <execution>

// holds cartesian product
#include <ranges>

// holds iota
#include <numeric>


namespace {
  inline constexpr auto translateIntoCPPExecutionPolicyForParallelLoop( peano4::utils::LoopPlacement placement ) {
    return placement==peano4::utils::LoopPlacement::Serial ? std::execution::seq : std::execution::par;
  }

  inline constexpr auto translateIntoCPPExecutionPolicyForSimtLoop( peano4::utils::LoopPlacement placement ) {
    if (placement==peano4::utils::LoopPlacement::Serial)
      return std::execution::seq;
    else if (placement==peano4::utils::LoopPlacement::Nested)
      return std::execution::unseq;
    else
      return std::execution::par_unseq;
  }
}


#define parallelForWithSchedulerInstructions(counter, max, loopParallelism) \
  { \
    auto counter##Range = std::iota(0, max); \
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
    auto                      counter##Range0 = std::iota(0, counter##Max(0)); \
    auto                      counter##Range1 = std::iota(0, counter##Max(1)); \
  std::for_each( std::execution::par, std::ranges::cartesian_product( counter##Range0, counter##Range1 ), [&]( auto counter##Native ) { \
    tarch::la::Vector<2, int> counter = { counter##Native[0], counter##Native[1] };


#define parallelDforWithSchedulerInstructions3d(counter, max, loopParallelism) \
  { \
    tarch::la::Vector<3, int> counter##Max(max); \
    auto                      counter##Range0 = std::iota(0, counter##Max(0)); \
    auto                      counter##Range1 = std::iota(0, counter##Max(1)); \
    auto                      counter##Range2 = std::iota(0, counter##Max(2)); \
  std::for_each( std::execution::par, std::ranges::cartesian_product( counter##Range0, counter##Range1, counter##Range2 ), [&]( auto counter##Native ) { \
    tarch::la::Vector<3, int> counter = { counter##Native[0], counter##Native[1], counter##Native[2] };


#define parallelDforWithSchedulerInstructions4d(counter, max, loopParallelism) \
  { \
    tarch::la::Vector<4, int> counter##Max(max); \
    auto                      counter##Range0 = std::iota(0, counter##Max(0)); \
    auto                      counter##Range1 = std::iota(0, counter##Max(1)); \
    auto                      counter##Range2 = std::iota(0, counter##Max(2)); \
    auto                      counter##Range3 = std::iota(0, counter##Max(3)); \
    std::for_each( std::execution::par, std::ranges::cartesian_product( counter##Range0, counter##Range1, counter##Range2, counter##Range3 ), [&]( auto counter##Native ) { \
      tarch::la::Vector<4, int> counter = { counter##Native[0], counter##Native[1], counter##Native[2], counter##Native[3] };


#define parallelDforWithSchedulerInstructions5d(counter, max, loopParallelism) \
  { \
    tarch::la::Vector<5, int> counter##Max(max); \
    auto                      counter##Range0 = std::iota(0, counter##Max(0)); \
    auto                      counter##Range1 = std::iota(0, counter##Max(1)); \
    auto                      counter##Range2 = std::iota(0, counter##Max(2)); \
    auto                      counter##Range3 = std::iota(0, counter##Max(3)); \
    auto                      counter##Range4 = std::iota(0, counter##Max(4)); \
  std::for_each( std::execution::par, std::ranges::cartesian_product( counter##Range0, counter##Range1, counter##Range2, counter##Range3, counter##Range4 ), [&]( auto counter##Native ) { \
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
    std::for_each( std::execution::par_unseq, std::iota(0, max), [&]( const int counter ) {


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
    auto                      counter##Range0 = std::iota(0, counter##Max(0)); \
    auto                      counter##Range1 = std::iota(0, counter##Max(1)); \
    std::for_each( std::execution::par_unseq, std::ranges::cartesian_product( counter##Range0, counter##Range1 ), [&]( auto counter##Native ) { \
      tarch::la::Vector<2, int> counter = { counter##Native[0], counter##Native[1] };


#define simtDforWithSchedulerInstructions3d(counter, max, loopParallelism) \
  { \
    tarch::la::Vector<3, int> counter##Max(max); \
    auto                      counter##Range0 = std::iota(0, counter##Max(0)); \
    auto                      counter##Range1 = std::iota(0, counter##Max(1)); \
    auto                      counter##Range2 = std::iota(0, counter##Max(2)); \
  std::for_each( std::execution::par_unseq, std::ranges::cartesian_product( counter##Range0, counter##Range1, counter##Range2 ), [&]( auto counter##Native ) { \
    tarch::la::Vector<3, int> counter = { counter##Native[0], counter##Native[1], counter##Native[2] };


#define simtDforWithSchedulerInstructions4d(counter, max, loopParallelism) \
  { \
    tarch::la::Vector<4, int> counter##Max(max); \
    auto                      counter##Range0 = std::iota(0, counter##Max(0)); \
    auto                      counter##Range1 = std::iota(0, counter##Max(1)); \
    auto                      counter##Range2 = std::iota(0, counter##Max(2)); \
    auto                      counter##Range3 = std::iota(0, counter##Max(3)); \
  std::for_each( std::execution::par_unseq, std::ranges::cartesian_product( counter##Range0, counter##Range1, counter##Range2, counter##Range3 ), [&]( auto counter##Native ) { \
    tarch::la::Vector<4, int> counter = { counter##Native[0], counter##Native[1], counter##Native[2], counter##Native[3] };


#define simtDforWithSchedulerInstructions5d(counter, max, loopParallelism) \
  { \
    tarch::la::Vector<5, int> counter##Max(max); \
    auto                      counter##Range0 = std::iota(0, counter##Max(0)); \
    auto                      counter##Range1 = std::iota(0, counter##Max(1)); \
    auto                      counter##Range2 = std::iota(0, counter##Max(2)); \
    auto                      counter##Range3 = std::iota(0, counter##Max(3)); \
    auto                      counter##Range4 = std::iota(0, counter##Max(4)); \
  std::for_each( std::execution::par_unseq, std::ranges::cartesian_product( counter##Range0, counter##Range1, counter##Range2, counter##Range3, counter##Range4 ), [&]( auto counter##Native ) { \
    tarch::la::Vector<5, int> counter = { counter##Native[0], counter##Native[1], counter##Native[2], counter##Native[3], counter##Native[4] };


/**
 * - Close functor body
 * - Close for each and add semicolon
 * - Close helper scope for ranges
 */
#define endSimtDfor \
  }); \
  }
