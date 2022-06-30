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
MassMatrix {0.1739274225687269248563637802362791262567043304443359375,0.0,0.0,0.0,0.0,0.32607257743127304738806060413480736315250396728515625,0.0,0.0,0.0,0.0,0.32607257743127304738806060413480736315250396728515625,0.0,0.0,0.0,0.0,0.1739274225687269248563637802362791262567043304443359375},
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
{
#if Dimensions==2
  file_stream.open("src/exahype2/dg/tests/results2D.log");
#else
  file_stream.open("src/exahype2/dg/tests/results3D.log");
#endif
}


void exahype2::dg::tests::DGTest::run(){
  testMethod(UtilsTest);
//  testMethod(runDGEuler);
//  testMethod(runDGElastic);
//  testMethod(runTest);
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


  /*
   * testing applyBoundaryConditions to make sure the nodes are written in the right positions
   * and with the right values.
   */

  const int nodesPerFace = nodesPerCell/nodesPerAxis;
  const int strideQ = 4; // in order to test Q, x, t, normal
  double QFaces[2*2*nodesPerFace*strideQ];
  double* QFacesL = &QFaces[0]; //left face construct
  double* QFacesR = &QFaces[2*nodesPerFace*strideQ]; //right face construct

  //utility parameters
  const tarch::la::Vector<Dimensions, double> x({0.0, 0.0});
  double t = 3.14;

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
      int                                          normal
    ) -> void {
      testBoundaryConditions(Qinside,Qoutside,x,t,normal);
    },
    QFacesL,
    order,
    4, //strideQ
    x,
    t,
    0, //faceNormal
    0 //lr=0, left face
    );

  applyBoundaryConditions(
    [&](
      const double * __restrict__                  Qinside,
      double * __restrict__                        Qoutside,
      const tarch::la::Vector<Dimensions,double>&  x,
      double                                       t,
      int                                          normal
    ) -> void {
      testBoundaryConditions(Qinside,Qoutside,x,t,normal);
    },
    QFacesR,
    order,
    4, //strideQ
    x,
    t,
    1, //faceNormal
    1 //lr=1, right face
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
    validateNumericalEquals( QFacesR[(2*node+1)*strideQ+3], 1);

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
            testEigenvalue(Q,x,t,dt,normal);
          },
      cellQin,
      order,
      2, //strideQ
      x,
      t,
      0.1 //dt
      );

  validateNumericalEquals(maxEval, (Dimensions-1)*(nodesPerCell-1));

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
/*

  file_stream << "Starting Euler test in " << Dimensions << " dimensions\n\n";

  // geometry
  const tarch::la::Vector<Dimensions, double> x({0.0, 0.0});
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
  double* QHullForRiemann = new double[2*Dimensions*nodesPerFace*strideQ];

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

  cellIntegral_patchwise_computeAndUpdate_functors(
      QIn,
      QOut,
      order,
      unknowns,
      auxiliaryVariables,
      x,
      dx,
      t,
      dt,
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
      nullptr,
      QuadraturePoints,
      QuadratureWeights,
      MassMatrix,
      StiffnessOperator,
      DerivativeOperator,
      true,
      false,
      false,
      false);

  file_stream << "\nComputed volume integral values are: \n";
  for(int i=0; i<nodesPerCell; i++){
    file_stream << "node " << i << ": [";
    for(int var=0; var<unknowns;var++){
      file_stream << QOut[i*strideQ+var] << ", ";
//      validateNumericalEquals(QOut[i*strideQ+var], QIn[i*strideQ+var]+dt);
    }
    file_stream << "]\n";
  }


  // Step 2), the values computed in step 1) are extrapolated to the surface boundary
  projectVolumetricDataOntoFaces(
    QIn,
    order,
    unknowns+auxiliaryVariables,
    BasisFunctionValuesLeft,
    BasisFunctionValuesRight,
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

    int lr = 1-f%2; //marks whether the face is left or right
    double* currentFace = F[f];
    file_stream << "\nFace: " << f << "[ \n";

    for(int node=0; node<nodesPerFace; node++){
      file_stream << "node: " << node << "[";
      for(int var=0; var<strideQ; var++){
        file_stream << currentFace[(2*node+lr)*strideQ+var] << ", ";
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
    BasisFunctionValuesRight,
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
//  atBoundary[4] = false;
//  atBoundary[5] = true;
#endif

  double maxEigenvaluePerFace[2*Dimensions];


  getMaxEigenvaluePerFace(
    [&](
      const double * __restrict__                 Q,
      const tarch::la::Vector<Dimensions,double>& x,
      double                                      t,
      double                                      dt,
      int                                         normal
      ) -> double {
        return eulerEigenvalue(Q,x,t,dt,normal);
      },
    QHull,
    order,
    unknowns,
    auxiliaryVariables,
    x,
    t,
    dt,
    maxEigenvaluePerFace
  );

  file_stream << "\nMax Eigenvalues per Face:\n";
  for(int i=0; i<Dimensions*2; i++){
    file_stream << "Max Eigenvalues at face " << i << ": " << maxEigenvaluePerFace[i] << std::endl;
//    validateNumericalEquals(maxEigenvaluePerFace[i], 2.6);
  }


  computeRusanovOnHull(
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
      nullptr, //flux
        QHull,
        QHullForRiemann,
        order,
        unknowns,
        auxiliaryVariables,
        x,
        dx,
        t,
        dt,
        maxEigenvaluePerFace,
        true, //useFlux
        false //useNCP
        );

  file_stream << "\nRusanov Fluxes per Face:\n";
  for(int f=0; f<2*Dimensions; f++){
    file_stream << "At face " << f << "\n";
    for(int i=0; i<nodesPerFace; i++){
      file_stream << "at node " << i << ": [";
      for(int var=0; var<strideQ; var++){
        file_stream << QHullForRiemann[(f*nodesPerFace+i)*strideQ+var] << ", ";
        //validateNumericalEquals(QHullForRiemann[(f*nodesPerFace+i)*strideQ+var], 0.0);
      }
      file_stream << "]\n";
    }
  }


  // * The contributions from the computed rusanov flux are then added to the nodes
  // * within the cell.
  applyRiemannToPatch(
      QOut,
      QHullForRiemann,
      order,
      unknowns,
      auxiliaryVariables,
      dx,
      dt,
      BasisFunctionValuesLeft,
      BasisFunctionValuesRight,
      QuadratureWeights
      );

  //Finally we check that after all these steps have been taken QOut, which should be the sum of the volume and face contributions, is zero

  file_stream << "\nFinal values are: \n";
  for(int i=0; i<nodesPerCell; i++){
    file_stream << "node " << i << ": [";
    for(int var=0; var<unknowns;var++){
      file_stream << QOut[i*strideQ+var] << ", ";
      validateNumericalEquals(QOut[i*strideQ+var], 0.0);
    }
    file_stream << "]\n";
  }


  file_stream << "\n\n";


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


*/
}//runDGEuler

