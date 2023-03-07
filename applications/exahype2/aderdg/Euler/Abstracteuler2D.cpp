#include <algorithm>


#include "Abstracteuler2D.h"
#include "Constants.h"


#include "tarch/multicore/Lock.h"
#include "exahype2/NonCriticalAssertions.h"


tarch::logging::Log   examples::exahype2::euler2D::Abstracteuler2D::Abstracteuler2D::_log( "examples::exahype2::euler2D::Abstracteuler2D::Abstracteuler2D" );

std::bitset<Dimensions> examples::exahype2::euler2D::Abstracteuler2D::Abstracteuler2D::PeriodicBC = examples::exahype2::euler2D::PeriodicBC;

double**   exahype2::solvers::aderdg::ADERDGSolver::weights                                 = kernels::legendre::weights;
double**   exahype2::solvers::aderdg::ADERDGSolver::nodes                                   = kernels::legendre::nodes;
double***  exahype2::solvers::aderdg::ADERDGSolver::Kxi                                     = kernels::legendre::Kxi;
double***  exahype2::solvers::aderdg::ADERDGSolver::dudx                                    = kernels::legendre::dudx;
double***  exahype2::solvers::aderdg::ADERDGSolver::iK1                                     = kernels::legendre::iK1;
double***  exahype2::solvers::aderdg::ADERDGSolver::equidistantGridProjector                = kernels::legendre::equidistantGridProjector;
double***  exahype2::solvers::aderdg::ADERDGSolver::FCoeff                                  = kernels::legendre::FCoeff;
double**** exahype2::solvers::aderdg::ADERDGSolver::fineGridProjector                       = kernels::legendre::fineGridProjector;
kernels::UnivariateFunction** exahype2::solvers::aderdg::ADERDGSolver::basisFunction                 = kernels::legendre::basisFunction;
kernels::UnivariateFunction** exahype2::solvers::aderdg::ADERDGSolver::basisFunctionFirstDerivative  = kernels::legendre::basisFunctionFirstDerivative;
kernels::UnivariateFunction** exahype2::solvers::aderdg::ADERDGSolver::basisFunctionSecondDerivative = kernels::legendre::basisFunctionSecondDerivative;


