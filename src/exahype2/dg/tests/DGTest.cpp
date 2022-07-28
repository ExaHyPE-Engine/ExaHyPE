#include "../../dg/tests/DGTest.h"

/*
 * We use precomputed values for lagrange polynomials as basis function
 *  on Gauss-Legendre nodes
 */
exahype2::dg::tests::DGTest::DGTest():
TestCase ("exahype2::dg::DGTest"),
QuadraturePoints {0.069431844202973713731097404888714663684368133544921875,0.33000947820757187134432797392946667969226837158203125,0.66999052179242812865567202607053332030773162841796875,0.9305681557970262307577513638534583151340484619140625},
QuadratureWeights {
  0.1739274225687269248563637802362791262567043304443359375,0.32607257743127304738806060413480736315250396728515625,
  0.32607257743127304738806060413480736315250396728515625,0.1739274225687269248563637802362791262567043304443359375},
MassMatrix {
    0.1739274225687269248563637802362791262567043304443359375,0.0,0.0,0.0,
    0.0,0.32607257743127304738806060413480736315250396728515625,0.0,0.0,
    0.0,0.0,0.32607257743127304738806060413480736315250396728515625,0.0,
    0.0,0.0,0.0,0.1739274225687269248563637802362791262567043304443359375},
StiffnessOperator {
  -1.1590524262142825051569161587394773960113525390625,-0.494037528020547400675610560938366688787937164306640625,0.214358954361956122180998818294028751552104949951171875,-0.201974321866382811041518152705975808203220367431640625,
  1.69062826161228674237690938753075897693634033203125,-0.250693983347795967819848783619818277657032012939453125,-0.95909046573029943516530693159438669681549072265625,0.73355015726438665968345276269246824085712432861328125,
  -0.7335501572643867707057552252081222832202911376953125,0.95909046573029943516530693159438669681549072265625,0.250693983347795634752941396072856150567531585693359375,-1.690628261612286298287699537468142807483673095703125,
  0.2019743218663829775749718464794568717479705810546875,-0.21435895436195628871445251206750981509685516357421875,0.494037528020547622720215485969674773514270782470703125,1.159052426214282949246126008802093565464019775390625},
DerivativeOperator {
    -6.6640004727045631938153746887110173702239990234375,9.7203088313703940315235740854404866695404052734375,-4.21756469699035818621268845163285732269287109375,1.161256338324529568950538305216468870639801025390625,
    -1.5151152295984677831341969067580066621303558349609375,-0.7688287844464174458636307463166303932666778564453125,2.941340462561433444221847821609117090702056884765625,-0.6573964485165488813578349436284042894840240478515625,
    0.65739644851654832624632263105013407766819000244140625,-2.941340462561433444221847821609117090702056884765625,0.768828784446416335640606121160089969635009765625,1.515115229598468449268011681851930916309356689453125,
    -1.161256338324528680772118605091236531734466552734375,4.21756469699035729803426875150762498378753662109375,-9.720308831370392255166734685190021991729736328125,6.66400047270456497017221408896148204803466796875},
BasisFunctionValuesLeft {
  1.5267881254572663873858573424513451755046844482421875,-0.8136324494869271450880887641687877476215362548828125,
  0.40076152031165046540905905203544534742832183837890625,-0.11391719628198997138479597879268112592399120330810546875} ,
BasisFunctionValuesRight {
    -0.113917196281990040773735017864964902400970458984375,0.400761520311650742964815208324580453336238861083984375,
    -0.813632449486927811221903539262712001800537109375,1.5267881254572674976088819676078855991363525390625}

/*
  ,MassMatrixDiagonal1d{
      0.1739274225687269248563637802362791262567043304443359375,
      0.32607257743127304738806060413480736315250396728515625,
      0.32607257743127304738806060413480736315250396728515625,
      0.1739274225687269248563637802362791262567043304443359375
    }
*/
{
#if Dimensions==2
  file_stream.open("src/exahype2/dg/tests/results2D.log");
#else
  file_stream.open("src/exahype2/dg/tests/results3D.log");
#endif
}


void exahype2::dg::tests::DGTest::run(){
  testMethod(UtilsTest);
  testMethod(runDGEuler);
  testMethod(runDGElastic);
}

