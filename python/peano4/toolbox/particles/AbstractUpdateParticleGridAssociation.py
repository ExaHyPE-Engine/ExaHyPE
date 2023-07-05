# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
from peano4.solversteps.ActionSet import ActionSet


import jinja2


class AbstractUpdateParticleGridAssociation(ActionSet):
    """!

    Associate particles to spacetree vertices

    Use one of the subclasses which realise various sorting algorithms.
    All subclasses realise the particle in dual tree storage scheme.


    Particles in our ***dual tree storage scheme*** are stored within the
    spacetree and associated to vertices:

    - A particle resides on a mesh level within the tree such that the
      search radius @f$ r \leq h@f$ if h is the resolution of this level.
    - A particle is associated to the vertex of this level that's closest to
      its centre x.

    These two conventions effectively span a dual grid over the spacetree if you
    think of particles to be stored within cells:
    Just impose a grid on top of your actual grid which is shifted by half a mesh
    lenght.
    Particles are contained within these shifted grid's cells.
    Alternatively, you can think of the vertices of the tree spanning a Voronoi
    diagram, and particles are then held by the cells of this Voronoi diagram.

      Our storage is vertex-oriented whereas most particle literature stores
      particles within cells.
      If we think of a dual multiscale mesh, i.e. a mesh that is shifed by @f$h/2@f$
      alon each coordinate axis, then Peano's approach also works with cells.
      Such shifted meshes are often called ***dual*** meshes and I therefore speak
      of a ***dual tree***.


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

             myParticle->setX( {17.3, 8.2} );

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
    i.e.~its grid-vertex association will be updated:
    The action set UpdateParticleGridAssociation updates the
    particle-tree association, i.e.~sorts the particles, on-the-fly between two mesh
    traversals.


    While you do not have to worry about how this sorting is realised, it is
    important to study the action set's documentation (just open the Python code)
    and read when data is consistent.
    Here is the general invariant however:

    - The very moment you alter a particle's position, you cannot be sure that
      it is still associated with the nearest vertex. Consequently, neighbourhood
      searchersmight yield invalid results.
    - In the grid sweep after the position change, the particle will be
      associated with the correct vertex (unless you change its position once again
      obviously).


    The action set UpdateParticleGridAssociation relies on some global
    data. Therefore, you have to call finishTraversal() on the used
    particle set in your main routine on each rank after all rank-local traversals have termined.


    Most codes move a particle in touchVertexLastTime().
    This is convenient as we know that any particle associated with the respective
    vertex also has been touched for the last time.
    We can safely update it.
    However, if we change a particle's position and merge
    UpdateParticleGridAssociation into the respective algorithmic step,
    then the particle might immediately be associated to another vertex after it has
    been updated and this vertex might later on become subject of touchVertexLastTime().


    Consequently, it can happen that a particle is updated multiple times.
    To avoid this, I recommend that you add a boolean to each vertex:

            particle.data.add_attribute( dastgen2.attributes.Enumeration("MoveState",["New","NotMovedYet","Moved"]) )


    You can then set the value of MoveState to NotMovedYet in
    touchVertexFirstTime(), set it to ```Moved``` once we update the
    position, but only update its position as long as it has not been moved yet.
    Peano's built-in re-assignment of positions does only move particles around
    when vertices are touched for the last time, so there's no risk here that you
    update a particle in touchVertexLastTime(), assign it to a particle which
    has not been used yet and then reset it wrongly to not moved again.


    It is absolutely key that you never alter the particle positions throughout the
    traversal. The only time you can set a new position is within
    touchVertexLastTime(). This Peano's particle toolbox the time to
     re-sort the particles in-between two traversals.
    If you use UpdateParticleGridAssocation, you typically combine it with
    peano4.toolbox.particles.UpdateParallelState.
    UpdateParticleGridAssocation should be the very first action set of a
    traversal, and UpdateParallelState should follow immediatly after.


    If you move around particles, it is important that you merge the two update
    action sets into the action set where you change the particle position (in the
    last access of a vertex) plus into the subsequent grid sweep:
    Once the particle position has changed, Peano tries to associate the particle
    again to the right vertex.
    If the associativity however has changed, it might (temporarily) move particles
    to a coarser level.
    It is the subsequent grid sweep that then sorts the particles again into the
    right resolution of the mesh plus associates it with the right vertex.
    So it needs access to \texttt{UpdateParticleGridAssocation}.


    A similar argument holds if you parallelise:
    If a particle leaves a domain, the owning tree sends it out by the end of the
    mesh sweep.
    The particle is now in transition.
    It is the subsequent grid sweep which works it into the target tree and updates
    its parallel state.


    ## Tunneling

    Tunneling means that a particle moves more than one mesh size per time
     step.
     In the context of Peano's multiscale tree structure, this is equivalent to
     particles which travel more than their search radius per step.


    Tunneling arises if search radii change suddenly, and if meshes are extremely
    refined where they should likely not refine.
    Peano does support tunneling in principle, but you can tell the update script
    if you want to get a warning if particles tunnel - in some applications, this is
    unphysical.


    """

    DefaultPriority = -65536

    def __init__(self, particle_set, guard="true"):
        super(AbstractUpdateParticleGridAssociation, self).__init__(
            priority=-65536, parallel=False
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
  _numberOfSieves = 0;
"""
    )

    _Template_EndTraversal = jinja2.Template(
        """
  vertexdata::{{PARTICLES_CONTAINER}}::updateLiftDropStatistics(
    _numberOfLifts,
    _numberOfDrops,
    _numberOfParticleReassignmentsOnCurrentLevel,
    _numberOfSieves
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

    """

   Happens when we touch a vertex for the first time. The only thing this routine
   does is to drop particles within the tree, i.e. move them to finer levels.
   No parallelisation is taken into account.

  """
    _Template_Drop = jinja2.Template(
        """
  for (int i=0; i<TwoPowerD; i++) {
    // Define inside for loop, as sieve will define it as well
    vertexdata::{{PARTICLES_CONTAINER}}::iterator p = coarseGridVertices{{PARTICLES_CONTAINER}}(i).begin();
    while ( p!=coarseGridVertices{{PARTICLES_CONTAINER}}(i).end() ) {
      if ( ::toolbox::particles::dropParticle(**p,marker) ) {
        p = fineGridVertex{{PARTICLES_CONTAINER}}.move( p, coarseGridVertices{{PARTICLES_CONTAINER}}(i) );
        _numberOfDrops++;
      }
      else p++;
    }
  }
"""
    )

    _Template_Sieve = jinja2.Template(
        """
  if ( vertexdata::{{PARTICLES_CONTAINER}}::hasParticlesToBeSievedIntoVertices() ) {
    auto sievedParticles = vertexdata::{{PARTICLES_CONTAINER}}::getParticlesToBeSievedIntoVertex( marker );

    _numberOfSieves += sievedParticles.size();

    fineGridVertex{{PARTICLES_CONTAINER}}.add( sievedParticles );
    fineGridVertex{{PARTICLES_CONTAINER}}.eliminateSpatialDuplicates();
  }
"""
    )

    """

  Explicit lift of particles to a coarser level. Usually, lifts are
  realised within touchCellLastTime aka the template
  __Template_ReassignAndLiftWithinCell. If a vertex is however destroyed or
  is hanging, i.e. not persistent in-between two mesh traversals,
  then we have to lift the particles explicitly.

  """
    _Template_LiftAllParticles = jinja2.Template(
        """
  if ( marker.isParentCellLocal() ) {
    std::bitset<Dimensions> target;
    for (int d=0; d<Dimensions; d++) {
      target[d] = marker.getRelativePositionWithinFatherCell()(d)>=2;
    }
    _numberOfLifts+=fineGridVertex{{PARTICLES_CONTAINER}}.size();
    coarseGridVertices{{PARTICLES_CONTAINER}}( target.to_ulong() ).move( fineGridVertex{{PARTICLES_CONTAINER}} );
  }
  else {
    fineGridVertex{{PARTICLES_CONTAINER}}.particlesCanNotBeLiftedLocally();
  }
"""
    )

    def get_constructor_body(self):
        return """
  _treeNumber  = treeNumber;
"""

    def get_body_of_getGridControlEvents(self):
        return "  return std::vector< peano4::grid::GridControlEvent >();\n"

    def user_should_modify_template(self):
        return False

    def get_includes(self):
        result = jinja2.Template(
            """
#include "tarch/multicore/Lock.h"
#include "toolbox/particles/MultiscaleTransitions.h"
#include "vertexdata/{{PARTICLES_CONTAINER}}.h"
#include "globaldata/{{PARTICLE}}.h"
"""
        )
        return result.render(**self.d)

    def get_attributes(self):
        return jinja2.Template(
            """
  int  _treeNumber;
  int  _numberOfParticleReassignmentsOnCurrentLevel;
  int  _numberOfLifts;
  int  _numberOfDrops;
  int  _numberOfSieves;
"""
        ).render(**self.d)

    def get_body_of_prepareTraversal(self):
        template = jinja2.Template(
            """
  vertexdata::{{PARTICLES_CONTAINER}}::clearReassignmentStatistics();
"""
        )
        return template.render(**self.d)
