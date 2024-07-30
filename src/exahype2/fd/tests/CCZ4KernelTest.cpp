#include "CCZ4KernelTest.h"

#include "peano4/utils/Globals.h"
#include "peano4/utils/Loop.h"
#include "tarch/la/Vector.h"

#include "tarch/logging/Log.h"

#include "exahype2/fd/PatchUtils.h"
//#include "exahype2/CellData.h"
//#include "exahype2/fd/Functors.h"
#include "exahype2/fd/fd4/FD4.h"
#include "../applications/exahype2/ccz4/CCZ4Kernels.h"
#include "../applications/exahype2/ccz4/CCZ4Kernels.cpph"

#include "exahype2/enumerator/AoSLexicographicEnumerator.h"
#include "exahype2/enumerator/SoALexicographicEnumerator.h"
#include "exahype2/enumerator/AoSoALexicographicEnumerator.h"

#include <algorithm>
#include <iomanip>
#include <math.h>


exahype2::fd::tests::CCZ4KernelTest::CCZ4KernelTest():
  TestCase ("exahype2::fd::tests::CCZ4KernelTest") {
}


void exahype2::fd::tests::CCZ4KernelTest::run() {
  testMethod (AppleWithAppleTest);
}


void exahype2::fd::tests::CCZ4KernelTest::AppleWithAppleTest() {
  //in this test, the out layers inside the patch is not valid as the volumes 
  // in the halos can not update their first-derivatives. So only volumes with index between [3,26]^3
  // holds valid test data.

  // make no sense to test in 2d
  #if Dimensions==3
  constexpr int unknowns = 59;
  constexpr int auxiliaryVariables = 0;
  constexpr int numberOfDoFsPerAxisInCell = 30;
  constexpr int haloSize = 3;
  constexpr int numberOfDoFsPerAxisInCellWithHalos= 30+2*haloSize;

  const tarch::la::Vector<3, double> x({0.5, 0.5, 0.5});
  const tarch::la::Vector<3, double> cellSize({30.0/29.0, 30.0/29.0, 30.0/29.0});
  tarch::la::Vector<3, int> currentPosition({0,0,0});
  tarch::la::Vector<3, double> volumeCoordinates({0,0,0});
 
  double oldQWithHalos[numberOfDoFsPerAxisInCellWithHalos*numberOfDoFsPerAxisInCellWithHalos*numberOfDoFsPerAxisInCellWithHalos*(unknowns+auxiliaryVariables)]={0};
  double QOut[numberOfDoFsPerAxisInCell*numberOfDoFsPerAxisInCell*numberOfDoFsPerAxisInCell*(unknowns)]={0};

  //random assignment here, keep the same with GRChombo.
  const int scenario=1; // 0-flat spacetime test case, 1-apple with apple test

  if (scenario==0){ //flat space
    for (int xx = -haloSize; xx < numberOfDoFsPerAxisInCell+haloSize; xx++)
    for (int yy = -haloSize; yy < numberOfDoFsPerAxisInCell+haloSize; yy++)
    for (int zz = -haloSize; zz < numberOfDoFsPerAxisInCell+haloSize; zz++){
      currentPosition=gridCellIndex3d(xx,yy,zz); //current index, notice the halo is included
      for (int i=0;i<3;i++){
        volumeCoordinates(i)  = x(i)-0.5*cellSize(i);
        volumeCoordinates(i) += (double(currentPosition(i))+0.5)*cellSize(i)/numberOfDoFsPerAxisInCell; //current position
      }
      //notice if you index using this routie the index need to start from 0 not -halo!
      int CellIndexSerialised  = peano4::utils::dLinearised((currentPosition+tarch::la::Vector<Dimensions,int>(3)),
                                                              numberOfDoFsPerAxisInCellWithHalos);

      //assign quantities
      oldQWithHalos[CellIndexSerialised*(unknowns+auxiliaryVariables)+ 0] = 1.0; //\tilde{\gamma}_11
      oldQWithHalos[CellIndexSerialised*(unknowns+auxiliaryVariables)+ 3] = 1.0; //\tilde{\gamma}_22
      oldQWithHalos[CellIndexSerialised*(unknowns+auxiliaryVariables)+ 5] = 1.0; //\tilde{\gamma}_33
      oldQWithHalos[CellIndexSerialised*(unknowns+auxiliaryVariables)+16] = 1.0; //\alpha
      oldQWithHalos[CellIndexSerialised*(unknowns+auxiliaryVariables)+54] = 1.0; //\phi
    }
  } else if (scenario==1){ //apple with apple
    for (int xx = -haloSize; xx < numberOfDoFsPerAxisInCell+haloSize; xx++)
    for (int yy = -haloSize; yy < numberOfDoFsPerAxisInCell+haloSize; yy++)
    for (int zz = -haloSize; zz < numberOfDoFsPerAxisInCell+haloSize; zz++){
      currentPosition=gridCellIndex3d(xx,yy,zz); //current index, notice the halo is included
      for (int i=0;i<3;i++){
        volumeCoordinates(i)  = x(i)-0.5*cellSize(i);
        volumeCoordinates(i) += (double(currentPosition(i))+0.5)*cellSize(i)/numberOfDoFsPerAxisInCell; //current position
      }

      int CellIndexSerialised  = peano4::utils::dLinearised((currentPosition+tarch::la::Vector<Dimensions,int>(3)),
                                                              numberOfDoFsPerAxisInCellWithHalos);
      //assign quantities
      double **g, **K;
      double phi, trK;
      double Theta, Gamma[3];
      double alpha, beta[3], b[3];
      g = new double* [3]; K = new double* [3];
      for (int l=0;l<3;l++){
        g[l]=new double[3]; K[l]=new double[3];
      }


      prepareFieldData(g, phi, K, trK, Theta, Gamma, alpha, beta, b,
        volumeCoordinates(0), volumeCoordinates(1), volumeCoordinates(2));

      //for (int i=0;i<3;i++)
      //for (int j=0;j<3;j++) 
      //double a=1.0/NAN;
      //std::cout<<1.0/NAN<<std::endl;

      oldQWithHalos[CellIndexSerialised*(unknowns+auxiliaryVariables)+ 0] = g[0][0];  // tilde{gamma}_ij
      oldQWithHalos[CellIndexSerialised*(unknowns+auxiliaryVariables)+ 1] = g[0][1];
      oldQWithHalos[CellIndexSerialised*(unknowns+auxiliaryVariables)+ 2] = g[0][2];
      oldQWithHalos[CellIndexSerialised*(unknowns+auxiliaryVariables)+ 3] = g[1][1];
      oldQWithHalos[CellIndexSerialised*(unknowns+auxiliaryVariables)+ 4] = g[1][2];
      oldQWithHalos[CellIndexSerialised*(unknowns+auxiliaryVariables)+ 5] = g[2][2];

      oldQWithHalos[CellIndexSerialised*(unknowns+auxiliaryVariables)+ 6] = K[0][0];  // tilde{A}_ij  
      oldQWithHalos[CellIndexSerialised*(unknowns+auxiliaryVariables)+ 7] = K[0][1];
      oldQWithHalos[CellIndexSerialised*(unknowns+auxiliaryVariables)+ 8] = K[0][2];
      oldQWithHalos[CellIndexSerialised*(unknowns+auxiliaryVariables)+ 9] = K[1][1];
      oldQWithHalos[CellIndexSerialised*(unknowns+auxiliaryVariables)+10] = K[1][2];
      oldQWithHalos[CellIndexSerialised*(unknowns+auxiliaryVariables)+11] = K[2][2];

      oldQWithHalos[CellIndexSerialised*(unknowns+auxiliaryVariables)+12] = Theta;    // Theta
      oldQWithHalos[CellIndexSerialised*(unknowns+auxiliaryVariables)+13] = Gamma[0]; // hat{Gamma}^i
      oldQWithHalos[CellIndexSerialised*(unknowns+auxiliaryVariables)+14] = Gamma[1];
      oldQWithHalos[CellIndexSerialised*(unknowns+auxiliaryVariables)+15] = Gamma[2];

      oldQWithHalos[CellIndexSerialised*(unknowns+auxiliaryVariables)+16] = alpha;    // alpha
      oldQWithHalos[CellIndexSerialised*(unknowns+auxiliaryVariables)+17] = beta[0];  // beta^i
      oldQWithHalos[CellIndexSerialised*(unknowns+auxiliaryVariables)+18] = beta[1];
      oldQWithHalos[CellIndexSerialised*(unknowns+auxiliaryVariables)+19] = beta[2];
      oldQWithHalos[CellIndexSerialised*(unknowns+auxiliaryVariables)+20] = b[0];     // b^i
      oldQWithHalos[CellIndexSerialised*(unknowns+auxiliaryVariables)+21] = b[1];
      oldQWithHalos[CellIndexSerialised*(unknowns+auxiliaryVariables)+22] = b[2];

      oldQWithHalos[CellIndexSerialised*(unknowns+auxiliaryVariables)+53] = trK;      // K
      oldQWithHalos[CellIndexSerialised*(unknowns+auxiliaryVariables)+54] = phi;      // phi

      delete g,K;
    }
  }

  ::exahype2::CellData patchData(oldQWithHalos, x, cellSize, 0, 0.01, QOut);

  //set parameters
  static constexpr double CCZ4KOSigma=8.0;
  static constexpr double CCZ4itau = 1.0;
  static constexpr double CCZ4k1 = 0.1;
  static constexpr double CCZ4k2 = 0.0;
  static constexpr double CCZ4k3 = 0.5;
  static constexpr double CCZ4eta = 1.0;
  static constexpr double CCZ4f = 0.75;
  static constexpr double CCZ4g = 0.0;
  static constexpr double CCZ4xi = 1.0;
  static constexpr double CCZ4e = 1.0;
  static constexpr double CCZ4c = 1.0;
  static constexpr double CCZ4mu = 0.2;
  static constexpr double CCZ4ds = 1.0;
  static constexpr double CCZ4sk = 1.0;
  static constexpr double CCZ4bs = 1.0;
  static constexpr int CCZ4LapseType = 1;
  static constexpr int CCZ4SO = 1;

  //then calculate the derivatives first
  //there is a potential seg error if number of auxiliary variables is not zero.
  //but without auxiliary variables everything should be fine. 
  //notice only the volumes inside the patch get updated
  ::exahype2::fd::fd4::reconstruct_first_derivatives(
    patchData,
    numberOfDoFsPerAxisInCell,
    haloSize,
    unknowns,
    auxiliaryVariables
  );

  //call the kernel
  ::exahype2::fd::fd4::timeStep_patchwise_heap_functors(
    patchData,
    numberOfDoFsPerAxisInCell,
    haloSize,
    unknowns,
    auxiliaryVariables,
    CCZ4KOSigma,
     false ,
     true ,
     true ,
    false, // do not copy the old solution. Abuse this to get the RHS directly
    ::exahype2::fd::fd4::DifferentialSourceTermVariant::CentralDifferencesWithLopsidedAdvection,
    [&](
      const double * __restrict__ Q,
      const tarch::la::Vector<Dimensions,double>&  faceCentre,
      const tarch::la::Vector<Dimensions,double>&  volumeH,
      double                                       t,
      double                                       dt,
      int                                          normal,
      double * __restrict__ F
    )->void {
      
    },
    [&](
      const double * __restrict__                  Q,
      const double * __restrict__     deltaQ,
      const tarch::la::Vector<Dimensions,double>&  faceCentre,
      const tarch::la::Vector<Dimensions,double>&  volumeH,
      double                                       t,
      double                                       dt,
      int                                          normal,
      double * __restrict__ BTimesDeltaQ
    )->void {   
      double gradQSerialised[unknowns*3];
      for (int i=0; i<unknowns; i++) {
        gradQSerialised[i+0*unknowns] = 0.0;
        gradQSerialised[i+1*unknowns] = 0.0;
        gradQSerialised[i+2*unknowns] = 0.0;

        gradQSerialised[i+normal*unknowns] = deltaQ[i];
      }
      //for (int i=NumberOfUnknowns; i<NumberOfUnknowns+NumberOfAuxiliaryVariables; i++) {
      //  BgradQ[i] = 0.0;
      //}
      applications::exahype2::ccz4::ncp(BTimesDeltaQ, Q, gradQSerialised, normal, CCZ4LapseType, CCZ4ds, CCZ4c, CCZ4e, CCZ4f, CCZ4bs, CCZ4sk, CCZ4xi, CCZ4mu, CCZ4SO);     
    },
    [&](
      const double * __restrict__ Q,
      const tarch::la::Vector<Dimensions,double>&  volumeX,
      const tarch::la::Vector<Dimensions,double>&  volumeH,
      double                                       t,
      double                                       dt,
      double * __restrict__ S
    )->void {
      tarch::memset(S, 0, unknowns*sizeof(double));
      applications::exahype2::ccz4::source(S,Q, CCZ4LapseType, CCZ4ds, CCZ4c, CCZ4e, CCZ4f, CCZ4bs, CCZ4sk, CCZ4xi, CCZ4itau, CCZ4eta, CCZ4k1, CCZ4k2, CCZ4k3, CCZ4SO);   
    }
  );

  //examine the output, notice voulme index should between [3,26]^3
  //tarch::la::Vector<3, int> volumeIndex({15,15,15});
  //tarch::la::Vector<3, int> volumeIndex({8,20,17});
  tarch::la::Vector<3, int> volumeIndex({23,9,18});

  //15,15,15
  //double dataArrayOfGRChombo[25]={-1.01737, -0.952962, 1.5358, 2.07569, -0.274332, -1.34847, 4.52609, 1.69924, 0.581672, -1.01265, 1.01734, -3.28122, -2.00355, 3.42196, 23.2832, 0.698199, 26.806, -2.87506, -1.21849, -4.32589, 4.44389, 18.9259, -2.17062, -11.1472, -0.0253941};
  //8,20,17
  //double dataArrayOfGRChombo[25]={-1.39787, -0.754864, 1.71128, 1.41365, 1.63204, -0.675397, 6.08035, 2.31647, 1.3964, -1.59352, 1.60353, -4.44641, -0.945263, 2.65077, 36.1263, 2.48858, 36.1493, -5.74057, -4.27386, -8.90803, 3.83137, 29.8071, -3.80586, -10.7919, -0.0296434}; 
  //23,9,18
  double dataArrayOfGRChombo[25]={-3.1866, 0.365012, 3.35262, 0.0372327, 3.36914, 1.34624, 4.88302, 3.09191, 0.296349, -3.06936, 0.936401, -2.12479, 3.00939, 9.47752, 35.5824, -9.94656, 42.4051, -0.753163, 3.48535, -2.7644, 10.7661, 34.7114, -11.501, -9.21531, -0.359624};

  int checkingQ=0;
  exahype2::enumerator::AoSLexicographicEnumerator QInEnumerator (1,numberOfDoFsPerAxisInCell,haloSize,unknowns,auxiliaryVariables);
  exahype2::enumerator::AoSLexicographicEnumerator QOutEnumerator(1,numberOfDoFsPerAxisInCell,0,unknowns,0); //halosize and auxiliary variables are both 0,
  for (int i=0;i<3;i++){
    volumeCoordinates(i)  = patchData.cellCentre[0](i)-0.5*patchData.cellSize[0](i);
    volumeCoordinates(i) += (double(volumeIndex(i))+0.5)*patchData.cellSize[0](i)/QOutEnumerator._numberOfDoFsPerAxisInCell;
  }
  
  /*validateNumericalEqualsWithParams3(0, 1, 
    patchData.toString(), 
    patchData.QOut[0][QOutEnumerator(0, volumeIndex, checkingQ)],
    volumeCoordinates
  );*/
  //for (checkingQ=0;checkingQ<unknowns;checkingQ++){
   // std::cout<<patchData.QOut[0][QOutEnumerator(0, volumeIndex, checkingQ)]<<"\t";
  //}
  //std::cout<<std::endl;
  //std::cout << "Index: " << volumeIndex << " Coordinates: " << volumeCoordinates << std::endl;
  //std::cout << std::fixed <<std::setprecision(6); 
  /*for (checkingQ=0;checkingQ<23;checkingQ++){
    std::cout<<"dtQ["<<checkingQ<<"]\t=  "<<patchData.QOut[0][QOutEnumerator(0, volumeIndex, checkingQ)]<<std::endl;
  }
  std::cout<<"dtQ[53]\t=  "<<patchData.QOut[0][QOutEnumerator(0, volumeIndex, 53)]<<std::endl;
  std::cout<<"dtQ[54]\t=  "<<patchData.QOut[0][QOutEnumerator(0, volumeIndex, 54)]<<std::endl;
  std::cout<<"dtchi=2*Q[54]*dtQ[54]\t=  "
    <<2*patchData.QIn[0][QInEnumerator(0, volumeIndex, 54)]*patchData.QOut[0][QOutEnumerator(0, volumeIndex, 54)]
    <<std::endl;*/

  /*for (checkingQ=0;checkingQ<23;checkingQ++){
    std::cout<<"dtQ["<<checkingQ<<"]\t=  "<<patchData.QOut[0][QOutEnumerator(0, volumeIndex, checkingQ)]
      <<"\tGRChombo result = "<<dataArrayOfGRChombo[checkingQ]<<"\t Ratio = "
      <<100*patchData.QOut[0][QOutEnumerator(0, volumeIndex, checkingQ)]/dataArrayOfGRChombo[checkingQ]
      <<"%"<<std::endl;
  }

  std::cout<<"dtQ[53]\t=  "<<patchData.QOut[0][QOutEnumerator(0, volumeIndex, 53)]
    <<"\tGRChombo result = "<<dataArrayOfGRChombo[23]<<"\t Ratio = "
      <<100*patchData.QOut[0][QOutEnumerator(0, volumeIndex, 53)]/dataArrayOfGRChombo[23]
      <<"%"<<std::endl;

  double dtphi=patchData.QOut[0][QOutEnumerator(0, volumeIndex, 54)];
  std::cout<<"dtchi=2*Q[54]*dtQ[54]\t=  "
    <<2*patchData.QIn[0][QInEnumerator(0, volumeIndex, 54)]*dtphi
    <<"\tGRChombo result = "<<dataArrayOfGRChombo[24]<<"\t Ratio = "
    <<100*2*patchData.QIn[0][QInEnumerator(0, volumeIndex, 54)]*dtphi/dataArrayOfGRChombo[24]
    <<"%"<<std::endl;*/

  /*for (checkingQ=0;checkingQ<23;checkingQ++){
    validateNumericalEqualsWithParams3(0, 1, 
      patchData.toString(), 
      patchData.QOut[0][QOutEnumerator(0, volumeIndex, checkingQ)],
      volumeCoordinates
    );
  }*/


  #endif
}


