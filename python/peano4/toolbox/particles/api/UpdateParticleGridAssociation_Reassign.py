# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
from peano4.solversteps.ActionSet import ActionSet


from .AbstractUpdateParticleGridAssociation import AbstractUpdateParticleGridAssociation
from .UpdateParticleGridAssociation_LiftDrop import (
    UpdateParticleGridAssociation_LiftDrop,
)

import jinja2


class UpdateParticleGridAssociation_Reassign(AbstractUpdateParticleGridAssociation):
    """!

    Associate particles to spacetree vertices

    Sort particles into the respective levels and cells. This mapping should
    be used every time you move particles around.

    ## Algorithm

    The re-association has different ingredients. We have two implementations
    currently available. In this version, we try to minimise the particle
    re-associations. For this, the algorithm realises the key checks within
    the cells:

    - Look at all the particles within a cell and check whether they are
      still assigned to the closest of the @f$ 2^d @f$ vertices of this
      cell. If not, reassign them.
    - Identify particles that have moved by more than one mesh width since the
      last update. If a particle travels fast, lift it to the next level. This
      check happens recursively whenever we touch a cell for the last time.
    - Drop particles down within the tree hierarchy such that they reside on
      an as fine level as possible. This happens in touchVertexFirstTime(), i.e.
      in-between two grid traversals, particles are distributed over all
      resolutions, but we move them down in the hierarchy as kind of a preamble
      of the subsequent grid sweep.

    The persent code variant assumes that particles do not move after
    touchVertexFirstTime(). So you can alter them in touchVertexFirstTime()
    and then the code will sort them properly in this sweep, but if you change
    their position again in touchCellLastTime() or touchVertexLastTime(), then
    you will have an unsorted state by the end of the traversal.


    ## Tunneling and horizontal data movements between trees

    All cores run through their trees. On the finest mesh level, we have a
    non-overlapping domain decomposition. Each cell has a unique owner tree.
    On the coarser resolutions, the association is not unique anymore. We
    make it unique, i.e. each cell on any level has a unique owner. The owners
    (aka threads or ranks) run through their trees concurrently and exchange
    their boundary data after the traversal. If particles are to be lifted, this
    causes issues, as we cannot exchange them between two levels after the
    traversal. We could, but then we can't lift them any further within the
    first traversal, and we also cannot drop them into another rank at the
    same time.

    Therefore, I employ a mixture of the pidt technique from the paper and the
    sieve approach also discussed there. Usually, I do all lifts and drops
    right throughout the mesh traversal. If I'd lift into a cell/level which
    is owned by another tree, I don't lift but instead dump the particle into
    a rank-global list. These lists are then exchanged after each traversal
    sweep. Drops now can either happen from the next coarser level or this
    global list. I call the latter a sieve.

    Due to this global list approach, I can support tunneling, i.e. particles
    racing through multiple cells in one time step, and all of this works with
    MPI, too.


    ## Statistics

    The mapping keeps some statistics on any state update. These statistics
    however are not shared via MPI or synchronised between different threads.
    It is the job of the ParticleSet to ensure that we have a proper global
    bookkeeping.


    ## Hanging vertices

    Hanging vertices are assigned particles when they are created and their
    particles are lifted again, when they are destroyed. You can disable to
    drops in the constructor. This might reduce the number of overall drops.
    However, you will always need the lift, as a cell might reassign a particle
    to a hanging vertex at any time, and then this particle has to be lifted
    before the vertex is destroyed.


    ## Where and how to use

    Within your algorithm step, this update action set should always be the
    first or one of the first action sets before you add any other action set.
    If you add it first, it ensures that the drop of particles is one of the
    first things that happen, before any user code is triggered. As Peano
    automatically inverts the action set order throughout the backtracking,
    adding UpdateParticleGridAssociation as first action set ensures that the
    lifts are basically the last thing you do. This is particular imporant if
    your user code alters particle positions in touchVertexLastTime().

    Your main code has to call finishedTraversal() on the tracer set after each
    sweep on each rank. This important to roll those particles over that have
    been sieved or have been lifted.

    This action set has to be used in each and every grid traversal. In-between
    time steps, it is idempotent if particles do not move or change their
    search radius. However, as we have hanging vertices, it might move
    particular up and down. So you have to insert it into each and every
    mesh traversal.

    If you disable drops, the action set becomes idempotent.


    ## Interpolay with particle storage

    This sorting scheme should not be used if you use a particle memory pool,
    i.e. if you try to store particles continuously - unless you insert
    explicit gather/resort steps. Read through the @ref toolbox_particles_memorypool "memory implications for any particle sorting".


    ## Parameters

    drop_into_hanging_vertices: Boolean

    """

    def __init__(self, particle_set, guard="true", drop_into_hanging_vertices=True):
        super(UpdateParticleGridAssociation_Reassign, self).__init__(
            particle_set, guard
        )

        self._drop_into_hanging_vertices = drop_into_hanging_vertices

    _Template_Sieve = UpdateParticleGridAssociation_LiftDrop._Template_Sieve

    """

  @see toolbox::particles::getParticleReassociationInstructionWithinCellWithIntraCellReassignment()

  """
    __Template_ReassignAndLiftWithinCell = jinja2.Template(
        """
  for (int i=0; i<TwoPowerD; i++) {
    vertexdata::{{PARTICLES_CONTAINER}}::iterator p = fineGridVertices{{PARTICLES_CONTAINER}}(i).begin();
    while ( p!=fineGridVertices{{PARTICLES_CONTAINER}}(i).end() ) {
      toolbox::particles::ParticleReassociationInstruction instruction =
        toolbox::particles::getParticleReassociationInstructionWithinCellWithIntraCellReassignment(
          p, marker, i
        );
      switch ( instruction ) {
        case toolbox::particles::ParticleReassociationInstruction_Keep:
          p++;
          break;
        case toolbox::particles::ParticleReassociationInstruction_SieveGlobally:
          logDebug( "touchVertexLastTime(...)", "particle " << (*p)->toString() << " has to be sieved globally. Current assocation=" << marker.toString() );
          _numberOfLiftsIntoSieveSet++;
          toolbox::particles::assignmentchecks::detachParticleFromVertex( "{{PARTICLE}}", (*p)->getX(), (*p)->getParallelState()==globaldata::{{PARTICLE}}::ParallelState::Local, marker.x(), marker.h(), _spacetreeId, "UpdateParticleGridAssociation_Reassign::__Template_ReassignAndLiftWithinCell" );
          toolbox::particles::assignmentchecks::assignParticleToSieveSet( "{{PARTICLE}}", (*p)->getX(), (*p)->getParallelState()==globaldata::{{PARTICLE}}::ParallelState::Local, _spacetreeId, "UpdateParticleGridAssociation_Reassign::__Template_ReassignAndLiftWithinCell" );
          p = fineGridVertex{{PARTICLES_CONTAINER}}.particleCanNotBeLiftedLocally(p);
          break;
        case 0:
        case 1:
        case 2:
        case 3:
        #if Dimensions==3
        case 4:
        case 5:
        case 6:
        case 7:
        #endif
          p = fineGridVertices{{PARTICLES_CONTAINER}}(instruction).move(p,fineGridVertices{{PARTICLES_CONTAINER}}(i));
          toolbox::particles::assignmentchecks::detachParticleFromVertex( "{{PARTICLE}}", p->getX(), (*p)->getParallelState()==globaldata::{{PARTICLE}}::ParallelState::Local, marker.x(), marker.h(), _spacetreeId, "UpdateParticleGridAssociation_Reassign::__Template_ReassignAndLiftWithinCell" );
          toolbox::particles::assignmentchecks::assignParticleToVertex( 
            "{{PARTICLE}}",
        (*p)->getX(),
        (*p)->getParallelState()==globaldata::{{PARTICLE}}::ParallelState::Local,  
        marker.x() 
        - 
        tarch::la::multiplyComponents( 
          tarch::la::convertScalar<double>(marker.getRelativePositionWithinFatherCell()),
          marker.h()
        )
        +
        tarch::la::multiplyComponents( 
          tarch::la::Vector<Dimensions,double>(target),
          3.0*marker.h()
        ),
            3.0*marker.h(), _spacetreeId, "UpdateParticleGridAssociation_Reassign::__Template_ReassignAndLiftWithinCell"
          );
          _numberOfParticleReassignmentsOnCurrentLevel++;
          break;
        case 0+TwoPowerD:
        case 1+TwoPowerD:
        case 2+TwoPowerD:
        case 3+TwoPowerD:
        #if Dimensions==3
        case 4+TwoPowerD:
        case 5+TwoPowerD:
        case 6+TwoPowerD:
        case 7+TwoPowerD:
        #endif
          (*p)->setCellH(3.0 * marker.h());
          toolbox::particles::assignmentchecks::detachParticleFromVertex( "{{PARTICLE}}", p->getX(), (*p)->getParallelState()==globaldata::{{PARTICLE}}::ParallelState::Local, marker.x(), marker.h(), _spacetreeId, "UpdateParticleGridAssociation_Reassign::__Template_ReassignAndLiftWithinCell" );
          toolbox::particles::assignmentchecks::assignParticleToVertex( 
            "{{PARTICLE}}",
            (*p)->getX(), 
            (*p)->getParallelState()==globaldata::{{PARTICLE}}::ParallelState::Local, 
            marker.x() 
            - 
            tarch::la::multiplyComponents( 
              tarch::la::convertScalar<double>(marker.getRelativePositionWithinFatherCell()),
              marker.h()
            )
            +
            tarch::la::multiplyComponents( 
              tarch::la::Vector<Dimensions,double>( std::bitset<Dimensions>(instruction) ),
              3.0*marker.h()
            ),
            3.0*marker.h(), _spacetreeId,
            "UpdateParticleGridAssociation_Reassign::__Template_ReassignAndLiftWithinCell"
          );
          coarseGridVertices{{PARTICLES_CONTAINER}}(instruction-TwoPowerD).push_back(*p);
          p = fineGridVertices{{PARTICLES_CONTAINER}}(i).erase(p);
          _numberOfLifts ++;
          break;
        default:
          assertionMsg( false, "value not implemented" );
          break;
      }
    }
  }

"""
    )

    def get_body_of_operation(self, operation_name):
        """

        Core algorithm. See class description.

        """
        result = "\n"
        if operation_name == ActionSet.OPERATION_BEGIN_TRAVERSAL:
            result = self._Template_BeginTraversal.render(**self.d)
        if (
            self._drop_into_hanging_vertices
            and operation_name == ActionSet.OPERATION_CREATE_HANGING_VERTEX
        ):
            result = self._Template_Drop.render(**self.d)
            result += self._Template_Sieve.render(**self.d)
        if operation_name == ActionSet.OPERATION_TOUCH_VERTEX_FIRST_TIME:
            result = self.__Template_Drop.render(**self.d)
            result += self.__Template_Sieve.render(**self.d)
        if operation_name == ActionSet.OPERATION_TOUCH_VERTEX_LAST_TIME:
            result = self.__Template_ValidateParticleLevelAssociation.render(**self.d)
        if operation_name == ActionSet.OPERATION_TOUCH_CELL_LAST_TIME:
            result = self.__Template_ReassignAndLiftWithinCell.render(**self.d)
        if operation_name == ActionSet.OPERATION_DESTROY_PERSISTENT_VERTEX:
            result = self._Template_LiftAllParticles.render(**self.d)
        if operation_name == ActionSet.OPERATION_END_TRAVERSAL:
            result = self._Template_EndTraversal.render(**self.d)
        return result

    def get_action_set_name(self):
        return __name__.replace(".py", "").replace(".", "_")
