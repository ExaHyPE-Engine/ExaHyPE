#include "DGTest.h"

exahype2::dg::tests::DGTest::DGTest():
TestCase ("exahype2::dg::DGTest"),
QuadraturePoints {0.069431844202973713731097404888714663684368133544921875,0.33000947820757187134432797392946667969226837158203125,0.66999052179242812865567202607053332030773162841796875,0.9305681557970262307577513638534583151340484619140625},
QuadratureWeights {0.1739274225687269248563637802362791262567043304443359375,0.32607257743127304738806060413480736315250396728515625,0.32607257743127304738806060413480736315250396728515625,0.1739274225687269248563637802362791262567043304443359375},
MassMatrix {0.1739274225687269248563637802362791262567043304443359375,0.0,0.0,0.0,0.0,0.32607257743127304738806060413480736315250396728515625,0.0,0.0,0.0,0.0,0.32607257743127304738806060413480736315250396728515625,0.0,0.0,0.0,0.0,0.1739274225687269248563637802362791262567043304443359375},
StiffnessOperator {-1.1590524262142825051569161587394773960113525390625,-0.494037528020547400675610560938366688787937164306640625,0.214358954361956122180998818294028751552104949951171875,-0.201974321866382811041518152705975808203220367431640625,1.69062826161228674237690938753075897693634033203125,-0.250693983347795967819848783619818277657032012939453125,-0.95909046573029943516530693159438669681549072265625,0.73355015726438665968345276269246824085712432861328125,-0.7335501572643867707057552252081222832202911376953125,0.95909046573029943516530693159438669681549072265625,0.250693983347795634752941396072856150567531585693359375,-1.690628261612286298287699537468142807483673095703125,0.2019743218663829775749718464794568717479705810546875,-0.21435895436195628871445251206750981509685516357421875,0.494037528020547622720215485969674773514270782470703125,1.159052426214282949246126008802093565464019775390625},
BasisFunctionValuesLeft {1.5267881254572663873858573424513451755046844482421875,-0.8136324494869271450880887641687877476215362548828125,0.40076152031165046540905905203544534742832183837890625,-0.11391719628198997138479597879268112592399120330810546875} ,
BasisFunctionValuesRight {-0.113917196281990040773735017864964902400970458984375,0.400761520311650742964815208324580453336238861083984375,-0.813632449486927811221903539262712001800537109375,1.5267881254572674976088819676078855991363525390625}
{
#if Dimensions==2
  file_stream.open("src/exahype2/dg/tests/results2D.log");
#else
  file_stream.open("src/exahype2/dg/tests/results3D.log");
#endif
}


void exahype2::dg::tests::DGTest::run() {
  testMethod(UtilsTest);
  testMethod(runDG);
}

void exahype2::dg::tests::DGTest::UtilsTest() {

  const int nodesPerAxis = 4;
  const int nodesPerCell = getNodesPerCell(nodesPerAxis);

  validateEquals(nodesPerCell, std::pow(nodesPerAxis,Dimensions));

  tarch::la::Vector<Dimensions,int> strides = getStrides(nodesPerAxis);
  validateEquals(strides[Dimensions-1], std::pow(nodesPerAxis,Dimensions-1));

  tarch::la::Vector<Dimensions,int> index = getIndex(nodesPerCell-1, strides);
  for(int i=0; i<Dimensions; i++){
    validateEquals(index[i], nodesPerAxis-1);
  }

}

