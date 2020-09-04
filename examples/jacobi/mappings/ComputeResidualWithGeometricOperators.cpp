#include "ComputeResidualWithGeometricOperators.h"
#include "SetupScenario.h"


#include "tarch/multicore/multicore.h"
#include "tarch/multicore/Tasks.h"


#include "toolbox/finiteelements/ElementMatrix.h"
#include "toolbox/finiteelements/StencilFactory.h"
#include "toolbox/multiprecision/CompressedLinearAlgebra.h"


tarch::logging::Log examples::algebraicmg::mappings::ComputeResidualWithGeometricOperators::_log( "examples::algebraicmg::mappings::ComputeResidualWithGeometricOperators");


examples::algebraicmg::mappings::ComputeResidualWithGeometricOperators::ComputeResidualWithGeometricOperators(int treeNumber) {
  _localStiffnessMatrixOneIntegrationPoint =
    toolbox::finiteelements::getElementWiseAssemblyMatrix(
      toolbox::finiteelements::getLaplacian(1.0)
    );
  logDebug( "ComputeResidualWithGeometricOperators(...)", "local element matrix " << _localStiffnessMatrixOneIntegrationPoint );
  logDebug( "ComputeResidualWithGeometricOperators(...)", "constructed from stencil " << toolbox::finiteelements::getLaplacian(1.0));
}


examples::algebraicmg::mappings::ComputeResidualWithGeometricOperators::~ComputeResidualWithGeometricOperators() {
}


std::vector< peano4::grid::GridControlEvent > examples::algebraicmg::mappings::ComputeResidualWithGeometricOperators::getGridControlEvents() {
  return std::vector< peano4::grid::GridControlEvent >();
}


void examples::algebraicmg::mappings::ComputeResidualWithGeometricOperators::beginTraversal() {
  _maximumN = 0;
  _minimumN = std::numeric_limits<int>::max();
  _accumulatedN = 0;
  _totalNumberOfLocalStiffnessMatrices = 0;
  _totalBytesOfStoredMatrices = 0;
  _minBytesPerMatrix = ThreePowerD*sizeof(double);
  _maxBytesPerMatrix = 0;
}


void examples::algebraicmg::mappings::ComputeResidualWithGeometricOperators::endTraversal() {
  std::cout << "\t max(N)=" << _maximumN
  	        << "\t min(N)=" << _minimumN
		    << "\t average(N)=" << (static_cast<double>(_accumulatedN)/static_cast<double>(_totalNumberOfLocalStiffnessMatrices))
			<< "\t average bytes per matrix=" << (static_cast<double>(_totalBytesOfStoredMatrices)/static_cast<double>(_totalNumberOfLocalStiffnessMatrices))
			<< "\t min bytes per matrix=" << _minBytesPerMatrix
			<< "\t max bytes per matrix=" << _maxBytesPerMatrix
			<< "\t compression=" << static_cast<double>(_totalNumberOfLocalStiffnessMatrices*TwoPowerD*TwoPowerD*sizeof(double)) / _totalBytesOfStoredMatrices
			<< std::endl;
  tarch::multicore::processPendingTasks();
}


void examples::algebraicmg::mappings::ComputeResidualWithGeometricOperators::createPersistentVertex(
	      const tarch::la::Vector<Dimensions,double>& center,
	      const tarch::la::Vector<Dimensions,double>& h,
	      examples::algebraicmg::vertexdata::MG& fineGridVertexMG,
	      peano4::datamanagement::VertexEnumerator<examples::algebraicmg::vertexdata::MG> coarseGridVerticesMG,
	      peano4::datamanagement::CellWrapper<examples::algebraicmg::celldata::A> coarseGridCellA,
	      peano4::datamanagement::CellWrapper<examples::algebraicmg::celldata::p> coarseGridCellp
) {
}


void examples::algebraicmg::mappings::ComputeResidualWithGeometricOperators::destroyPersistentVertex(
	      const tarch::la::Vector<Dimensions,double>& center,
	      const tarch::la::Vector<Dimensions,double>& h,
	      examples::algebraicmg::vertexdata::MG& fineGridVertexMG,
	      peano4::datamanagement::VertexEnumerator<examples::algebraicmg::vertexdata::MG> coarseGridVerticesMG,
	      peano4::datamanagement::CellWrapper<examples::algebraicmg::celldata::A> coarseGridCellA,
	      peano4::datamanagement::CellWrapper<examples::algebraicmg::celldata::p> coarseGridCellp
) {
}


