#ifndef _EXAMPLES_ALGEBRAICMG_MAPPINGS_COMPUTERESIDUALWITHGEOMETRICOPERATORS_H_
#define _EXAMPLES_ALGEBRAICMG_MAPPINGS_COMPUTERESIDUALWITHGEOMETRICOPERATORS_H_


#include "peano4/utils/Globals.h" 
#include "peano4/datamanagement/VertexEnumerator.h" 
#include "peano4/datamanagement/FaceEnumerator.h" 
#include "peano4/grid/GridControlEvent.h" 
#include "tarch/la/Vector.h" 


#include <vector>

#include "../../../../src/peano4/datamanagement/CellMarker.h"

#include "vertexdata/MG.h"
#include "celldata/A.h"
#include "celldata/p.h"


#include "tarch/la/Matrix.h"


namespace examples {
  namespace algebraicmg {
    namespace mappings {
      class ComputeResidualWithGeometricOperators;
    }
  }
}


class examples::algebraicmg::mappings::ComputeResidualWithGeometricOperators{
  private:
    static tarch::logging::Log  _log;

    /**
     * Holds the stiffness matrix for the Laplacian for h=1.0.
     */
    tarch::la::Matrix<TwoPowerD,TwoPowerD,double>  _localStiffnessMatrixOneIntegrationPoint;

    int _maximumN;
    int _minimumN;
    int _accumulatedN;
    int _totalNumberOfLocalStiffnessMatrices;
    int _totalBytesOfStoredMatrices;
    int _minBytesPerMatrix;
    int _maxBytesPerMatrix;

    /**
     * @param center I need the center for the hierarchical transform  to compute the reference matrix
     * @param h      I need the mesh size for the hierarchical transform to compute the reference matrix
     */
    void embedMatrixIntoCellStream(
      const tarch::la::Vector<Dimensions,double>&   center,
      const tarch::la::Vector<Dimensions,double>&   h,
	  const tarch::la::Matrix<TwoPowerD,TwoPowerD,double>& localStiffnessMatrix,
	  peano4::datamanagement::CellWrapper<examples::algebraicmg::celldata::A>& fineGridCellA
    );
  public:

    /**
     * Create mapping instance for one tree for one grid sweep
     *
     * <h2> Thread safety </h2>
     *
     * The creation of individual trees usually happens through peano4::parallel::SpacetreeSet::createObserverCloneIfRequired().
     * This routine is called lazily when we start to traverse a subtree. 
     * Therefore, the creation of mappings is not thread-safe.
     *
     *
     * @param treeNumber Number of the spacetree for which we create the tree instance. Is 
     *                   smaller 0 if this is the prototype mapping used on a rank from which 
     *                   the real mappings are constructed from.
     */      
    ComputeResidualWithGeometricOperators(int treeNumber);

    ~ComputeResidualWithGeometricOperators();

    std::vector< peano4::grid::GridControlEvent > getGridControlEvents();

    void beginTraversal(
      );

    void endTraversal(
      );

    void createPersistentVertex(
      const tarch::la::Vector<Dimensions,double>& center,
      const tarch::la::Vector<Dimensions,double>& h,
      examples::algebraicmg::vertexdata::MG& fineGridVertexMG,
      peano4::datamanagement::VertexEnumerator<examples::algebraicmg::vertexdata::MG> coarseGridVerticesMG,
      peano4::datamanagement::CellWrapper<examples::algebraicmg::celldata::A> coarseGridCellA,
      peano4::datamanagement::CellWrapper<examples::algebraicmg::celldata::p> coarseGridCellp);

    void destroyPersistentVertex(
      const tarch::la::Vector<Dimensions,double>& center,
      const tarch::la::Vector<Dimensions,double>& h,
      examples::algebraicmg::vertexdata::MG& fineGridVertexMG,
      peano4::datamanagement::VertexEnumerator<examples::algebraicmg::vertexdata::MG> coarseGridVerticesMG,
      peano4::datamanagement::CellWrapper<examples::algebraicmg::celldata::A> coarseGridCellA,
      peano4::datamanagement::CellWrapper<examples::algebraicmg::celldata::p> coarseGridCellp);

    void createHangingVertex(
      const tarch::la::Vector<Dimensions,double>& center,
      const tarch::la::Vector<Dimensions,double>& h,
      examples::algebraicmg::vertexdata::MG& fineGridVertexMG,
      peano4::datamanagement::VertexEnumerator<examples::algebraicmg::vertexdata::MG> coarseGridVerticesMG,
      peano4::datamanagement::CellWrapper<examples::algebraicmg::celldata::A> coarseGridCellA,
      peano4::datamanagement::CellWrapper<examples::algebraicmg::celldata::p> coarseGridCellp);

