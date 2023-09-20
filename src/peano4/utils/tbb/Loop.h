//
// Included by peano4/utils/Loop.h. Do not use directly
//
#define parallelForWithDynamicScheduling(counter, max, loopParallelism) \
  tbb::parallel_for( \
    tbb::blocked_range<int>( \
      0, max, loopParallelism==::peano4::utils::LoopParallelism::None ? max : 1 \
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



#define parallelDforWithDynamicScheduling2d(counter, max, loopParallelism) \
{ \
tarch::la::Vector<2,int> counter##Max(max); \
  tbb::parallel_for( \
    tbb::blocked_range2d<int>( \
      0, counter##Max(0), loopParallelism==::peano4::utils::LoopParallelism::None ? counter##Max(0) : 1, \
      0, counter##Max(1), loopParallelism==::peano4::utils::LoopParallelism::None ? counter##Max(1) : 1 \
    ), \
    [&]( const tbb::blocked_range2d<int>&  range ) { \
      for( int counter##1 = range.rows().begin();  counter##1 < range.rows().end();  counter##1++ ) \
      for( int counter##2 = range.cols().begin();  counter##2 < range.cols().end();  counter##2++ ) { \
        tarch::la::Vector<2, int> counter = { counter##1, counter##2 };


#define parallelDforWithDynamicScheduling3d(counter, max, loopParallelism) \
  { \
  tarch::la::Vector<3,int> counter##Max(max); \
  tbb::parallel_for( \
    tbb::blocked_range3d<int>( \
      0, counter##Max(0), loopParallelism==::peano4::utils::LoopParallelism::None ? counter##Max(0) : 1, \
      0, counter##Max(1), loopParallelism==::peano4::utils::LoopParallelism::None ? counter##Max(1) : 1, \
      0, counter##Max(2), loopParallelism==::peano4::utils::LoopParallelism::None ? counter##Max(2) : 1 \
    ), \
    [&]( const tbb::blocked_range3d<int>&  range ) { \
      for( int counter##1 = range.pages().begin();  counter##1 < range.pages().end();  counter##1++ ) \
      for( int counter##2 = range.rows().begin();   counter##2 < range.rows().end();   counter##2++ ) \
      for( int counter##3 = range.cols().begin();   counter##3 < range.cols().end();   counter##3++ ) { \
        tarch::la::Vector<3, int> counter = { counter##1, counter##2, counter##3 };


#define parallelDforWithDynamicScheduling4d(counter, max, loopParallelism) \
  { \
  tarch::la::Vector<4,int> counter##Max(max); \
  int counter##Begin[] = {0,0,0,0}; \
  int counter##End[]   = {counter##Max(0),counter##Max(1),counter##Max(2),counter##Max(3)}; \
  tbb::parallel_for( \
    tbb::blocked_rangexd<int,4>( \
      counter##Begin, \
      counter##End, \
      loopParallelism==::peano4::utils::LoopParallelism::None ? counter##Max(0) : 1 \
    ), \
    [&]( const tbb::blocked_rangexd<int,4>&  range ) { \
      for( int counter##1 = range.subrange(0).begin();  counter##1 < range.subrange(0).end();  counter##1++ ) \
      for( int counter##2 = range.subrange(1).begin();  counter##2 < range.subrange(1).end();  counter##2++ ) \
      for( int counter##3 = range.subrange(2).begin();  counter##3 < range.subrange(2).end();  counter##3++ ) \
      for( int counter##4 = range.subrange(3).begin();  counter##4 < range.subrange(3).end();  counter##4++ ) { \
        tarch::la::Vector<4, int> counter = { counter##1, counter##2, counter##3, counter##4 };


#define parallelDforWithDynamicScheduling5d(counter, max, loopParallelism) \
  { \
  tarch::la::Vector<5,int> counter##Max(max); \
  int counter##Begin[] = {0,0,0,0,0}; \
  int counter##End[]   = {counter##Max(0),counter##Max(1),counter##Max(2),counter##Max(3),counter##Max(4)}; \
  tbb::parallel_for( \
    tbb::blocked_rangexd<int,5>( \
      counter##Begin, \
      counter##End, \
      loopParallelism==::peano4::utils::LoopParallelism::None ? counter##Max(0) : 1 \
    ), \
    [&]( const tbb::blocked_rangexd<int,5>&  range ) { \
      for( int counter##1 = range.subrange(0).begin();  counter##1 < range.subrange(0).end();  counter##1++ ) \
      for( int counter##2 = range.subrange(1).begin();  counter##2 < range.subrange(1).end();  counter##2++ ) \
      for( int counter##3 = range.subrange(2).begin();  counter##3 < range.subrange(2).end();  counter##3++ ) \
      for( int counter##4 = range.subrange(3).begin();  counter##4 < range.subrange(3).end();  counter##4++ ) \
      for( int counter##5 = range.subrange(4).begin();  counter##5 < range.subrange(4).end();  counter##5++ ) { \
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



#define simtForWithDynamicScheduling(counter, max, loopParallelism) \
  parallelForWithDynamicScheduling(counter, max, loopParallelism)

#define endSimtFor \
  endParallelFor

#define simtDforWithDynamicScheduling2d(counter, max, loopParallelism) \
  parallelDforWithDynamicScheduling2d(counter, max, loopParallelism)

#define simtDforWithDynamicScheduling3d(counter, max, loopParallelism) \
  parallelDforWithDynamicScheduling3d(counter, max, loopParallelism)

#define simtDforWithDynamicScheduling4d(counter, max, loopParallelism) \
  parallelDforWithDynamicScheduling4d(counter, max, loopParallelism)

#define simtDforWithDynamicScheduling5d(counter, max, loopParallelism) \
  parallelDforWithDynamicScheduling5d(counter, max, loopParallelism)

#define endSimtDfor \
  endParallelDfor