void exahype2::fd::tests::CCZ4KernelTest::prepareFieldData(
  double** g, 
  double& phi, 
  double** K, 
  double& trK,
  double& Theta,
  double* Gamma, 
  double& alpha, 
  double* beta,
  double* b,
  double x, double y, double z
){
  double g_UU[3][3];

  g[0][0] = 1.36778 + 2.39731 * x + 4.53541 * x * x +
            19.9771 * x * y * y * y + 6.13801 * y * z +
            5.65185 * z * z + 9.35842 * z * z * z * z;
  g[0][1] = -0.07646 - 0.48786 * x - 0.75098 * x * x -
            1.73683 * x * y * y * y + 1.71676 * y * z +  
            1.03662 * z * z + 0.35630 * z * z * z * z;
  g[0][2] = -0.10083 + 0.12445 * x - 1.26649 * x * x -
            1.95052 * x * y * y * y + 0.73091 * y * z -
            1.49835 * z * z - 2.39024 * z * z * z * z;
  g[1][1] = 0.84072 + 2.31163 * x + 3.32275 * x * x +
            15.1662 * x * y * y * y + 8.48730 * y * z +
            3.05098 * z * z + 17.8448 * z * z * z * z;
  g[1][2] = -0.42495 - 0.33464 * x - 0.47012 * x * x -
            7.38477 * x * y * y * y + 0.41896 * y * z -
            1.36394 * z * z + 5.25894 * z * z * z * z;
  g[2][2] = 0.60995 + 1.30428 * x + 3.86237 * x * x +
            22.7614 * x * y * y * y + 6.93818 * y * z +
            4.39250 * z * z + 19.0244 * z * z * z * z;
  g[1][0] = g[0][1];
  g[2][0] = g[0][2];
  g[2][1] = g[1][2];

  double detg = g[0][0] * g[1][1] * g[2][2] +
                2 * g[0][1] * g[0][2] * g[1][2] -
                g[0][0] * g[1][2] * g[1][2] -
                g[1][1] * g[0][2] * g[0][2] -
                g[2][2] * g[0][1] * g[0][1];
  g_UU[0][0] = (g[1][1] * g[2][2] - g[1][2] * g[1][2]) / detg;
  g_UU[0][1] = (g[0][2] * g[1][2] - g[0][1] * g[2][2]) / detg;
  g_UU[0][2] = (g[0][1] * g[1][2] - g[0][2] * g[1][1]) / detg;
  g_UU[1][1] = (g[0][0] * g[2][2] - g[0][2] * g[0][2]) / detg;
  g_UU[1][2] = (g[0][1] * g[0][2] - g[0][0] * g[1][2]) / detg;
  g_UU[2][2] = (g[0][0] * g[1][1] - g[0][1] * g[0][1]) / detg;
  g_UU[1][0] = g_UU[0][1];
  g_UU[2][0] = g_UU[0][2];
  g_UU[2][1] = g_UU[1][2];

  phi=std::pow(detg,-1./6.);
  double phisq=phi*phi;
  trK=0;
  K[0][0] = -0.16238 - 0.74295 * x + 0.51595 * x * x -
            6.60239 * x * y * y * y - 0.76401 * y * z -
            1.81131 * z * z - 3.88228 * z * z * z * z;
  K[0][1] = 0.15054 - 0.60088 * x - 0.15428 * x * x +
            3.16779 * x * y * y * y - 2.00687 * y * z -
            1.35442 * z * z - 0.67601 * z * z * z * z;
  K[0][2] = -0.02174 - 0.36243 * x + 0.81531 * x * x +
            4.34918 * x * y * y * y + 0.90419 * y * z -
            0.85088 * z * z - 6.45097 * z * z * z * z;
  K[1][1] = -0.47653 - 0.43889 * x + 0.87342 * x * x +
            4.24684 * x * y * y * y + 0.26290 * y * z +
            1.90095 * z * z + 3.69515 * z * z * z * z;
  K[1][2] = 0.37472 + 0.03657 * x - 0.10327 * x * x -
            0.95744 * x * y * y * y - 1.20800 * y * z -
            0.43064 * z * z - 0.25419 * z * z * z * z;
  K[2][2] = 0.34184 + 0.21495 * x - 0.73195 * x * x +
            7.81626 * x * y * y * y + 2.48359 * y * z +
            1.89657 * z * z - 4.10980 * z * z * z * z;
  K[1][0] = K[0][1];
  K[2][0] = K[0][2];
  K[2][1] = K[1][2];
  for (int i=0;i<3;i++)
  for (int j=0;j<3;j++) trK += g_UU[i][j]*K[i][j];

  for (int i=0;i<3;i++)
  for (int j=0;j<3;j++) {
    g[i][j]=phisq*g[i][j]; //\tilde{\gamma}_ij
    K[i][j]=phisq*(K[i][j]- 1./3. * trK * g[i][j]/phisq); //\tilde{A}_ij
  }

  Theta = 0.27579 + 0.25791 * x + 1.40488 * x * x +
          5.68276 * x * y * y * y +
          3.04325 * y * z + 1.81250 * z * z +
          1.01832 * z * z * z * z;
  Gamma[0] = -0.49482 + 0.89227 * x + 0.05571 * x * x -
             5.38570 * x * y * y * y + 0.13979 * y * z -
             0.68588 * z * z - 4.39964 * z * z * z * z;
  Gamma[1] = -0.09082 - 0.31017 * x + 1.06980 * x * x +
             7.81524 * x * y * y * y - 1.65016 * y * z -
             0.53352 * z * z - 3.20997 * z * z * z * z;
  Gamma[2] = -0.42367 + 0.03891 * x - 0.87898 * x * x +
             6.67657 * x * y * y * y - 3.44662 * y * z -
             0.19655 * z * z + 2.97524 * z * z * z * z;

  alpha = 0.73578 + 0.36898 * x + 0.64348 * x * x +
          9.33487 * x * y * y * y +
          0.99469 * y * z + 0.20515 * z * z +
          8.88385 * z * z * z * z;


  beta[0] = 0.00000 + 0.18795 * x - 0.52389 * x * x -
            4.14079 * x * y * y * y +
            0.73135 * y * z - 0.27057 * z * z +
            3.24187 * z * z * z * z;
  beta[1] = 0.00000 - 0.30316 * x - 0.15184 * x * x -
            0.48815 * x * y * y * y +
            2.45991 * y * z - 0.79248 * z * z +
            7.14007 * z * z * z * z;
  beta[2] = 0.00000 + 0.68835 * x - 0.52219 * x * x -
            7.50449 * x * y * y * y -
            2.35372 * y * z - 0.21476 * z * z +
            4.36363 * z * z * z * z;

  b[0] = -0.26928 + 0.35045 * x - 0.48884 * x * x +
         2.72465 * x * y * y * y - 2.59022 * y * z -
         0.27384 * z * z + 0.38748 * z * z * z * z;
  b[1] = 0.40234 + 0.26741 * x + 1.94822 * x * x -
         0.78276 * x * y * y * y + 2.12346 * y * z +
         0.69086 * z * z - 4.47639 * z * z * z * z;
  b[2] = 0.40313 + 0.00569 * x - 1.12452 * x * x -
         5.49255 * x * y * y * y - 2.21932 * y * z +
         0.49523 * z * z + 1.29460 * z * z * z * z;
}