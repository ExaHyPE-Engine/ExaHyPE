# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
from peano4.solversteps.ActionSet import ActionSet


import jinja2


class ParticleAMR(ActionSet):
    """!

    AMR criterion based upon the particle density

    Most codes use this criterion in combination with a classic mesh-based criterion
    which ensures that the mesh is not coarser than a certain maximal mesh sizes.

    The algorithm relies heavily on the ParticleTreeAnalysis action set which
    annotates each cell in the tree with the sum of the vertices associated to
    the 2^d adjacent vertices. If this sum exceeds certain thresholds and, depending
    on the setup, some search radii of hosted particles are small enough, the code
    refines the tree. It is then the job of subsequent runs with
    UpdateParticleGridAssociation to drop the particles into the newly created
    cells.


    As we work with cell data to guide the adaptivity yet store the particles
    within vertices, it is clear that refinement pattern tends to smear out. We
    get rather regularly refined areas around the particles. For most applications,
    that's totally fine, as it also ensures that

    """

    def __init__(
        self,
        particle_set,
        particle_tree_analysis,
        min_particles_per_cell=1,
        max_particles_per_cell=65536,
        min_h=0.1,
        max_h=0.01,
    ):
        """

        :: Arguments



        particle_set: peano4.toolbox.particles.ParticleSet
           Instance of the particle set that is to be used as trigger. Each particle in the
           set has a search radius and this one is used to guide the AMR.

        particle_tree_analysis: peano4.toolbox.particles.ParticleTreeAnalysis
           Tree analysis algorithm. Usually is instantiated over particle_set, but you can
           also impose another analysis algorithm.

        min_particles_per_cell: Integer (>=0)
           The tree is not refined if there are less than min_particles_per_cell particles
           within the cell. By default, this parameter is set to 1 and thus the code refines
           always if there are more than two particles held in a cell. You can set the value
           to 0. In this case, we refine whenever there's a particle in a cell and its search
           radius would allow us to refine further. Set
           it to a higher number to ensure that there's (approximately) a certain particle
           counter per cell.

        max_particles_per_cell: Integer
           Forces the code to refine if there are more than max_particles_per_cell particles
           held. Has to be bigger or equal to min_particles_per_cell.

           Whenever the AMR criterion refines, it can happen that particles do not
           drop down further, as their search radius is too small.

           You can also set this argument to zero as well. In this case, the action sets
           refines all the way down until the mesh size just fits the search radius. It
           even refines a cell if there's only one particle in it.

        """
        super(ParticleAMR, self).__init__(descend_invocation_order=1, parallel=False)
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
            "CELL_DATA_NAME": particle_tree_analysis._cell_data_name,
            "MIN_PARTICLES_PER_CELL": min_particles_per_cell,
            "MAX_PARTICLES_PER_CELL": max_particles_per_cell,
            "MIN_H": min_h,
            "MAX_H": max_h,
            "PARTICLE_SET": particle_set.name,
            "PARTICLE": particle_set.particle_model.name,
        }
        pass

    def user_should_modify_template(self):
        return False

    __Template_TouchCellLastTime = jinja2.Template(
        """
  /*

   Erase
   =====

   If we want to erase a cell, we actually make the region that is to be
   erased slightly bigger. While we can pick this blow-up factor arbitrarily
   (up to a factor of three might make sense in some cases), but if we make
   is 2.0, then an erased cell covers half of its neighbour. If we have a
   cell which hosts a particle, this cell will then maybe not coarsen, but
   all of its neighbours do, so the cell is eliminated. This triggers a
   mesh oscillation. So the factor should either be smaller than 2.0, or we
   have to veto that an area with particles is overwritten. Indeed we go
   down the latter route and mark cells that we may not erase with a refine
   event which describes the mesh status quo.


   This is a tricky part. Our idea is that we mark only examine refined cells
   one level above the finest mesh. These are cells which will be refined and
   have been refined, and also carry only few particles such that we could
   safely erase their children. While we only examine refined cells, we do not
   examine refined cells which have refined children. To find this out, we
   look at the flag getParentOfRefinedCell().

  */
  if (
    not marker.willBeRefined()
    and
    fineGridCell{{CELL_DATA_NAME}}.getNumberOfParticles() > {{MAX_PARTICLES_PER_CELL}}
    and
    marker.h()(0)/3.0>{{MIN_H}}
  ) {
    const double ScaleMarker = 0.8;
    peano4::grid::GridControlEvent newEntry;
    newEntry.setRefinementControl( peano4::grid::GridControlEvent::RefinementControl::Refine );
    newEntry.setOffset( marker.getOffset() + marker.h() * (1.0-ScaleMarker) / 2.0 );
    newEntry.setWidth( marker.h() * ScaleMarker );
    newEntry.setH( marker.h()/3.0 * 1.1 );
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
    const double ScaleMarker = 1.4;
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

  // insert and then merge immediately to reduce the
  // event count
  _localGridControlEvents = peano4::grid::merge( _localGridControlEvents );
  _newGridControlEvents.insert( _newGridControlEvents.end(), _localGridControlEvents.begin(), _localGridControlEvents.end() );
  _newGridControlEvents = peano4::grid::merge( _newGridControlEvents );
"""
    )

    def get_body_of_operation(self, operation_name):
        result = "\n"
        if operation_name == ActionSet.OPERATION_BEGIN_TRAVERSAL:
            result = """
  _localGridControlEvents.clear();
"""
        if operation_name == ActionSet.OPERATION_TOUCH_CELL_LAST_TIME:
            result = self.__Template_TouchCellLastTime.render(**self.d)
        if operation_name == ActionSet.OPERATION_END_TRAVERSAL:
            result = self.__Template_EndTraversal.render(**self.d)
        return result

    def get_body_of_getGridControlEvents(self):
        return """
  return _oldGridControlEvents;
"""

    def get_attributes(self):
        return """
  std::vector< peano4::grid::GridControlEvent >          _localGridControlEvents;
  static std::vector< peano4::grid::GridControlEvent >   _oldGridControlEvents;
  static std::vector< peano4::grid::GridControlEvent >   _newGridControlEvents;
"""

    def get_action_set_name(self):
        return __name__.replace(".py", "").replace(".", "_")

    def get_static_initialisations(self, full_qualified_classname):
        return (
            """
std::vector< peano4::grid::GridControlEvent >  """
            + full_qualified_classname
            + """::_oldGridControlEvents;
std::vector< peano4::grid::GridControlEvent >  """
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
"""
        )
        return result.render(**self.d)

    def get_body_of_unprepareTraversal(self):
        return """
    #if defined(Parallel)
    #error not implemented yet
    #endif
_oldGridControlEvents.clear();
_oldGridControlEvents = _newGridControlEvents;
_newGridControlEvents.clear();
_oldGridControlEvents = peano4::grid::merge( _oldGridControlEvents );
"""