examples::exahype2::euler2D::Abstracteuler2D::Abstracteuler2D():
  QuadraturePoints1d {0.069431844202973713731097404888714663684368133544921875,0.33000947820757187134432797392946667969226837158203125,0.66999052179242812865567202607053332030773162841796875,0.9305681557970262307577513638534583151340484619140625} ,
  QuadratureWeights {0.1739274225687269248563637802362791262567043304443359375,0.32607257743127304738806060413480736315250396728515625,0.32607257743127304738806060413480736315250396728515625,0.1739274225687269248563637802362791262567043304443359375} ,
  BarycentricWeights {-7.4205400680389477230392003548331558704376220703125,18.79544940755506132745722425170242786407470703125,-18.79544940755506132745722425170242786407470703125,7.42054006803894861121762005495838820934295654296875} ,
  BasisFunctionValuesLeft {1.5267881254572663873858573424513451755046844482421875,-0.8136324494869271450880887641687877476215362548828125,0.40076152031165046540905905203544534742832183837890625,-0.11391719628198997138479597879268112592399120330810546875} ,
  BasisFunctionValuesRight {-0.113917196281990040773735017864964902400970458984375,0.400761520311650742964815208324580453336238861083984375,-0.813632449486927811221903539262712001800537109375,1.5267881254572674976088819676078855991363525390625} ,
  DerivativeOperator {-6.6640004727045631938153746887110173702239990234375,9.7203088313703940315235740854404866695404052734375,-4.21756469699035818621268845163285732269287109375,1.161256338324529568950538305216468870639801025390625,-1.5151152295984677831341969067580066621303558349609375,-0.7688287844464174458636307463166303932666778564453125,2.941340462561433444221847821609117090702056884765625,-0.6573964485165488813578349436284042894840240478515625,0.65739644851654832624632263105013407766819000244140625,-2.941340462561433444221847821609117090702056884765625,0.768828784446416335640606121160089969635009765625,1.515115229598468449268011681851930916309356689453125,-1.161256338324528680772118605091236531734466552734375,4.21756469699035729803426875150762498378753662109375,-9.720308831370392255166734685190021991729736328125,6.66400047270456497017221408896148204803466796875} ,
  MassMatrix {0.1739274225687269248563637802362791262567043304443359375,0.0,0.0,0.0,0.0,0.32607257743127304738806060413480736315250396728515625,0.0,0.0,0.0,0.0,0.32607257743127304738806060413480736315250396728515625,0.0,0.0,0.0,0.0,0.1739274225687269248563637802362791262567043304443359375} ,
  StiffnessOperator {-1.1590524262142825051569161587394773960113525390625,-0.494037528020547400675610560938366688787937164306640625,0.214358954361956122180998818294028751552104949951171875,-0.201974321866382811041518152705975808203220367431640625,1.69062826161228674237690938753075897693634033203125,-0.250693983347795967819848783619818277657032012939453125,-0.95909046573029943516530693159438669681549072265625,0.73355015726438665968345276269246824085712432861328125,-0.7335501572643867707057552252081222832202911376953125,0.95909046573029943516530693159438669681549072265625,0.250693983347795634752941396072856150567531585693359375,-1.690628261612286298287699537468142807483673095703125,0.2019743218663829775749718464794568717479705810546875,-0.21435895436195628871445251206750981509685516357421875,0.494037528020547622720215485969674773514270782470703125,1.159052426214282949246126008802093565464019775390625} ,
  InvertedPredictorLhsOperator {0.546435362419644660267499525208023669620160944759845733642578125,-0.14432618329325673533526652736469486626447178423404693603515625,0.1014623598288631781773111092959105405952868750318884849548339844,-0.06687578310368458280271795196592066190532932523638010025024414063,1.0188533167713026317442726043083212061901576817035675048828125,0.5847599728573232642954426996340089317527599632740020751953125,-0.1702637242678432449784643037959952494020399171859025955200195313,0.1014623598288624077161422867843221240491402568295598030090332031,1.0240105066930918847022125017787175238481722772121429443359375,1.0007437785531960714770216558378024274134077131748199462890625,0.58475997285732400610656911421614267965196631848812103271484375,-0.1443261832932565697233846802038925716260564513504505157470703125,0.97400505826439649705202061813480440832790918648242950439453125,1.02401050669309141394162920857269227781216613948345184326171875,1.0188533167713030021077347253566358631360344588756561279296875,0.546435362419644295325048266587231182711548171937465667724609375} ,
  EquidistantGridProjector {1.5267881254572663873858573424513451755046844482421875,-0.0049591884824765342099084364235750399529933929443359375,-0.0021913277260105974188209021491502426215447485446929931640625,-0.113917196281990040773735017864964902400970458984375,-0.8136324494869271450880887641687877476215362548828125,0.997304018973186767738070557243190705776214599609375,0.0098464972353006600946923043693459476344287395477294921875,0.400761520311650742964815208324580453336238861083984375,0.40076152031165046540905905203544534742832183837890625,0.00984649723530049529596208657267197850160300731658935546875,0.99730401897318643467116316969622857868671417236328125,-0.813632449486927811221903539262712001800537109375,-0.11391719628198997138479597879268112592399120330810546875,-0.00219132772601056229067051361880658078007400035858154296875,-0.004959188482476617476635283310315571725368499755859375,1.5267881254572674976088819676078855991363525390625} ,
  FineGridProjector{ {1.3365809435384148340375531915924511849880218505859375,0.75017378205554174908087361473008058965206146240234375,0.25006831351327185597455127208377234637737274169921875,0.03282922721362747930928804862560355104506015777587890625,-0.5106599509271045889136075857095420360565185546875,0.350399125663716670686653742450289428234100341796875,0.9137546431442424843538674394949339330196380615234375,1.01007139472141194147525311564095318317413330078125,0.2422582771987365768406874622087343595921993255615234375,-0.1376638598070825392216676164025557227432727813720703125,-0.2182390043731604889476471953457803465425968170166015625,-0.055641038587549929150810612554778344929218292236328125,-0.068179269810046794209057452462729997932910919189453125,0.037090952087824181904185394387241103686392307281494140625,0.054416047715646100046971156416475423611700534820556640625,0.01274041665251061418440148287345436983741819858551025390625} , {-0.035350042596419377349814539002181845717132091522216796875,-0.0928683884767426970352488524440559558570384979248046875,-0.071267786528997401074292383782449178397655487060546875,-0.01767502129820965051099079801133484579622745513916015625,0.97104619867609065497759956997469998896121978759765625,0.7760907833371601949323803637525998055934906005859375,0.388045391668580041955038950618472881615161895751953125,0.08197886521853813002191913028582348488271236419677734375,0.081978865218538310433160631873761303722858428955078125,0.38804539166857987542158525684499181807041168212890625,0.776090783337159972887775438721291720867156982421875,0.97104619867609087702220449500600807368755340576171875,-0.0176750212982097025526950773155476781539618968963623046875,-0.07126778652899741495208019159690593369305133819580078125,-0.0928683884767427525464000837018829770386219024658203125,-0.035350042596419335716451115558811579830944538116455078125} , {0.01274041665251053785656853989394221571274101734161376953125,0.054416047715646016780244309529734891839325428009033203125,0.037090952087824084759670739686043816618621349334716796875,-0.0681792698100467664534818368338164873421192169189453125,-0.055641038587549630778372744543958106078207492828369140625,-0.2182390043731603779253447328301263041794300079345703125,-0.1376638598070822616659114601134206168353557586669921875,0.242258277198736549085111846579820849001407623291015625,1.01007139472141194147525311564095318317413330078125,0.9137546431442427063984723645262420177459716796875,0.350399125663715838019385273582884110510349273681640625,-0.5106599509271045889136075857095420360565185546875,0.032829227213627291959152643130437354557216167449951171875,0.250068313513271689441097578310291282832622528076171875,0.7501737820555423041923859273083508014678955078125,1.3365809435384148340375531915924511849880218505859375} },

  _solverState(SolverState::GridConstruction),
  _minTimeStamp(0.0),
  _maxTimeStamp(0.0),
  _minTimeStampThisTimeStep(0.0),
  _maxTimeStampThisTimeStep(0.0),
  _minCellH(0.0),
  _maxCellH(0.0),
  _minCellHThisTimeStep(0.0),
  _maxCellHThisTimeStep(0.0),
  _minTimeStepSize(std::numeric_limits<double>::max()),
  _maxTimeStepSize(0.0),
  _minTimeStepSizeThisTimeStep(std::numeric_limits<double>::max()),
  _maxTimeStepSizeThisTimeStep(0.0),
  _cellUpdates(0) {

  _admissibleTimeStepSize = 0.0;_admissibleTimeStepSize = 0.0;

  
  //initialise nodes, weights and matrices for use of the kernels
  generated::kernels::AderDG::nonlinear::initQuadratureNodesAndWeights();
  generated::kernels::AderDG::nonlinear::initDGMatrices();
  
  
  //setting coarsestMeshLevel and maxAdaptiveMeshLevel
  double maxDSize = std::numeric_limits<double>::min();
  double minDSize = std::numeric_limits<double>::max();
  tarch::la::Vector<Dimensions, double> sizes = examples::exahype2::euler2D::DomainSize;
  
  for(int i=0; i<Dimensions; i++){
    if(sizes[i]>maxDSize){
      maxDSize = sizes[i];
    }
    if(sizes[i]<minDSize){
      minDSize = sizes[i];
    }    
  }
  
  int depth = 0;
  
  //finding coarsest possible Mesh level
  while(maxDSize>MaxAdmissibleCellH){
    depth += 1;
    maxDSize /= 3.0;
  }
  
  realCoarsestMeshSize = maxDSize;
  coarsestMeshLevel = depth;  
  
  depth = 0;
  while(minDSize>MinAdmissibleCellH){
    depth += 1;
    minDSize /= 3.0;
  }
  
  maxAdaptiveMeshLevel = depth;
  

}

