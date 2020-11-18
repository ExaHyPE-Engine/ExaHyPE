#ifndef _EXAMPLES_EXAHYPE2_ACOUSTICS_OBSERVERS_TIMESTEP2EXAHYPE2_GRID_SETLABELS5_H_
#define _EXAMPLES_EXAHYPE2_ACOUSTICS_OBSERVERS_TIMESTEP2EXAHYPE2_GRID_SETLABELS5_H_



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


#include "facedata/AcousticsQ.h"
#include "facedata/AcousticsQNew.h"
#include "facedata/Label.h"
#include "celldata/AcousticsQ.h"
namespace examples {
namespace exahype2 {
namespace acoustics {
namespace observers {
  class TimeStep2exahype2_grid_SetLabels5;
}
}
}
}
class examples::exahype2::acoustics::observers::TimeStep2exahype2_grid_SetLabels5{
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
    TimeStep2exahype2_grid_SetLabels5(int treeNumber);

    ~TimeStep2exahype2_grid_SetLabels5();

    std::vector< peano4::grid::GridControlEvent > getGridControlEvents() const;

    void beginTraversal(
      );

    void endTraversal(
      );

    void createPersistentFace(
      const peano4::datamanagement::FaceMarker& marker,
      examples::exahype2::acoustics::facedata::AcousticsQ& fineGridFaceAcousticsQ,
      examples::exahype2::acoustics::facedata::AcousticsQNew& fineGridFaceAcousticsQNew,
      examples::exahype2::acoustics::facedata::Label& fineGridFaceLabel,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::acoustics::facedata::AcousticsQ> coarseGridFacesAcousticsQ,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::acoustics::facedata::AcousticsQNew> coarseGridFacesAcousticsQNew,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::acoustics::facedata::Label> coarseGridFacesLabel,
      examples::exahype2::acoustics::celldata::AcousticsQ& coarseGridCellAcousticsQ);

    void destroyPersistentFace(
      const peano4::datamanagement::FaceMarker& marker,
      examples::exahype2::acoustics::facedata::AcousticsQ& fineGridFaceAcousticsQ,
      examples::exahype2::acoustics::facedata::AcousticsQNew& fineGridFaceAcousticsQNew,
      examples::exahype2::acoustics::facedata::Label& fineGridFaceLabel,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::acoustics::facedata::AcousticsQ> coarseGridFacesAcousticsQ,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::acoustics::facedata::AcousticsQNew> coarseGridFacesAcousticsQNew,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::acoustics::facedata::Label> coarseGridFacesLabel,
      examples::exahype2::acoustics::celldata::AcousticsQ& coarseGridCellAcousticsQ);

    void createHangingFace(
      const peano4::datamanagement::FaceMarker& marker,
      examples::exahype2::acoustics::facedata::AcousticsQ& fineGridFaceAcousticsQ,
      examples::exahype2::acoustics::facedata::AcousticsQNew& fineGridFaceAcousticsQNew,
      examples::exahype2::acoustics::facedata::Label& fineGridFaceLabel,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::acoustics::facedata::AcousticsQ> coarseGridFacesAcousticsQ,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::acoustics::facedata::AcousticsQNew> coarseGridFacesAcousticsQNew,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::acoustics::facedata::Label> coarseGridFacesLabel,
      examples::exahype2::acoustics::celldata::AcousticsQ& coarseGridCellAcousticsQ);

    void destroyHangingFace(
      const peano4::datamanagement::FaceMarker& marker,
      examples::exahype2::acoustics::facedata::AcousticsQ& fineGridFaceAcousticsQ,
      examples::exahype2::acoustics::facedata::AcousticsQNew& fineGridFaceAcousticsQNew,
      examples::exahype2::acoustics::facedata::Label& fineGridFaceLabel,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::acoustics::facedata::AcousticsQ> coarseGridFacesAcousticsQ,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::acoustics::facedata::AcousticsQNew> coarseGridFacesAcousticsQNew,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::acoustics::facedata::Label> coarseGridFacesLabel,
      examples::exahype2::acoustics::celldata::AcousticsQ& coarseGridCellAcousticsQ);

