#include "peano4/utils/Loop.h"
#include <vector>
// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "CopyPatchTest.h"

exahype2::fv::tests::CopyPatchTest::CopyPatchTest():
  TestCase ("exahype2::fv::tests::CopyPatchTest") {
}

void exahype2::fv::tests::CopyPatchTest::run() {
  testMethod (testMe);
}

void exahype2::fv::tests::CopyPatchTest::testMe() {
  int    unknowns(5);
  int    auxiliaryVariables(2);
  int    numberOfVolumesPerAxisInPatch(17);
  int    haloSize(1);
  // NOTE: the test does not work for halosize !=1
  double*  QinWithHalo;
  double* QOutWithoutHalo;
  std::vector<int> thesource, dest, thesource2, dest2;
  dfor(k,numberOfVolumesPerAxisInPatch) {
    tarch::la::Vector<Dimensions,int>   source = k + tarch::la::Vector<Dimensions,int>(haloSize);
    int sourceSerialised      = peano4::utils::dLinearised(source,numberOfVolumesPerAxisInPatch+haloSize*2);
    int destinationSerialised = peano4::utils::dLinearised(k,numberOfVolumesPerAxisInPatch);
    thesource.push_back(sourceSerialised);
    dest.push_back(destinationSerialised);
  }
 
  #if Dimensions==2
  int sourceSerialised      = numberOfVolumesPerAxisInPatch + 3*haloSize;
  int offset = numberOfVolumesPerAxisInPatch;
  for (int y=0; y<numberOfVolumesPerAxisInPatch; y++) {
    for (int x=0; x<numberOfVolumesPerAxisInPatch; x++) {
      thesource2.push_back((y+1)*(offset+3*haloSize) + x - y);
      dest2.push_back(y*offset+x);
    }
  }
  #else
  int offset  = numberOfVolumesPerAxisInPatch;
  int offset2 = (numberOfVolumesPerAxisInPatch+haloSize*2)*(numberOfVolumesPerAxisInPatch+haloSize*2) + numberOfVolumesPerAxisInPatch+haloSize*2 + haloSize;
  int offset3 = numberOfVolumesPerAxisInPatch+haloSize*2;
  for (int z=0; z<numberOfVolumesPerAxisInPatch; z++) {
    for (int y=0; y<numberOfVolumesPerAxisInPatch; y++) {
      for (int x=0; x<numberOfVolumesPerAxisInPatch; x++) {
         int mydest = z*offset*offset + y*offset + x;
         int mysrc  = z*offset3*offset3 + y*offset3 + x +offset2;
        thesource2.push_back(mysrc);
        dest2.push_back(mydest);
      }
    }
  }
  #endif

  for (size_t idx=0;idx< thesource.size();++idx)
  {
     if (thesource[idx]!=thesource2[idx]) std::cerr <<" source error at " << idx <<"\n";
     validateEquals(thesource[idx], thesource2[idx]);
  }
  
  for (size_t idx=0;idx< dest.size();++idx)
  {
     if (dest[idx]!=dest2[idx]) std::cerr <<" dest error at " << idx <<"\n";
     validateEquals(dest[idx], dest2[idx]);
  }

}