examples::exahype2::euler2D::Abstracteuler2D::~Abstracteuler2D(){

  
  //free the generated nodes, weights and matrices
  generated::kernels::AderDG::nonlinear::freeQuadratureNodesAndWeights();
  generated::kernels::AderDG::nonlinear::freeDGMatrices();
  
}


double examples::exahype2::euler2D::Abstracteuler2D::getMaxMeshSize() const {
  return getMaxCellSize();
}


double examples::exahype2::euler2D::Abstracteuler2D::getMinMeshSize() const {
  return getMinCellSize();
}


tarch::la::Vector<Dimensions,double> examples::exahype2::euler2D::Abstracteuler2D::getDomainSize() const {
  return examples::exahype2::euler2D::DomainSize;

}


tarch::la::Vector<Dimensions,double> examples::exahype2::euler2D::Abstracteuler2D::getDomainOffset() const {
  return examples::exahype2::euler2D::DomainOffset;
}


double examples::exahype2::euler2D::Abstracteuler2D::getCoarsestMeshSize() const {
  return realCoarsestMeshSize;
}


int examples::exahype2::euler2D::Abstracteuler2D::getCoarsestMeshLevel() const {
  return coarsestMeshLevel;
}


int examples::exahype2::euler2D::Abstracteuler2D::getMaximumAdaptiveMeshLevel() const {
  return maxAdaptiveMeshLevel;
}