    void touchFaceFirstTime(
      const peano4::datamanagement::FaceMarker& marker,
      examples::exahype2::acoustics::facedata::AcousticsQ& fineGridFaceAcousticsQ,
      examples::exahype2::acoustics::facedata::AcousticsQNew& fineGridFaceAcousticsQNew,
      examples::exahype2::acoustics::facedata::Label& fineGridFaceLabel,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::acoustics::facedata::AcousticsQ> coarseGridFacesAcousticsQ,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::acoustics::facedata::AcousticsQNew> coarseGridFacesAcousticsQNew,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::acoustics::facedata::Label> coarseGridFacesLabel,
      examples::exahype2::acoustics::celldata::AcousticsQ& coarseGridCellAcousticsQ);

    void touchFaceLastTime(
      const peano4::datamanagement::FaceMarker& marker,
      examples::exahype2::acoustics::facedata::AcousticsQ& fineGridFaceAcousticsQ,
      examples::exahype2::acoustics::facedata::AcousticsQNew& fineGridFaceAcousticsQNew,
      examples::exahype2::acoustics::facedata::Label& fineGridFaceLabel,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::acoustics::facedata::AcousticsQ> coarseGridFacesAcousticsQ,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::acoustics::facedata::AcousticsQNew> coarseGridFacesAcousticsQNew,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::acoustics::facedata::Label> coarseGridFacesLabel,
      examples::exahype2::acoustics::celldata::AcousticsQ& coarseGridCellAcousticsQ);

    void createCell(
      const peano4::datamanagement::CellMarker& marker,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::acoustics::facedata::AcousticsQ> fineGridFacesAcousticsQ,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::acoustics::facedata::AcousticsQNew> fineGridFacesAcousticsQNew,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::acoustics::facedata::Label> fineGridFacesLabel,
      examples::exahype2::acoustics::celldata::AcousticsQ& fineGridCellAcousticsQ,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::acoustics::facedata::AcousticsQ> coarseGridFacesAcousticsQ,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::acoustics::facedata::AcousticsQNew> coarseGridFacesAcousticsQNew,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::acoustics::facedata::Label> coarseGridFacesLabel,
      examples::exahype2::acoustics::celldata::AcousticsQ& coarseGridCellAcousticsQ);

    void destroyCell(
      const peano4::datamanagement::CellMarker& marker,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::acoustics::facedata::AcousticsQ> fineGridFacesAcousticsQ,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::acoustics::facedata::AcousticsQNew> fineGridFacesAcousticsQNew,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::acoustics::facedata::Label> fineGridFacesLabel,
      examples::exahype2::acoustics::celldata::AcousticsQ& fineGridCellAcousticsQ,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::acoustics::facedata::AcousticsQ> coarseGridFacesAcousticsQ,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::acoustics::facedata::AcousticsQNew> coarseGridFacesAcousticsQNew,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::acoustics::facedata::Label> coarseGridFacesLabel,
      examples::exahype2::acoustics::celldata::AcousticsQ& coarseGridCellAcousticsQ);

    void touchCellFirstTime(
      const peano4::datamanagement::CellMarker& marker,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::acoustics::facedata::AcousticsQ> fineGridFacesAcousticsQ,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::acoustics::facedata::AcousticsQNew> fineGridFacesAcousticsQNew,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::acoustics::facedata::Label> fineGridFacesLabel,
      examples::exahype2::acoustics::celldata::AcousticsQ& fineGridCellAcousticsQ,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::acoustics::facedata::AcousticsQ> coarseGridFacesAcousticsQ,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::acoustics::facedata::AcousticsQNew> coarseGridFacesAcousticsQNew,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::acoustics::facedata::Label> coarseGridFacesLabel,
      examples::exahype2::acoustics::celldata::AcousticsQ& coarseGridCellAcousticsQ);

    void touchCellLastTime(
      const peano4::datamanagement::CellMarker& marker,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::acoustics::facedata::AcousticsQ> fineGridFacesAcousticsQ,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::acoustics::facedata::AcousticsQNew> fineGridFacesAcousticsQNew,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::acoustics::facedata::Label> fineGridFacesLabel,
      examples::exahype2::acoustics::celldata::AcousticsQ& fineGridCellAcousticsQ,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::acoustics::facedata::AcousticsQ> coarseGridFacesAcousticsQ,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::acoustics::facedata::AcousticsQNew> coarseGridFacesAcousticsQNew,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::acoustics::facedata::Label> coarseGridFacesLabel,
      examples::exahype2::acoustics::celldata::AcousticsQ& coarseGridCellAcousticsQ);

};


#endif

