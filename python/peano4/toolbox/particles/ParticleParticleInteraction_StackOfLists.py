# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
from peano4.solversteps.ActionSet import ActionSet

import jinja2

import peano4.datamodel.DaStGen2

import dastgen2.attributes.Integer



class ParticleParticleInteraction_StackOfLists(ActionSet):
  """!

Tree walker to realise particle-particle interactions with particles held by different adjacent cells or on different mesh levels.

 This code snippet creates a tree walker, i.e. an action set that runs
 through the underlying spacetree top down. It builds up two sets: The
 active set and the local set. Both are of the same type if you stick
 to the default value of None for active_particle_set. But you can also
 let particles of different type interact by selecting a local particle
 set that is different to the active one. Users can inject code to modify
 the local set per cell and hence to alter particle states.


 ## Set definitions

 image:: dependency_sets.png

 Per tree node there are two different sets. We explain it by means of
 the light blue cell in the cartoon. Cells of this level hold particles
 whose search radius is stricly smaller or equal to the mesh size.

 The set of local particles of a cell are all of these particles whose
 centre is contained within the cell plus a halo of h/2. This is the
 dotted area around the cell. That is, the local set holds also particles
 that are not centred within the cell.

 We need the local set to cover an area that is bigger than the actual
 cell as we want to capture all particles of this level that might
 theoretically interact with the particles located within the cell. If
 you want to update particles of a cell, you should thus only update
 particles whose centre is within the cell: Particles belong the the
 local sets of up to 2^d surrounding cells, so otherwise might be updated
 multiple times.

 The yellow particle on the fine grid in the sketch above is so far away
 from the cell that it does not belong into the set of local particles of
 the highlighted cell.

 The set of active particles is the local set plus the active set from
 the father cell in the spacetree. This is a recursive cell. So the set
 of active particles holds all local particles plus all the particles of
 coarser levels which might overlap with a local particle whose centre is
 within the current cell.

 The dark blue particles in the sketch is an example of such a particle.
 The yellow particle on the coarser mesh level in constrast is not
 contained within the active set of the blue cell, as it is simply too
 far away - even on the coarser mesh where we again work with a "halo"
 around the coarse cell of h_{coarse}/2.


 ## Realisation

 This tree walker realises the two sets via two lists: Per cell, we
 append the local set to the active set and memorise on a stack how
 many of these particles have been added. When we backtrace, i.e.
 ascend within the tree again, this memorised size is used to remove
 the last n elements from the active set.

 So the active set grows while we descend in the tree, and it is
 truncated when we ascend again.

 This is a very efficient implementation yet fails if particles move
 up and down within the tree while we walk. It should work perfectly
 fine if particles do not move. If particles move up and down, the
 present algorithms might have redundant pointers within the active set.
 I am also not 100% sure if we might run into memory issues if particles
 suddenly leave cells and hence "disappear" from the active sets.

 In the case of a dynamic setup, i.e. in combinatino with the action
 set UpdateParticleGridAssociation, you might be better of with using
 the set-based implementation of the particle-particle interaction.


 ## Further plug-in points

 Besides the cell kernel which is there to realise particle-to-particle
 interactions, we also have a vertex kernel which we call whenever a
 vertex is loaded for the first time. That is, you can assume that the
 vertex kernel has been launched for all 2^d vertices of a cell before
 its cell kernel is triggered. The vertex kernel is also passed on the
 active set (from the coarser level). Its local set is all the particles
 whose centre lies within the square/cube around the vertex with mesh size
 h. So this area goes h/2 along each each coordinate axis into the
 neighbouring cells.


\section{Dual tree traversal and particle-particle interaction}
\label{section:pidt:particle-particle-interaction}


\begin{definition}
 In \Peano's \emph{dual tree} storage scheme, we always hold the
 complete spacetree with all tree (grid) levels. Particles are always stored on
 the finest mesh (tree) level where the mesh size is bigger than the particle's
 search radius.
 Different particles with different search radii might thus be stored on
 different tree levels.
\end{definition}



\noindent
\Peano\ always runs through the mesh top-down, i.e.~it traverses the spacetree
in a depth-first fashion.
With the definition above, we can realise particle-particle interactions as
follows:

Per cell, the code assembles a \texttt{local set}.
This is a list of pointers to those particles that are associated to a cell's
$2^d$ vertices.
With the local set, you can make particles held on one resolution level
interact with each other.
You get an cubic area of $2h$ side-lenght per cell:
As a particle is always associated to its closest vertex, the local set holds all particles whose search radius overlap with the cell.


Adaptivity and particles with different search radii are not yet captured through the
local set notion.
There's thus a second set that I call active set.
The active set is the local set plus all local sets that have been active on
coarser levels within the tree.

\begin{definition}
 The active set is built up top-down while we run through the
 tree.
 So it is a data structure that grows and shrinks while you run through the
 meshes.
 The local set \texttt{localParticles} is created on-the-fly every time you hit
 a cell or a vertex.
 Do not alter the active set: You can modify the values of the particles within
 an active set, but you should not add or remove entries from it.
 The local set can be modified.
\end{definition}


\noindent
If you use the \texttt{toolbox.particles.ParticleParticleInteraction} action
set, you pass it some C source code that works with the particles.
This C code has access to three variables: the active set, the local set and the
cell marker (of type \linebreak \texttt{peano4::datatraversal::CellMarker}).
Let's assume that each particle carries some velocity and that we could cut-off
gravity---which we clearly should not.


The straightforward particle-particle interaction then looks conceptionally as
follows:

\begin{code}
// Run over local set
for (auto& p: localParticles) {
  // Pick only those particles that reside within the current cell, otherwise
  // we would update each particle up to 2^d times. I do not exploit the
  // symmetry of forces here
  if ( marker.isContained( p->getX() ) ) {
    for (auto& pp: activeParticles) {
      // No interaction with particle itself
      if (p!=pp) {
        tarch::la::Vector<Dimensions,double> dist = pp->getX() - p->getX();
        [...]
      }
    }
  }
}
\end{code}


\noindent
Typical \Peano\ codes introduce some variables per particle into which they
accumulate data.
In \texttt{touchVertexFirstTime} we use a vertex for the very first time.
Consequently, we also read the particles associated to this vertex for the first
time and thus set the accumulation variable to the neutral element 0.
Within \texttt{touchCellFirstTime}, we now evaluate particle-particle
interactions and write data to the accumulated values.
In \texttt{touchVertexLastTime}, we finally have all data accumulated and hence
can update the particle state.


\begin{remark}
 Study the sort algorithms that we use if you
 update the vertex (cmp.~Section \ref{section:pidt:move-particles}). Once you
 alter a particle position, particles might not be tied to their closest vertex
 anymore.
 Neighbourhood searches thus might yield invalid results.
\end{remark}



"""


  def __init__(self,
               particle_set,
               particle_particle_interaction_kernel,
               touch_vertex_first_time_compute_particle_update_kernel=None,
               touch_vertex_last_time_compute_particle_update_kernel=None,
               prepare_traversal_kernel="",
               unprepare_traversal_kernel="",
               additional_includes="",
               active_particle_set=None ):
    """!

 Initialise object

 ## Arguments

 particle_set: ParticleSet

 particle_particle_interaction_kernel: String holding C++ code
   This C++ code can access three different types of variables: There's
   a list of particles called activeParticles, there's a list of particles
   called localParticles, and there's the cell marker. See the guidebook
   for further info. A typical kernel resembles

       for (auto* localParticle: localParticles )
       for (auto* activeParticle: activeParticles ) {
         localParticle->doSomethingFancy( activeParticle );
       }

 active_particle_set: ParticleSet or None
   You can compare different particles species with this argument. It
   allows you to make the active particles stem from another species than
   the local ones that you actually update. Pick None if both sets are of
   the same type.

 touch_vertex_first_time_compute_particle_update_kernel: String or None
   Can be empty, but if you wanna move particles, then a minimal example
   string passed equals

       for (auto* localParticle: localParticles ) {
         localParticle->setMoveState( globaldata::Particle::MoveState::NotMovedYet );
       }

   i.e. you will have a variable localParticle available in this kernel
   and it is a pointer.

 use_biased_locality_check: Boolean
   When we hit a cell, we have to decide what defines the active set within
   this cell. For this, we can either use peano4::datamanagement::CellMarker::isContained()
   or peano4::datamanagement::CellMarker::isContainedWithBias(). The the
   documentation of these routines for further details.



    """
    super(ParticleParticleInteraction_StackOfLists,self).__init__(priority=1,parallel=False)

    self._particle_set = particle_set
    self.d = {}
    self.d[ "LOCAL_PARTICLE" ]                    = particle_set.particle_model.name
    self.d[ "LOCAL_PARTICLES_CONTAINER" ]         = particle_set.name
    if active_particle_set==None:
      self.d[ "ACTIVE_PARTICLE" ]                 = particle_set.particle_model.name
      self.d[ "ACTIVE_PARTICLES_CONTAINER" ]      = particle_set.name
    else:
      self.d[ "ACTIVE_PARTICLE" ]                 = active_particle_set.particle_model.name
      self.d[ "ACTIVE_PARTICLES_CONTAINER" ]      = active_particle_set.name
    self.d[ "PARTICLE_PARTICLE_INTERACTION_KERNEL" ]               = particle_particle_interaction_kernel
    self.d[ "TOUCH_VERTEX_FIRST_COMPUTE_KERNEL" ] = touch_vertex_first_time_compute_particle_update_kernel
    self.d[ "TOUCH_VERTEX_LAST_COMPUTE_KERNEL" ]  = touch_vertex_last_time_compute_particle_update_kernel
    self.d[ "ADDITIONAL_INCLUDES" ]               = additional_includes
    self.d[ "PREPARE_TRAVERSAL_KERNEL" ]          = prepare_traversal_kernel
    self.d[ "UNPREPARE_TRAVERSAL_KERNEL" ]        = unprepare_traversal_kernel


  __Template_TouchVertexFirstTime = jinja2.Template("""
  {% if TOUCH_VERTEX_FIRST_COMPUTE_KERNEL!=None %}
  auto& localParticles  = fineGridVertex{{LOCAL_PARTICLES_CONTAINER}};
  auto& activeParticles = fineGridVertex{{ACTIVE_PARTICLES_CONTAINER}};

  {{TOUCH_VERTEX_FIRST_COMPUTE_KERNEL}}
  {% endif %}
""")


  __Template_TouchVertexLastTime = jinja2.Template("""
  {% if TOUCH_VERTEX_LAST_COMPUTE_KERNEL!=None %}
  auto& localParticles  = fineGridVertex{{LOCAL_PARTICLES_CONTAINER}};
  auto& activeParticles = fineGridVertex{{ACTIVE_PARTICLES_CONTAINER}};

  {{TOUCH_VERTEX_LAST_COMPUTE_KERNEL}}
  {% endif %}
""")


  __Template_TouchCellFirstTime = jinja2.Template("""
  std::list< globaldata::{{LOCAL_PARTICLE}}* >  localParticles;
  _numberOfActiveParticlesAdded.push_back(0);
  for (int i=0; i<TwoPowerD; i++) {
    for (auto* p: fineGridVertices{{ACTIVE_PARTICLES_CONTAINER}}(i) ) {
      _activeParticles.push_front( p );
      _numberOfActiveParticlesAdded.back()++;
    }
    for (auto* p: fineGridVertices{{LOCAL_PARTICLES_CONTAINER}}(i) ) {
      bool append = marker.isContained( p->getX() );
      if (append) {
        localParticles.push_front( p );
      }
    }
  }

  {% if PARTICLE_CELL_UPDATE_KERNEL!=None %}
  std::list< globaldata::{{ACTIVE_PARTICLE}}* >&  activeParticles = _activeParticles;
  {{PARTICLE_PARTICLE_INTERACTION_KERNEL}}
  {% endif %}
""")


  __Template_TouchCellLastTime = jinja2.Template("""
  for (int i=0; i<_numberOfActiveParticlesAdded.back(); i++) {
    _activeParticles.pop_back();
  }
  _numberOfActiveParticlesAdded.pop_back();
""")


  __Template_EndTraversal = jinja2.Template("""
  assertion1( _activeParticles.empty(), (*_activeParticles.begin())->toString() );
""")


  def get_body_of_operation(self,operation_name):
    result = "\n"
    #if operation_name==ActionSet.OPERATION_BEGIN_TRAVERSAL:
    #  result = self.__Template_BeginTraversal.render(**self.d)
    if operation_name==ActionSet.OPERATION_TOUCH_CELL_FIRST_TIME:
      result = self.__Template_TouchCellFirstTime.render(**self.d)
    if operation_name==ActionSet.OPERATION_TOUCH_CELL_LAST_TIME:
      result = self.__Template_TouchCellLastTime.render(**self.d)
    if operation_name==ActionSet.OPERATION_TOUCH_VERTEX_FIRST_TIME:
      result = self.__Template_TouchVertexFirstTime.render(**self.d)
    if operation_name==ActionSet.OPERATION_TOUCH_VERTEX_LAST_TIME:
      result = self.__Template_TouchVertexLastTime.render(**self.d)
    if operation_name==ActionSet.OPERATION_END_TRAVERSAL:
      result = self.__Template_EndTraversal.render(**self.d)
    return result


  def get_body_of_getGridControlEvents(self):
    return "  return std::vector< peano4::grid::GridControlEvent >();\n"


  def get_body_of_prepareTraversal(self):
    return self.d[ "PREPARE_TRAVERSAL_KERNEL" ]


  def get_body_of_unprepareTraversal(self):
    return self.d[ "UNPREPARE_TRAVERSAL_KERNEL" ]


  def get_action_set_name(self):
    return __name__.replace(".py", "").replace(".", "_")


  def user_should_modify_template(self):
    return False


  def get_includes(self):
    result = jinja2.Template( """
#include "tarch/multicore/Lock.h"
#include "toolbox/particles/particles.h"
#include "vertexdata/{{LOCAL_PARTICLES_CONTAINER}}.h"
#include "globaldata/{{LOCAL_PARTICLE}}.h"
#include "vertexdata/{{ACTIVE_PARTICLES_CONTAINER}}.h"
#include "globaldata/{{ACTIVE_PARTICLE}}.h"

{{ADDITIONAL_INCLUDES}}

#include <list>
#include <vector>
""" )
    return result.render(**self.d)


  def get_attributes(self):
    result = jinja2.Template( """
  std::list< globaldata::{{ACTIVE_PARTICLE}}* >  _activeParticles;
  std::vector< int >                             _numberOfActiveParticlesAdded;
""")
    return result.render(**self.d)