void exahype2::dg::tests::DGTest::UtilsTest(){

  const int order = 3;
  const int nodesPerAxis = order+1;
  const int nodesPerCell = getNodesPerCell(nodesPerAxis);

  validateEquals(nodesPerCell, std::pow(nodesPerAxis,Dimensions));

  tarch::la::Vector<Dimensions,int> strides = getStrides(nodesPerAxis);
  validateEquals(strides[Dimensions-1], std::pow(nodesPerAxis,Dimensions-1));

  tarch::la::Vector<Dimensions,int> firstIndex = getIndex(0, strides);
  for(int i=0; i<Dimensions; i++){
    validateEquals(firstIndex[i], 0);
  }

  tarch::la::Vector<Dimensions,int> lastIndex = getIndex(nodesPerCell-1, strides);
  for(int i=0; i<Dimensions; i++){
    validateEquals(lastIndex[i], nodesPerAxis-1);
  }

  /*
   * testing gradient by running it on a cell with constant values.
   * since the values are constant, the gradient should be zero everywhere
   */
  // 1 unknown, 1 auxiliary variable
  double cellQin[2*nodesPerCell];
  double gradientValues[Dimensions];
  for(int i=0; i<2*nodesPerCell; i++){
    cellQin[i] = 1.0;
  }

  for(int node=0; node<nodesPerCell; node++){
    computeGradient(
      cellQin,
      DerivativeOperator,
      10, //invDx
      nodesPerAxis,
      2, //strideQ
      node,
      gradientValues
      );
    for(int var=0; var<Dimensions; var++){
      validateNumericalEquals(gradientValues[var], 0.0);
    }
  }

  /*
   * testing cellIndexToHullIndex. First node should logically get the first node on
   * each dimensions, last one should get last one on each dimension
   */
  /*
  for(int d=0; d<Dimensions; d++){
    for(int lr=0; lr<2; lr++){
      int firstCellIndex = cellIndexToHullIndex(
          firstIndex,
          d,
          lr,
          nodesPerAxis
          );

      int lastCellIndex = cellIndexToHullIndex(
          lastIndex,
          d,
          lr,
          nodesPerAxis
          );

      validateNumericalEquals((float)(firstCellIndex%nodesPerAxis), 0.0);
      validateNumericalEquals((float)(lastCellIndex%nodesPerAxis), (float)nodesPerAxis-1.0);
    }
  }
  */


  /*
   * testing applyBoundaryConditions to make sure the nodes are written in the right positions
   * and with the right values.
   */

  const int nodesPerFace = nodesPerCell/nodesPerAxis;
  int strideQ = 4; // in order to test Q, x, t, normal
  double QFaces[2*2*nodesPerFace*strideQ];
  double* QFacesL = &QFaces[0]; //left face construct
  double* QFacesR = &QFaces[2*nodesPerFace*strideQ]; //right face construct

  //utility parameters
#if Dimensions==2
  const tarch::la::Vector<Dimensions, double> x({0.0, 0.0});
  const tarch::la::Vector<Dimensions, double> cellSize({1.0, 1.0});
#else
  const tarch::la::Vector<Dimensions, double> x({0.0, 0.0, 0.0});
  const tarch::la::Vector<Dimensions, double> cellSize({1.0, 1.0, 1.0});
#endif

  double t = 3.14;
  double dt = 0.0;


  for(int node=0; node<nodesPerFace; node++){
    for(int i=0; i<strideQ; i++){
      /*
       * Initialize right nodes of left face and left nodes of right face
       */
      QFacesL[(2*node+1)*strideQ+i] = i+1.0;
      QFacesR[2*node*strideQ+i]     = i+1.0;
    }
  }

  applyBoundaryConditions(
    [&](
      const double * __restrict__                  Qinside,
      double * __restrict__                        Qoutside,
      const tarch::la::Vector<Dimensions,double>&  x,
      double                                       t,
      double                                       dt,
      int                                          normal
    ) -> void {
      testBoundaryConditions(Qinside,Qoutside,x,t,dt,normal);
    },
    x, //faceCentre
    cellSize,
    t,
    dt,
    order,
    2, //unknowns
    2, //aux vars
    0, //faceNumber
    QuadraturePoints,
    QFacesL
  );

  applyBoundaryConditions(
    [&](
      const double * __restrict__                  Qinside,
      double * __restrict__                        Qoutside,
      const tarch::la::Vector<Dimensions,double>&  x,
      double                                       t,
      double                                       dt,
      int                                          normal
    ) -> void {
      testBoundaryConditions(Qinside,Qoutside,x,t,dt,normal);
    },
    x, //faceCentre
    cellSize,
    t,
    dt,
    order,
    2, //unknowns
    2, //aux vars
    Dimensions, //faceNumber
    QuadraturePoints,
    QFacesR
  );

  for(int node=0; node<nodesPerFace; node++){

    //values on left face
    validateNumericalEquals( QFacesL[2*node*strideQ+0], 1.0);
    validateNumericalEquals( QFacesL[2*node*strideQ+1], 0.0);
    validateNumericalEquals( QFacesL[2*node*strideQ+2], 3.14);
    validateNumericalEquals( QFacesL[2*node*strideQ+3], 0);

    //values on right face
    validateNumericalEquals( QFacesR[(2*node+1)*strideQ+0], 1.0);
    validateNumericalEquals( QFacesR[(2*node+1)*strideQ+1], 0.0);
    validateNumericalEquals( QFacesR[(2*node+1)*strideQ+2], 3.14);
    validateNumericalEquals( QFacesR[(2*node+1)*strideQ+3], 0);

  }


  /*
   * testing getMaxEigenvalue
   */

  //reusing array from gradient test
  for(int i=0; i<nodesPerCell; i++){
    cellQin[2*i+0] = i;
    cellQin[2*i+1] = i+1;
  }

  double maxEval = getMaxEigenvalue(
      [&](
          const double * __restrict__                 Q,
          const tarch::la::Vector<Dimensions,double>& x,
          double                                      t,
          double                                      dt,
          int                                         normal
          ) -> double {
            return testEigenvalue(Q,x,t,dt,normal);
          },
      cellQin,
      order,
      2, //strideQ
      x,
      t,
      0.1 //dt
      );

  validateNumericalEquals(maxEval, (Dimensions-1)*(nodesPerCell-1));



  /*
   * Testing projection to faces by creating fully constant cell and ensuring that the
   * projected values are also the same constant.
   */

  strideQ = 2;

  //values on the faces
  double QxL[2*nodesPerFace*strideQ];
  double QxR[2*nodesPerFace*strideQ];
  double QyL[2*nodesPerFace*strideQ];
  double QyR[2*nodesPerFace*strideQ];
#if Dimensions==3
  double QzL[2*nodesPerFace*strideQ];
  double QzR[2*nodesPerFace*strideQ];
#endif

  for(int i=0; i<nodesPerCell; i++){
    cellQin[2*i+0] = 0.0;
    cellQin[2*i+1] = 0.0;
  }

  for(int i=0; i<2*nodesPerFace*strideQ;i++){
    QxL[i] = 0.0;
    QxR[i] = 0.0;
    QyL[i] = 0.0;
    QyR[i] = 0.0;
#if Dimensions==3
    QzL[i] = 0.0;
    QzR[i] = 0.0;
#endif
  }


  projectVolumetricDataOntoFaces(
    cellQin,
    order,
    strideQ,
    BasisFunctionValuesLeft,
    QxL,
    QxR,
    QyL,
    QyR
#if Dimensions==3
    ,QzL,
    QzR
#endif
  );

  double* QF[2*Dimensions];
  QF[0] = QxL;
  QF[1] = QxR;
  QF[2] = QyL;
  QF[3] = QyR;
#if Dimensions==3
  QF[4] = QzL;
  QF[5] = QzR;
#endif

  /*
  file_stream << "Faces after projection: \n";
  for(int d=0; d<2*Dimensions; d++){
    file_stream << "Face " << d << ": \n";
    int lr = 1 - d%2;


    for(int n=0; n<2*nodesPerFace; n++){
      file_stream << "Node " << n << ": [";
      for(int var=0; var<strideQ; var++){
//        file_stream << QF[d][(2*n+lr)*strideQ+var] << ", ";
        file_stream << QF[d][n*strideQ+var] << ", ";
      }
      file_stream << "]\n";
    }
  }
  */


  //Testing integrateOverRiemannSolutionsAndAddToVolume_GaussLegendre
/*
  for(int i=0; i<nodesPerFace*strideQ;i++){
    QxL[i] = 1.0;
    QxR[i] = 1.0;
    QyL[i] = 0.0;
    QyR[i] = 0.0;
#if Dimensions==3
    QzL[i] = 0.0;
    QzR[i] = 0.0;
#endif
  }

  integrateOverRiemannSolutionsAndAddToVolume_GaussLegendre(
      QxL,
      QxR,
      QyL,
      QyR,
#if Dimensions==3
      QzL,
      QzR,
#endif
      order,
      2, //unk
      0, //aux
      cellSize,
//      0.1, //dt
      BasisFunctionValuesLeft,
      QuadratureWeights,
      cellQin
      );

  file_stream << "CellQ values after adding riemann Integral:\n";
#if Dimensions==2
  for (int x=0; x<order+1; x++){
    for (int y=0; y<order+1; y++){
      file_stream << "node (" << x << ", " << y << "): [";
      for(int var=0; var<strideQ; var++){
        file_stream << cellQin[(y*nodesPerAxis+x)*strideQ + var] << ", ";
      }
      file_stream << "]\n";

    }
  }
#else
  for (int x=0; x<order+1; x++){
    for (int y=0; y<order+1; y++){
      for (int z=0; z<order+1; z++){

        file_stream << "node (" << x << ", " << y << ", " << z <<"): [";
        for(int var=0; var<strideQ; var++){
          file_stream << cellQin[(z*nodesPerFace+y*nodesPerAxis+x)*strideQ + var] << ", ";
        }
        file_stream << "]\n";

      }
    }
  }
#endif
*/
}


