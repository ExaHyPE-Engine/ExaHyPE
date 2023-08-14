# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
from peano4.solversteps.ActionSet import ActionSet


import jinja2
import dastgen2
import peano4


class DynamicMeshRefinementAnalysis(ActionSet):
    """!

    AMR criterion based upon the particle density

    This implementation is almsot exactly the same as the toolbox variant,
    but it does not work with a static field, but instead pipes all the
    refinement events a global variable. Actually, it fuses two different
    action sets of the toolbox: The actual stats over the markers which
    count the particles and the translation fo this info into mesh refinement
    and coarsening commands.

    Different to the default toolbox variant, Swift's marker doesn't need to
    distinguish a new and an old marker and we use the marker information
    straightaway throughout the steps up.

    Please consult the constructor, i.e. __init__(), for some information
    on algorithm variants and the implementation.

    To understand the code's behaviour, it is resonable to read through
    @ref peano_amr_tex "Peano's generic AMR description".

    """

    def marker_name(name):
        return name + "CellStatistics"

    def create_cell_marker(name):
        """

        Create the cell marker that is used by the analysis

        t.b.d.

        """
        cell_marker = peano4.datamodel.DaStGen2(
            DynamicMeshRefinementAnalysis.marker_name(name)
        )

        cell_marker.data.add_attribute(dastgen2.attributes.Integer("NumberOfParticles"))
        cell_marker.data.add_attribute(
            dastgen2.attributes.Boolean("ParentOfRefinedCell")
        )
        cell_marker.peano4_mpi_and_storage_aspect.store_predicate = "false"
        cell_marker.peano4_mpi_and_storage_aspect.load_predicate = "false"

        return cell_marker

    def __init__(
        self,
        particle_set,
        min_particles_per_cell=1,
        max_particles_per_cell=65536,
        min_h=0.1,
        max_h=0.01,
        scale_marker=0.95,
    ):
        """!

        Construct the action set

        ## Arguments

        particle_set: peano4.toolbox.particles.ParticleSet
           Instance of the particle set that is to be used as trigger. Each particle in the
           set has a search radius and this one is used to guide the AMR.

        min_particles_per_cell: Integer (>=0)
           The tree is not refined if there are less than min_particles_per_cell particles
           within the cell. By default, this parameter is set to 1 and thus the code refines
           always if there are more than two particles held in a cell. You can set the value
           to 0. In this case, we refine whenever there's a particle in a cell and its search
           radius would allow us to refine further. Set
           it to a higher number to ensure that there's (approximately) a certain particle
           counter per cell.

        max_particles_per_cell: Integer
           Forces the code to refine if there are more than max_particles_per_cell
           particles held. Has to be bigger or equal to min_particles_per_cell.

           Whenever the AMR criterion refines, it can happen that particles do not
           drop down further, as their search radius is too small.

           You can also set this argument to zero as well. In this case, the action
           sets refines all the way down until the mesh size just fits the search
           radius. It even refines a cell if there's only one particle in it.

        scale_marker: Float
           In principle, many particle codes create a mesh that is by magnitudes
           too fine. Therefore, it makes sense to make the refinement events a
           little bit smaller than the actual underlying cell. However, if they are
           small, the ::peano4::grid::merge() operation might not be able to fuse
           them, i.e. you have to be careful that you increase the tolerance there
           in return.

        """
        if min_particles_per_cell < 0:
            raise Exception(
                "invalid min number of particles pre cell (has to be positive): {}".format(
                    min_particles_per_cell
                )
            )

        if (
            max_particles_per_cell < min_particles_per_cell
            or max_particles_per_cell < 0
        ):
            raise Exception(
                "max number of particles ({}) has to be bigger or equal to min number ({}) and it has to be positive".format(
                    max_particles_per_cell, min_particles_per_cell
                )
            )

        self.d = {
            "CELL_DATA_NAME": DynamicMeshRefinementAnalysis.marker_name(
                particle_set.name
            ),
            "MIN_PARTICLES_PER_CELL": min_particles_per_cell,
            "MAX_PARTICLES_PER_CELL": max_particles_per_cell,
            "MIN_H": min_h,
            "MAX_H": max_h,
            "PARTICLE_SET": particle_set.name,
            "PARTICLE": particle_set.particle_model.name,
            "SCALE_MARKER": scale_marker,
        }
        pass

    def user_should_modify_template(self):
        return False

    __Template_TouchCellFirstTime = jinja2.Template(
        """
  fineGridCell{{CELL_DATA_NAME}}.setNumberOfParticles(0);
  fineGridCell{{CELL_DATA_NAME}}.setParentOfRefinedCell( false );
"""
    )

    __Template_CreateCell = jinja2.Template(
        """
  fineGridCell{{CELL_DATA_NAME}}.setNumberOfParticles( std::numeric_limits<int>::max() );
  fineGridCell{{CELL_DATA_NAME}}.setParentOfRefinedCell( true );
"""
    )

    __Template_TouchCellLastTime = jinja2.Template(
        """
  /*

   Local particles
   ===============

   Determine local number of particles associated with the adjacent
   vertices.

   */
  int count = 0;
  for (int i=0; i<TwoPowerD; i++) {
    count += fineGridVertices{{PARTICLE_SET}}(i).size();
  }


  /*

   Determine local counter
   =======================

   We might already have received data from finer cells (if we are
   in a refined cell), so we add the data. We might also be in a
   newly generated cell, where adding new particles might cause an
   overflow. So I protect the accumulation.

   */
  if ( fineGridCell{{CELL_DATA_NAME}}.getNumberOfParticles() < std::numeric_limits<int>::max() ) {
    fineGridCell{{CELL_DATA_NAME}}.setNumberOfParticles(
      fineGridCell{{CELL_DATA_NAME}}.getNumberOfParticles()
      +
      count
    );
  }

  /*

   Restrict the particle count to the next coarser level
   =====================================================

   */
  coarseGridCell{{CELL_DATA_NAME}}.setNumberOfParticles(
    coarseGridCell{{CELL_DATA_NAME}}.getNumberOfParticles()
    +
    fineGridCell{{CELL_DATA_NAME}}.getNumberOfParticles()
  );

  if ( marker.willBeRefined() ) {
    coarseGridCell{{CELL_DATA_NAME}}.setParentOfRefinedCell( true );
  }

  /*

   To understand the code's behaviour, it is resonable to read through 
   @ref peano_amr_tex "Peano's generic AMR description".

   Refine
   ======
   
   Refine if we have not yet reached the finest mesh resolution and violate
   the max particle count. This implies that we actually get a mesh which is
   slightly finer then min_h or this species.
   
   Erase
   =====

   If we want to erase a cell, we actually make the region that is to be
   erased slightly bigger. This is in line with @ref peano_amr_tex "Peano's generic AMR description". 
   Furthermore, we mark only refined cells
   one level above the finest mesh. These are cells which will be refined and
   have been refined, and also carry only few particles such that we could
   safely erase their children plus have no refined children. To find out if
   this holds for a cell, we look at the flag getParentOfRefinedCell().
   
  */
  if (
    not marker.willBeRefined()
    and
    fineGridCell{{CELL_DATA_NAME}}.getNumberOfParticles() > {{MAX_PARTICLES_PER_CELL}}
  ) {
    const double ScaleMarker = {{SCALE_MARKER}};
    peano4::grid::GridControlEvent newEntry;
    newEntry.setRefinementControl( peano4::grid::GridControlEvent::RefinementControl::Refine );
    newEntry.setOffset( marker.getOffset() + marker.h() * (1.0-ScaleMarker) / 2.0 );
    newEntry.setWidth( marker.h() * ScaleMarker );
    newEntry.setH( 
      tarch::la::greater( marker.h()(0), {{MIN_H}} ) ? marker.h()/3.0 * 1.1 : marker.h() * 1.1
    );
    _localGridControlEvents.push_back(newEntry);
    logDebug( "touchCellLastTime(...)", "add " << newEntry.toString() );
  }

  if (
    marker.willBeRefined()
    and
    marker.hasBeenRefined()
    and
    fineGridCell{{CELL_DATA_NAME}}.getNumberOfParticles() <= {{MIN_PARTICLES_PER_CELL}}
    and
    not fineGridCell{{CELL_DATA_NAME}}.getParentOfRefinedCell()
    and
    marker.h()(0)<{{MAX_H}} // we will remove the suboctants
  ) {
    const double ScaleMarker = 3.0 * (1.0+{{SCALE_MARKER}});
    peano4::grid::GridControlEvent newEntry;
    newEntry.setRefinementControl( peano4::grid::GridControlEvent::RefinementControl::Erase );
    newEntry.setOffset( marker.getOffset() + marker.h() * (1.0-ScaleMarker) / 2.0 );
    newEntry.setWidth( marker.h() * ScaleMarker );
    newEntry.setH( marker.h() * ScaleMarker );
    _localGridControlEvents.push_back(newEntry);
    logDebug( "touchCellLastTime(...)", "add " << newEntry.toString() );
  }
"""
    )

    __Template_EndTraversal = jinja2.Template(
        """
  static tarch::multicore::BooleanSemaphore semaphore;
  tarch::multicore::Lock lock(semaphore);

  _newGridControlEvents.insert( _newGridControlEvents.end(), _localGridControlEvents.begin(), _localGridControlEvents.end() );
"""
    )

    def get_body_of_operation(self, operation_name):
        result = "\n"
        if operation_name == ActionSet.OPERATION_BEGIN_TRAVERSAL:
            result = """
  _localGridControlEvents.clear();
"""
        if operation_name == ActionSet.OPERATION_CREATE_CELL:
            result = self.__Template_CreateCell.render(**self.d)
        if operation_name == ActionSet.OPERATION_TOUCH_CELL_FIRST_TIME:
            result = self.__Template_TouchCellFirstTime.render(**self.d)
        if operation_name == ActionSet.OPERATION_TOUCH_CELL_LAST_TIME:
            result = self.__Template_TouchCellLastTime.render(**self.d)
        if operation_name == ActionSet.OPERATION_END_TRAVERSAL:
            result = self.__Template_EndTraversal.render(**self.d)
        return result

    #  def get_body_of_getGridControlEvents(self):
    #    return """
    #  return ::swift2::committedGridControlEvents;
    # """

    def get_attributes(self):
        return """
  std::list< peano4::grid::GridControlEvent >          _localGridControlEvents;
  static std::list< peano4::grid::GridControlEvent >   _newGridControlEvents;
"""

    def get_action_set_name(self):
        return __name__.replace(".py", "").replace(".", "_")

    def get_static_initialisations(self, full_qualified_classname):
        return (
            """
std::list< peano4::grid::GridControlEvent >  """
            + full_qualified_classname
            + """::_newGridControlEvents;
"""
        )

    def get_includes(self):
        result = jinja2.Template(
            """
#include "tarch/multicore/Lock.h"
#include "vertexdata/{{PARTICLE_SET}}.h"
#include "globaldata/{{PARTICLE}}.h"
#include "peano4/grid/grid.h"
#include "swift2/GridControlEvents.h"

#include <list>
"""
        )
        return result.render(**self.d)

    def get_body_of_unprepareTraversal(self):
        return """
::swift2::commitGridControlEvents( _newGridControlEvents );
_newGridControlEvents.clear();
"""
