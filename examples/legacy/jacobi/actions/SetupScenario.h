#ifndef _EXAMPLES_JACOBI_ACTIONS_SETUPSCENARIO_H_
#define _EXAMPLES_JACOBI_ACTIONS_SETUPSCENARIO_H_


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

#include "vertexdata/MG.h"



namespace examples {
  namespace jacobi {
    namespace actions {
      class SetupScenario;
    }
  }
}


class examples::jacobi::actions::SetupScenario{
  private:
    static tarch::logging::Log  _log;

    enum class Scenario {
      /**
       * Init Dirichlet problem following either
       *
       * https://math.nist.gov/~WMitchell/papers/test_problems_paper.pdf
       *
       * By default, I use the setup from Table 5.
       */
      Mitchell,

      /*
       * Alternatively, I implemented Uli Ruede's "Mathematical and
       * Computational Techniques for Multilevel Adaptive Methods".
       * You find the description in 3.10.2. The domain here's not the
       *  unit cube, but the domain (-1,1)^d.
       */
      Ruede,

      /**
       * From my CCPE paper
       */
      EpsilonFrequencies
    };

    static constexpr Scenario _scenario = Scenario::Ruede;

  public:
	static double Theta;
    static examples::jacobi::vertexdata::MG::VertexType getVertexType(const tarch::la::Vector<Dimensions,double>& x);
    static double                                            getSolution(const tarch::la::Vector<Dimensions,double>& x);
    static double                                            getEpsilon(const tarch::la::Vector<Dimensions,double>& x);
    static void                                              init(const tarch::la::Vector<Dimensions,double>& x, examples::jacobi::vertexdata::MG& fineGridVertexMG);
    static std::pair<double,double>                          getMinMax();
    static tarch::la::Vector<Dimensions,double>              getDomainOffset();
    static tarch::la::Vector<Dimensions,double>              getDomainSize();

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
    SetupScenario(int treeNumber);

    ~SetupScenario();

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

