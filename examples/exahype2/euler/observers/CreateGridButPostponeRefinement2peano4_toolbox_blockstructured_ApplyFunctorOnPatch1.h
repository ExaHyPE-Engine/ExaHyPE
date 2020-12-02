#ifndef _EXAMPLES_EXAHYPE2_EULER_OBSERVERS_CREATEGRIDBUTPOSTPONEREFINEMENT2PEANO4_TOOLBOX_BLOCKSTRUCTURED_APPLYFUNCTORONPATCH1_H_
#define _EXAMPLES_EXAHYPE2_EULER_OBSERVERS_CREATEGRIDBUTPOSTPONEREFINEMENT2PEANO4_TOOLBOX_BLOCKSTRUCTURED_APPLYFUNCTORONPATCH1_H_



#include "peano4/utils/Globals.h"
#include "peano4/datamanagement/VertexEnumerator.h"
#include "peano4/datamanagement/VertexMarker.h"
#include "peano4/datamanagement/FaceEnumerator.h"
#include "peano4/datamanagement/FaceMarker.h"
#include "peano4/datamanagement/CellMarker.h"
#include "peano4/grid/GridControlEvent.h"
#include "tarch/la/Vector.h"

#include <vector>



#include <functional>

#include "tarch/la/Vector.h" 

#include "peano4/utils/Globals.h"
#include "peano4/utils/Loop.h"

#include "SolverRepository.h"

#include "exahype2/PatchUtils.h"
#include "exahype2/fv/BoundaryConditions.h"

#include "exahype2/fv/Rusanov.h"


#include "facedata/EulerQ.h"
#include "facedata/EulerQNew.h"
#include "facedata/Label.h"
#include "celldata/EulerQ.h"
namespace examples {
namespace exahype2 {
namespace euler {
namespace observers {
  class CreateGridButPostponeRefinement2peano4_toolbox_blockstructured_ApplyFunctorOnPatch1;
}
}
}
}
class examples::exahype2::euler::observers::CreateGridButPostponeRefinement2peano4_toolbox_blockstructured_ApplyFunctorOnPatch1{
  private:
    static tarch::logging::Log  _log;
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
    CreateGridButPostponeRefinement2peano4_toolbox_blockstructured_ApplyFunctorOnPatch1(int treeNumber);

    ~CreateGridButPostponeRefinement2peano4_toolbox_blockstructured_ApplyFunctorOnPatch1();

    std::vector< peano4::grid::GridControlEvent > getGridControlEvents();

    void beginTraversal(
      );

    void endTraversal(
      );

    void createPersistentFace(
      const peano4::datamanagement::FaceMarker& marker,
      examples::exahype2::euler::facedata::EulerQ& fineGridFaceEulerQ,
      examples::exahype2::euler::facedata::EulerQNew& fineGridFaceEulerQNew,
      examples::exahype2::euler::facedata::Label& fineGridFaceLabel,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQ> coarseGridFacesEulerQ,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQNew> coarseGridFacesEulerQNew,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::Label> coarseGridFacesLabel,
      examples::exahype2::euler::celldata::EulerQ& coarseGridCellEulerQ);

    void destroyPersistentFace(
      const peano4::datamanagement::FaceMarker& marker,
      examples::exahype2::euler::facedata::EulerQ& fineGridFaceEulerQ,
      examples::exahype2::euler::facedata::EulerQNew& fineGridFaceEulerQNew,
      examples::exahype2::euler::facedata::Label& fineGridFaceLabel,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQ> coarseGridFacesEulerQ,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQNew> coarseGridFacesEulerQNew,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::Label> coarseGridFacesLabel,
      examples::exahype2::euler::celldata::EulerQ& coarseGridCellEulerQ);

    void createHangingFace(
      const peano4::datamanagement::FaceMarker& marker,
      examples::exahype2::euler::facedata::EulerQ& fineGridFaceEulerQ,
      examples::exahype2::euler::facedata::EulerQNew& fineGridFaceEulerQNew,
      examples::exahype2::euler::facedata::Label& fineGridFaceLabel,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQ> coarseGridFacesEulerQ,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQNew> coarseGridFacesEulerQNew,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::Label> coarseGridFacesLabel,
      examples::exahype2::euler::celldata::EulerQ& coarseGridCellEulerQ);

