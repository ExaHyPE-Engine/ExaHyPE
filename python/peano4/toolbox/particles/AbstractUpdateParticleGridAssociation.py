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
class documentation.


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
  searchersmight yield invalid results.
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
priority value thus should be very small. As priorities are inverted when we 
walk from fine to coarse resolutions, this means that all dropping into the mesh
(in the second grid sweep) happens before anybody else touches the particles,
while the extraction of particles or moves from fine to coarse are epilogues to
any operation in the primary mesh sweep.

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
  _numberOfLiftsIntoSieveSet = 0;
  _numberOfDropsFromSieveSet = 0;
"""
    )

    _Template_EndTraversal = jinja2.Template(
        """
  vertexdata::{{PARTICLES_CONTAINER}}::updateLiftDropStatistics(
    _numberOfLifts,
    _numberOfDrops,
    _numberOfParticleReassignmentsOnCurrentLevel,
    _numberOfLiftsIntoSieveSet,
    _numberOfDropsFromSieveSet
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
          assertion3( 
            fineGridVertex{{PARTICLES_CONTAINER}}.isValid(),        
            fineGridVertex{{PARTICLES_CONTAINER}}.toString(), 
            coarseGridVertices{{PARTICLES_CONTAINER}}(i).toString(),
            _treeNumber 
          );
          assertion3( 
            coarseGridVertices{{PARTICLES_CONTAINER}}(i).isValid(), 
            fineGridVertex{{PARTICLES_CONTAINER}}.toString(), 
            coarseGridVertices{{PARTICLES_CONTAINER}}(i).toString(),
            _treeNumber 
          );
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
  if ( vertexdata::{{PARTICLES_CONTAINER}}::hasParticlesToBeSievedIntoVertices() ) {
    auto sievedParticles = vertexdata::{{PARTICLES_CONTAINER}}::getParticlesToBeSievedIntoVertex( marker );

    _numberOfDropsFromSieveSet += sievedParticles.size();

    fineGridVertex{{PARTICLES_CONTAINER}}.addParticles( sievedParticles );
    fineGridVertex{{PARTICLES_CONTAINER}}.eliminateSpatialDuplicates();
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
    coarseGridVertices{{PARTICLES_CONTAINER}}( target.to_ulong() ).grabParticles( fineGridVertex{{PARTICLES_CONTAINER}} );
  }
  else {
    _numberOfLiftsIntoSieveSet += fineGridVertex{{PARTICLES_CONTAINER}}.size();
    fineGridVertex{{PARTICLES_CONTAINER}}.particlesCanNotBeLiftedLocally();
    assertion1( fineGridVertex{{PARTICLES_CONTAINER}}.isValid(), fineGridVertex{{PARTICLES_CONTAINER}}.toString() );
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
  int  _numberOfLiftsIntoSieveSet;
  int  _numberOfDropsFromSieveSet;
"""
        ).render(**self.d)
