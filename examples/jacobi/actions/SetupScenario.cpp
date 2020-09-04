#include "SetupScenario.h"


tarch::logging::Log examples::jacobi::actions::SetupScenario::_log( "examples::jacobi::actions::SetupScenario");


double examples::jacobi::actions::SetupScenario::Theta = 0.0;


examples::jacobi::actions::SetupScenario::SetupScenario(int treeNumber) {
// @todo Please implement
}


examples::jacobi::actions::SetupScenario::~SetupScenario() {
// @todo Please implement
}


std::vector< peano4::grid::GridControlEvent > examples::jacobi::actions::SetupScenario::getGridControlEvents() {
  return std::vector< peano4::grid::GridControlEvent >();
}


void examples::jacobi::actions::SetupScenario::beginTraversal() {}
void examples::jacobi::actions::SetupScenario::endTraversal() {}


void examples::jacobi::actions::SetupScenario::createPersistentVertex(
      const peano4::datamanagement::VertexMarker& marker,
      examples::jacobi::vertexdata::MG& fineGridVertexMG,
      peano4::datamanagement::VertexEnumerator<examples::jacobi::vertexdata::MG> coarseGridVerticesMG) {}

void examples::jacobi::actions::SetupScenario::destroyPersistentVertex(
      const peano4::datamanagement::VertexMarker& marker,
      examples::jacobi::vertexdata::MG& fineGridVertexMG,
      peano4::datamanagement::VertexEnumerator<examples::jacobi::vertexdata::MG> coarseGridVerticesMG) {}

void examples::jacobi::actions::SetupScenario::createHangingVertex(
      const peano4::datamanagement::VertexMarker& marker,
      examples::jacobi::vertexdata::MG& fineGridVertexMG,
      peano4::datamanagement::VertexEnumerator<examples::jacobi::vertexdata::MG> coarseGridVerticesMG) {}

void examples::jacobi::actions::SetupScenario::destroyHangingVertex(
      const peano4::datamanagement::VertexMarker& marker,
      examples::jacobi::vertexdata::MG& fineGridVertexMG,
      peano4::datamanagement::VertexEnumerator<examples::jacobi::vertexdata::MG> coarseGridVerticesMG) {}


void examples::jacobi::actions::SetupScenario::touchVertexFirstTime(
  const peano4::datamanagement::VertexMarker& marker,
  examples::jacobi::vertexdata::MG& fineGridVertexMG,
  peano4::datamanagement::VertexEnumerator<examples::jacobi::vertexdata::MG> coarseGridVerticesMG
) {
  init( marker.x(), fineGridVertexMG );
}


void examples::jacobi::actions::SetupScenario::touchVertexLastTime(
      const peano4::datamanagement::VertexMarker& marker,
      examples::jacobi::vertexdata::MG& fineGridVertexMG,
      peano4::datamanagement::VertexEnumerator<examples::jacobi::vertexdata::MG> coarseGridVerticesMG) {}

void examples::jacobi::actions::SetupScenario::touchCellFirstTime(
      const peano4::datamanagement::CellMarker& marker,
      peano4::datamanagement::VertexEnumerator<examples::jacobi::vertexdata::MG> fineGridVerticesMG,
      peano4::datamanagement::VertexEnumerator<examples::jacobi::vertexdata::MG> coarseGridVerticesMG) {}


void examples::jacobi::actions::SetupScenario::touchCellLastTime(
      const peano4::datamanagement::CellMarker& marker,
      peano4::datamanagement::VertexEnumerator<examples::jacobi::vertexdata::MG> fineGridVerticesMG,
      peano4::datamanagement::VertexEnumerator<examples::jacobi::vertexdata::MG> coarseGridVerticesMG) {}



examples::jacobi::vertexdata::MG::VertexType examples::jacobi::actions::SetupScenario::getVertexType(const tarch::la::Vector<Dimensions,double>& x) {
  examples::jacobi::vertexdata::MG::VertexType result;
  switch (_scenario) {
    case Scenario::Mitchell:
    case Scenario::EpsilonFrequencies:
      {
        const bool isBoundaryVertex =
          tarch::la::oneEquals(x,0.0)
          or
          tarch::la::oneEquals(x,1.0);

        result = isBoundaryVertex ? examples::jacobi::vertexdata::MG::VertexType::Boundary
                                  : examples::jacobi::vertexdata::MG::VertexType::Inside;
      }
      break;
    case Scenario::Ruede:
    {
      const bool isBoundaryVertex =
        tarch::la::oneEquals(x,-1.0)
        or
        tarch::la::oneEquals(x,1.0);

      result = isBoundaryVertex ? examples::jacobi::vertexdata::MG::VertexType::Boundary
                                : examples::jacobi::vertexdata::MG::VertexType::Inside;
      }
      break;
  }
  return result;
}