void exahype2::dg::tests::DGTest::runDG(){

  // geometry
  const tarch::la::Vector<Dimensions, double> x({0.0, 0.0});
  const double dx = 0.01;
  const double t  = 0.0;
  const double dt = 0.01;

  //using euler equations as example
  const int unknowns = 5;
  const int auxiliaryVariables = 0;
  const int order = 3;

  const int nodesPerAxis = order+1;
  const int nodesPerCell = getNodesPerCell(order+1);
  const int nodesPerFace = nodesPerCell/nodesPerAxis;
  const int strideQ = unknowns+auxiliaryVariables;

  double*  QIn     = new double[nodesPerCell*(unknowns+auxiliaryVariables)]{1.0};
  double*  QOut     = new double[2*nodesPerCell*(unknowns+auxiliaryVariables)]{0.0};
  double* QLeft   = new double[2*nodesPerFace*(unknowns+auxiliaryVariables)]{0.0};
  double* QRight   = new double[2*nodesPerFace*(unknowns+auxiliaryVariables)]{0.0};
  double* QLower   = new double[2*nodesPerFace*(unknowns+auxiliaryVariables)]{0.0};
  double* QUpper   = new double[2*nodesPerFace*(unknowns+auxiliaryVariables)]{0.0};
  #if Dimensions==3
  double* QFront   = new double[2*nodesPerFace*(unknowns+auxiliaryVariables)]{0.0};
  double* QBack   = new double[2*nodesPerFace*(unknowns+auxiliaryVariables)]{0.0};
  #endif

  //initialisation
  for(int i=0; i<nodesPerCell*(unknowns+auxiliaryVariables); i++){
    QIn[i] = 1.0;
  }

  file_stream << "Starting values are: \n";
  for(int i=0; i<nodesPerCell; i++){
    file_stream << "node " << i << ": [";
        for(int var=0; var<strideQ;var++){
          file_stream << QIn[i*(order+1)  +var] << ", ";
        }
    file_stream << "]\n";
  }

  explicitEulerOnCell(
      QIn,
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
          int                                          normal,
          double * __restrict__                        F
          ) -> void {
          eulerFlux(Q,x,t,normal,F);
        },
      nullptr, //ncp
      [&](
          const double * __restrict__                  Q,
          const tarch::la::Vector<Dimensions,double>&  x,
          double                                       t,
          double * __restrict__                        S
        ) -> void { eulerSource(Q,x,t,S);
        },
      QuadraturePoints,
      QuadratureWeights,
      MassMatrix,
      StiffnessOperator,
      QOut);

  file_stream << "\nComputed volume integral values are: \n";
  for(int i=0; i<nodesPerCell; i++){
    file_stream << "node " << i << ": [";
        for(int var=0; var<strideQ;var++){
          file_stream << QOut[i*(order+1)  +var] << ", ";
          validateNumericalEquals(QOut[i*nodesPerAxis  +var], QIn[i*nodesPerAxis  +var]);
        }
    file_stream << "]\n";
  }

  projectVolumetricDataOntoFaces(
    QOut,
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

  file_stream << "\nProjected Face values are: \n Left Face:\n";
  for(int i=0; i<2*nodesPerFace; i++){
    file_stream << "node " << i << ": [";
    for(int var=0; var<strideQ; var++){
      file_stream << QLeft[i*strideQ  + var] << ", ";
      if(i%2==1){
        validateNumericalEquals(QLeft[i*strideQ  +var], 1.0);
      }
    }
    file_stream << "]\n";
  }

  file_stream << "Right Face:\n";
  for(int i=0; i<2*nodesPerFace; i++){
    file_stream << "node " << i << ": [";
    for(int var=0; var<strideQ;var++){
      file_stream << QRight[i*strideQ + var] << ", ";
      if(i%2==0){
        validateNumericalEquals(QRight[i*strideQ  +var], 1.0);
      }
    }
    file_stream << "]\n";
  }

  file_stream << "Lower Face:\n";
  for(int i=0; i<2*nodesPerFace; i++){
    file_stream << "node " << i << ": [";
    for(int var=0; var<strideQ;var++){
      file_stream << QLower[i*strideQ + var] << ", ";
      if(i%2==1){
        validateNumericalEquals(QLower[i*strideQ  +var], 1.0);
      }
    }
    file_stream << "]\n";
  }

  file_stream << "Upper Face:\n";
  for(int i=0; i<2*nodesPerFace; i++){
    file_stream << "node " << i << ": [";
    for(int var=0; var<strideQ;var++){
      file_stream << QUpper[i*strideQ  + var] << ", ";
      if(i%2==0){
        validateNumericalEquals(QUpper[i*strideQ  +var], 1.0);
      }
    }
    file_stream << "]\n";
  }

#if Dimension==3
  file_stream << "Lower Face:\n";
  for(int i=0; i<2*nodesPerFace; i++){
    file_stream << "node " << i << ": [";
    for(int var=0; var<strideQ;var++){
      file_stream << QLower[i*strideQ + var] << ", ";
      if(i%2==1){
        validateNumericalEquals(QLower[i*strideQ  +var], 1.0);
      }
    }
    file_stream << "]\n";
  }

  file_stream << "Upper Face:\n";
  for(int i=0; i<2*nodesPerFace; i++){
    file_stream << "node " << i << ": [";
    for(int var=0; var<strideQ;var++){
      file_stream << QUpper[i*strideQ  + var] << ", ";
      if(i%2==0){
        validateNumericalEquals(QUpper[i*strideQ  +var], 1.0);
      }
    }
    file_stream << "]\n";
  }
#endif

  // project data onto opposite face as well to trade with "identical" cell
  projectVolumetricDataOntoFaces(
    QOut,
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

  getMaxEigenvaluePerFace(
    [&](
      const double * __restrict__                 Q,
      const tarch::la::Vector<Dimensions,double>& x,
      double                                      t,
      int                                          normal
      ) -> double {
        return eulerEigenvalue(Q,x,t,normal);
      },
    QHull,
    order,
    unknowns,
    auxiliaryVariables,
    x,
    t,
    maxEigenvaluePerFace
  );

  file_stream << "\nMax Eigenvalues per Face:\n";
  for(int i=0; i<Dimensions*2; i++){
    file_stream << "Max Eigenvalues at face " << i << ": " << maxEigenvaluePerFace[i] << std::endl;
    validateNumericalEquals(maxEigenvaluePerFace[i], 1.0);
  }

  double* QHullForRiemann = new double[2*Dimensions*nodesPerFace*strideQ]{0.0};
  computeRusanovOnHull(
      [&](
          const double * __restrict__                  Q,
          const tarch::la::Vector<Dimensions,double>&  x,
          double                                       t,
          int                                          normal,
          double * __restrict__                        F
          ) -> void {
          eulerFlux(Q,x,t,normal,F);
        },
        [&](
          const double * __restrict__                  Qinside,
          double * __restrict__                         Qoutside,
          const tarch::la::Vector<Dimensions,double>&  x,
          double                                       t,
          int                                          normal
        ) -> void {
          eulerBoundaryConditions(Qinside, Qoutside, x, t, normal);
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
        atBoundary
        );

  file_stream << "\nRusanov Fluxes per Face:\n";
  for(int f=0; f<2*Dimensions; f++){
    file_stream << "At face " << f << "\n";
    for(int i=0; i<nodesPerFace; i++){
      file_stream << "at node " << i << ": [";
      for(int var=0; var<strideQ; var++){
        file_stream << QHullForRiemann[(f*nodesPerFace+i)*strideQ+var] << ", ";
        validateNumericalEquals(QHullForRiemann[(f*nodesPerFace+i)*strideQ+var], 0.0);
      }
      file_stream << "]\n";
    }
  }

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
        for(int var=0; var<strideQ;var++){
          file_stream << QOut[i*(order+1)  +var] << ", ";
          validateNumericalEquals(QOut[i*(order+1)+var], QIn[i*(order+1)+var]);
        }
    file_stream << "]\n";
  }

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

}

