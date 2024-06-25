#include "peano4/utils/Globals.h"
#include "peano4/utils/Loop.h"
#include "ParallelDForTest.h"

#include "tarch/la/Vector.h"


tarch::logging::Log peano4::utils::tests::ParallelDForTest::_log("peano4::grid::tests::SpacetreeTest");


#ifdef UseTestSpecificCompilerSettings
#pragma optimize("",off)
#endif


peano4::utils::tests::ParallelDForTest::ParallelDForTest():
  TestCase( "peano4::utils::tests::ParallelDForTest" ) {
}


void peano4::utils::tests::ParallelDForTest::testParallelDFor() {
  constexpr int Size = 24;
  constexpr int Samples = 64;
  #if Dimension==2
  constexpr int DSize = Size*Size;
  #else
  constexpr int DSize = Size*Size*Size;
  #endif
  int output[ DSize ];

  for (int i=0; i<DSize; i++) {
    output[i] = 0;
  }

  for (int sample=0; sample<Samples; sample++) {
    parallelDfor(counter,Size) {
      int index = peano4::utils::dLinearised(counter,Size);
      output[index]++;
    } endParallelDfor
  }

  dfor(counter,Size) {
    int index = peano4::utils::dLinearised(counter,Size);
    validateEqualsWithParams2( output[index], Samples, index, counter );
  }
}


void peano4::utils::tests::ParallelDForTest::run() {
  testMethod( testParallelDFor );
}


#ifdef UseTestSpecificCompilerSettings
#pragma optimize("",on)
#endif
