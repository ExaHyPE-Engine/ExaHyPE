# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
from peano4.solversteps.ActionSet import ActionSet


import jinja2


class AbstractUpdateParticleGridAssociation(ActionSet):
    """!

    Associate particles to spacetree vertices

    Use one of the subclasses which realise various sorting algorithms.
    All subclasses realise the particle in dual tree storage scheme.
    Please read through @ref page_toolbox_particles_mesh_storage and
    @ref page_toolbox_particles_mesh_consistency besides the current
    class documentation. If you use a particle memory pool, i.e. if you
    don't hold the particles scattered all over the place, then it is also
    relevant to read through the @ref toolbox_particles_memorypool "implications for any particle sorting".


    If you add the action set
    peano4.toolbox.particles.PlotParticlesInVTKFormat to your algorithm
    step, the resulting file will contain all associativity information.
    You can plot to which vertex a particle belongs to.


    Here's a hand-written cartoon to explain the particle storage:

    @image html dual-tree.png


    A tree with three levels.
    Particles with a large search radius (red) are held on rather coarse levels.
    Particles with very small search radius are sorted (dropped) into the fine grid
    levels.
    Each particle is always associated to its closest vertex (dotted line).
    It is the mesh which holds (owns) the vertices.
    The toolset's predefined action set can automatically maintain/update these
    associations after each grid sweep.
    It also moves particles up and down the grid hierarchy if the particles' search
    radius changes or the grid changes.


    ## Move particles

    To move a particle, all you have to do is to alter its position:

    ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    myParticle->setX( {17.3, 8.2} );
    ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

    Obviously, any change of a particle position could mean that the particle is no
    longer associated to its nearest vertex.
    We have to sort them.


    Peano automatically maintains these relations if you add the action set
    peano4.toolbox.particles.UpdateParticleGridAssociation to
    your algorithmic steps.
    As the convention relies on a smaller-equals sign, adaptive meshes are
    automatically supported.
    If particles travel through your domain and the mesh is not very fine, they
    end up in the tree leaves.
    If particles have a large search radius, they are stored in rather coarse
    levels of the tree.


    If you change the search radius or the position of a particle, the particle
    will stay within its current cell/vertex within the tree.
    In the next grid sweep, it will however we held by the ``correct'' vertex,
    i.e. its grid-vertex association will be updated:
    The action set UpdateParticleGridAssociation updates the
    particle-tree association, i.e.~sorts the particles, on-the-fly between two mesh
    traversals.


    While you do not have to worry about how this sorting is realised, it is
    important to study the action set's documentation (just open the Python code
    documentation of subclasses) and read when data is consistent.
    Here is the general invariants that hold for all of them:

    - The very moment you alter a particle's position, you cannot be sure that
      it is still associated with the nearest vertex. Consequently, neighbourhood
      searchers might yield invalid results.
    - In the grid sweep after the position change, the particle will be
      associated with the correct vertex (unless you change its position once again
      obviously).


    The action set UpdateParticleGridAssociation relies on some global
    data. Therefore, you have to call finishTraversal() on the used
    particle set in your main routine on each rank after all rank-local traversals have termined.


    ## Domain decomposition

    You should combine this action set (or any subclass in fact) with the action
    set UpdateParallelState. Actually, it is sufficient if only the second grid
    sweep - remember that association updates always have to be added to two grid
    sweeps in a row - is added an additional action set that updates the particles'
    parallel state.


    ## Priorities

    The update association should precede any other action set in your step. The
    descend_invocation_order value thus should be very small. As priorities are inverted when we
    walk from fine to coarse resolutions, this means that all dropping into the mesh
    (in the second grid sweep) happens before anybody else touches the particles,
    while the extraction of particles or moves from fine to coarse are epilogues to
    any operation in the primary mesh sweep.


    ## Grid resorting followed by particle-particle interactions

    If you have particle-particle interactions while particles may drop within
    the mesh (due to dynamic AMR or particle movements), you have to be extra
    careful. There are two major issues to consider, which both can be
    illustrated with the sketch below.

    @image html SievingAlongAMRBoundaries.png

    ### Particles along AMR boundaries

    In the sketch, the green particle would logically belong into the finer mesh
    level. However, we should not move it up and down: The routine
    touchVertexFirstTime() should grab particles from the coarser level and drop
    them, but the routine createHangingVertex() should not. If we dropped them,
    we would have to lift them immediately afterwards again, as hanging vertices
    are not persistent in Peano.

    This would yield a lot of memory movements and also potentially destroy
    any well-aligned and ordered memory arrangement which is poisonous for
    vectorisation. Therefore, we leave particles on coarser levels. The green
    particles is not dropped.

    Let the green particle affect the blue one. As a consequence of the
    constraint above, the green particle has to be updated with contributions
    from blue. We need a proper multiscale interaction.

    ### Multiscale interaction

    Let green and blue interact. If blue resides on the coarser level and is
    dropped in the very same mesh traversal, we have to be extra careful:

    - The blue and the green particle are both cleared on the coarse level.
      Blue gets contributions from green within the coarse cell, and green gets
      contributions from blue.
    - Now we drop blue.
    - Blue is cleared due to the drop.
    - Blue is now getting contributions from the coarser level, i.e. green. As
      this is a multiscale relation, green is also added (fine grid)
      contributions from blue.

    Consequently, the particles on the coarser level gets two times the
    contributions from the particle which is dropped in this very mesh
    traversal. To avoid this, we either veto any particle-particle interaction
    in a mesh traversal which also drops, or we find a mechanism to remove the
    contributions to coarse meshes whenever we drop a particle. I don't do the
    latter usually, as it is more complicated.



    """

    DefaultDescendInvocationOrder = -65536

    def __init__(self, particle_set, guard="true"):
        super(AbstractUpdateParticleGridAssociation, self).__init__(
            descend_invocation_order=-65536, parallel=False
        )
        self._particle_set = particle_set
        self.d = {}
        self.d["PARTICLE"] = particle_set.particle_model.name
        self.d["PARTICLES_CONTAINER"] = particle_set.name
        self.d["GUARD"] = guard

    _Template_BeginTraversal = jinja2.Template(
        """
  _numberOfParticleReassignmentsOnCurrentLevel = 0;
  _numberOfLifts  = 0;
  _numberOfDrops  = 0;
  _numberOfLiftsIntoSieveSet = 0;
  _numberOfDropsFromSieveSet = 0;
  _numberOfSkippedDropsFromSieveSet = 0;
"""
    )

    _Template_EndTraversal = jinja2.Template(
        """
  vertexdata::{{PARTICLES_CONTAINER}}::updateLiftDropStatistics(
    _numberOfLifts,
    _numberOfDrops,
    _numberOfParticleReassignmentsOnCurrentLevel,
    _numberOfLiftsIntoSieveSet,
    _numberOfDropsFromSieveSet,
    _numberOfSkippedDropsFromSieveSet
  );
"""
    )

    _Template_ValidateParticleLevelAssociation = jinja2.Template(
        """
  for (auto p: fineGridVertex{{PARTICLES_CONTAINER}} ) {
    assertion3(
      marker.isContainedInAdjacentCells( p->getX(), 0.5*(1.0+SortingTolerance) ),
      marker.x(),
      marker.toString(),
      p->toString()
    );
  }
"""
    )

    """!

   Drop fitting particles from coarser mesh level into fine grid mesh

   Happens when we touch a vertex for the first time. The only thing this routine
   does is to drop particles within the tree, i.e. move them to finer levels.
   No parallelisation is taken into account.

   We have to check carefully if all coarse grid particles are local. In the
   example below

   @image html UpdateParticleGridAssociation_LiftDrop.png

   the coarse vertices could be on another rank if the tree is cut vertically.
   If could also be that the green vertex is actually from an adjacent domain
   and hence does not hold any reasonable information.

  """
    _Template_Drop = jinja2.Template(
        """
  for (int i=0; i<TwoPowerD; i++) {
    if ( marker.isParentVertexLocal(i) ) {
      // Define inside for loop, as sieve will define it as well
      vertexdata::{{PARTICLES_CONTAINER}}::iterator p = coarseGridVertices{{PARTICLES_CONTAINER}}(i).begin();
      while ( p!=coarseGridVertices{{PARTICLES_CONTAINER}}(i).end() ) {
        if ( ::toolbox::particles::dropParticle(**p,marker) ) {
          _numberOfDrops++;
          (*p)->setCellH(marker.h());
          // @todo introduce new function in vertex marker
          toolbox::particles::assignmentchecks::removeParticleFromVertex(
            "{{PARTICLE}}",
            (*p)->getX(), (*p)->getParallelState()==globaldata::{{PARTICLE}}::ParallelState::Local,
            marker.x()
            -
            tarch::la::multiplyComponents(
              tarch::la::convertScalar<double>(marker.getRelativePositionWithinFatherCell()),
              marker.h()
            )
            +
            tarch::la::multiplyComponents(
              tarch::la::Vector<Dimensions,double>( std::bitset<Dimensions>(i) ),
              3.0*marker.h()
            ),
            marker.h()*3.0, _spacetreeId
          );
          toolbox::particles::assignmentchecks::assignParticleToVertex( "{{PARTICLE}}", (*p)->getX(), (*p)->getParallelState()==globaldata::{{PARTICLE}}::ParallelState::Local, marker.x(), marker.h(), _spacetreeId );
          assertion3(
            fineGridVertex{{PARTICLES_CONTAINER}}.isValid(),
            fineGridVertex{{PARTICLES_CONTAINER}}.toString(),
            coarseGridVertices{{PARTICLES_CONTAINER}}(i).toString(),
            _spacetreeId
          );
          assertion3(
            coarseGridVertices{{PARTICLES_CONTAINER}}(i).isValid(),
            fineGridVertex{{PARTICLES_CONTAINER}}.toString(),
            coarseGridVertices{{PARTICLES_CONTAINER}}(i).toString(),
            _spacetreeId
          );
          logDebug( "touchVertexFirstTime(...)", "drop particle " << (*p)->toString() << " from " << coarseGridVertices{{PARTICLES_CONTAINER}}(i).toString() << " into " << marker.toString() );
          p = fineGridVertex{{PARTICLES_CONTAINER}}.grabParticle( p, coarseGridVertices{{PARTICLES_CONTAINER}}(i) );
        }
        else p++;
      } // while loop over coarse particles
    } // if coarse vertex local
  }
"""
    )

    _Template_Sieve = jinja2.Template(
        """
  if ( vertexdata::{{PARTICLES_CONTAINER}}::hasParticlesToBeSievedIntoVertices( vertexdata::{{PARTICLES_CONTAINER}}::SieveParticles::GlobalSort ) ) {
    auto sievedParticles = vertexdata::{{PARTICLES_CONTAINER}}::getParticlesToBeSievedIntoVertex(
      marker,
      vertexdata::{{PARTICLES_CONTAINER}}::SieveParticles::GlobalSort,
      _spacetreeId
    );

    for (auto& newParticle: sievedParticles) {
      if ( ::toolbox::particles::particleIsDuplicate(
        newParticle,
        fineGridVertex{{PARTICLES_CONTAINER}}.begin(),
        fineGridVertex{{PARTICLES_CONTAINER}}.end()
      ) != fineGridVertex{{PARTICLES_CONTAINER}}.end() ) {
        _numberOfSkippedDropsFromSieveSet++;
        delete newParticle;
      }
      else {
        fineGridVertex{{PARTICLES_CONTAINER}}.addParticle( newParticle );
        _numberOfDropsFromSieveSet++;
        toolbox::particles::assignmentchecks::assignParticleToVertex(
          "{{PARTICLE}}",
          newParticle->getX(),
          newParticle->getParallelState()==globaldata::{{PARTICLE}}::ParallelState::Local,
          marker.x(),
          marker.h(),
          _spacetreeId
        );
      }
    }

    assertion1( fineGridVertex{{PARTICLES_CONTAINER}}.isValid(), fineGridVertex{{PARTICLES_CONTAINER}}.toString() );
  }
"""
    )

    """!

  Lift all particles to next coarser level

  Explicit lift of particles to a coarser level. Usually, lifts are
  realised within touchCellLastTime aka the template
  __Template_ReassignAndLiftWithinCell. If a vertex is however destroyed or
  is hanging, i.e. not persistent in-between two mesh traversals,
  then we have to lift the particles explicitly.

  If the parent cell is local, we know that all @f$ 2^d @f$ parent
  vertices are local. However, we have to know if the real parent
  is local, too.

  """
    _Template_LiftAllParticles = jinja2.Template(
        """
  std::bitset<Dimensions> target;
  for (int d=0; d<Dimensions; d++) {
    target[d] = marker.getRelativePositionWithinFatherCell()(d)>=2;
  }

  if ( marker.isParentVertexLocal(target.to_ulong()) ) {
    assertion2( coarseGridVertices{{PARTICLES_CONTAINER}}( target.to_ulong() ).isValid(), coarseGridVertices{{PARTICLES_CONTAINER}}( target.to_ulong() ).toString(), fineGridVertex{{PARTICLES_CONTAINER}}.toString() );
    assertion2( fineGridVertex{{PARTICLES_CONTAINER}}.isValid(),                          coarseGridVertices{{PARTICLES_CONTAINER}}( target.to_ulong() ).toString(), fineGridVertex{{PARTICLES_CONTAINER}}.toString() );
    _numberOfLifts+=fineGridVertex{{PARTICLES_CONTAINER}}.size();
    for (auto& p: fineGridVertex{{PARTICLES_CONTAINER}}) {
      p->setCellH(3.0 * marker.h());
    }
    for (auto* p: fineGridVertex{{PARTICLES_CONTAINER}}) {
      toolbox::particles::assignmentchecks::removeParticleFromVertex( "{{PARTICLE}}", p->getX(), p->getParallelState()==globaldata::{{PARTICLE}}::ParallelState::Local, marker.x(), marker.h(), _spacetreeId );
      toolbox::particles::assignmentchecks::assignParticleToVertex(
        "{{PARTICLE}}",
        p->getX(),
        p->getParallelState()==globaldata::{{PARTICLE}}::ParallelState::Local,
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
        3.0*marker.h(), _spacetreeId
      );
    }
    coarseGridVertices{{PARTICLES_CONTAINER}}( target.to_ulong() ).grabParticles( fineGridVertex{{PARTICLES_CONTAINER}} );
  }
  else {
    _numberOfLiftsIntoSieveSet += fineGridVertex{{PARTICLES_CONTAINER}}.size();
    for (auto* p: fineGridVertex{{PARTICLES_CONTAINER}}) {
      toolbox::particles::assignmentchecks::assignParticleToVertex( "{{PARTICLE}}", p->getX(), p->getParallelState()==globaldata::{{PARTICLE}}::ParallelState::Local, marker.x(), marker.h(), _spacetreeId );
    }
    fineGridVertex{{PARTICLES_CONTAINER}}.particlesCanNotBeLiftedLocally();
    assertion1( fineGridVertex{{PARTICLES_CONTAINER}}.isValid(), fineGridVertex{{PARTICLES_CONTAINER}}.toString() );
  }
"""
    )

    def get_body_of_getGridControlEvents(self):
        return "  return std::vector< peano4::grid::GridControlEvent >();\n"

    def user_should_modify_template(self):
        return False

    def get_includes(self):
        result = jinja2.Template(
            """
#include "tarch/multicore/Lock.h"
#include "toolbox/particles/MultiscaleTransitions.h"
#include "toolbox/particles/assignmentchecks/TracingAPI.h"
#include "vertexdata/{{PARTICLES_CONTAINER}}.h"
#include "globaldata/{{PARTICLE}}.h"
"""
        )
        return result.render(**self.d)

    def get_constructor_body(self):
        return (
            super(AbstractUpdateParticleGridAssociation, self).get_constructor_body()
            + """
            _spacetreeId              = treeNumber;
          """
        )

    def get_attributes(self):
        return jinja2.Template(
            """
  int  _spacetreeId;
  int  _numberOfParticleReassignmentsOnCurrentLevel;
  int  _numberOfLifts;
  int  _numberOfDrops;
  int  _numberOfLiftsIntoSieveSet;
  int  _numberOfDropsFromSieveSet;
  int  _numberOfSkippedDropsFromSieveSet;
"""
        ).render(**self.d)

    def get_body_of_unprepareTraversal(self):
        return """
toolbox::particles::assignmentchecks::endMeshSweep();
"""