void exahype2::dg::tests::DGTest::runDGElastic(){

  file_stream << "Starting Elastic test in " << Dimensions << " dimensions\n\n";

  // geometry
  const tarch::la::Vector<Dimensions, double> x({0.0, 0.0});
  const double dx = 0.01;
  const double t  = 0.0;
  const double dt = 0.01;

  //using euler equations as example
  const int unknowns = 9;
  const int auxiliaryVariables = 3;
  const int order = 3;

  //Parameter initialisation and memory allocation
  const int nodesPerAxis = order+1;
  const int nodesPerCell = getNodesPerCell(nodesPerAxis);
  const int nodesPerFace = nodesPerCell/nodesPerAxis;
  const int strideQ = unknowns+auxiliaryVariables;

  double*  QIn     = new double[nodesPerCell*(unknowns+auxiliaryVariables)]{1.0};
  double*  QOut    = new double[2*nodesPerCell*(unknowns+auxiliaryVariables)]{0.0};
  double* QLeft    = new double[2*nodesPerFace*(unknowns+auxiliaryVariables)]{0.0};
  double* QRight   = new double[2*nodesPerFace*(unknowns+auxiliaryVariables)]{0.0};
  double* QLower   = new double[2*nodesPerFace*(unknowns+auxiliaryVariables)]{0.0};
  double* QUpper   = new double[2*nodesPerFace*(unknowns+auxiliaryVariables)]{0.0};
  #if Dimensions==3
  double* QFront   = new double[2*nodesPerFace*(unknowns+auxiliaryVariables)]{0.0};
  double* QBack    = new double[2*nodesPerFace*(unknowns+auxiliaryVariables)]{0.0};
  #endif

  //initialisation of the initial value
  for(int i=0; i<nodesPerCell; i++){
    elasticInitial(&QIn[i*strideQ]);
  }

  //logging the results (and in the next steps, verify their correctness)
  file_stream << "Starting values are: \n";
  for(int i=0; i<nodesPerCell; i++){
    file_stream << "node " << i << ": [";
        for(int var=0; var<strideQ; var++){
          file_stream << QIn[i*strideQ+var] << ", ";
        }
    file_stream << "]\n";
  }

  const double invDx = 1.0/dx;
  computeGradient(
    QIn,
    DerivativeOperator,
    invDx,
    nodesPerAxis,
    strideQ,
    0,
    QOut
    );

  cellIntegral_patchwise_computeAndUpdate_functors(
      QIn,
      QOut,
      order,
      unknowns,
      auxiliaryVariables,
      x,
      dx,
      t,
      dt,
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
          double * __restrict__                        F
        ) -> void {
          elasticNonConservativeProduct(Q, dQdx, x, t,dt, normal, F);
        },
      [&](
          const double * __restrict__                  Q,
          const tarch::la::Vector<Dimensions,double>&  x,
          double                                       t,
          double                                       dt,
          double * __restrict__                        S
        ) -> void {
          elasticSource(Q,x,t,dt,S);
        },
      nullptr,
      QuadraturePoints,
      QuadratureWeights,
      MassMatrix,
      StiffnessOperator,
      DerivativeOperator,
      true,
      true,
      true,
      false);

  for(int i=0; i<nodesPerCell; i++){
    for(int var=0; var<auxiliaryVariables;var++){
      QOut[i*strideQ+unknowns+var] += QIn[i*strideQ+unknowns+var];
    }
  }

  file_stream << "\nComputed volume integral values are: \n";
  for(int i=0; i<nodesPerCell; i++){
    file_stream << "node " << i << ": [";
    for(int var=0; var<unknowns;var++){
      file_stream << QOut[i*strideQ+var] << ", ";
      validateNumericalEquals(QOut[i*strideQ+var], QIn[i*strideQ+var]+dt);
    }
    for(int var=0; var<auxiliaryVariables;var++){
      file_stream << QOut[i*strideQ+unknowns+var] << ", ";
      validateNumericalEquals(QOut[i*strideQ+unknowns+var], QIn[i*strideQ+unknowns+var]);
    }
    file_stream << "]\n";
  }


  /*
   * Step 2), the values computed in step 1) are extrapolated to the surface boundary
   */
  projectVolumetricDataOntoFaces(
    QIn,
    order,
    unknowns+auxiliaryVariables,
    BasisFunctionValuesLeft,
    BasisFunctionValuesRight,
    QLeft,
    QRight,
    QLower,
    QUpper
#if Dimensions==3
    ,QFront,
    QBack
#endif
  );

  double initialValues[auxiliaryVariables];
  elasticInitial(initialValues);

  file_stream << "\nProjected Face values are: \n Left Face:\n";
  for(int i=0; i<2*nodesPerFace; i++){
    file_stream << "node " << i << ": [";
    for(int var=0; var<strideQ; var++){
      file_stream << QLeft[i*strideQ+var] << ", ";
      if(i%2==1){
        validateNumericalEquals(QLeft[i*strideQ+var], initialValues[var]);
      }
    }
    file_stream << "]\n";
  }

  file_stream << "Right Face:\n";
  for(int i=0; i<2*nodesPerFace; i++){
    file_stream << "node " << i << ": [";
    for(int var=0; var<strideQ;var++){
      file_stream << QRight[i*strideQ+var] << ", ";
      if(i%2==0){
        validateNumericalEquals(QRight[i*strideQ+var], initialValues[var]);
      }
    }
    file_stream << "]\n";
  }

  file_stream << "Lower Face:\n";
  for(int i=0; i<2*nodesPerFace; i++){
    file_stream << "node " << i << ": [";
    for(int var=0; var<strideQ;var++){
      file_stream << QLower[i*strideQ+var] << ", ";
      if(i%2==1){
        validateNumericalEquals(QLower[i*strideQ+var], initialValues[var]);
      }
    }
    file_stream << "]\n";
  }

  file_stream << "Upper Face:\n";
  for(int i=0; i<2*nodesPerFace; i++){
    file_stream << "node " << i << ": [";
    for(int var=0; var<strideQ;var++){
      file_stream << QUpper[i*strideQ+var] << ", ";
      if(i%2==0){
        validateNumericalEquals(QUpper[i*strideQ+var], initialValues[var]);
      }
    }
    file_stream << "]\n";
  }

