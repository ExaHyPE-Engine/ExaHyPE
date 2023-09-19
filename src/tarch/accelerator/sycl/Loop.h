  #define parallelForWithDynamicScheduling(counter, max, loopParallelism) \
    tarch::multicore::getHostSYCLQueue().submit([&](auto& handle) { \
      handle.parallel_for( \
        ::sycl::range<1>{max}, \
        [&](::sycl::id<1> counter##Iterator) { \
          int counter = counter##Iterator[0];


  /**
   * - The first (curly) bracket closes functor of the loop body.
   * - The second (round) bracket closes the parallel_for, which is a mere
   *   function call. So it is followed by a semicolon.
   * - The third combination of a curly plus round bracket close the submit().
   *   It also comprises the wait().
   */
  #define endParallelFor \
      } \
      ); \
      }).wait();


  /**
   * We need an additional opening bracket here, as we need one more nested
   * scope for 4d and 5d.
   *
   * @see parallelDforWithDynamicScheduling4d
   * @see parallelDforWithDynamicScheduling5d
   */
  #define parallelDforWithDynamicScheduling2d(counter, max, loopParallelism) \
    { \
    tarch::la::Vector<2,int> counter##Max(tarch::la::expand<2>(max)); \
    tarch::multicore::getHostSYCLQueue().submit([&](auto& handle) { \
      handle.parallel_for( \
        ::sycl::range<2>{static_cast<std::size_t>(counter##Max(0)),static_cast<std::size_t>(counter##Max(1))}, \
        [&](::sycl::id<2> counter##Iterator) {{ \
          tarch::la::Vector<2, int> counter = { static_cast<int>(counter##Iterator[0]), static_cast<int>(counter##Iterator[1]) };


  #define parallelDforWithDynamicScheduling3d(counter, max, loopParallelism) \
    { \
    tarch::la::Vector<3,int> counter##Max(tarch::la::expand<3>(max)); \
    tarch::multicore::getHostSYCLQueue().submit([&](auto& handle) { \
      handle.parallel_for( \
        ::sycl::range<3>{static_cast<std::size_t>(counter##Max(0)),static_cast<std::size_t>(counter##Max(1)),static_cast<std::size_t>(counter##Max(2))}, \
        [&](::sycl::id<3> counter##Iterator) {{ \
          tarch::la::Vector<3, int> counter = { static_cast<int>(counter##Iterator[0]), static_cast<int>(counter##Iterator[1]), static_cast<int>(counter##Iterator[2]) };


  #define parallelDforWithDynamicScheduling4d(counter, max, loopParallelism) \
    { \
    tarch::la::Vector<4,int> counter##Max(tarch::la::expand<4>(max)); \
    tarch::multicore::getHostSYCLQueue().submit([&](auto& handle) { \
      handle.parallel_for( \
        ::sycl::range<3>{static_cast<std::size_t>(counter##Max(0)),static_cast<std::size_t>(counter##Max(1)),static_cast<std::size_t>(counter##Max(2))}, \
        [&](::sycl::id<3> counter##Iterator) { \
          for (int counter##Iterator3 = 0; counter##Iterator3<counter##Max(3); counter##Iterator3++ ) { \
            tarch::la::Vector<4, int> counter = { static_cast<int>(counter##Iterator[0]), static_cast<int>(counter##Iterator[1]), static_cast<int>(counter##Iterator[2]), static_cast<int>(counter##Iterator3) };


  #define parallelDforWithDynamicScheduling5d(counter, max, loopParallelism) \
    { \
    tarch::la::Vector<5,int> counter##Max(tarch::la::expand<4>(max)); \
    tarch::multicore::getHostSYCLQueue().submit([&](auto& handle) { \
      handle.parallel_for( \
        ::sycl::range<3>{static_cast<std::size_t>(counter##Max(0)),static_cast<std::size_t>(counter##Max(1)),static_cast<std::size_t>(counter##Max(2))}, \
        [&](::sycl::id<3> counter##Iterator) { \
          for (int counter##Iterator3 = 0; counter##Iterator3<counter##Max(3); counter##Iterator3++ ) \
          for (int counter##Iterator4 = 0; counter##Iterator4<counter##Max(4); counter##Iterator4++ ) { \
            tarch::la::Vector<5, int> counter = { static_cast<int>(counter##Iterator[0]), static_cast<int>(counter##Iterator[1]), static_cast<int>(counter##Iterator[2]), static_cast<int>(counter##Iterator3), static_cast<int>(counter##Iterator4) };

/*
  #define parallelDforWithDynamicScheduling4d(counter, max, loopParallelism) \
    { \
    tarch::la::Vector<4,std::size_t> counter##Max(max); \
    tarch::multicore::getHostSYCLQueue().submit([&](auto& handle) { \
      handle.parallel_for( \
        ::sycl::range<4>{counter##Max(0),counter##Max(1),counter##Max(2),counter##Max(3)}, \
        [=](::sycl::id<4> counter##Iterator) { \
          tarch::la::Vector<4, int> counter = { static_cast<int>(counter##Iterator[0]), static_cast<int>(counter##Iterator[1]), static_cast<int>(counter##Iterator[2]), static_cast<int>(counter##Iterator[3]) };


  #define parallelDforWithDynamicScheduling5d(counter, max, loopParallelism) \
    { \
    tarch::la::Vector<5,std::size_t> counter##Max(max); \
    tarch::multicore::getHostSYCLQueue().submit([&](auto& handle) { \
      handle.parallel_for( \
        ::sycl::range<3>{counter##Max(0),counter##Max(1),counter##Max(2),counter##Max(3),counter##Max(4)}, \
        [=](::sycl::id<5> counter##Iterator) { \
          tarch::la::Vector<5, int> counter = { static_cast<int>(counter##Iterator[0]), static_cast<int>(counter##Iterator[1]), static_cast<int>(counter##Iterator[2]), static_cast<int>(counter##Iterator[3]), static_cast<int>(counter##Iterator[4]) };
*/

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
  #define endParallelDfor \
    }} \
    ); \
    }).wait(); \
    }
