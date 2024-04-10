// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
//
// Included by peano4/utils/Loop.h. Do not use directly
//
#define parallelForWithSchedulerInstructions(counter, max, loopParallelism) \
  { \
  auto __loopParallelismUsedWithinMacroWithUniqueName = loopParallelism; \
  _Pragma( "omp parallel for default(shared) if ( __loopParallelismUsedWithinMacroWithUniqueName==::peano4::utils::LoopPlacement::SpreadOut)" ) \
  for( int counter = 0; counter < max; counter++ ) {

#define endParallelFor }}


#define parallelDforWithSchedulerInstructions2d(counter, max, loopParallelism) \
  { \
  tarch::la::Vector<2,int> counter##Max(max); \
  auto __loopParallelismUsedWithinMacroWithUniqueName = loopParallelism; \
  _Pragma( "omp parallel for collapse(2) default(shared) if (__loopParallelismUsedWithinMacroWithUniqueName==::peano4::utils::LoopPlacement::SpreadOut)" ) \
  for( int counter##0 = 0; counter##0 < counter##Max(0); counter##0++ ) \
  for( int counter##1 = 0; counter##1 < counter##Max(1); counter##1++ ) { \
    tarch::la::Vector<2, int> counter = { counter##0, counter##1 };

#define parallelDforWithSchedulerInstructions3d(counter, max, loopParallelism) \
  { \
  tarch::la::Vector<3,int> counter##Max(max); \
  auto __loopParallelismUsedWithinMacroWithUniqueName = loopParallelism; \
  _Pragma( "omp parallel for collapse(3) default(shared) if (__loopParallelismUsedWithinMacroWithUniqueName==::peano4::utils::LoopPlacement::SpreadOut)" ) \
  for( int counter##0 = 0; counter##0 < counter##Max(0); counter##0++ ) \
  for( int counter##1 = 0; counter##1 < counter##Max(1); counter##1++ ) \
  for( int counter##2 = 0; counter##2 < counter##Max(2); counter##2++ ) { \
    tarch::la::Vector<3, int> counter = { counter##0, counter##1, counter##2 };

#define parallelDforWithSchedulerInstructions4d(counter, max, loopParallelism) \
  { \
  tarch::la::Vector<4,int> counter##Max(max); \
  auto __loopParallelismUsedWithinMacroWithUniqueName = loopParallelism; \
  _Pragma( "omp parallel for collapse(4) default(shared) if (__loopParallelismUsedWithinMacroWithUniqueName==::peano4::utils::LoopPlacement::SpreadOut)" ) \
  for( int counter##0 = 0; counter##0 < counter##Max(0); counter##0++ ) \
  for( int counter##1 = 0; counter##1 < counter##Max(1); counter##1++ ) \
  for( int counter##2 = 0; counter##2 < counter##Max(2); counter##2++ ) \
  for( int counter##3 = 0; counter##3 < counter##Max(3); counter##3++ ) { \
    tarch::la::Vector<4, int> counter = { counter##0, counter##1, counter##2, counter##3 };

#define parallelDforWithSchedulerInstructions5d(counter, max, loopParallelism) \
  { \
  tarch::la::Vector<5,int> counter##Max(max); \
  auto __loopParallelismUsedWithinMacroWithUniqueName = loopParallelism; \
  _Pragma( "omp parallel for collapse(5) default(shared) if (__loopParallelismUsedWithinMacroWithUniqueName==::peano4::utils::LoopPlacement::SpreadOut)" ) \
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
  _Pragma( "omp simd if ( __loopParallelismUsedWithinMacroWithUniqueName!=::peano4::utils::LoopPlacement::Serial)" ) \
  for( int counter = 0; counter < max; counter++ ) {

#define endSimtFor }}


/**
 * SIMT loop macro for OpenMP
 */
#define simtDforWithSchedulerInstructions2d(counter, max, loopParallelism) \
  { \
  tarch::la::Vector<2,int> counter##Max(max); \
  auto __loopParallelismUsedWithinMacroWithUniqueName = loopParallelism; \
  _Pragma( "omp parallel for collapse(1) default(shared) if (__loopParallelismUsedWithinMacroWithUniqueName==::peano4::utils::LoopPlacement::SpreadOut)" ) \
  for( int counter##0 = 0; counter##0 < counter##Max(0); counter##0++ ) { \
    _Pragma( "omp simd if (__loopParallelismUsedWithinMacroWithUniqueName!=::peano4::utils::LoopPlacement::Serial)" ) \
    for( int counter##1 = 0; counter##1 < counter##Max(1); counter##1++ ) { \
      tarch::la::Vector<2, int> counter = { counter##0, counter##1 };

#define simtDforWithSchedulerInstructions3d(counter, max, loopParallelism) \
  { \
  tarch::la::Vector<3,int> counter##Max(max); \
  auto __loopParallelismUsedWithinMacroWithUniqueName = loopParallelism; \
  _Pragma( "omp parallel for collapse(2) default(shared) if (__loopParallelismUsedWithinMacroWithUniqueName==::peano4::utils::LoopPlacement::SpreadOut)" ) \
  for( int counter##0 = 0; counter##0 < counter##Max(0); counter##0++ ) \
  for( int counter##1 = 0; counter##1 < counter##Max(1); counter##1++ ) { \
    _Pragma( "omp simd if (__loopParallelismUsedWithinMacroWithUniqueName!=::peano4::utils::LoopPlacement::Serial)" ) \
    for( int counter##2 = 0; counter##2 < counter##Max(2); counter##2++ ) { \
      tarch::la::Vector<3, int> counter = { counter##0, counter##1, counter##2 };

#define simtDforWithSchedulerInstructions4d(counter, max, loopParallelism) \
  { \
  tarch::la::Vector<4,int> counter##Max(max); \
  auto __loopParallelismUsedWithinMacroWithUniqueName = loopParallelism; \
  _Pragma( "omp parallel for collapse(3) default(shared) if (__loopParallelismUsedWithinMacroWithUniqueName==::peano4::utils::LoopPlacement::SpreadOut)" ) \
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
  _Pragma( "omp parallel for collapse(4) default(shared) if (__loopParallelismUsedWithinMacroWithUniqueName==::peano4::utils::LoopPlacement::SpreadOut)" ) \
  for( int counter##0 = 0; counter##0 < counter##Max(0); counter##0++ ) \
  for( int counter##1 = 0; counter##1 < counter##Max(1); counter##1++ ) \
  for( int counter##2 = 0; counter##2 < counter##Max(2); counter##2++ ) \
  for( int counter##3 = 0; counter##3 < counter##Max(3); counter##3++ ) { \
    _Pragma( "omp simd if (__loopParallelismUsedWithinMacroWithUniqueName!=::peano4::utils::LoopPlacement::Serial)" ) \
    for( int counter##4 = 0; counter##4 < counter##Max(4); counter##4++ ) { \
      tarch::la::Vector<5, int> counter = { counter##0, counter##1, counter##2, counter##3, counter##4 };

#define endSimtDfor }}}
