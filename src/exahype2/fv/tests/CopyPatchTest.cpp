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

 
  std::cerr << "Now the alternative implementation without dfor\n"; 
  #if Dimensions==2
  int sourceSerialised      = numberOfVolumesPerAxisInPatch+haloSize*2+haloSize;
  int destinationSerialised = 0;
  for (int y=0; y<numberOfVolumesPerAxisInPatch; y++) {
    for (int x=0; x<numberOfVolumesPerAxisInPatch; x++) {
      thesource2.push_back(sourceSerialised);
      dest2.push_back(destinationSerialised);
      sourceSerialised++;
      destinationSerialised++;
    }
    sourceSerialised += 2*haloSize;
  }
  #else
  int sourceSerialised      = (numberOfVolumesPerAxisInPatch+haloSize*2)*(numberOfVolumesPerAxisInPatch+haloSize*2) + numberOfVolumesPerAxisInPatch+haloSize*2+haloSize;
  int destinationSerialised = 0;
  for (int z=0; z<numberOfVolumesPerAxisInPatch; z++) {
    for (int y=0; y<numberOfVolumesPerAxisInPatch; y++) {
      for (int x=0; x<numberOfVolumesPerAxisInPatch; x++) {
        thesource2.push_back(sourceSerialised);
        dest2.push_back(destinationSerialised);
        sourceSerialised++;
        destinationSerialised++;
      }
      sourceSerialised += 2*haloSize;
    }
    sourceSerialised += 2*haloSize+2*(numberOfVolumesPerAxisInPatch+haloSize*2)-2;
  }
  #endif

  for (size_t idx=0;idx< thesource.size();++idx)
  {
     if (thesource[idx]!=thesource2[idx]) std::cerr <<" source error at " << idx <<"\n";
  }
  
  for (size_t idx=0;idx< dest.size();++idx)
  {
     if (dest[idx]!=dest2[idx]) std::cerr <<" dest error at " << idx <<"\n";
  }

}