/*
 * In one step of a DG solver, following substeps are taken:
 * 1) computation of the integral within the cell, for this all contributions within the cell
 *    are computed, so the local source as well as flux and ncp contributions of neighbouring
 *    nodes are computed and added to each node.
 * 2) These local solutions are extrapolated to the cell surface and exchanged with the
 *    neigbouring nodes
 * 3) Using extrapolated values on the surface of the own and neighbouring cells, the flux
 *    between the two cells is computed (here a rusanov flux is used)
 * 4) Finally, the contributions from the neighbouring cells are added to each node in a cell
 */
void exahype2::dg::tests::DGTest::runDGEuler(){


  file_stream << "Starting Euler test in " << Dimensions << " dimensions\n\n";

  // geometry
#if Dimensions==2
  const tarch::la::Vector<2, double> x({0.0, 0.0});
  const tarch::la::Vector<2, double> cellSize({1.0, 1.0});
#else
  const tarch::la::Vector<3, double> x({0.0, 0.0, 0.0});
  const tarch::la::Vector<3, double> cellSize({1.0, 1.0, 1.0});
#endif
  const double dx = 0.01;
  const double t  = 0.0;
  const double dt = 0.01;

  //using euler equations as example
  const int unknowns = 5;
  const int auxiliaryVariables = 0;
  const int order = 3;

  //Parameter initialisation and memory allocation
  const int nodesPerAxis  = order+1;
  const int nodesPerCell  = getNodesPerCell(nodesPerAxis);
  const int nodesPerFace  = nodesPerCell/nodesPerAxis;
  const int strideQ       = unknowns+auxiliaryVariables;

  //values in the cell at the beginning and end
  double* QIn = new double[nodesPerCell*strideQ];
  double* QOut = new double[nodesPerCell*strideQ];
  //values on the faces
  double* QLeft = new double[2*nodesPerFace*strideQ];
  double* QRight = new double[2*nodesPerFace*strideQ];
  double* QDown = new double[2*nodesPerFace*strideQ];
  double* QUp = new double[2*nodesPerFace*strideQ];
#if Dimensions==3
  double* QFront = new double[2*nodesPerFace*strideQ];
  double* QBack = new double[2*nodesPerFace*strideQ];
#endif

  //values of the rusanov fluxes on the faces
  double* QHullForRiemann = new double[2*Dimensions*2*nodesPerFace*strideQ];


  ::exahype2::CellData cellData(
      QIn,
      x,
      cellSize,
      t,
      dt,
      QOut
      );


  //setting initial conditions

  file_stream << "Initial conditions: \n";
  for(int i=0; i<nodesPerCell; i++){
    eulerInitial(&QIn[i*strideQ]);
    file_stream << "node " << i << " [";
    for(int var=0; var<strideQ; var++){
      file_stream << QIn[i*strideQ+var] << ", ";
    }
    file_stream << "]\n";
  }

  //for later comparisons
  double QInit[strideQ];
  eulerInitial(QInit);

    cellIntegral_patchwise_GaussLegendre_functors(
      cellData,
      order,
      unknowns,
      auxiliaryVariables,
      [&](
          const double * __restrict__                  Q,
          const tarch::la::Vector<Dimensions,double>&  x,
          double                                       t,
          double                                       dt,
          int                                          normal,
          double * __restrict__                        F
          ) -> void {
          eulerFlux(Q,x,t,dt,normal,F);
        },
      nullptr, //ncp
      nullptr, //source
      nullptr, //pointSources
      QuadraturePoints,
      QuadratureWeights, //massMatrixDiagonal
      StiffnessOperator,
      DerivativeOperator,
      true,     //flux
      false,    //ncp
      false,    //source
      false);   //pointSources


  file_stream << "\nComputed volume integral values are: \n";
  for(int i=0; i<nodesPerCell; i++){
    file_stream << "node " << i << ": [";
    for(int var=0; var<unknowns;var++){
      file_stream << QOut[i*unknowns+var] << ", ";
    }
    file_stream << "]\n";
  }
  file_stream << "\n";

  // Step 2), the values computed in step 1) are extrapolated to the surface boundary
  projectVolumetricDataOntoFaces(
    QIn,
    order,
    unknowns+auxiliaryVariables,
    BasisFunctionValuesLeft,
    QLeft,
    QRight,
    QDown,
    QUp
#if Dimensions==3
    ,QFront,
    QBack
#endif
  );

  double* F[2*Dimensions];

  F[0] = QLeft;
  F[1] = QRight;
  F[2] = QDown;
  F[3] = QUp;
#if Dimensions==3
  F[4] = QFront;
  F[5] = QBack;
#endif

  for(int f=0; f<2*Dimensions; f++){

    double* currentFace = F[f];
    file_stream << "\nFace " << f << ": \n";

    for(int node=0; node<2*nodesPerFace; node++){
      file_stream << "node " << node << ": [";
      for(int var=0; var<strideQ; var++){
        file_stream << currentFace[node*strideQ+var] << ", ";
      }
      file_stream << "]\n";
    }

  }


  // *  project data onto opposite face as well to trade with "identical" cell,
  // *  this essentially simulates the exchange of surface data with the
  // *  neighbouring cells which Peano would take care of for us normally
  projectVolumetricDataOntoFaces(
    QIn,
    order,
    unknowns+auxiliaryVariables,
    BasisFunctionValuesLeft,
    QRight,
    QLeft,
    QUp,
    QDown
#if Dimensions==3
    ,QBack,
    QFront
#endif
  );



  //* The faces are assembled into one pointer construct for ease of access in the
  //* next steps. In addition information about whether the faces are at the boundary
  //* is provided since at the boundary the flux must be computed with boundary values
  //* instead of using the extrapolated solution from the neighbour.
  double* QHull[Dimensions*2];
  QHull[0] = QLeft;
  QHull[1] = QRight;
  QHull[2] = QDown;
  QHull[3] = QUp;

#if Dimensions==3
  QHull[4] = QBack;
  QHull[5] = QFront;
#endif


  for(int face=0; face<2*Dimensions; face++){

    rusanov::solveRiemannProblem_pointwise_in_situ(
      [&](
          const double * __restrict__                  Q,
          const tarch::la::Vector<Dimensions,double>&  x,
          double                                       t,
          double                                       dt,
          int                                          normal,
          double * __restrict__                        F
          ) -> void {
          eulerFlux(Q,x,t,dt,normal,F);
      },
      nullptr, //ncp
      [&](
        const double * __restrict__                 Q,
        const tarch::la::Vector<Dimensions,double>& x,
        double                                      t,
        double                                      dt,
        int                                         normal
        ) -> double {
          return eulerEigenvalue(Q,x,t,dt,normal);
      },
      x,
      cellSize,
      t,
      dt,
      order,
      unknowns,
      auxiliaryVariables,
      face, //faceNumber
      QuadraturePoints,
      true, //useFlux
      false, //useNCP
      QHull[face], //TODO
      &QHullForRiemann[face*2*nodesPerFace*strideQ]
      );

  }


  file_stream << "\nRusanov Fluxes per Face:\n";
  for(int f=0; f<2*Dimensions; f++){
    file_stream << "At face " << f << "\n";
    for(int i=0; i<2*nodesPerFace; i++){
      file_stream << "at node " << i << ": [";
      for(int var=0; var<unknowns; var++){
        file_stream << QHullForRiemann[(f*2*nodesPerFace+i)*unknowns+var] << ", ";
      }
      file_stream << "]\n";
    }
  }

/*
  for(int i=0; i<nodesPerCell*strideQ; i++){
    QOut[i] = 0.0;
  }
*/

  // * The contributions from the computed rusanov flux are then added to the nodes
  // * within the cell.
  integrateOverRiemannSolutionsAndAddToVolume_GaussLegendre(
#if Dimensions==2
    &QHullForRiemann[(0*2+0)*nodesPerFace*strideQ], //left
    &QHullForRiemann[(2*2+0)*nodesPerFace*strideQ], //right
    &QHullForRiemann[(1*2+0)*nodesPerFace*strideQ], //down
    &QHullForRiemann[(3*2+0)*nodesPerFace*strideQ], //up
#else
    &QHullForRiemann[(0*2+0)*nodesPerFace*strideQ], //left
    &QHullForRiemann[(3*2+0)*nodesPerFace*strideQ], //right
    &QHullForRiemann[(1*2+0)*nodesPerFace*strideQ], //down
    &QHullForRiemann[(4*2+0)*nodesPerFace*strideQ], //up
    &QHullForRiemann[(2*2+0)*nodesPerFace*strideQ], //front
    &QHullForRiemann[(5*2+0)*nodesPerFace*strideQ], //back
#endif
    order,
    unknowns,
    auxiliaryVariables,
    cellSize,
    BasisFunctionValuesLeft,
    QuadratureWeights,
    QOut
  );


  file_stream << "\nValues from Riemann Integral are: \n";
  for(int i=0; i<nodesPerCell; i++){
    file_stream << "node " << i << ": [";
    for(int var=0; var<unknowns;var++){
      file_stream << QOut[i*unknowns+var] << ", ";
    }
    file_stream << "]\n";
  }



  multiplyWithInvertedMassMatrixAndClearAuxiliaryVariables_GaussLegendre(
    cellData,
    order,
    unknowns,
    auxiliaryVariables,
    QuadratureWeights
  );

  //Finally we check that after all these steps have been taken QOut, which should be the sum of the volume and face contributions, is zero

  file_stream << "\nFinal values are: \n";
  for(int i=0; i<nodesPerCell; i++){
    file_stream << "node " << i << ": [";
    for(int var=0; var<unknowns;var++){
      file_stream << QOut[i*unknowns+var] << ", ";
      validateNumericalEquals(QOut[i*unknowns+var], 0.0);
    }
    file_stream << "]\n";
  }


  delete[] QIn;
  delete[] QOut;
  delete[] QLeft;
  delete[] QRight;
  delete[] QDown;
  delete[] QUp;
#if Dimensions==3
  delete[] QFront;
  delete[] QBack;
#endif
  delete[] QHullForRiemann;



}//runDGEuler