double examples::jacobi::actions::SetupScenario::getSolution(const tarch::la::Vector<Dimensions,double>& x) {
  double result;
  switch (_scenario) {
    case Scenario::Mitchell:
      {
        tarch::la::Vector<Dimensions,double> c(0.0);
        const double alpha = 20.0;
        c(0)               = -0.05;
        c(1)               = -0.05;
        const double r0    =  0.7;

        double r = tarch::la::norm2(x-c);

        result = std::atan(alpha*(r-r0));
      }
      break;
    case Scenario::Ruede:
      {
        result = std::cos( 2.0*tarch::la::PI * (x(0)-x(1)) )
               * std::sinh( 2.0*tarch::la::PI * (x(0)+x(1)+2.0) )
               / std::sinh( 8.0*tarch::la::PI );
      }
      break;
    case Scenario::EpsilonFrequencies:
      {
        result = 0.0;
      }
      break;
  }
  return result;
}


std::pair<double,double>  examples::jacobi::actions::SetupScenario::getMinMax() {
  switch (_scenario) {
    case Scenario::Mitchell:
   	  return std::pair<double,double>(-tarch::la::PI/2.0,tarch::la::PI/2.0);
      break;
    case Scenario::Ruede:
   	  return std::pair<double,double>(-1.0,1.0);
      break;
    case Scenario::EpsilonFrequencies:
   	  return std::pair<double,double>(0.0,4.0/3.0);
      break;
  }
  return std::pair<double,double>(0.0,0.0);
}



double examples::jacobi::actions::SetupScenario::getEpsilon(const tarch::la::Vector<Dimensions,double>& x) {
  double result;
  switch (_scenario) {
    case Scenario::Mitchell:
    case Scenario::Ruede:
      result = 1.0;
      break;
    case Scenario::EpsilonFrequencies:
      {
        result = 1.0
	           + 0.3/Dimensions * std::exp(-Theta * x(0)) * std::cos( tarch::la::PI * x(0) * Theta )
               + 0.3/Dimensions * std::exp(-Theta * x(1)) * std::cos( tarch::la::PI * x(1) * Theta );
      }
      break;
  }
  return result;
}


void   examples::jacobi::actions::SetupScenario::init(const tarch::la::Vector<Dimensions,double>& x, examples::jacobi::vertexdata::MG& fineGridVertexMG) {
  if ( getVertexType(x)==examples::jacobi::vertexdata::MG::VertexType::Boundary ) {
    fineGridVertexMG.setVertexType( examples::jacobi::vertexdata::MG::VertexType::Boundary );
    fineGridVertexMG.setU( getSolution(x) );
    fineGridVertexMG.setRhs( 0.0 );
    // @todo Document: Important to allow fusion
    fineGridVertexMG.setDiag( 1.0 );
    fineGridVertexMG.setEps( getEpsilon(x) );
  }
  else {
    std::pair<double,double> minMax = getMinMax();
    const double scaledRandomValue = static_cast<double>( std::rand() ) /  static_cast<double>(RAND_MAX);

    fineGridVertexMG.setVertexType( examples::jacobi::vertexdata::MG::VertexType::Inside );
    fineGridVertexMG.setU( minMax.first + scaledRandomValue*(minMax.second-minMax.first) );
    fineGridVertexMG.setRhs( 0.0 );
    fineGridVertexMG.setDiag( 1.0 );
    fineGridVertexMG.setEps( getEpsilon(x) );
  }
}


tarch::la::Vector<Dimensions,double>  examples::jacobi::actions::SetupScenario::getDomainOffset() {
  switch (_scenario) {
    case Scenario::Mitchell:
      return {0.0, 0.0};
    case Scenario::Ruede:
      return {-1.0, -1.0};
    case Scenario::EpsilonFrequencies:
      return {0.0, 0.0};
  }
  return tarch::la::Vector<Dimensions,double>(0.0,0.0);
}


tarch::la::Vector<Dimensions,double>  examples::jacobi::actions::SetupScenario::getDomainSize() {
  switch (_scenario) {
    case Scenario::Mitchell:
      return {1.0, 1.0};
    case Scenario::Ruede:
      return {2.0, 2.0};
    case Scenario::EpsilonFrequencies:
      return {1.0, 1.0};
  }
  return tarch::la::Vector<Dimensions,double>(-1.0,-1.0);
}