void examples::algebraicmg::mappings::ComputeResidualWithGeometricOperators::createHangingVertex(
	      const tarch::la::Vector<Dimensions,double>& center,
	      const tarch::la::Vector<Dimensions,double>& h,
	      examples::algebraicmg::vertexdata::MG& fineGridVertexMG,
	      peano4::datamanagement::VertexEnumerator<examples::algebraicmg::vertexdata::MG> coarseGridVerticesMG,
	      peano4::datamanagement::CellWrapper<examples::algebraicmg::celldata::A> coarseGridCellA,
	      peano4::datamanagement::CellWrapper<examples::algebraicmg::celldata::p> coarseGridCellp
) {
}


void examples::algebraicmg::mappings::ComputeResidualWithGeometricOperators::destroyHangingVertex(
	      const tarch::la::Vector<Dimensions,double>& center,
	      const tarch::la::Vector<Dimensions,double>& h,
	      examples::algebraicmg::vertexdata::MG& fineGridVertexMG,
	      peano4::datamanagement::VertexEnumerator<examples::algebraicmg::vertexdata::MG> coarseGridVerticesMG,
	      peano4::datamanagement::CellWrapper<examples::algebraicmg::celldata::A> coarseGridCellA,
	      peano4::datamanagement::CellWrapper<examples::algebraicmg::celldata::p> coarseGridCellp
) {
}


void examples::algebraicmg::mappings::ComputeResidualWithGeometricOperators::touchVertexFirstTime(
	      const tarch::la::Vector<Dimensions,double>& center,
	      const tarch::la::Vector<Dimensions,double>& h,
	      examples::algebraicmg::vertexdata::MG& fineGridVertexMG,
	      peano4::datamanagement::VertexEnumerator<examples::algebraicmg::vertexdata::MG> coarseGridVerticesMG,
	      peano4::datamanagement::CellWrapper<examples::algebraicmg::celldata::A> coarseGridCellA,
	      peano4::datamanagement::CellWrapper<examples::algebraicmg::celldata::p> coarseGridCellp
) {
  fineGridVertexMG.setRes(0.0);
  fineGridVertexMG.setDiag(0.0);
}


void examples::algebraicmg::mappings::ComputeResidualWithGeometricOperators::touchVertexLastTime(
	      const tarch::la::Vector<Dimensions,double>& center,
	      const tarch::la::Vector<Dimensions,double>& h,
	      examples::algebraicmg::vertexdata::MG& fineGridVertexMG,
	      peano4::datamanagement::VertexEnumerator<examples::algebraicmg::vertexdata::MG> coarseGridVerticesMG,
	      peano4::datamanagement::CellWrapper<examples::algebraicmg::celldata::A> coarseGridCellA,
	      peano4::datamanagement::CellWrapper<examples::algebraicmg::celldata::p> coarseGridCellp
) {
}


const double AcceptableError = 1e-8;


void examples::algebraicmg::mappings::ComputeResidualWithGeometricOperators::embedMatrixIntoCellStream(
  const tarch::la::Vector<Dimensions,double>&   centre,
  const tarch::la::Vector<Dimensions,double>&   h,
  const tarch::la::Matrix<TwoPowerD,TwoPowerD,double>& localStiffnessMatrix,
  peano4::datamanagement::CellWrapper<examples::algebraicmg::celldata::A>& fineGridCellA
) {
   tarch::la::Matrix<TwoPowerD,TwoPowerD,double>  hierarchicalTransformOfLocalStiffnessMatrix = toolbox::finiteelements::hierarchicalTransform(
    localStiffnessMatrix,
	h,
	SetupScenario::getEpsilon( centre )
  );

  std::vector<unsigned char> compressedLocalStiffnessMatrix = toolbox::multiprecision::compress(
    hierarchicalTransformOfLocalStiffnessMatrix,
    AcceptableError
  );

  //  assertionEquals5( compressedLocalStiffnessMatrix.size(), 1, localStiffnessMatrix, hierarchicalTransformOfLocalStiffnessMatrix, SetupScenario::getEpsilon( centre ), centre, h );

  fineGridCellA.data().entries.clear();
  fineGridCellA.data().entries.insert( fineGridCellA.data().entries.begin(), compressedLocalStiffnessMatrix.begin(), compressedLocalStiffnessMatrix.end() );
}


