//
// Included by peano4/utils/Loop.h. Do not use directly
//
#include <ranges>
#include <execution>
#include <ranges>
#include <algorithm>



#define parallelForWithDynamicScheduling(counter, max, loopParallelism) \
  { auto counter##Range = std::views::iota(0, max); \
    std::for_each( std::execution::par_unseq, counter##Range, [&]( const int counter ) { \


/**
 * - the first curly bracket closes the functor
 * - the second round bracket closes the tbb::parallel_for call
 */
#define endParallelFor \
    })); \
  }



#define parallelDforWithDynamicScheduling2d(counter, max, loopParallelism) \
{ \
  tarch::la::Vector<2,int> counter##Max(max); \
  auto counter##Range0 = std::views::iota(0, counter##Max(0)); \
  auto counter##Range1 = std::views::iota(0, counter##Max(1)); \
  std::for_each( std::execution::par, std::ranges::views::cartesian_product( counter##Range0, counter##Range1 ), [&]( auto counter##Native ) { \
    tarch::la::Vector<2, int> counter = { counter##Native[0], counter##Native[1] };


#define parallelDforWithDynamicScheduling3d(counter, max, loopParallelism) \
{ \
  tarch::la::Vector<3,int> counter##Max(max); \
  auto counter##Range0 = std::views::iota(0, counter##Max(0)); \
  auto counter##Range1 = std::views::iota(0, counter##Max(1)); \
  auto counter##Range2 = std::views::iota(0, counter##Max(2)); \
  std::for_each( std::execution::par, std::ranges::views::cartesian_product( counter##Range0, counter##Range1, counter##Range2 ), [&]( auto counter##Native ) { \
    tarch::la::Vector<3, int> counter = { counter##Native[0], counter##Native[1], counter##Native[2] };


#define parallelDforWithDynamicScheduling4d(counter, max, loopParallelism) \
{ \
  tarch::la::Vector<4,int> counter##Max(max); \
  auto counter##Range0 = std::views::iota(0, counter##Max(0)); \
  auto counter##Range1 = std::views::iota(0, counter##Max(1)); \
  auto counter##Range2 = std::views::iota(0, counter##Max(2)); \
  auto counter##Range3 = std::views::iota(0, counter##Max(3)); \
  std::for_each( std::execution::par, std::ranges::views::cartesian_product( counter##Range0, counter##Range1, counter##Range2, counter##Range3 ), [&]( auto counter##Native ) { \
    tarch::la::Vector<4, int> counter = { counter##Native[0], counter##Native[1], counter##Native[2], counter##Native[3] };


#define parallelDforWithDynamicScheduling5d(counter, max, loopParallelism) \
{ \
  tarch::la::Vector<5,int> counter##Max(max); \
  auto counter##Range0 = std::views::iota(0, counter##Max(0)); \
  auto counter##Range1 = std::views::iota(0, counter##Max(1)); \
  auto counter##Range2 = std::views::iota(0, counter##Max(2)); \
  auto counter##Range3 = std::views::iota(0, counter##Max(3)); \
  auto counter##Range4 = std::views::iota(0, counter##Max(4)); \
  std::for_each( std::execution::par, std::ranges::views::cartesian_product( counter##Range0, counter##Range1, counter##Range2, counter##Range3, counter##Range4 ), [&]( auto counter##Native ) { \
    tarch::la::Vector<5, int> counter = { counter##Native[0], counter##Native[1], counter##Native[2], counter##Native[3], counter##Native[4] };




/**
 * - Close functor body
 * - Close for each and add semicolon
 * - Close helper scope for ranges
 */
#define endParallelDfor \
      }); \
  }


#define simtDforWithDynamicScheduling2d(counter, max, loopParallelism) \
{ \
  tarch::la::Vector<2,int> counter##Max(max); \
  auto counter##Range0 = std::views::iota(0, counter##Max(0)); \
  auto counter##Range1 = std::views::iota(0, counter##Max(1)); \
  std::for_each( std::execution::par_unseq, std::ranges::views::cartesian_product( counter##Range0, counter##Range1 ), [&]( auto counter##Native ) { \
    tarch::la::Vector<2, int> counter = { counter##Native[0], counter##Native[1] };


#define simtDforWithDynamicScheduling3d(counter, max, loopParallelism) \
{ \
  tarch::la::Vector<3,int> counter##Max(max); \
  auto counter##Range0 = std::views::iota(0, counter##Max(0)); \
  auto counter##Range1 = std::views::iota(0, counter##Max(1)); \
  auto counter##Range2 = std::views::iota(0, counter##Max(2)); \
  std::for_each( std::execution::par_unseq, std::ranges::views::cartesian_product( counter##Range0, counter##Range1, counter##Range2 ), [&]( auto counter##Native ) { \
    tarch::la::Vector<3, int> counter = { counter##Native[0], counter##Native[1], counter##Native[2] };


#define simtDforWithDynamicScheduling4d(counter, max, loopParallelism) \
{ \
  tarch::la::Vector<4,int> counter##Max(max); \
  auto counter##Range0 = std::views::iota(0, counter##Max(0)); \
  auto counter##Range1 = std::views::iota(0, counter##Max(1)); \
  auto counter##Range2 = std::views::iota(0, counter##Max(2)); \
  auto counter##Range3 = std::views::iota(0, counter##Max(3)); \
  std::for_each( std::execution::par_unseq, std::ranges::views::cartesian_product( counter##Range0, counter##Range1, counter##Range2, counter##Range3 ), [&]( auto counter##Native ) { \
    tarch::la::Vector<4, int> counter = { counter##Native[0], counter##Native[1], counter##Native[2], counter##Native[3] };


#define simtDforWithDynamicScheduling5d(counter, max, loopParallelism) \
{ \
  tarch::la::Vector<5,int> counter##Max(max); \
  auto counter##Range0 = std::views::iota(0, counter##Max(0)); \
  auto counter##Range1 = std::views::iota(0, counter##Max(1)); \
  auto counter##Range2 = std::views::iota(0, counter##Max(2)); \
  auto counter##Range3 = std::views::iota(0, counter##Max(3)); \
  auto counter##Range4 = std::views::iota(0, counter##Max(4)); \
  std::for_each( std::execution::par_unseq, std::ranges::views::cartesian_product( counter##Range0, counter##Range1, counter##Range2, counter##Range3, counter##Range4 ), [&]( auto counter##Native ) { \
    tarch::la::Vector<5, int> counter = { counter##Native[0], counter##Native[1], counter##Native[2], counter##Native[3], counter##Native[4] };


#define endSimtDfor \
  endParallelDfor