int examples::exahype2::euler2D::Abstracteuler2D::getMaximumAdaptiveMeshDepth() const {
  return (maxAdaptiveMeshLevel- coarsestMeshLevel);
}


double examples::exahype2::euler2D::Abstracteuler2D::getMinCellSize(bool ofCurrentlyRunningGridSweep) const {
  return ofCurrentlyRunningGridSweep ? _minCellHThisTimeStep : _minCellH;
}


double examples::exahype2::euler2D::Abstracteuler2D::getMaxCellSize(bool ofCurrentlyRunningGridSweep) const {
  return ofCurrentlyRunningGridSweep ? _maxCellHThisTimeStep : _maxCellH;
}


double examples::exahype2::euler2D::Abstracteuler2D::getMinTimeStamp(bool ofCurrentlyRunningGridSweep) const {
  return ofCurrentlyRunningGridSweep ? _minTimeStampThisTimeStep : _minTimeStamp;
}


double examples::exahype2::euler2D::Abstracteuler2D::getMaxTimeStamp(bool ofCurrentlyRunningGridSweep) const {
  return ofCurrentlyRunningGridSweep ? _maxTimeStampThisTimeStep : _maxTimeStamp;
}


double examples::exahype2::euler2D::Abstracteuler2D::getMinTimeStepSize() const {
  return _minTimeStepSize;
}


double examples::exahype2::euler2D::Abstracteuler2D::getMaxTimeStepSize() const {
  return _maxTimeStepSize;
}


void examples::exahype2::euler2D::Abstracteuler2D::update(double timeStepSize, double timeStamp, double cellSize) {
  tarch::multicore::Lock lock(_semaphore);

  if ( tarch::la::greater(timeStepSize,0.0) ) {
    assertion1(timeStepSize<std::numeric_limits<double>::max()/10.0,timeStepSize);
    assertion1(timeStepSize>=0.0,timeStepSize);
    _minTimeStepSizeThisTimeStep = std::min(timeStepSize,_minTimeStepSizeThisTimeStep);
    _maxTimeStepSizeThisTimeStep = std::max(timeStepSize,_maxTimeStepSizeThisTimeStep);
    assertion2(_minTimeStepSizeThisTimeStep<=_maxTimeStepSizeThisTimeStep, _minTimeStepSizeThisTimeStep, _maxTimeStepSizeThisTimeStep );

    _cellUpdates++;

    _localMinTimeStampThisTimeStep = std::min(_localMinTimeStampThisTimeStep, timeStamp);
    _localMaxTimeStampThisTimeStep = std::max(_localMaxTimeStampThisTimeStep, timeStamp);
  }

  assertion1(timeStamp<std::numeric_limits<double>::max()/10.0,timeStamp);
  assertion1(timeStamp>=0.0,timeStamp);
  _maxTimeStampThisTimeStep = std::max(timeStamp,_maxTimeStampThisTimeStep);
  _minTimeStampThisTimeStep = std::min(timeStamp,_minTimeStampThisTimeStep);
  assertion6( tarch::la::smallerEquals(_minTimeStamp,_maxTimeStampThisTimeStep), _minTimeStamp, _minTimeStampThisTimeStep, _maxTimeStampThisTimeStep, timeStepSize, timeStamp, cellSize );

  assertion1(cellSize<std::numeric_limits<double>::max()/10.0,cellSize);
  assertion1(cellSize>0.0,cellSize);
  _maxCellHThisTimeStep = std::max(_maxCellHThisTimeStep,cellSize);
  _minCellHThisTimeStep = std::min(_minCellHThisTimeStep,cellSize);

  logDebug( "update(double,double,double)", "updated with dt=" << timeStepSize << ", t=" << timeStamp << ", h=" << cellSize << ": t(new)=" << _minTimeStampThisTimeStep );
}


