#include "ComputeGlobalResidualAndError.h"
#include "SetupScenario.h"


#include "tarch/tarch.h"


tarch::logging::Log examples::jacobi::actions::ComputeGlobalResidualAndError::_log( "examples::jacobi::actions::ComputeGlobalResidualAndError");


examples::jacobi::actions::ComputeGlobalResidualAndError::ComputeGlobalResidualAndError(int treeNumber):
  _treeNumber(treeNumber) {}


examples::jacobi::actions::ComputeGlobalResidualAndError::~ComputeGlobalResidualAndError() {}


std::vector< peano4::grid::GridControlEvent > examples::jacobi::actions::ComputeGlobalResidualAndError::getGridControlEvents() {
  return std::vector< peano4::grid::GridControlEvent >();
}


void examples::jacobi::actions::ComputeGlobalResidualAndError::beginTraversal() {
  _globalResidualL2        = 0.0;
  _globalResidualMax       = 0.0;
  _globalResidualEukledian = 0.0;
  _globalErrorL2           = 0.0;
  _globalErrorMax          = 0.0;
  _globalErrorEukledian    = 0.0;
}


void examples::jacobi::actions::ComputeGlobalResidualAndError::endTraversal() {
  if (_treeNumber==0) {
    logInfo(
      "endTraversal",
      " \t |res|_h="   << std::sqrt(_globalResidualL2) <<
      " \t |res|_max=" << _globalResidualMax <<
      " \t |res|_2="   << std::sqrt(_globalResidualEukledian) <<
      " \t |e|_h="     << std::sqrt(_globalErrorL2) <<
      " \t |e|_max="   << _globalErrorMax <<
      " \t |e|_2="     << std::sqrt(_globalErrorEukledian) <<
      " \t mem="       << tarch::getMemoryUsage(tarch::MemoryUsageFormat::MByte) << "MB"
    );
  }
}


void examples::jacobi::actions::ComputeGlobalResidualAndError::createPersistentVertex(
      const peano4::datamanagement::VertexMarker& marker,
      examples::jacobi::vertexdata::MG& fineGridVertexMG,
      peano4::datamanagement::VertexEnumerator<examples::jacobi::vertexdata::MG> coarseGridVerticesMG) {}

void examples::jacobi::actions::ComputeGlobalResidualAndError::destroyPersistentVertex(
      const peano4::datamanagement::VertexMarker& marker,
      examples::jacobi::vertexdata::MG& fineGridVertexMG,
      peano4::datamanagement::VertexEnumerator<examples::jacobi::vertexdata::MG> coarseGridVerticesMG) {}

void examples::jacobi::actions::ComputeGlobalResidualAndError::createHangingVertex(
      const peano4::datamanagement::VertexMarker& marker,
      examples::jacobi::vertexdata::MG& fineGridVertexMG,
      peano4::datamanagement::VertexEnumerator<examples::jacobi::vertexdata::MG> coarseGridVerticesMG) {}

void examples::jacobi::actions::ComputeGlobalResidualAndError::destroyHangingVertex(
      const peano4::datamanagement::VertexMarker& marker,
      examples::jacobi::vertexdata::MG& fineGridVertexMG,
      peano4::datamanagement::VertexEnumerator<examples::jacobi::vertexdata::MG> coarseGridVerticesMG) {}

void examples::jacobi::actions::ComputeGlobalResidualAndError::touchVertexFirstTime(
      const peano4::datamanagement::VertexMarker& marker,
      examples::jacobi::vertexdata::MG& fineGridVertexMG,
      peano4::datamanagement::VertexEnumerator<examples::jacobi::vertexdata::MG> coarseGridVerticesMG) {}

void examples::jacobi::actions::ComputeGlobalResidualAndError::touchCellFirstTime(
      const peano4::datamanagement::CellMarker& marker,
      peano4::datamanagement::VertexEnumerator<examples::jacobi::vertexdata::MG> fineGridVerticesMG,
      peano4::datamanagement::VertexEnumerator<examples::jacobi::vertexdata::MG> coarseGridVerticesMG) {}

void examples::jacobi::actions::ComputeGlobalResidualAndError::touchCellLastTime(
      const peano4::datamanagement::CellMarker& marker,
      peano4::datamanagement::VertexEnumerator<examples::jacobi::vertexdata::MG> fineGridVerticesMG,
      peano4::datamanagement::VertexEnumerator<examples::jacobi::vertexdata::MG> coarseGridVerticesMG) {}


void examples::jacobi::actions::ComputeGlobalResidualAndError::touchVertexLastTime(
      const peano4::datamanagement::VertexMarker& marker,
      examples::jacobi::vertexdata::MG& fineGridVertexMG,
      peano4::datamanagement::VertexEnumerator<examples::jacobi::vertexdata::MG> coarseGridVerticesMG
) {
  if (
    not marker.isRefined()
	and
    fineGridVertexMG.getVertexType()==examples::jacobi::vertexdata::MG::VertexType::Inside
  ) {
    double solution = SetupScenario::getSolution(marker.x());
    double error    = fineGridVertexMG.getU() - solution;

    _globalResidualMax         = std::max( _globalResidualMax, fineGridVertexMG.getRes() );
    _globalResidualL2         += fineGridVertexMG.getRes() * fineGridVertexMG.getRes() * tarch::la::volume(marker.h());
    _globalResidualEukledian  += fineGridVertexMG.getRes() * fineGridVertexMG.getRes();

    _globalErrorMax         = std::max( _globalErrorMax, std::abs(error) );
    _globalErrorL2         += error * error * tarch::la::volume(marker.h());
    _globalErrorEukledian  += error * error;
  }
}