void examples::algebraicmg::mappings::ComputeResidualWithGeometricOperators::touchCellFirstTime(
  peano4::datamanagement::VertexEnumerator<examples::algebraicmg::vertexdata::MG> fineGridVerticesMG,
  peano4::datamanagement::CellWrapper<examples::algebraicmg::celldata::A> fineGridCellA,
  peano4::datamanagement::CellWrapper<examples::algebraicmg::celldata::p> fineGridCellp,
  peano4::datamanagement::VertexEnumerator<examples::algebraicmg::vertexdata::MG> coarseGridVerticesMG,
  peano4::datamanagement::CellWrapper<examples::algebraicmg::celldata::A> coarseGridCellA,
  peano4::datamanagement::CellWrapper<examples::algebraicmg::celldata::p> coarseGridCellp
) {
  if (not fineGridCellA.isRefined()) {
    tarch::la::Vector<TwoPowerD, double> u;
    tarch::la::Vector<TwoPowerD, double> r;

    // gather solution values
    for (int i=0; i<TwoPowerD; i++) {
      u(i) = fineGridVerticesMG(i).getU();
    }

    if ( fineGridCellA.data().entries.empty() ) {
      //
      // Create an initial guess of the matrix through low order discretisation
      //
      tarch::la::Matrix<TwoPowerD,TwoPowerD,double>  localStiffnessMatrix =
        toolbox::finiteelements::getPoissonMatrixWithJumpingCoefficient(
          fineGridCellA.centre(), fineGridCellA.h(), 1,
  	      [](const tarch::la::Vector<Dimensions,double>& x) -> double {
            return SetupScenario::getEpsilon(x);
          }
	    );

      embedMatrixIntoCellStream( fineGridCellA.centre(), fineGridCellA.h(), localStiffnessMatrix, fineGridCellA );

      assertionEquals2( fineGridCellA.data().entries.size(), 1, localStiffnessMatrix, fineGridCellA.centre() );

      fineGridCellp.data().setN(1);
    }

    //
    // Get the compressed data from the stream and convert it into local assembly matrix
    //
    tarch::la::Matrix<TwoPowerD,TwoPowerD,double>  hierarchicalTransformOfLocalStiffnessMatrix = toolbox::multiprecision::uncompressMatrix<TwoPowerD,TwoPowerD>(
       fineGridCellA.data().entries
    );

    tarch::la::Matrix<TwoPowerD,TwoPowerD,double>  localStiffnessMatrix = toolbox::finiteelements::inverseHierarchicalTransform(
      hierarchicalTransformOfLocalStiffnessMatrix,
      fineGridCellA.h(),
      SetupScenario::getEpsilon( fineGridCellA.centre() )
    );

    //
    // Update stats
    //
    _maximumN      = std::max(_maximumN, std::abs(fineGridCellp.data().getN()) );
    _minimumN      = std::min(_minimumN, std::abs(fineGridCellp.data().getN()) );
    _accumulatedN += std::abs(fineGridCellp.data().getN());
    _totalNumberOfLocalStiffnessMatrices++;
    _totalBytesOfStoredMatrices += fineGridCellA.data().entries.size();
    _minBytesPerMatrix = std::min(_minBytesPerMatrix,(int)(fineGridCellA.data().entries.size()));
    _maxBytesPerMatrix = std::max(_maxBytesPerMatrix,(int)(fineGridCellA.data().entries.size()));

    // compute residual contribution. Mind the minus sign here that
    // results from the residual's definition: r = b-Au
    // const double scaling = tarch::la::pow(fineGridCellA.h()(0), (double)(Dimensions-2));
    // r = - scaling * _localStiffnessMatrixOneIntegrationPoint * u;
    r = - 1.0 * localStiffnessMatrix * u;

    // scatter residual contributions
    for (int i=0; i<TwoPowerD; i++) {
      fineGridVerticesMG(i).setRes(  fineGridVerticesMG(i).getRes() + r(i) );
      fineGridVerticesMG(i).setDiag( fineGridVerticesMG(i).getDiag() + localStiffnessMatrix(i,i) );
    }

    if (fineGridCellp.data().getN()>0) {
      //
      // Create an initial guess of the matrix through low order discretisation
      //
      tarch::la::Matrix<TwoPowerD,TwoPowerD,double>  improvedLocalStiffnessMatrix =
        toolbox::finiteelements::getPoissonMatrixWithJumpingCoefficient(
            fineGridCellA.centre(), fineGridCellA.h(), fineGridCellp.data().getN()+1,
    	      [](const tarch::la::Vector<Dimensions,double>& x) -> double {
              return SetupScenario::getEpsilon(x);
            }
  	    );

        double oldFrobeniusNorm = tarch::la::frobeniusNorm(localStiffnessMatrix);
        double newFrobeniusNorm = tarch::la::frobeniusNorm(improvedLocalStiffnessMatrix);
        double improvement = std::abs(oldFrobeniusNorm-newFrobeniusNorm)/oldFrobeniusNorm;

        ///std::cout << " " << improvement << "(" << oldFrobeniusNorm << ")";
        //double improvement = tarch::la::frobeniusNorm(improvedLocalStiffnessMatrix-localStiffnessMatrix)/oldFrobeniusNorm;

        //const double MaxDifference = AcceptableError;
        const double MaxDifference = 0.01;
        if (improvement>MaxDifference) {
          embedMatrixIntoCellStream( fineGridCellA.h(), fineGridCellA.centre(), improvedLocalStiffnessMatrix, fineGridCellA );
          fineGridCellp.data().setN( fineGridCellp.data().getN()+1 );
        }
        else {
          fineGridCellp.data().setN( -fineGridCellp.data().getN() );
        }
    }
  }
}