#if Dimension==3
  file_stream << "Front Face:\n";
  for(int i=0; i<2*nodesPerFace; i++){
    file_stream << "node " << i << ": [";
    for(int var=0; var<strideQ;var++){
      file_stream << QFront[i*strideQ+var] << ", ";
      if(i%2==1){
        validateNumericalEquals(QFront[i*strideQ+var], initialValues[var]);
      }
    }
    file_stream << "]\n";
  }

  file_stream << "Back Face:\n";
  for(int i=0; i<2*nodesPerFace; i++){
    file_stream << "node " << i << ": [";
    for(int var=0; var<strideQ;var++){
      file_stream << QBack[i*strideQ+var] << ", ";
      if(i%2==0){
        validateNumericalEquals(QBack[i*strideQ+var], initialValues[var]);
      }
    }
    file_stream << "]\n";
  }
#endif

  /*
   *  project data onto opposite face as well to trade with "identical" cell,
   *  this essentially simulates the exchange of surface data with the
   *  neighbouring cells which Peano would take care of for us normally
   */
  projectVolumetricDataOntoFaces(
    QIn,
    order,
    unknowns+auxiliaryVariables,
    BasisFunctionValuesLeft,
    BasisFunctionValuesRight,
    QRight,
    QLeft,
    QUpper,
    QLower
#if Dimensions==3
    ,QBack,
    QFront
#endif
  );

  /*
  * The faces are assembled into one pointer construct for ease of access in the
  * next steps. In addition information about whether the faces are at the boundary
  * is provided since at the boundary the flux must be computed with boundary values
  * instead of using the extrapolated solution from the neighbour.
  */
  double* QHull[Dimensions*2];
  QHull[0] = QLeft;
  QHull[1] = QRight;
  QHull[2] = QLower;
  QHull[3] = QUpper;
  bool atBoundary[Dimensions*2];
  atBoundary[0] = false;
  atBoundary[1] = true;
  atBoundary[2] = false;
  atBoundary[3] = true;

