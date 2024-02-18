// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
//
// Included by peano4/utils/Loop.h. Do not use directly
//

/**
 * Parallel for
 *
 * SYCL does not support a classic parallel for which can host virtual
 * function calls, semaphores, etc. Therefore, this is maps onto a plain
 * for.
 */
#define parallelForWithSchedulerInstructions(counter, max, loopParallelism) \
  for( int counter = 0; counter < max; counter++ ) {

#define endParallelFor }


#define parallelDforWithDynamicScheduling2d(counter, max, loopParallelism) \
  { \
  tarch::la::Vector<2,int> counter##Max(max); \
  for( int counter##0 = 0; counter##0 < counter##Max(0); counter##0++ ) \
  for( int counter##1 = 0; counter##1 < counter##Max(1); counter##1++ ) { \
    tarch::la::Vector<2, int> counter = { counter##0, counter##1 };

#define parallelDforWithDynamicScheduling3d(counter, max, loopParallelism) \
  { \
  tarch::la::Vector<3,int> counter##Max(max); \
  for( int counter##0 = 0; counter##0 < counter##Max(0); counter##0++ ) \
  for( int counter##1 = 0; counter##1 < counter##Max(1); counter##1++ ) \
  for( int counter##2 = 0; counter##2 < counter##Max(2); counter##2++ ) { \
    tarch::la::Vector<3, int> counter = { counter##0, counter##1, counter##2 };

#define parallelDforWithDynamicScheduling4d(counter, max, loopParallelism) \
  { \
  tarch::la::Vector<4,int> counter##Max(max); \
  for( int counter##0 = 0; counter##0 < counter##Max(0); counter##0++ ) \
  for( int counter##1 = 0; counter##1 < counter##Max(1); counter##1++ ) \
  for( int counter##2 = 0; counter##2 < counter##Max(2); counter##2++ ) \
  for( int counter##3 = 0; counter##3 < counter##Max(3); counter##3++ ) { \
    tarch::la::Vector<4, int> counter = { counter##0, counter##1, counter##2, counter##3 };

#define parallelDforWithDynamicScheduling5d(counter, max, loopParallelism) \
  { \
  tarch::la::Vector<5,int> counter##Max(max); \
  for( int counter##0 = 0; counter##0 < counter##Max(0); counter##0++ ) \
  for( int counter##1 = 0; counter##1 < counter##Max(1); counter##1++ ) \
  for( int counter##2 = 0; counter##2 < counter##Max(2); counter##2++ ) \
  for( int counter##3 = 0; counter##3 < counter##Max(3); counter##3++ ) \
  for( int counter##4 = 0; counter##4 < counter##Max(4); counter##4++ ) { \
    tarch::la::Vector<5, int> counter = { counter##0, counter##1, counter##2, counter##3, counter##4 };

#define endParallelDfor }}


#define simtForWithSchedulerInstructions(counter, max, loopParallelism) \
  tarch::multicore::getHostSYCLQueue().submit([&](auto& handle) { \
    handle.parallel_for( \
      ::sycl::range<1>{static_cast<std::size_t>(max)}, \
      [=](::sycl::id<1> counter##Iterator) { \
        int counter = counter##Iterator[0];


/**
 * - The first (curly) bracket closes functor of the loop body.
 * - The second (round) bracket closes the parallel_for, which is a mere
 *   function call. So it is followed by a semicolon.
 * - The third combination of a curly plus round bracket close the submit().
 *   It also comprises the wait().
 */
#define endSimtFor \
      } \
    ); \
  }).wait();

  /**
   * We need an additional opening bracket here, as we need one more nested
   * scope for 4d and 5d.
   *
   * @see simtDforWithDynamicScheduling4d
   * @see simtDforWithDynamicScheduling5d
   */
  #define simtDforWithDynamicScheduling2d(counter, max, loopParallelism) \
    { \
    tarch::la::Vector<2,int> counter##Max(tarch::la::expandOrSlice<2>(max)); \
    tarch::multicore::getHostSYCLQueue().submit([&](auto& handle) { \
      handle.parallel_for( \
        ::sycl::range<2>{static_cast<std::size_t>(counter##Max(0)),static_cast<std::size_t>(counter##Max(1))}, \
        [=](::sycl::id<2> counter##Iterator) {{ \
          tarch::la::Vector<2, int> counter = { static_cast<int>(counter##Iterator[0]), static_cast<int>(counter##Iterator[1]) };


  #define simtDforWithDynamicScheduling3d(counter, max, loopParallelism) \
    { \
    tarch::la::Vector<3,int> counter##Max(tarch::la::expandOrSlice<3>(max)); \
    tarch::multicore::getHostSYCLQueue().submit([&](auto& handle) { \
      handle.parallel_for( \
        ::sycl::range<3>{static_cast<std::size_t>(counter##Max(0)),static_cast<std::size_t>(counter##Max(1)),static_cast<std::size_t>(counter##Max(2))}, \
        [=](::sycl::id<3> counter##Iterator) {{ \
          tarch::la::Vector<3, int> counter = { static_cast<int>(counter##Iterator[0]), static_cast<int>(counter##Iterator[1]), static_cast<int>(counter##Iterator[2]) };


  #define simtDforWithDynamicScheduling4d(counter, max, loopParallelism) \
    { \
    tarch::la::Vector<4,int> counter##Max(tarch::la::expandOrSlice<4>(max)); \
    tarch::multicore::getHostSYCLQueue().submit([&](auto& handle) { \
      handle.parallel_for( \
        ::sycl::range<3>{static_cast<std::size_t>(counter##Max(0)),static_cast<std::size_t>(counter##Max(1)),static_cast<std::size_t>(counter##Max(2))}, \
        [=](::sycl::id<3> counter##Iterator) { \
          for (int counter##Iterator3 = 0; counter##Iterator3<counter##Max(3); counter##Iterator3++ ) { \
            tarch::la::Vector<4, int> counter = { static_cast<int>(counter##Iterator[0]), static_cast<int>(counter##Iterator[1]), static_cast<int>(counter##Iterator[2]), static_cast<int>(counter##Iterator3) };


  #define simtDforWithDynamicScheduling5d(counter, max, loopParallelism) \
    { \
    tarch::la::Vector<5,int> counter##Max(tarch::la::expandOrSlice<5>(max)); \
    tarch::multicore::getHostSYCLQueue().submit([&](auto& handle) { \
      handle.parallel_for( \
        ::sycl::range<3>{static_cast<std::size_t>(counter##Max(0)),static_cast<std::size_t>(counter##Max(1)),static_cast<std::size_t>(counter##Max(2))}, \
        [=](::sycl::id<3> counter##Iterator) { \
          for (int counter##Iterator3 = 0; counter##Iterator3<counter##Max(3); counter##Iterator3++ ) \
          for (int counter##Iterator4 = 0; counter##Iterator4<counter##Max(4); counter##Iterator4++ ) { \
            tarch::la::Vector<5, int> counter = { static_cast<int>(counter##Iterator[0]), static_cast<int>(counter##Iterator[1]), static_cast<int>(counter##Iterator[2]), static_cast<int>(counter##Iterator3), static_cast<int>(counter##Iterator4) };


  /**
   * - The first two (curly) brackets close the functor and the for loop
   *   within this functor (4d and 5d). For d=2 and d=3, the functor is
   *   using two opening brackets.
   * - The third (round) bracket closes the parallel_for, which is a mere
   *   function call. So it followed by a semicolon.
   * - The fourth combination of a curly plus round bracket close the submit().
   *   This one is followed by a wait().
   * - We have another curly bracket here as we also embed the counter##Max
   *   thing into a scope. If we have two loops with the same counter name,
   *   we don't want to run into a duplicate variable error.
   */
  #define endSimtDfor \
    }} \
    ); \
    }).wait(); \
    }
