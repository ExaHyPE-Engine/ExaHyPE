// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
//
// Included by peano4/utils/Loop.h. Do not use directly
//
#define parallelForWithSchedulerInstructions(counter, max, loopParallelism) \
  tbb::parallel_for( \
    tbb::blocked_range<int>( \
      0, max, loopParallelism==::peano4::utils::LoopPlacement::Serial ? max : 1 \
    ), \
    [&]( const tbb::blocked_range<int>&  range ) { \
      for( int counter = range.begin();  counter < range.end();  counter++ ) \


/**
 * - the first curly bracket closes the functor
 * - the second round bracket closes the tbb::parallel_for call
 */
#define endParallelFor \
    } \
  );


#define parallelDforWithSchedulerInstructions2d(counter, max, loopParallelism) \
{ \
  tarch::la::Vector<2,int> counter##Max(max); \
  int counter##Begin[] = {0,0}; \
  int counter##End[]   = {counter##Max(0),counter##Max(1)}; \
  tbb::parallel_for( \
    tbb::blocked_rangeNd<int,2>( \
      counter##Begin, \
      counter##End, \
      loopParallelism==::peano4::utils::LoopPlacement::Serial ? counter##Max(0) : 1 \
    ), \
    [&]( const tbb::blocked_rangeNd<int,2>&  range ) { \
      for( int counter##1 = range.dim(0).begin();  counter##1 < range.dim(0).end();  counter##1++ ) \
      for( int counter##2 = range.dim(1).begin();  counter##2 < range.dim(1).end();  counter##2++ ) { \
        tarch::la::Vector<2, int> counter = { counter##1, counter##2 };


#define parallelDforWithSchedulerInstructions3d(counter, max, loopParallelism) \
  { \
  tarch::la::Vector<3,int> counter##Max(max); \
  int counter##Begin[] = {0,0,0}; \
  int counter##End[]   = {counter##Max(0),counter##Max(1),counter##Max(2)}; \
  tbb::parallel_for( \
    tbb::blocked_rangeNd<int,3>( \
      counter##Begin, \
      counter##End, \
      loopParallelism==::peano4::utils::LoopPlacement::Serial ? counter##Max(0) : 1 \
    ), \
    [&]( const tbb::blocked_rangeNd<int,3>&  range ) { \
      for( int counter##1 = range.dim(0).begin();  counter##1 < range.dim(0).end();  counter##1++ ) \
      for( int counter##2 = range.dim(1).begin();  counter##2 < range.dim(1).end();  counter##2++ ) \
      for( int counter##3 = range.dim(2).begin();  counter##3 < range.dim(2).end();  counter##3++ ) {\
        tarch::la::Vector<3, int> counter = { counter##1, counter##2, counter##3 };


#define parallelDforWithSchedulerInstructions4d(counter, max, loopParallelism) \
  { \
  tarch::la::Vector<4,int> counter##Max(max); \
  int counter##Begin[] = {0,0,0,0}; \
  int counter##End[]   = {counter##Max(0),counter##Max(1),counter##Max(2),counter##Max(3)}; \
  tbb::parallel_for( \
    tbb::blocked_rangeNd<int,4>( \
      counter##Begin, \
      counter##End, \
      loopParallelism==::peano4::utils::LoopPlacement::Serial ? counter##Max(0) : 1 \
    ), \
    [&]( const tbb::blocked_rangeNd<int,4>&  range ) { \
      for( int counter##1 = range.dim(0).begin();  counter##1 < range.dim(0).end();  counter##1++ ) \
      for( int counter##2 = range.dim(1).begin();  counter##2 < range.dim(1).end();  counter##2++ ) \
      for( int counter##3 = range.dim(2).begin();  counter##3 < range.dim(2).end();  counter##3++ ) \
      for( int counter##4 = range.dim(3).begin();  counter##4 < range.dim(3).end();  counter##4++ ) { \
        tarch::la::Vector<4, int> counter = { counter##1, counter##2, counter##3, counter##4 };


#define parallelDforWithSchedulerInstructions5d(counter, max, loopParallelism) \
  { \
  tarch::la::Vector<5,int> counter##Max(max); \
  int counter##Begin[] = {0,0,0,0,0}; \
  int counter##End[]   = {counter##Max(0),counter##Max(1),counter##Max(2),counter##Max(3),counter##Max(4)}; \
  tbb::parallel_for( \
    tbb::blocked_rangeNd<int,5>( \
      counter##Begin, \
      counter##End, \
      loopParallelism==::peano4::utils::LoopPlacement::Serial ? counter##Max(0) : 1 \
    ), \
    [&]( const tbb::blocked_rangeNd<int,5>&  range ) { \
      for( int counter##1 = range.dim(0).begin();  counter##1 < range.dim(0).end();  counter##1++ ) \
      for( int counter##2 = range.dim(1).begin();  counter##2 < range.dim(1).end();  counter##2++ ) \
      for( int counter##3 = range.dim(2).begin();  counter##3 < range.dim(2).end();  counter##3++ ) \
      for( int counter##4 = range.dim(3).begin();  counter##4 < range.dim(3).end();  counter##4++ ) \
      for( int counter##5 = range.dim(4).begin();  counter##5 < range.dim(4).end();  counter##5++ ) { \
        tarch::la::Vector<5, int> counter = { counter##1, counter##2, counter##3, counter##4, counter##5 };


/**
 * - The first (curly) bracket closes the nested for loops within the
 *   functor. We need brackets here - even if users decide to add only one
 *   line - as the loop body defines the counter vector.
 * - The second (curly) bracket closes the functor handed into the
 *   blocked_range.
 * - The third (round) bracket closes the parallel_for, which is a mere
 *   function call. So it followed by a semicolon.
 * - The fourth (curly) bracket closes the whole block into which the for
 *   loop is embedded. This block not only hosts the loop, but also the
 *   construction of the max vector and the actual loop ranges.
 */
#define endParallelDfor \
      } \
      } \
      ); \
  }


/**
 * Brackets here are important. Otherwise macro resolution will fail.
 */
#define simtForWithSchedulerInstructions(counter, max, loopParallelism) \
  parallelForWithSchedulerInstructions(counter, max, (loopParallelism==::peano4::utils::LoopPlacement::Nested ? ::peano4::utils::LoopPlacement::Nested : ::peano4::utils::LoopPlacement::Serial))

#define endSimtFor \
  endParallelFor

#define simtDforWithSchedulerInstructions2d(counter, max, loopParallelism) \
  parallelDforWithSchedulerInstructions2d(counter, max, (loopParallelism==::peano4::utils::LoopPlacement::Nested ? ::peano4::utils::LoopPlacement::Nested : ::peano4::utils::LoopPlacement::Serial))

#define simtDforWithSchedulerInstructions3d(counter, max, loopParallelism) \
  parallelDforWithSchedulerInstructions3d(counter, max, (loopParallelism==::peano4::utils::LoopPlacement::Nested ? ::peano4::utils::LoopPlacement::Nested : ::peano4::utils::LoopPlacement::Serial))

#define simtDforWithSchedulerInstructions4d(counter, max, loopParallelism) \
  parallelDforWithSchedulerInstructions4d(counter, max, (loopParallelism==::peano4::utils::LoopPlacement::Nested ? ::peano4::utils::LoopPlacement::Nested : ::peano4::utils::LoopPlacement::Serial))

#define simtDforWithSchedulerInstructions5d(counter, max, loopParallelism) \
  parallelDforWithSchedulerInstructions5d(counter, max, (loopParallelism==::peano4::utils::LoopPlacement::Nested ? ::peano4::utils::LoopPlacement::Nested : ::peano4::utils::LoopPlacement::Serial))

#define endSimtDfor \
  endParallelDfor