examples::exahype2::euler2D::Abstracteuler2D::SolverState examples::exahype2::euler2D::Abstracteuler2D::getSolverState() const {
  return _solverState;
}











void examples::exahype2::euler2D::Abstracteuler2D::startGridConstructionStep() {
  assertion( _solverState == SolverState::GridConstruction );
}


void examples::exahype2::euler2D::Abstracteuler2D::finishGridConstructionStep() {
}


void examples::exahype2::euler2D::Abstracteuler2D::startGridInitialisationStep() {
  _solverState = SolverState::GridInitialisation;
}


void examples::exahype2::euler2D::Abstracteuler2D::finishGridInitialisationStep() {
}


void examples::exahype2::euler2D::Abstracteuler2D::startTimeStep(
  double globalMinTimeStamp,
  double globalMaxTimeStamp,
  double globalMinTimeStepSize,
  double globalMaxTimeStepSize
) {
  switch (_solverState) {
    case SolverState::GridConstruction:
      _solverState = SolverState::GridInitialisation;
      break;
    case SolverState::GridInitialisation:
      _solverState = SolverState::Prediction;
      break;
    case SolverState::Plotting:
      _solverState = SolverState::Prediction;
      break;
    case SolverState::Prediction:
      _solverState = SolverState::Correction;
      break;
    case SolverState::Correction:
      _solverState = SolverState::Prediction;
      break;
  }

  
  if (
    tarch::mpi::Rank::getInstance().isGlobalMaster() 
    and
    _maxCellH>0.0
  ) {
    logInfo( "startTimeStep()", "Solver euler2D:" );
    logInfo( "startTimeStep()", "t            = " << _minTimeStampThisTimeStep );
    logInfo( "startTimeStep()", "dt           = " << getAdmissibleTimeStepSize() );
    logInfo( "startTimeStep()", "h_{min}      = " << _minCellHThisTimeStep << " (volume size)");
    logInfo( "startTimeStep()", "h_{max}      = " << _maxCellHThisTimeStep << " (volume size)" );
    logInfo( "startTimeStep()", "lambda_{max} = " << _maxEigenvalue );
  }
if (_solverState == SolverState::Correction) {
  _maxEigenvalue = 0.0;
}

  if (_solverState==SolverState::Prediction) {
    _minTimeStamp = _minTimeStampThisTimeStep;
    _maxTimeStamp = _maxTimeStampThisTimeStep;

    _minTimeStampThisTimeStep = std::numeric_limits<double>::max();
    _maxTimeStampThisTimeStep = std::numeric_limits<double>::min();

    _minTimeStepSize = _minTimeStepSizeThisTimeStep;
    _maxTimeStepSize = _maxTimeStepSizeThisTimeStep;

    _minTimeStepSizeThisTimeStep = std::numeric_limits<double>::max();
    _maxTimeStepSizeThisTimeStep = std::numeric_limits<double>::min();

    _minCellH = _minCellHThisTimeStep;
    _maxCellH = _maxCellHThisTimeStep;

    _maxCellHThisTimeStep      = 0.0;
    _minCellHThisTimeStep      = std::numeric_limits<double>::max();

    _localMinTimeStampThisTimeStep = std::numeric_limits<double>::max();
    _localMaxTimeStampThisTimeStep = std::numeric_limits<double>::min();

    _cellUpdates    = 0;
  }

  logInfo( "startTimeStep(...)", "phase " + toString(_solverState) );

}


