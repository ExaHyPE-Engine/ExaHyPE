#ifndef _EXAMPLES_EXAHYPE2_LOH1_OBSERVERS_CREATEGRID2EXAHYPE2_GRID_ENCLAVELABELS3_H_
#define _EXAMPLES_EXAHYPE2_LOH1_OBSERVERS_CREATEGRID2EXAHYPE2_GRID_ENCLAVELABELS3_H_



#include "peano4/utils/Globals.h"
#include "peano4/datamanagement/VertexEnumerator.h"
#include "peano4/datamanagement/VertexMarker.h"
#include "peano4/datamanagement/FaceEnumerator.h"
#include "peano4/datamanagement/FaceMarker.h"
#include "peano4/datamanagement/CellMarker.h"
#include "peano4/grid/GridControlEvent.h"
#include "tarch/la/Vector.h"

#include <vector>



#include "Constants.h"
#include "exahype2/EnclaveBookkeeping.h"


#include "facedata/LOH1Q.h"
#include "facedata/LOH1QNew.h"
#include "facedata/Label.h"
#include "celldata/LOH1Q.h"
#include "celldata/LOH1CellSemaphoreLabel.h"
namespace examples {
namespace exahype2 {
namespace loh1 {
namespace observers {
  class CreateGrid2exahype2_grid_EnclaveLabels3;
}
}
}
}
class examples::exahype2::loh1::observers::CreateGrid2exahype2_grid_EnclaveLabels3{
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
    CreateGrid2exahype2_grid_EnclaveLabels3(int treeNumber);

    ~CreateGrid2exahype2_grid_EnclaveLabels3();

    std::vector< peano4::grid::GridControlEvent > getGridControlEvents();

    void beginTraversal(
      );

    void endTraversal(
      );

    void createPersistentFace(
      const peano4::datamanagement::FaceMarker& marker,
      examples::exahype2::loh1::facedata::LOH1Q& fineGridFaceLOH1Q,
      examples::exahype2::loh1::facedata::LOH1QNew& fineGridFaceLOH1QNew,
      examples::exahype2::loh1::facedata::Label& fineGridFaceLabel,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::loh1::facedata::LOH1Q> coarseGridFacesLOH1Q,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::loh1::facedata::LOH1QNew> coarseGridFacesLOH1QNew,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::loh1::facedata::Label> coarseGridFacesLabel,
      examples::exahype2::loh1::celldata::LOH1Q& coarseGridCellLOH1Q,
      examples::exahype2::loh1::celldata::LOH1CellSemaphoreLabel& coarseGridCellLOH1CellSemaphoreLabel);

    void destroyPersistentFace(
      const peano4::datamanagement::FaceMarker& marker,
      examples::exahype2::loh1::facedata::LOH1Q& fineGridFaceLOH1Q,
      examples::exahype2::loh1::facedata::LOH1QNew& fineGridFaceLOH1QNew,
      examples::exahype2::loh1::facedata::Label& fineGridFaceLabel,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::loh1::facedata::LOH1Q> coarseGridFacesLOH1Q,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::loh1::facedata::LOH1QNew> coarseGridFacesLOH1QNew,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::loh1::facedata::Label> coarseGridFacesLabel,
      examples::exahype2::loh1::celldata::LOH1Q& coarseGridCellLOH1Q,
      examples::exahype2::loh1::celldata::LOH1CellSemaphoreLabel& coarseGridCellLOH1CellSemaphoreLabel);

    void createHangingFace(
      const peano4::datamanagement::FaceMarker& marker,
      examples::exahype2::loh1::facedata::LOH1Q& fineGridFaceLOH1Q,
      examples::exahype2::loh1::facedata::LOH1QNew& fineGridFaceLOH1QNew,
      examples::exahype2::loh1::facedata::Label& fineGridFaceLabel,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::loh1::facedata::LOH1Q> coarseGridFacesLOH1Q,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::loh1::facedata::LOH1QNew> coarseGridFacesLOH1QNew,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::loh1::facedata::Label> coarseGridFacesLabel,
      examples::exahype2::loh1::celldata::LOH1Q& coarseGridCellLOH1Q,
      examples::exahype2::loh1::celldata::LOH1CellSemaphoreLabel& coarseGridCellLOH1CellSemaphoreLabel);

    void destroyHangingFace(
      const peano4::datamanagement::FaceMarker& marker,
      examples::exahype2::loh1::facedata::LOH1Q& fineGridFaceLOH1Q,
      examples::exahype2::loh1::facedata::LOH1QNew& fineGridFaceLOH1QNew,
      examples::exahype2::loh1::facedata::Label& fineGridFaceLabel,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::loh1::facedata::LOH1Q> coarseGridFacesLOH1Q,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::loh1::facedata::LOH1QNew> coarseGridFacesLOH1QNew,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::loh1::facedata::Label> coarseGridFacesLabel,
      examples::exahype2::loh1::celldata::LOH1Q& coarseGridCellLOH1Q,
      examples::exahype2::loh1::celldata::LOH1CellSemaphoreLabel& coarseGridCellLOH1CellSemaphoreLabel);

