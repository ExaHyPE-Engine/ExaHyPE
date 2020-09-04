#ifndef _EXAMPLES_JACOBI_ACTIONS_JACOBIUPDATE_H_
#define _EXAMPLES_JACOBI_ACTIONS_JACOBIUPDATE_H_


#include "peano4/utils/Globals.h" 
#include "peano4/datamanagement/VertexEnumerator.h"
#include "peano4/datamanagement/VertexMarker.h"
#include "peano4/datamanagement/FaceEnumerator.h"
#include "peano4/datamanagement/FaceMarker.h"
#include "peano4/datamanagement/CellMarker.h"
#include "peano4/grid/GridControlEvent.h" 
#include "tarch/la/Vector.h" 


#include <vector>

#include "../../../../src/peano4/datamanagement/CellMarker.h"

// @todo Please implement


#include "vertexdata/MG.h"
namespace examples {
namespace jacobi {
namespace actions {
  class JacobiUpdate;
}
}
}
class examples::jacobi::actions::JacobiUpdate{
  private:
    static tarch::logging::Log  _log;
// @todo Please implement
  public:

    /**
     * Create action instance for one tree for one grid sweep
     *
     * <h2> Thread safety </h2>
     *
     * The creation of individual trees usually happens through peano4::parallel::SpacetreeSet::createObserverCloneIfRequired().
     * This routine is called lazily when we start to traverse a subtree. 
     * Therefore, the creation of actions is not thread-safe.
     *
     *
     * @param treeNumber Number of the spacetree for which we create the tree instance. Is 
     *                   smaller 0 if this is the prototype action used on a rank from which 
     *                   the real actions are constructed from.
     */      
    JacobiUpdate(int treeNumber);

    ~JacobiUpdate();

    std::vector< peano4::grid::GridControlEvent > getGridControlEvents();

    void beginTraversal(
      );

    void endTraversal(
      );

    void createPersistentVertex(
      const peano4::datamanagement::VertexMarker& marker,
      examples::jacobi::vertexdata::MG& fineGridVertexMG,
      peano4::datamanagement::VertexEnumerator<examples::jacobi::vertexdata::MG> coarseGridVerticesMG);

    void destroyPersistentVertex(
      const peano4::datamanagement::VertexMarker& marker,
      examples::jacobi::vertexdata::MG& fineGridVertexMG,
      peano4::datamanagement::VertexEnumerator<examples::jacobi::vertexdata::MG> coarseGridVerticesMG);

    void createHangingVertex(
      const peano4::datamanagement::VertexMarker& marker,
      examples::jacobi::vertexdata::MG& fineGridVertexMG,
      peano4::datamanagement::VertexEnumerator<examples::jacobi::vertexdata::MG> coarseGridVerticesMG);

    void destroyHangingVertex(
      const peano4::datamanagement::VertexMarker& marker,
      examples::jacobi::vertexdata::MG& fineGridVertexMG,
      peano4::datamanagement::VertexEnumerator<examples::jacobi::vertexdata::MG> coarseGridVerticesMG);

    void touchVertexFirstTime(
      const peano4::datamanagement::VertexMarker& marker,
      examples::jacobi::vertexdata::MG& fineGridVertexMG,
      peano4::datamanagement::VertexEnumerator<examples::jacobi::vertexdata::MG> coarseGridVerticesMG);

    void touchVertexLastTime(
      const peano4::datamanagement::VertexMarker& marker,
      examples::jacobi::vertexdata::MG& fineGridVertexMG,
      peano4::datamanagement::VertexEnumerator<examples::jacobi::vertexdata::MG> coarseGridVerticesMG);

    void touchCellFirstTime(
      const peano4::datamanagement::CellMarker& marker,
      peano4::datamanagement::VertexEnumerator<examples::jacobi::vertexdata::MG> fineGridVerticesMG,
      peano4::datamanagement::VertexEnumerator<examples::jacobi::vertexdata::MG> coarseGridVerticesMG);

    void touchCellLastTime(
      const peano4::datamanagement::CellMarker& marker,
      peano4::datamanagement::VertexEnumerator<examples::jacobi::vertexdata::MG> fineGridVerticesMG,
      peano4::datamanagement::VertexEnumerator<examples::jacobi::vertexdata::MG> coarseGridVerticesMG);

};


#endif