void examples::exahype2::euler2D::Abstracteuler2D::finishTimeStep() {
	if ( _solverState==SolverState::Correction ) {

  #ifdef Parallel
  double newMinTimeStamp    = _minTimeStampThisTimeStep;
  double newMaxTimeStamp    = _maxTimeStampThisTimeStep;
  double newMinCellH        = _minCellHThisTimeStep;
  double newMaxCellH        = _maxCellHThisTimeStep;
  double newMinTimeStepSize = _minTimeStepSizeThisTimeStep;
  double newMaxTimeStepSize = _maxTimeStepSizeThisTimeStep;

  double newLocalMinTimeStamp = _localMinTimeStampThisTimeStep;
  double newLocalMaxTimeStamp = _localMaxTimeStampThisTimeStep;

  int newCellUpdates = _cellUpdates;

  tarch::mpi::Rank::getInstance().allReduce(
      &newMinTimeStamp,
      &_minTimeStampThisTimeStep,
      1, MPI_DOUBLE,
      MPI_MIN,
      [&]() -> void { tarch::services::ServiceRepository::getInstance().receiveDanglingMessages(); }
      );
  tarch::mpi::Rank::getInstance().allReduce(
      &newMaxTimeStamp,
      &_maxTimeStampThisTimeStep,
      1, MPI_DOUBLE,
      MPI_MAX,
      [&]() -> void { tarch::services::ServiceRepository::getInstance().receiveDanglingMessages(); }
      );

  tarch::mpi::Rank::getInstance().allReduce(
      &newMinCellH,
      &_minCellHThisTimeStep,
      1, MPI_DOUBLE,
      MPI_MIN,
      [&]() -> void { tarch::services::ServiceRepository::getInstance().receiveDanglingMessages(); }
      );
  tarch::mpi::Rank::getInstance().allReduce(
      &newMaxCellH,
      &_maxCellHThisTimeStep,
      1, MPI_DOUBLE,
      MPI_MAX,
      [&]() -> void { tarch::services::ServiceRepository::getInstance().receiveDanglingMessages(); }
      );

  tarch::mpi::Rank::getInstance().allReduce(
      &newMinTimeStepSize,
      &_minTimeStepSizeThisTimeStep,
      1, MPI_DOUBLE,
      MPI_MIN,
      [&]() -> void { tarch::services::ServiceRepository::getInstance().receiveDanglingMessages(); }
      );
  tarch::mpi::Rank::getInstance().allReduce(
      &newMaxTimeStepSize,
      &_maxTimeStepSizeThisTimeStep,
      1, MPI_DOUBLE,
      MPI_MAX,
      [&]() -> void { tarch::services::ServiceRepository::getInstance().receiveDanglingMessages(); }
      );

  tarch::mpi::Rank::getInstance().allReduce(
      &newLocalMinTimeStamp,
      &_localMinTimeStampThisTimeStep,
      1, MPI_DOUBLE,
      MPI_MIN,
      [&]() -> void { tarch::services::ServiceRepository::getInstance().receiveDanglingMessages(); }
      );
  tarch::mpi::Rank::getInstance().allReduce(
      &newLocalMaxTimeStamp,
      &_localMaxTimeStampThisTimeStep,
      1, MPI_DOUBLE,
      MPI_MAX,
      [&]() -> void { tarch::services::ServiceRepository::getInstance().receiveDanglingMessages(); }
      );

  tarch::mpi::Rank::getInstance().allReduce(
      &newCellUpdates,
      &_cellUpdates,
      1, MPI_INT,
      MPI_SUM,
      [&]() -> void { tarch::services::ServiceRepository::getInstance().receiveDanglingMessages(); }
      );
  #endif

		if ( _minTimeStampThisTimeStep>=std::numeric_limits<double>::max()/10.0) {
			_minTimeStampThisTimeStep = 0.0;
		}
		if ( _maxTimeStampThisTimeStep>=std::numeric_limits<double>::max()/10.0 ) {
			_maxTimeStampThisTimeStep = 0.0;
		}
	}

  
  #ifdef Parallel
  double newMaxEigenvalue = _maxEigenvalue;
  tarch::mpi::Rank::getInstance().allReduce(
        &newMaxEigenvalue,
        &_maxEigenvalue,
        1,
        MPI_DOUBLE,
        MPI_MAX, 
        [&]() -> void { tarch::services::ServiceRepository::getInstance().receiveDanglingMessages(); }
        );
  #endif

  if (_solverState == SolverState::Correction) {  
    if ( tarch::la::smaller(_maxEigenvalue, 0.0 ) ) {
      ::exahype2::triggerNonCriticalAssertion( __FILE__, __LINE__, "_maxEigenvalue>=0", "invalid max eigenvalue: " + std::to_string(_maxEigenvalue) );
      // keep time step size invariant
      // _admissibleTimeStepSize = _admissibleTimeStepSize;
    }
    else if ( tarch::la::equals(_maxEigenvalue, 0.0 ) ) {
      logWarning( "finishTimeStep(...)", "maximum eigenvalue approaches 0.0. For nonlinear PDEs, this often means the PDE becomes stationary. It could also be a bug however" ); 
      _admissibleTimeStepSize = 0.0;
    }
    else {
      const double minVolumeSize = _minCellHThisTimeStep;
      _admissibleTimeStepSize = CFL * PNPM / _maxEigenvalue; //the max eigenvalue here is already scaled with the volume size
      if ( std::isnan(_admissibleTimeStepSize) or std::isinf(_admissibleTimeStepSize) ) {
        ::exahype2::triggerNonCriticalAssertion( __FILE__, __LINE__, "_admissibleTimeStepSize>0", "invalid (NaN of inf) time step size: " + std::to_string(_admissibleTimeStepSize) );
      }
      if (tarch::la::smallerEquals(_admissibleTimeStepSize,0.0,1e-10) ) {
        logWarning( "finishTimeStep(...)", "degenerated time step size of " << std::to_string(_admissibleTimeStepSize) << ". Problem might be extremely stiff (and can't be solved) or there could be a bug (h_volume=" << minVolumeSize << ")" );
      }
    }
  }
}


