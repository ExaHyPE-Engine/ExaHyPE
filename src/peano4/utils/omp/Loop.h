// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
//
// Included by peano4/utils/Loop.h. Do not use directly
//

// TODO: It does not work if you have multiple pragmas with if, so I only support taskloop
//       as I assume that parallel for in nested parallelism would not work anyway.
// TODO: Use variadic templates for n-dimensions. -> Might not work, but maybe there is some (similar) trick.

#define parallelForWithSchedulerInstructions(counter, max, loopParallelism) \
  { \
  auto __loopParallelismUsedWithinMacroWithUniqueName = loopParallelism; \
  _Pragma( "omp taskloop default(shared) if ( __loopParallelismUsedWithinMacroWithUniqueName==::peano4::utils::LoopPlacement::SpreadOut)" ) \
  for( int counter = 0; counter < max; counter++ ) {

#define endParallelFor }}


#define parallelDforWithSchedulerInstructions2d(counter, max, loopParallelism) \
  { \
  tarch::la::Vector<2,int> counter##Max(max); \
  auto __loopParallelismUsedWithinMacroWithUniqueName = loopParallelism; \
  _Pragma( "omp taskloop collapse(2) default(shared) if (__loopParallelismUsedWithinMacroWithUniqueName==::peano4::utils::LoopPlacement::SpreadOut)" ) \
  for( int counter##0 = 0; counter##0 < counter##Max(0); counter##0++ ) \
  for( int counter##1 = 0; counter##1 < counter##Max(1); counter##1++ ) { \
    tarch::la::Vector<2, int> counter = { counter##0, counter##1 };

#define parallelDforWithSchedulerInstructions3d(counter, max, loopParallelism) \
  { \
  tarch::la::Vector<3,int> counter##Max(max); \
  auto __loopParallelismUsedWithinMacroWithUniqueName = loopParallelism; \
  _Pragma( "omp taskloop collapse(3) default(shared) if (__loopParallelismUsedWithinMacroWithUniqueName==::peano4::utils::LoopPlacement::SpreadOut)" ) \
  for( int counter##0 = 0; counter##0 < counter##Max(0); counter##0++ ) \
  for( int counter##1 = 0; counter##1 < counter##Max(1); counter##1++ ) \
  for( int counter##2 = 0; counter##2 < counter##Max(2); counter##2++ ) { \
    tarch::la::Vector<3, int> counter = { counter##0, counter##1, counter##2 };

#define parallelDforWithSchedulerInstructions4d(counter, max, loopParallelism) \
  { \
  tarch::la::Vector<4,int> counter##Max(max); \
  auto __loopParallelismUsedWithinMacroWithUniqueName = loopParallelism; \
  _Pragma( "omp taskloop collapse(4) default(shared) if (__loopParallelismUsedWithinMacroWithUniqueName==::peano4::utils::LoopPlacement::SpreadOut)" ) \
  for( int counter##0 = 0; counter##0 < counter##Max(0); counter##0++ ) \
  for( int counter##1 = 0; counter##1 < counter##Max(1); counter##1++ ) \
  for( int counter##2 = 0; counter##2 < counter##Max(2); counter##2++ ) \
  for( int counter##3 = 0; counter##3 < counter##Max(3); counter##3++ ) { \
    tarch::la::Vector<4, int> counter = { counter##0, counter##1, counter##2, counter##3 };

#define parallelDforWithSchedulerInstructions5d(counter, max, loopParallelism) \
  { \
  tarch::la::Vector<5,int> counter##Max(max); \
  auto __loopParallelismUsedWithinMacroWithUniqueName = loopParallelism; \
  _Pragma( "omp taskloop collapse(5) default(shared) if (__loopParallelismUsedWithinMacroWithUniqueName==::peano4::utils::LoopPlacement::SpreadOut)" ) \
  for( int counter##0 = 0; counter##0 < counter##Max(0); counter##0++ ) \
  for( int counter##1 = 0; counter##1 < counter##Max(1); counter##1++ ) \
  for( int counter##2 = 0; counter##2 < counter##Max(2); counter##2++ ) \
  for( int counter##3 = 0; counter##3 < counter##Max(3); counter##3++ ) \
  for( int counter##4 = 0; counter##4 < counter##Max(4); counter##4++ ) { \
    tarch::la::Vector<5, int> counter = { counter##0, counter##1, counter##2, counter##3, counter##4 };

#define endParallelDfor }}


#define simtForWithSchedulerInstructions(counter, max, loopParallelism) \
  { \
  auto __loopParallelismUsedWithinMacroWithUniqueName = loopParallelism; \
  _Pragma( "omp taskloop simd default(shared) if ( __loopParallelismUsedWithinMacroWithUniqueName!=::peano4::utils::LoopPlacement::Serial)" ) \
  for( int counter = 0; counter < max; counter++ ) {

#define endSimtFor }}


/**
 * SIMT loop macro for OpenMP
 *
 * I originally wanted to have one single OpenMP macro outside of the nested
 * for loops in combination with a collapse statement. So something similar
 * to
 *
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 * { \
 *   tarch::la::Vector<5,int> counter##Max(max); \
 *   auto __loopParallelismUsedWithinMacroWithUniqueName = loopParallelism; \
 *   _Pragma( "omp taskloop simd collapse(5) default(shared) if (__loopParallelismUsedWithinMacroWithUniqueName==::peano4::utils::LoopPlacement::SpreadOut)" ) \
 *   for( int counter##0 = 0; counter##0 < counter##Max(0); counter##0++ ) \
 *   for( int counter##1 = 0; counter##1 < counter##Max(1); counter##1++ ) \
 *   for( int counter##2 = 0; counter##2 < counter##Max(2); counter##2++ ) \
 *   for( int counter##3 = 0; counter##3 < counter##Max(3); counter##3++ ) \
 *   for( int counter##4 = 0; counter##4 < counter##Max(4); counter##4++ ) { \
 *     tarch::la::Vector<5, int> counter = { counter##0, counter##1, counter##2, counter##3, counter##4 };
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 *
 * This will make the code crash.
 * the omp taskloop outside of the nested for
 * loops and to use the simt.
 *
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 * { \
 *   tarch::la::Vector<5,int> counter##Max(max); \
 *   auto __loopParallelismUsedWithinMacroWithUniqueName = loopParallelism; \
 *   _Pragma( "omp taskloop collapse(4) default(shared) if (__loopParallelismUsedWithinMacroWithUniqueName==::peano4::utils::LoopPlacement::SpreadOut)" ) \
 *   for( int counter##0 = 0; counter##0 < counter##Max(0); counter##0++ ) \
 *   for( int counter##1 = 0; counter##1 < counter##Max(1); counter##1++ ) \
 *   for( int counter##2 = 0; counter##2 < counter##Max(2); counter##2++ ) \
 *   for( int counter##3 = 0; counter##3 < counter##Max(3); counter##3++ ) { \
 *     _Pragma( "omp simd if (__loopParallelismUsedWithinMacroWithUniqueName==::peano4::utils::LoopPlacement::SpreadOut)" ) \
 *     for( int counter##4 = 0; counter##4 < counter##Max(4); counter##4++ ) { \
 *       tarch::la::Vector<5, int> counter = { counter##0, counter##1, counter##2, counter##3, counter##4 };
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 *
 */
#define simtDforWithSchedulerInstructions2d(counter, max, loopParallelism) \
  { \
  tarch::la::Vector<2,int> counter##Max(max); \
  auto __loopParallelismUsedWithinMacroWithUniqueName = loopParallelism; \
  _Pragma( "omp parallel for if (__loopParallelismUsedWithinMacroWithUniqueName==::peano4::utils::LoopPlacement::SpreadOut)" ) \
  for( int counter##0 = 0; counter##0 < counter##Max(0); counter##0++ ) { \
    _Pragma( "omp simd if (__loopParallelismUsedWithinMacroWithUniqueName!=::peano4::utils::LoopPlacement::Serial)" ) \
    for( int counter##1 = 0; counter##1 < counter##Max(1); counter##1++ ) { \
      tarch::la::Vector<2, int> counter = { counter##0, counter##1 };

#define simtDforWithSchedulerInstructions3d(counter, max, loopParallelism) \
  { \
  tarch::la::Vector<3,int> counter##Max(max); \
  auto __loopParallelismUsedWithinMacroWithUniqueName = loopParallelism; \
  _Pragma( "omp parallel for if (__loopParallelismUsedWithinMacroWithUniqueName==::peano4::utils::LoopPlacement::SpreadOut)" ) \
  for( int counter##0 = 0; counter##0 < counter##Max(0); counter##0++ ) \
  for( int counter##1 = 0; counter##1 < counter##Max(1); counter##1++ ) { \
    _Pragma( "omp simd if (__loopParallelismUsedWithinMacroWithUniqueName!=::peano4::utils::LoopPlacement::Serial)" ) \
    for( int counter##2 = 0; counter##2 < counter##Max(2); counter##2++ ) { \
      tarch::la::Vector<3, int> counter = { counter##0, counter##1, counter##2 };

#define simtDforWithSchedulerInstructions4d(counter, max, loopParallelism) \
  { \
  tarch::la::Vector<4,int> counter##Max(max); \
  auto __loopParallelismUsedWithinMacroWithUniqueName = loopParallelism; \
  _Pragma( "omp parallel for if (__loopParallelismUsedWithinMacroWithUniqueName==::peano4::utils::LoopPlacement::SpreadOut)" ) \
  for( int counter##0 = 0; counter##0 < counter##Max(0); counter##0++ ) \
  for( int counter##1 = 0; counter##1 < counter##Max(1); counter##1++ ) \
  for( int counter##2 = 0; counter##2 < counter##Max(2); counter##2++ ) { \
    _Pragma( "omp simd if (__loopParallelismUsedWithinMacroWithUniqueName!=::peano4::utils::LoopPlacement::Serial)" ) \
    for( int counter##3 = 0; counter##3 < counter##Max(3); counter##3++ ) { \
      tarch::la::Vector<4, int> counter = { counter##0, counter##1, counter##2, counter##3 };

#define simtDforWithSchedulerInstructions5d(counter, max, loopParallelism) \
  { \
  tarch::la::Vector<5,int> counter##Max(max); \
  auto __loopParallelismUsedWithinMacroWithUniqueName = loopParallelism; \
  _Pragma( "omp parallel for if (__loopParallelismUsedWithinMacroWithUniqueName==::peano4::utils::LoopPlacement::SpreadOut)" ) \
  for( int counter##0 = 0; counter##0 < counter##Max(0); counter##0++ ) \
  for( int counter##1 = 0; counter##1 < counter##Max(1); counter##1++ ) \
  for( int counter##2 = 0; counter##2 < counter##Max(2); counter##2++ ) \
  for( int counter##3 = 0; counter##3 < counter##Max(3); counter##3++ ) { \
    _Pragma( "omp simd if (__loopParallelismUsedWithinMacroWithUniqueName!=::peano4::utils::LoopPlacement::Serial)" ) \
    for( int counter##4 = 0; counter##4 < counter##Max(4); counter##4++ ) { \
      tarch::la::Vector<5, int> counter = { counter##0, counter##1, counter##2, counter##3, counter##4 };

#define endSimtDfor }}}
