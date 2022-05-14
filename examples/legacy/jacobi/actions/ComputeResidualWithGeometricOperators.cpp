#include "ComputeResidualWithGeometricOperators.h"
#include "SetupScenario.h"


#include "tarch/la/Matrix.h"


#include "toolbox/finiteelements/ElementMatrix.h"
#include "toolbox/finiteelements/StencilFactory.h"


tarch::logging::Log examples::jacobi::actions::ComputeResidualWithGeometricOperators::_log( "examples::jacobi::actions::ComputeResidualWithGeometricOperators");


examples::jacobi::actions::ComputeResidualWithGeometricOperators::ComputeResidualWithGeometricOperators(int treeNumber) {}

examples::jacobi::actions::ComputeResidualWithGeometricOperators::~ComputeResidualWithGeometricOperators() {}

std::vector< peano4::grid::GridControlEvent > examples::jacobi::actions::ComputeResidualWithGeometricOperators::getGridControlEvents() {
  return std::vector< peano4::grid::GridControlEvent >();
}


void examples::jacobi::actions::ComputeResidualWithGeometricOperators::beginTraversal() {}

void examples::jacobi::actions::ComputeResidualWithGeometricOperators::endTraversal() {}

void examples::jacobi::actions::ComputeResidualWithGeometricOperators::createPersistentVertex(
      const peano4::datamanagement::VertexMarker& marker,
      examples::jacobi::vertexdata::MG& fineGridVertexMG,
      peano4::datamanagement::VertexEnumerator<examples::jacobi::vertexdata::MG> coarseGridVerticesMG) {}

void examples::jacobi::actions::ComputeResidualWithGeometricOperators::destroyPersistentVertex(
      const peano4::datamanagement::VertexMarker& marker,
      examples::jacobi::vertexdata::MG& fineGridVertexMG,
      peano4::datamanagement::VertexEnumerator<examples::jacobi::vertexdata::MG> coarseGridVerticesMG) {}

void examples::jacobi::actions::ComputeResidualWithGeometricOperators::createHangingVertex(
      const peano4::datamanagement::VertexMarker& marker,
      examples::jacobi::vertexdata::MG& fineGridVertexMG,
      peano4::datamanagement::VertexEnumerator<examples::jacobi::vertexdata::MG> coarseGridVerticesMG) {}

void examples::jacobi::actions::ComputeResidualWithGeometricOperators::destroyHangingVertex(
      const peano4::datamanagement::VertexMarker& marker,
      examples::jacobi::vertexdata::MG& fineGridVertexMG,
      peano4::datamanagement::VertexEnumerator<examples::jacobi::vertexdata::MG> coarseGridVerticesMG) {}


void examples::jacobi::actions::ComputeResidualWithGeometricOperators::touchVertexFirstTime(
  const peano4::datamanagement::VertexMarker& marker,
  examples::jacobi::vertexdata::MG& fineGridVertexMG,
  peano4::datamanagement::VertexEnumerator<examples::jacobi::vertexdata::MG> coarseGridVerticesMG
) {
  fineGridVertexMG.setRes(0.0);
  fineGridVertexMG.setDiag(0.0);
}


void examples::jacobi::actions::ComputeResidualWithGeometricOperators::touchVertexLastTime(
      const peano4::datamanagement::VertexMarker& marker,
      examples::jacobi::vertexdata::MG& fineGridVertexMG,
      peano4::datamanagement::VertexEnumerator<examples::jacobi::vertexdata::MG> coarseGridVerticesMG) {}


void examples::jacobi::actions::ComputeResidualWithGeometricOperators::touchCellFirstTime(
  const peano4::datamanagement::CellMarker& marker,
  peano4::datamanagement::VertexEnumerator<examples::jacobi::vertexdata::MG> fineGridVerticesMG,
  peano4::datamanagement::VertexEnumerator<examples::jacobi::vertexdata::MG> coarseGridVerticesMG
) {
  logTraceInWith1Argument( "touchCellFirstTime()", marker.toString() );
  if (not marker.isRefined()) {
    tarch::la::Vector<TwoPowerD, double> u;
    tarch::la::Vector<TwoPowerD, double> r;

    // gather solution values
    for (int i=0; i<TwoPowerD; i++) {
      u(i) = fineGridVerticesMG(i).getU();
    }

    const double epsilon = SetupScenario::getEpsilon(marker.x());

    // Holds the stiffness matrix for the Laplacian. Not the most sophisticated
    // version. Obviously, we could compute it once for h=1.0 and then just
    // rescale it. But in this code, I prefer to have everything in one place.
    tarch::la::Matrix<TwoPowerD,TwoPowerD,double> localStiffnessMatrix =
      toolbox::finiteelements::getElementWiseAssemblyMatrix(
        toolbox::finiteelements::getLaplacian(marker.h())
      );

    // compute residual contribution. Mind the minus sign here that
    // results from the residual's definition: r = b-Au
    r = - epsilon * localStiffnessMatrix * u;

    // scatter residual contributions
    for (int i=0; i<TwoPowerD; i++) {
      fineGridVerticesMG(i).setRes(  fineGridVerticesMG(i).getRes() + r(i) );
      fineGridVerticesMG(i).setDiag( fineGridVerticesMG(i).getDiag() + localStiffnessMatrix(i,i) );
    }
  }
  logTraceOut( "touchCellFirstTime()" );
}


void examples::jacobi::actions::ComputeResidualWithGeometricOperators::touchCellLastTime(
  const peano4::datamanagement::CellMarker& marker,
  peano4::datamanagement::VertexEnumerator<examples::jacobi::vertexdata::MG> fineGridVerticesMG,
  peano4::datamanagement::VertexEnumerator<examples::jacobi::vertexdata::MG> coarseGridVerticesMG) {}
