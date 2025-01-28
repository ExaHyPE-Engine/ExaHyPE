#include "SommerfeldBCTest.h"

#include "peano4/utils/Globals.h"
#include "peano4/utils/Loop.h"
#include "tarch/la/Vector.h"

#include "tarch/logging/Log.h"

#include "exahype2/fd/PatchUtils.h"
#include "exahype2/enumerator/FaceAoSLexicographicEnumerator.h"
#include "exahype2/fd/BoundaryConditions.h"

#include <algorithm>
#include <iomanip>


exahype2::fd::tests::SommerfeldBCTest::SommerfeldBCTest():
  TestCase ("exahype2::fd::tests::SommerfeldBCTest") {
}


void exahype2::fd::tests::SommerfeldBCTest::run() {
  testMethod (flatsolutiontest);
}


void exahype2::fd::tests::SommerfeldBCTest::flatsolutiontest() {

  //we consider facenumber 3, i.e. face on the x positive, patch size 2, evolving quantity 2. Thus the array has 2x2x2x6=48 entries 
  double Qold[] = {
    1.0, 2.0, 1.0, 2.0, 1.0, 2.0,   1.0, 2.0, 1.0, 2.0, 1.0, 2.0,
    1.0, 2.0, 1.0, 2.0, 1.0, 2.0,   1.0, 2.0, 1.0, 2.0, 1.0, 2.0,
    1.0, 2.0, 1.0, 2.0, 1.0, 2.0,   1.0, 2.0, 1.0, 2.0, 1.0, 2.0,
    1.0, 2.0, 1.0, 2.0, 1.0, 2.0,   1.0, 2.0, 1.0, 2.0, 1.0, 2.0
  };

  double Qnew[] = {
    1.0, 2.0, 1.0, 2.0, 1.0, 2.0,   0.0, 0.0, 0.0, 0.0, 0.0, 0.0,
    1.0, 2.0, 1.0, 2.0, 1.0, 2.0,   0.0, 0.0, 0.0, 0.0, 0.0, 0.0,
    1.0, 2.0, 1.0, 2.0, 1.0, 2.0,   0.0, 0.0, 0.0, 0.0, 0.0, 0.0,
    1.0, 2.0, 1.0, 2.0, 1.0, 2.0,   0.0, 0.0, 0.0, 0.0, 0.0, 0.0
  };


  ::exahype2::fd::applySommerfeldConditions(
    [&](
        const double * __restrict__                  Q,
        const tarch::la::Vector<Dimensions,double>&  faceCentre,
        const tarch::la::Vector<Dimensions,double>&  gridCellH,
        double                                       t,
        double                                       dt,
        int                                          normal
    ) -> double {
        return 2.0;
    },
    [&](
        double * __restrict__                        Q,
        const tarch::la::Vector<Dimensions,double>&  faceCentre,
        const tarch::la::Vector<Dimensions,double>&  gridCellH
    ) -> void {
      Q[0] = 0.0;
      Q[1] = 1.0;
    },
    {1.0,0.0,0.0}, //face center
    {0.05,0.05,0.05}, //patch size (in length)
    1.0, //t
    0.01, //dt
    2, //cell number per axis in patch
    3, //overlap
    2, //unknowns number
    0, //auxiliary
    3, //facenumber
    {0.0,0.0,0.0}, //origin
    Qold,
    Qnew,
    -1
  );


  /*std::cout<<std::setprecision(8);
  std::cout<<"external layers along positive"<<std::endl;
    for (int i=0;i<4;i++){
      for (int j=6;j<12;j++){
        std::cout<<Qnew[i*12+j] << " ";
      }
      std::cout<<"\n";
  }

  std::cout<<"\n";*/

  //we consider facenumber 0, i.e. face on the x negative, patch size 2, evolving quantity 2. Thus the array has 2x2x2x6=48 entries 
  double Qold2[] = {
    1.0, 2.0, 1.0, 2.0, 1.0, 2.0,   1.0, 2.0, 1.0, 2.0, 1.0, 2.0,
    1.0, 2.0, 1.0, 2.0, 1.0, 2.0,   1.0, 2.0, 1.0, 2.0, 1.0, 2.0,
    1.0, 2.0, 1.0, 2.0, 1.0, 2.0,   1.0, 2.0, 1.0, 2.0, 1.0, 2.0,
    1.0, 2.0, 1.0, 2.0, 1.0, 2.0,   1.0, 2.0, 1.0, 2.0, 1.0, 2.0
  };

  double Qnew2[] = {
    0.0, 0.0, 0.0, 0.0, 0.0, 0.0,   1.0, 2.0, 1.0, 2.0, 1.0, 2.0,   
    0.0, 0.0, 0.0, 0.0, 0.0, 0.0,   1.0, 2.0, 1.0, 2.0, 1.0, 2.0,   
    0.0, 0.0, 0.0, 0.0, 0.0, 0.0,   1.0, 2.0, 1.0, 2.0, 1.0, 2.0,   
    0.0, 0.0, 0.0, 0.0, 0.0, 0.0,   1.0, 2.0, 1.0, 2.0, 1.0, 2.0  
  };

  ::exahype2::fd::applySommerfeldConditions(
    [&](
        const double * __restrict__                  Q,
        const tarch::la::Vector<Dimensions,double>&  faceCentre,
        const tarch::la::Vector<Dimensions,double>&  gridCellH,
        double                                       t,
        double                                       dt,
        int                                          normal
    ) -> double {
        return 2.0;
    },
    [&](
        double * __restrict__                        Q,
        const tarch::la::Vector<Dimensions,double>&  faceCentre,
        const tarch::la::Vector<Dimensions,double>&  gridCellH
    ) -> void {
      Q[0] = 0.0;
      Q[1] = 1.0;
    },
    {-1.0,0.0,0.0}, //face center
    {0.05,0.05,0.05}, //patch size (in length)
    1.0, //t
    0.01, //dt
    2, //cell number per axis in patch
    3, //overlap
    2, //unknowns number
    0, //auxiliary
    0, //facenumber
    {0.0,0.0,0.0}, //origin
    Qold2,
    Qnew2,
    -1
  );

  //std::cout<<std::setprecision(8);
  /*std::cout<<"external layers along negative"<<std::endl;
    for (int i=0;i<4;i++){
      for (int j=2;j>=0;j--){
        std::cout<<Qnew2[i*12+j*2] << " ";
        std::cout<<Qnew2[i*12+j*2+1] << " ";
      }
      std::cout<<"\n";
  }*/

  //validateNumericalEquals( 1.0,  2.0 );//
}