#if Dimensions==3
  QHull[4] = QBack;
  QHull[5] = QFront;
  atBoundary[4] = false;
  atBoundary[5] = true;
#endif

  double maxEigenvaluePerFace[Dimensions*2];

  /*
   * this computes the maximal eigenvalue of the solution on each surface
   * for the rusanov flux computation
   */
  getMaxEigenvaluePerFace(
    [&](
      const double * __restrict__                 Q,
      const tarch::la::Vector<Dimensions,double>& x,
      double                                      t,
      double                                      dt,
      int                                         normal
      ) -> double {
        return elasticEigenvalue(Q,x,t,dt,normal);
      },
    QHull,
    order,
    unknowns,
    auxiliaryVariables,
    x,
    t,
    dt,
    maxEigenvaluePerFace
  );

  file_stream << "\nMax Eigenvalues per Face:\n";
  for(int i=0; i<Dimensions*2; i++){
    file_stream << "Max Eigenvalues at face " << i << ": " << maxEigenvaluePerFace[i] << std::endl;
    validateNumericalEquals(maxEigenvaluePerFace[i], 2.6);
  }

  double* QHullForRiemann = new double[2*Dimensions*nodesPerFace*strideQ];

  /*
   * This step computes the rusanov flux for each surface at each node
   */
/*
  computeRusanovOnHull(
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
          double * __restrict__                        F
        ) -> void {
          elasticNonConservativeProduct(Q, dQdx, x, t, dt,normal, F);
        },
        QHull,
        QHullForRiemann,
        order,
        unknowns,
        auxiliaryVariables,
        x,
        dx,
        t,
        dt,
        maxEigenvaluePerFace,
//        atBoundary,
        true,
        true
        );
*/

  file_stream << "\nRusanov Fluxes per Face:\n";
  for(int f=0; f<2*Dimensions; f++){
    file_stream << "At face " << f << "\n";
    for(int i=0; i<nodesPerFace; i++){
      file_stream << "at node " << i << ": [";
      for(int var=0; var<strideQ; var++){
        file_stream << QHullForRiemann[(f*nodesPerFace+i)*strideQ+var] << ", ";
        //validateNumericalEquals(QHullForRiemann[(f*nodesPerFace+i)*strideQ+var], 0.0);
      }
      file_stream << "]\n";
    }
  }

  /*
   * The contributions from the computed rusanov flux are then added to the nodes
   * within the cell.
   */
  applyRiemannToPatch(
      QOut,
      QHullForRiemann,
      order,
      unknowns,
      auxiliaryVariables,
      dx,
      dt,
      BasisFunctionValuesLeft,
      BasisFunctionValuesRight,
      QuadratureWeights
      );

  file_stream << "\nFinal values are: \n";
  for(int i=0; i<nodesPerCell; i++){
    file_stream << "node " << i << ": [";
    for(int var=0; var<unknowns;var++){
      file_stream << QOut[i*strideQ+var] << ", ";
      validateNumericalEquals(QOut[i*strideQ+var], dt);
    }
    for(int var=0; var<auxiliaryVariables;var++){
      file_stream << QOut[i*strideQ+unknowns+var] << ", ";
      validateNumericalEquals(QOut[i*strideQ+unknowns+var], QIn[i*strideQ+unknowns+var]);
    }
    file_stream << "]\n";
  }


  file_stream << "\n\n";

  delete [] QIn;
  delete [] QOut;
  delete [] QLeft;
  delete [] QRight;
  delete [] QLower;
  delete [] QUpper;
#if Dimensions==3
  delete [] QFront;
  delete [] QBack;
#endif
  delete [] QHullForRiemann;

} //runDGElastic