void exahype2::dg::tests::DGTest::runDGElastic(){

  file_stream << "\n\nStarting Elastic test in " << Dimensions << " dimensions\n\n";

  // geometry
#if Dimensions==2
  const tarch::la::Vector<2, double> x({0.0, 0.0});
  const tarch::la::Vector<2, double> cellSize({1.0, 1.0});
#else
  const tarch::la::Vector<3, double> x({0.0, 0.0, 0.0});
  const tarch::la::Vector<3, double> cellSize({1.0, 1.0, 1.0});
#endif
  const double dx = 0.01;
  const double t  = 0.0;
  const double dt = 0.01;

  //using euler equations as example
  const int unknowns = 9;
  const int auxiliaryVariables = 3;
  const int order = 4;

  double QuadraturePoints1d[5]{ 0.04691007703066802,0.23076534494715845,0.5,0.7692346550528415,0.9530899229693319};
  double MassMatrixDiagonal1d[5]{ 0.11846344252809464,0.23931433524968335,0.2844444444444444,0.2393143352496833,0.11846344252809476};
  double BasisFunctionValuesLeft1d[5]{ 1.5514080490943134,-0.8931583920000723,0.5333333333333335,-0.2679416522233877,0.07635866179581295};
  /*
  double StiffnessOperator1d[5*5]{
    -0.0035693018244916616,-0.001366470610763652,0.0005093907475922976,-0.00034781153498870645,0.0003886731102028964,
    0.0006764192061275383,-0.00013454606992855555,-0.00030268967651351695,0.00016474359224335756,-0.0001721708469438526,
    -0.00021214751326784775,0.00025466476894373674,1.0550734570157016e-19,-0.00025466476894373685,0.00021214751326784797,
    0.00017217084694385224,-0.00016474359224335724,0.0003026896765135165,0.000134546069928556,-0.0006764192061275387,
    -0.00038867311020289555,0.0003478115349887054,-0.0005093907475922961,0.0013664706107636491,0.0035693018244916633};
  double DerivativeOperator1d[5*5]{
    -0.030129985659037146,0.005709940481994012,-0.0017908268470041727,0.0014533669060226783,-0.0032809540387172065,
    -0.005709940481994011,-0.0005622148367676333,0.0010641433939928332,-0.000688398344676995,0.0014533669060226787,
    0.0017908268470041724,-0.001064143393992833,-1.0842021724855044e-19,0.001064143393992833,-0.0017908268470041724,
    -0.0014533669060226787,0.000688398344676995,-0.0010641433939928332,0.0005622148367676332,0.005709940481994009,
    0.0032809540387172074,-0.0014533669060226787,0.0017908268470041724,-0.005709940481994009,0.030129985659037132};
  */
  double DerivativeOperator1d[5*5]{
    -10.1340811913090842466544927447102963924407958984375,15.4039041703444912201348415692336857318878173828125,-8.0870875087753280041624748264439404010772705078125,3.920798231666558830710300753707997500896453857421875,-1.1035337019266335811806811761925928294658660888671875,
    -1.9205120472639156670169313656515441834926605224609375,-1.516706434335750142139431773102842271327972412109375,4.80550130432859834428427348029799759387969970703125,-1.8571160532876695992143822877551428973674774169921875,0.488833230558736009374598552312818355858325958251953125,
    0.602336319455663016242397134192287921905517578125,-2.870776484669482986333832741365768015384674072265625,-0.000000000000001332267629550187848508358001708984375,2.870776484669483874512252441491000354290008544921875,-0.60233631945566312726469959670794196426868438720703125,
    -0.48883323055873584284114485853933729231357574462890625,1.8571160532876682669467527375672943890094757080078125,-4.80550130432860012064111288054846227169036865234375,1.5167064343357505862286416231654584407806396484375,1.9205120472639156670169313656515441834926605224609375,
    1.1035337019266331370914713261299766600131988525390625,-3.920798231666557942531881053582765161991119384765625,8.087087508775329780519314226694405078887939453125,-15.4039041703444912201348415692336857318878173828125,10.13408119130908602301133214496076107025146484375};
  double StiffnessOperator1d[5*5]{
    -1.2005181447816875017764459698810242116451263427734375,-0.459606063929972219472830374797922559082508087158203125,0.1713312197562774918946360003246809355914592742919921875,-0.11698479961911900648630080468137748539447784423828125,0.130728401276001104935886587554705329239368438720703125,
    1.824799516391880427335081549244932830333709716796875,-0.362969592101977378550969888237887062132358551025390625,-0.81657642230598626031934372804244048893451690673828125,0.4444344937740536405357261173776350915431976318359375,-0.464471255981286024194787387386895716190338134765625,
    -0.95802422631547734521717529787565581500530242919921875,1.15002535018688423207322557573206722736358642578125,-0.0000000000000003789561257387200741506709440429594952952620993784360603484628882,-1.150025350186884676162435425794683396816253662109375,0.95802422631547756726178022290696389973163604736328125,
    0.464471255981286079705938618644722737371921539306640625,-0.44443449377405397360263350492459721863269805908203125,0.81657642230598648236394865307374857366085052490234375,0.362969592101977489573272350753541104495525360107421875,-1.824799516391880427335081549244932830333709716796875,
    -0.13072840127600116044703781881253235042095184326171875,0.11698479961911904811966422812474775128066539764404296875,-0.1713312197562775196502116159535944461822509765625,0.459606063929972219472830374797922559082508087158203125,1.2005181447816877238210508949123322963714599609375};

  /*
  DerivativeOperator1d{
    -10.1340811913090842466544927447102963924407958984375,15.4039041703444912201348415692336857318878173828125,-8.0870875087753280041624748264439404010772705078125,3.920798231666558830710300753707997500896453857421875,-1.1035337019266335811806811761925928294658660888671875,
    -1.9205120472639156670169313656515441834926605224609375,-1.516706434335750142139431773102842271327972412109375,4.80550130432859834428427348029799759387969970703125,-1.8571160532876695992143822877551428973674774169921875,0.488833230558736009374598552312818355858325958251953125,
    0.602336319455663016242397134192287921905517578125,-2.870776484669482986333832741365768015384674072265625,-0.000000000000001332267629550187848508358001708984375,2.870776484669483874512252441491000354290008544921875,-0.60233631945566312726469959670794196426868438720703125,
    -0.48883323055873584284114485853933729231357574462890625,1.8571160532876682669467527375672943890094757080078125,-4.80550130432860012064111288054846227169036865234375,1.5167064343357505862286416231654584407806396484375,1.9205120472639156670169313656515441834926605224609375,
    1.1035337019266331370914713261299766600131988525390625,-3.920798231666557942531881053582765161991119384765625,8.087087508775329780519314226694405078887939453125,-15.4039041703444912201348415692336857318878173828125,10.13408119130908602301133214496076107025146484375},
  StiffnessOperator1d{
    -1.2005181447816875017764459698810242116451263427734375,-0.459606063929972219472830374797922559082508087158203125,0.1713312197562774918946360003246809355914592742919921875,-0.11698479961911900648630080468137748539447784423828125,0.130728401276001104935886587554705329239368438720703125,
    1.824799516391880427335081549244932830333709716796875,-0.362969592101977378550969888237887062132358551025390625,-0.81657642230598626031934372804244048893451690673828125,0.4444344937740536405357261173776350915431976318359375,-0.464471255981286024194787387386895716190338134765625,
    -0.95802422631547734521717529787565581500530242919921875,1.15002535018688423207322557573206722736358642578125,-0.0000000000000003789561257387200741506709440429594952952620993784360603484628882,-1.150025350186884676162435425794683396816253662109375,0.95802422631547756726178022290696389973163604736328125,
    0.464471255981286079705938618644722737371921539306640625,-0.44443449377405397360263350492459721863269805908203125,0.81657642230598648236394865307374857366085052490234375,0.362969592101977489573272350753541104495525360107421875,-1.824799516391880427335081549244932830333709716796875,
    -0.13072840127600116044703781881253235042095184326171875,0.11698479961911904811966422812474775128066539764404296875,-0.1713312197562775196502116159535944461822509765625,0.459606063929972219472830374797922559082508087158203125,1.2005181447816877238210508949123322963714599609375},
  */

  //Parameter initialisation and memory allocation
  const int nodesPerAxis  = order+1;
  const int nodesPerCell  = getNodesPerCell(nodesPerAxis);
  const int nodesPerFace  = nodesPerCell/nodesPerAxis;
  const int strideQ       = unknowns+auxiliaryVariables;

  //values in the cell at the beginning and end
  double* QIn = new double[nodesPerCell*strideQ];
  double* QOut = new double[nodesPerCell*unknowns];
  //values on the faces
  double* QLeft = new double[2*nodesPerFace*strideQ];
  double* QRight = new double[2*nodesPerFace*strideQ];
  double* QDown = new double[2*nodesPerFace*strideQ];
  double* QUp = new double[2*nodesPerFace*strideQ];
#if Dimensions==3
  double* QFront = new double[2*nodesPerFace*strideQ];
  double* QBack = new double[2*nodesPerFace*strideQ];
#endif

  //values of the rusanov fluxes on the faces
  double* QHullForRiemann = new double[2*Dimensions*2*nodesPerFace*strideQ];


  ::exahype2::CellData cellData(
      QIn,
      x,
      cellSize,
      t,
      dt,
      QOut
      );


  //setting initial conditions

  file_stream << "Initial conditions: \n";
  for(int i=0; i<nodesPerCell; i++){
    elasticInitial(&QIn[i*strideQ]);
    file_stream << "node " << i << " [";
    for(int var=0; var<strideQ; var++){
      file_stream << QIn[i*strideQ+var] << ", ";
    }
    file_stream << "]\n";
  }

/*
  //initial stresses
  for(int var=0; var<6; var++){
    QIn[12*strideQ+var] = 1.0;
  }
*/

  //for later comparisons
  double QInit[strideQ];
  elasticInitial(QInit);

  cellIntegral_patchwise_GaussLegendre_functors(
    cellData,
    order,
    unknowns,
    auxiliaryVariables,
    [&](
        const double * __restrict__                  Q,
        const tarch::la::Vector<Dimensions,double>&  x,
        double                                       t,
        double                                       dt,
        int                                          normal,
        double * __restrict__                        F
        ) -> void {
        elasticFlux(Q,x,t,dt,normal,F);
    },
    [&](
        const double * __restrict__                  Q,
        const double * __restrict__                  dQdx,
        const tarch::la::Vector<Dimensions,double>&  x,
        double                                       t,
        double                                       dt,
        int                                          normal,
        double * __restrict__                        BgradQ
      ) -> void{
      elasticNonConservativeProduct(Q, dQdx, x, t, dt, normal, BgradQ);
    },
    nullptr, //source
    nullptr, //pointSources
    QuadraturePoints1d,
    MassMatrixDiagonal1d, //massMatrixDiagonal
    StiffnessOperator1d,
    DerivativeOperator1d,
    true,     //flux
    false,    //ncp TODO: check error
    false,    //source
    false);   //pointSources

  file_stream << "\nComputed volume integral values are: \n";
  for(int i=0; i<nodesPerCell; i++){
    file_stream << "node " << i << ": [";
    for(int var=0; var<unknowns;var++){
      file_stream << QOut[i*unknowns+var] << ", ";
    }
    file_stream << "]\n";
  }
  file_stream << "\n";
/*
  multiplyWithInvertedMassMatrixAndClearAuxiliaryVariables_GaussLegendre(
    cellData,
    order,
    unknowns,
    auxiliaryVariables,
    MassMatrixDiagonal1d
  );

  file_stream << "\nComputed volume integral values multiplied with inverted mass matrix are: \n";
  for(int i=0; i<nodesPerCell; i++){
    file_stream << "node " << i << ": [";
    for(int var=0; var<strideQ;var++){
      file_stream << QOut[i*strideQ+var] << ", ";
    }
    file_stream << "]\n";
  }
  file_stream << "\n";
*/

  // Step 2), the values computed in step 1) are extrapolated to the surface boundary
  projectVolumetricDataOntoFaces(
    QIn,
    order,
    unknowns+auxiliaryVariables,
    BasisFunctionValuesLeft1d,
    QLeft,
    QRight,
    QDown,
    QUp
#if Dimensions==3
    ,QFront,
    QBack
#endif
  );

  double* F[2*Dimensions];

  F[0] = QLeft;
  F[1] = QRight;
  F[2] = QDown;
  F[3] = QUp;
#if Dimensions==3
  F[4] = QFront;
  F[5] = QBack;
#endif


  // *  project data onto opposite face as well to trade with "identical" cell,
  // *  this essentially simulates the exchange of surface data with the
  // *  neighbouring cells which Peano would take care of for us normally
  projectVolumetricDataOntoFaces(
    QIn,
    order,
    unknowns+auxiliaryVariables,
    BasisFunctionValuesLeft1d,
    QRight,
    QLeft,
    QUp,
    QDown
#if Dimensions==3
    ,QBack,
    QFront
#endif
  );


  for(int f=0; f<2*Dimensions; f++){

    double* currentFace = F[f];
    file_stream << "\nFace " << f << ": \n";

    for(int node=0; node<2*nodesPerFace; node++){
      file_stream << "node " << node << ": [";
      for(int var=0; var<strideQ; var++){
        file_stream << (std::abs(currentFace[node*strideQ+var])<0.00001 ? 0.0 : currentFace[node*strideQ+var]) << ", ";
      }
      file_stream << "]\n";
    }

  }


  //* The faces are assembled into one pointer construct for ease of access in the
  //* next steps. In addition information about whether the faces are at the boundary
  //* is provided since at the boundary the flux must be computed with boundary values
  //* instead of using the extrapolated solution from the neighbour.
  double* QHull[Dimensions*2];
#if Dimensions==2
  QHull[0] = QLeft;
  QHull[2] = QRight;
  QHull[1] = QDown;
  QHull[3] = QUp;
#elif Dimensions==3
  QHull[0] = QLeft;
  QHull[3] = QRight;
  QHull[1] = QDown;
  QHull[4] = QUp;
  QHull[2] = QBack;
  QHull[5] = QFront;
#endif


  for(int i=0; i<2*Dimensions*2*nodesPerFace*strideQ; i++){
    QHullForRiemann[i] = 0.0;
  }


  for(int face=0; face<2*Dimensions; face++){

    rusanov::solveRiemannProblem_pointwise_in_situ(
      [&](
          const double * __restrict__                  Q,
          const tarch::la::Vector<Dimensions,double>&  x,
          double                                       t,
          double                                       dt,
          int                                          normal,
          double * __restrict__                        F
          ) -> void {
          elasticFlux(Q,x,t,dt,normal,F);
      },
      [&](
          const double * __restrict__                  Q,
          const double * __restrict__                  dQdx,
          const tarch::la::Vector<Dimensions,double>&  x,
          double                                       t,
          double                                       dt,
          int                                          normal,
          double * __restrict__                        BgradQ
        ) -> void{
        elasticNonConservativeProduct(Q, dQdx, x, t, dt, normal, BgradQ);
      },
      [&](
        const double * __restrict__                 Q,
        const tarch::la::Vector<Dimensions,double>& x,
        double                                      t,
        double                                      dt,
        int                                         normal
        ) -> double {
          return elasticEigenvalue(Q,x,t,dt,normal);
      },
      x,
      cellSize,
      t,
      dt,
      order,
      unknowns,
      auxiliaryVariables,
      face, //faceNumber
      QuadraturePoints1d,
      true, //useFlux
      false, //useNCP
      QHull[face], //TODO
      &QHullForRiemann[face*2*nodesPerFace*unknowns]
      );

  }


  file_stream << "\nRusanov Fluxes per Face:\n";
  for(int f=0; f<2*Dimensions; f++){
    file_stream << "At face " << f << "\n";
    for(int i=0; i<2*nodesPerFace; i++){
      file_stream << "at node " << i << ": [";
      for(int var=0; var<unknowns; var++){
        file_stream << (std::abs(QHullForRiemann[(f*2*nodesPerFace+i)*unknowns+var])<0.000001 ? 0.0 : QHullForRiemann[(f*2*nodesPerFace+i)*unknowns+var]) << ", ";
      }
      file_stream << "]\n";
    }
  }

/*
  for(int i=0; i<strideQ*nodesPerCell; i++){
    QOut[i] = 0.0;
  }
*/

  //  The contributions from the computed rusanov flux are then added to the nodes
  //  within the cell.
  integrateOverRiemannSolutionsAndAddToVolume_GaussLegendre(
#if Dimensions==2
    &QHullForRiemann[(0*2+0)*nodesPerFace*unknowns], //left
    &QHullForRiemann[(2*2+0)*nodesPerFace*unknowns], //right
    &QHullForRiemann[(1*2+0)*nodesPerFace*unknowns], //down
    &QHullForRiemann[(3*2+0)*nodesPerFace*unknowns], //up
#else
    &QHullForRiemann[(0*2+0)*nodesPerFace*unknowns], //left
    &QHullForRiemann[(3*2+0)*nodesPerFace*unknowns], //right
    &QHullForRiemann[(1*2+0)*nodesPerFace*unknowns], //down
    &QHullForRiemann[(4*2+0)*nodesPerFace*unknowns], //up
    &QHullForRiemann[(2*2+0)*nodesPerFace*unknowns], //front
    &QHullForRiemann[(5*2+0)*nodesPerFace*unknowns], //back
#endif
    order,
    unknowns,
    auxiliaryVariables,
    cellSize,
    BasisFunctionValuesLeft1d,
    MassMatrixDiagonal1d,
    QOut
  );


  file_stream << "\nValues after Riemann Integral are: \n";
  for(int i=0; i<nodesPerCell; i++){
    file_stream << "node " << i << ": [";
    for(int var=0; var<unknowns;var++){
      file_stream << (std::abs(QOut[i*unknowns+var]) <0.0000001 ? 0.0 : QOut[i*unknowns+var]) << ", ";
    }
    file_stream << "]\n";
  }


  multiplyWithInvertedMassMatrixAndClearAuxiliaryVariables_GaussLegendre(
    cellData,
    order,
    unknowns,
    auxiliaryVariables,
    MassMatrixDiagonal1d
  );

  //Finally we check that after all these steps have been taken QOut, which should be the sum of the volume and face contributions, is zero

  file_stream << "\nFinal values are: \n";
  for(int i=0; i<nodesPerCell; i++){
    file_stream << "node " << i << ": [";
    for(int var=0; var<unknowns;var++){
      file_stream << (std::abs(QOut[i*unknowns+var]) <0.0000001 ? 0.0 : QOut[i*unknowns+var]) << ", ";
//      validateNumericalEquals(QOut[i*strideQ+var], 0.0);
    }
    file_stream << "]\n";
  }


  delete[] QIn;
  delete[] QOut;
  delete[] QLeft;
  delete[] QRight;
  delete[] QDown;
  delete[] QUp;
#if Dimensions==3
  delete[] QFront;
  delete[] QBack;
#endif
  delete[] QHullForRiemann;

} //runDGElastic