    void touchFaceFirstTime(
      const peano4::datamanagement::FaceMarker& marker,
      examples::exahype2::loh1::facedata::LOH1Q& fineGridFaceLOH1Q,
      examples::exahype2::loh1::facedata::LOH1QNew& fineGridFaceLOH1QNew,
      examples::exahype2::loh1::facedata::Label& fineGridFaceLabel,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::loh1::facedata::LOH1Q> coarseGridFacesLOH1Q,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::loh1::facedata::LOH1QNew> coarseGridFacesLOH1QNew,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::loh1::facedata::Label> coarseGridFacesLabel,
      examples::exahype2::loh1::celldata::LOH1Q& coarseGridCellLOH1Q,
      examples::exahype2::loh1::celldata::LOH1CellSemaphoreLabel& coarseGridCellLOH1CellSemaphoreLabel);

    void touchFaceLastTime(
      const peano4::datamanagement::FaceMarker& marker,
      examples::exahype2::loh1::facedata::LOH1Q& fineGridFaceLOH1Q,
      examples::exahype2::loh1::facedata::LOH1QNew& fineGridFaceLOH1QNew,
      examples::exahype2::loh1::facedata::Label& fineGridFaceLabel,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::loh1::facedata::LOH1Q> coarseGridFacesLOH1Q,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::loh1::facedata::LOH1QNew> coarseGridFacesLOH1QNew,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::loh1::facedata::Label> coarseGridFacesLabel,
      examples::exahype2::loh1::celldata::LOH1Q& coarseGridCellLOH1Q,
      examples::exahype2::loh1::celldata::LOH1CellSemaphoreLabel& coarseGridCellLOH1CellSemaphoreLabel);

    void createCell(
      const peano4::datamanagement::CellMarker& marker,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::loh1::facedata::LOH1Q> fineGridFacesLOH1Q,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::loh1::facedata::LOH1QNew> fineGridFacesLOH1QNew,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::loh1::facedata::Label> fineGridFacesLabel,
      examples::exahype2::loh1::celldata::LOH1Q& fineGridCellLOH1Q,
      examples::exahype2::loh1::celldata::LOH1CellSemaphoreLabel& fineGridCellLOH1CellSemaphoreLabel,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::loh1::facedata::LOH1Q> coarseGridFacesLOH1Q,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::loh1::facedata::LOH1QNew> coarseGridFacesLOH1QNew,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::loh1::facedata::Label> coarseGridFacesLabel,
      examples::exahype2::loh1::celldata::LOH1Q& coarseGridCellLOH1Q,
      examples::exahype2::loh1::celldata::LOH1CellSemaphoreLabel& coarseGridCellLOH1CellSemaphoreLabel);

    void destroyCell(
      const peano4::datamanagement::CellMarker& marker,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::loh1::facedata::LOH1Q> fineGridFacesLOH1Q,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::loh1::facedata::LOH1QNew> fineGridFacesLOH1QNew,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::loh1::facedata::Label> fineGridFacesLabel,
      examples::exahype2::loh1::celldata::LOH1Q& fineGridCellLOH1Q,
      examples::exahype2::loh1::celldata::LOH1CellSemaphoreLabel& fineGridCellLOH1CellSemaphoreLabel,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::loh1::facedata::LOH1Q> coarseGridFacesLOH1Q,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::loh1::facedata::LOH1QNew> coarseGridFacesLOH1QNew,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::loh1::facedata::Label> coarseGridFacesLabel,
      examples::exahype2::loh1::celldata::LOH1Q& coarseGridCellLOH1Q,
      examples::exahype2::loh1::celldata::LOH1CellSemaphoreLabel& coarseGridCellLOH1CellSemaphoreLabel);

    void touchCellFirstTime(
      const peano4::datamanagement::CellMarker& marker,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::loh1::facedata::LOH1Q> fineGridFacesLOH1Q,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::loh1::facedata::LOH1QNew> fineGridFacesLOH1QNew,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::loh1::facedata::Label> fineGridFacesLabel,
      examples::exahype2::loh1::celldata::LOH1Q& fineGridCellLOH1Q,
      examples::exahype2::loh1::celldata::LOH1CellSemaphoreLabel& fineGridCellLOH1CellSemaphoreLabel,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::loh1::facedata::LOH1Q> coarseGridFacesLOH1Q,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::loh1::facedata::LOH1QNew> coarseGridFacesLOH1QNew,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::loh1::facedata::Label> coarseGridFacesLabel,
      examples::exahype2::loh1::celldata::LOH1Q& coarseGridCellLOH1Q,
      examples::exahype2::loh1::celldata::LOH1CellSemaphoreLabel& coarseGridCellLOH1CellSemaphoreLabel);

    void touchCellLastTime(
      const peano4::datamanagement::CellMarker& marker,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::loh1::facedata::LOH1Q> fineGridFacesLOH1Q,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::loh1::facedata::LOH1QNew> fineGridFacesLOH1QNew,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::loh1::facedata::Label> fineGridFacesLabel,
      examples::exahype2::loh1::celldata::LOH1Q& fineGridCellLOH1Q,
      examples::exahype2::loh1::celldata::LOH1CellSemaphoreLabel& fineGridCellLOH1CellSemaphoreLabel,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::loh1::facedata::LOH1Q> coarseGridFacesLOH1Q,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::loh1::facedata::LOH1QNew> coarseGridFacesLOH1QNew,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::loh1::facedata::Label> coarseGridFacesLabel,
      examples::exahype2::loh1::celldata::LOH1Q& coarseGridCellLOH1Q,
      examples::exahype2::loh1::celldata::LOH1CellSemaphoreLabel& coarseGridCellLOH1CellSemaphoreLabel);

};


#endif

