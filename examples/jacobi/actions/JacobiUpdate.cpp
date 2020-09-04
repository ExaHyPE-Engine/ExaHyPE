#include "JacobiUpdate.h"
#include "SetupScenario.h"


tarch::logging::Log examples::jacobi::actions::JacobiUpdate::_log( "examples::jacobi::actions::JacobiUpdate");


examples::jacobi::actions::JacobiUpdate::JacobiUpdate(int treeNumber) {
// @todo Please implement
}


examples::jacobi::actions::JacobiUpdate::~JacobiUpdate() {
// @todo Please implement
}


std::vector< peano4::grid::GridControlEvent > examples::jacobi::actions::JacobiUpdate::getGridControlEvents() {
// @todo Please implement
return std::vector< peano4::grid::GridControlEvent >();
}


void examples::jacobi::actions::JacobiUpdate::beginTraversal() {}

void examples::jacobi::actions::JacobiUpdate::endTraversal() {}

void examples::jacobi::actions::JacobiUpdate::createPersistentVertex(
      const peano4::datamanagement::VertexMarker& marker,
      examples::jacobi::vertexdata::MG& fineGridVertexMG,
      peano4::datamanagement::VertexEnumerator<examples::jacobi::vertexdata::MG> coarseGridVerticesMG) {}

void examples::jacobi::actions::JacobiUpdate::destroyPersistentVertex(
      const peano4::datamanagement::VertexMarker& marker,
      examples::jacobi::vertexdata::MG& fineGridVertexMG,
      peano4::datamanagement::VertexEnumerator<examples::jacobi::vertexdata::MG> coarseGridVerticesMG) {}

void examples::jacobi::actions::JacobiUpdate::createHangingVertex(
      const peano4::datamanagement::VertexMarker& marker,
      examples::jacobi::vertexdata::MG& fineGridVertexMG,
      peano4::datamanagement::VertexEnumerator<examples::jacobi::vertexdata::MG> coarseGridVerticesMG) {}

void examples::jacobi::actions::JacobiUpdate::destroyHangingVertex(
      const peano4::datamanagement::VertexMarker& marker,
      examples::jacobi::vertexdata::MG& fineGridVertexMG,
      peano4::datamanagement::VertexEnumerator<examples::jacobi::vertexdata::MG> coarseGridVerticesMG) {}

void examples::jacobi::actions::JacobiUpdate::touchVertexFirstTime(
      const peano4::datamanagement::VertexMarker& marker,
      examples::jacobi::vertexdata::MG& fineGridVertexMG,
      peano4::datamanagement::VertexEnumerator<examples::jacobi::vertexdata::MG> coarseGridVerticesMG) {}


void examples::jacobi::actions::JacobiUpdate::touchVertexLastTime(
  const peano4::datamanagement::VertexMarker& marker,
  examples::jacobi::vertexdata::MG& fineGridVertexMG,
  peano4::datamanagement::VertexEnumerator<examples::jacobi::vertexdata::MG> coarseGridVerticesMG
) {
  const double omega = 0.7;

  if (
    fineGridVertexMG.getVertexType()==examples::jacobi::vertexdata::MG::VertexType::Inside
	and
	fineGridVertexMG.getDiag()>0.0
  ) {
    logDebug( "touchVertexLastTime(...)", "update " << fineGridVertexMG.toString() )
    assertion2( fineGridVertexMG.getDiag()>0.0, fineGridVertexMG.toString(), marker.toString() );
    fineGridVertexMG.setU(
      fineGridVertexMG.getU()
      +
      omega * fineGridVertexMG.getRes() / fineGridVertexMG.getDiag()
    );
  }
  else if (
    fineGridVertexMG.getVertexType()==examples::jacobi::vertexdata::MG::VertexType::Boundary
  ) {
    fineGridVertexMG.setU( SetupScenario::getSolution(marker.x()) );
  }
}


void examples::jacobi::actions::JacobiUpdate::touchCellFirstTime(
      const peano4::datamanagement::CellMarker& marker,
      peano4::datamanagement::VertexEnumerator<examples::jacobi::vertexdata::MG> fineGridVerticesMG,
      peano4::datamanagement::VertexEnumerator<examples::jacobi::vertexdata::MG> coarseGridVerticesMG) {}


void examples::jacobi::actions::JacobiUpdate::touchCellLastTime(
      const peano4::datamanagement::CellMarker& marker,
      peano4::datamanagement::VertexEnumerator<examples::jacobi::vertexdata::MG> fineGridVerticesMG,
      peano4::datamanagement::VertexEnumerator<examples::jacobi::vertexdata::MG> coarseGridVerticesMG) {}