void examples::algebraicmg::mappings::ComputeResidualWithGeometricOperators::touchCellLastTime(
  peano4::datamanagement::VertexEnumerator<examples::algebraicmg::vertexdata::MG> fineGridVerticesMG,
  peano4::datamanagement::CellWrapper<examples::algebraicmg::celldata::A> fineGridCellA,
  peano4::datamanagement::CellWrapper<examples::algebraicmg::celldata::p> fineGridCellp,
  peano4::datamanagement::VertexEnumerator<examples::algebraicmg::vertexdata::MG> coarseGridVerticesMG,
  peano4::datamanagement::CellWrapper<examples::algebraicmg::celldata::A> coarseGridCellA,
  peano4::datamanagement::CellWrapper<examples::algebraicmg::celldata::p> coarseGridCellp
) {
}



void examples::algebraicmg::mappings::ComputeResidualWithGeometricOperators::createCell(
     const tarch::la::Vector<Dimensions,double>& center,
     const tarch::la::Vector<Dimensions,double>& h,
     peano4::datamanagement::VertexEnumerator<examples::algebraicmg::vertexdata::MG> fineGridVerticesMG,
     examples::algebraicmg::celldata::A& fineGridCellA,
     examples::algebraicmg::celldata::p& fineGridCellp,
     peano4::datamanagement::VertexEnumerator<examples::algebraicmg::vertexdata::MG> coarseGridVerticesMG,
     peano4::datamanagement::CellWrapper<examples::algebraicmg::celldata::A> coarseGridCellA,
     peano4::datamanagement::CellWrapper<examples::algebraicmg::celldata::p> coarseGridCellp
) {}


void examples::algebraicmg::mappings::ComputeResidualWithGeometricOperators::destroyCell(
    const tarch::la::Vector<Dimensions,double>& center,
    const tarch::la::Vector<Dimensions,double>& h,
    peano4::datamanagement::VertexEnumerator<examples::algebraicmg::vertexdata::MG> fineGridVerticesMG,
    examples::algebraicmg::celldata::A& fineGridCellA,
    examples::algebraicmg::celldata::p& fineGridCellp,
    peano4::datamanagement::VertexEnumerator<examples::algebraicmg::vertexdata::MG> coarseGridVerticesMG,
    peano4::datamanagement::CellWrapper<examples::algebraicmg::celldata::A> coarseGridCellA,
    peano4::datamanagement::CellWrapper<examples::algebraicmg::celldata::p> coarseGridCellp
) {}