    void destroyHangingVertex(
      const tarch::la::Vector<Dimensions,double>& center,
      const tarch::la::Vector<Dimensions,double>& h,
      examples::algebraicmg::vertexdata::MG& fineGridVertexMG,
      peano4::datamanagement::VertexEnumerator<examples::algebraicmg::vertexdata::MG> coarseGridVerticesMG,
      peano4::datamanagement::CellWrapper<examples::algebraicmg::celldata::A> coarseGridCellA,
      peano4::datamanagement::CellWrapper<examples::algebraicmg::celldata::p> coarseGridCellp);

    void touchVertexFirstTime(
      const tarch::la::Vector<Dimensions,double>& center,
      const tarch::la::Vector<Dimensions,double>& h,
      examples::algebraicmg::vertexdata::MG& fineGridVertexMG,
      peano4::datamanagement::VertexEnumerator<examples::algebraicmg::vertexdata::MG> coarseGridVerticesMG,
      peano4::datamanagement::CellWrapper<examples::algebraicmg::celldata::A> coarseGridCellA,
      peano4::datamanagement::CellWrapper<examples::algebraicmg::celldata::p> coarseGridCellp);

    void touchVertexLastTime(
      const tarch::la::Vector<Dimensions,double>& center,
      const tarch::la::Vector<Dimensions,double>& h,
      examples::algebraicmg::vertexdata::MG& fineGridVertexMG,
      peano4::datamanagement::VertexEnumerator<examples::algebraicmg::vertexdata::MG> coarseGridVerticesMG,
      peano4::datamanagement::CellWrapper<examples::algebraicmg::celldata::A> coarseGridCellA,
      peano4::datamanagement::CellWrapper<examples::algebraicmg::celldata::p> coarseGridCellp);

    void createCell(
      const tarch::la::Vector<Dimensions,double>& center,
      const tarch::la::Vector<Dimensions,double>& h,
      peano4::datamanagement::VertexEnumerator<examples::algebraicmg::vertexdata::MG> fineGridVerticesMG,
      examples::algebraicmg::celldata::A& fineGridCellA,
      examples::algebraicmg::celldata::p& fineGridCellp,
      peano4::datamanagement::VertexEnumerator<examples::algebraicmg::vertexdata::MG> coarseGridVerticesMG,
      peano4::datamanagement::CellWrapper<examples::algebraicmg::celldata::A> coarseGridCellA,
      peano4::datamanagement::CellWrapper<examples::algebraicmg::celldata::p> coarseGridCellp);

    void destroyCell(
      const tarch::la::Vector<Dimensions,double>& center,
      const tarch::la::Vector<Dimensions,double>& h,
      peano4::datamanagement::VertexEnumerator<examples::algebraicmg::vertexdata::MG> fineGridVerticesMG,
      examples::algebraicmg::celldata::A& fineGridCellA,
      examples::algebraicmg::celldata::p& fineGridCellp,
      peano4::datamanagement::VertexEnumerator<examples::algebraicmg::vertexdata::MG> coarseGridVerticesMG,
      peano4::datamanagement::CellWrapper<examples::algebraicmg::celldata::A> coarseGridCellA,
      peano4::datamanagement::CellWrapper<examples::algebraicmg::celldata::p> coarseGridCellp);

    void touchCellFirstTime(
      peano4::datamanagement::VertexEnumerator<examples::algebraicmg::vertexdata::MG> fineGridVerticesMG,
      peano4::datamanagement::CellWrapper<examples::algebraicmg::celldata::A> fineGridCellA,
      peano4::datamanagement::CellWrapper<examples::algebraicmg::celldata::p> fineGridCellp,
      peano4::datamanagement::VertexEnumerator<examples::algebraicmg::vertexdata::MG> coarseGridVerticesMG,
      peano4::datamanagement::CellWrapper<examples::algebraicmg::celldata::A> coarseGridCellA,
      peano4::datamanagement::CellWrapper<examples::algebraicmg::celldata::p> coarseGridCellp);

    void touchCellLastTime(
      peano4::datamanagement::VertexEnumerator<examples::algebraicmg::vertexdata::MG> fineGridVerticesMG,
      peano4::datamanagement::CellWrapper<examples::algebraicmg::celldata::A> fineGridCellA,
      peano4::datamanagement::CellWrapper<examples::algebraicmg::celldata::p> fineGridCellp,
      peano4::datamanagement::VertexEnumerator<examples::algebraicmg::vertexdata::MG> coarseGridVerticesMG,
      peano4::datamanagement::CellWrapper<examples::algebraicmg::celldata::A> coarseGridCellA,
      peano4::datamanagement::CellWrapper<examples::algebraicmg::celldata::p> coarseGridCellp);

};


#endif