    void destroyHangingFace(
      const peano4::datamanagement::FaceMarker& marker,
      examples::exahype2::euler::facedata::EulerQ& fineGridFaceEulerQ,
      examples::exahype2::euler::facedata::EulerQNew& fineGridFaceEulerQNew,
      examples::exahype2::euler::facedata::Label& fineGridFaceLabel,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQ> coarseGridFacesEulerQ,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQNew> coarseGridFacesEulerQNew,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::Label> coarseGridFacesLabel,
      examples::exahype2::euler::celldata::EulerQ& coarseGridCellEulerQ);

    void touchFaceFirstTime(
      const peano4::datamanagement::FaceMarker& marker,
      examples::exahype2::euler::facedata::EulerQ& fineGridFaceEulerQ,
      examples::exahype2::euler::facedata::EulerQNew& fineGridFaceEulerQNew,
      examples::exahype2::euler::facedata::Label& fineGridFaceLabel,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQ> coarseGridFacesEulerQ,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQNew> coarseGridFacesEulerQNew,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::Label> coarseGridFacesLabel,
      examples::exahype2::euler::celldata::EulerQ& coarseGridCellEulerQ);

    void touchFaceLastTime(
      const peano4::datamanagement::FaceMarker& marker,
      examples::exahype2::euler::facedata::EulerQ& fineGridFaceEulerQ,
      examples::exahype2::euler::facedata::EulerQNew& fineGridFaceEulerQNew,
      examples::exahype2::euler::facedata::Label& fineGridFaceLabel,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQ> coarseGridFacesEulerQ,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQNew> coarseGridFacesEulerQNew,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::Label> coarseGridFacesLabel,
      examples::exahype2::euler::celldata::EulerQ& coarseGridCellEulerQ);

    void createCell(
      const peano4::datamanagement::CellMarker& marker,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQ> fineGridFacesEulerQ,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQNew> fineGridFacesEulerQNew,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::Label> fineGridFacesLabel,
      examples::exahype2::euler::celldata::EulerQ& fineGridCellEulerQ,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQ> coarseGridFacesEulerQ,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQNew> coarseGridFacesEulerQNew,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::Label> coarseGridFacesLabel,
      examples::exahype2::euler::celldata::EulerQ& coarseGridCellEulerQ);

    void destroyCell(
      const peano4::datamanagement::CellMarker& marker,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQ> fineGridFacesEulerQ,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQNew> fineGridFacesEulerQNew,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::Label> fineGridFacesLabel,
      examples::exahype2::euler::celldata::EulerQ& fineGridCellEulerQ,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQ> coarseGridFacesEulerQ,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQNew> coarseGridFacesEulerQNew,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::Label> coarseGridFacesLabel,
      examples::exahype2::euler::celldata::EulerQ& coarseGridCellEulerQ);

    void touchCellFirstTime(
      const peano4::datamanagement::CellMarker& marker,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQ> fineGridFacesEulerQ,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQNew> fineGridFacesEulerQNew,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::Label> fineGridFacesLabel,
      examples::exahype2::euler::celldata::EulerQ& fineGridCellEulerQ,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQ> coarseGridFacesEulerQ,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQNew> coarseGridFacesEulerQNew,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::Label> coarseGridFacesLabel,
      examples::exahype2::euler::celldata::EulerQ& coarseGridCellEulerQ);

    void touchCellLastTime(
      const peano4::datamanagement::CellMarker& marker,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQ> fineGridFacesEulerQ,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQNew> fineGridFacesEulerQNew,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::Label> fineGridFacesLabel,
      examples::exahype2::euler::celldata::EulerQ& fineGridCellEulerQ,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQ> coarseGridFacesEulerQ,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQNew> coarseGridFacesEulerQNew,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::Label> coarseGridFacesLabel,
      examples::exahype2::euler::celldata::EulerQ& coarseGridCellEulerQ);

};


#endif