void examples::exahype2::euler2D::Abstracteuler2D::startPlottingStep(
  double globalMinTimeStamp,
  double globalMaxTimeStamp,
  double globalMinTimeStepSize,
  double globalMaxTimeStepSize
) {
  assertion( mayPlot() );
  _solverState = SolverState::Plotting;
}


void examples::exahype2::euler2D::Abstracteuler2D::finishPlottingStep() {
}


std::string examples::exahype2::euler2D::Abstracteuler2D::toString(SolverState state) {
  switch (state) {
    case SolverState::GridConstruction:
      return "grid-construction";
    case SolverState::GridInitialisation:
      return "grid-initialisation";
    case SolverState::Plotting:
      return "plotting";
    case SolverState::Prediction:
      return "prediction";
      break;
    case SolverState::Correction:
			return "correction";
  }
  return "<undef>";
}


bool examples::exahype2::euler2D::Abstracteuler2D::mayPlot() const {
  return isLastGridSweepOfTimeStep()
      or _solverState==SolverState::GridInitialisation
      or _solverState==SolverState::Plotting;
}


bool examples::exahype2::euler2D::Abstracteuler2D::isFirstGridSweepOfTimeStep() const {
  return _solverState==SolverState::Prediction;
}


bool examples::exahype2::euler2D::Abstracteuler2D::isLastGridSweepOfTimeStep() const {
  return _solverState==SolverState::Correction;
}


void examples::exahype2::euler2D::Abstracteuler2D::startSimulation() {

}


void examples::exahype2::euler2D::Abstracteuler2D::finishSimulation() {

}



















void ::examples::exahype2::euler2D::Abstracteuler2D::setMaxEigenvalue( double eigenvalue ) {
  if ( tarch::la::greater( eigenvalue, 0.0 ) ) {
    tarch::multicore::Lock lock(_semaphore);
    _maxEigenvalue = std::max(_maxEigenvalue,eigenvalue);
  }
}    


double ::examples::exahype2::euler2D::Abstracteuler2D::getAdmissibleTimeStepSize() const {
  return _admissibleTimeStepSize;
}