# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import jinja2



"""

  AMR Logic
  
  The AMR logic is based upon the observation that Peano's toolbox always
  holds the particles in the right place and hence allows us to realise
  all particle-particle interactions. If we don't get the mesh right, we 
  might not be optimal in terms of cost. But we are never wrong!
  
  ## Naive workflow 
  
  In principle, SPH's AMR logic is pretty simple in the context of a the dual
  tree with its lifts and drops. The illustration below shows this trivial 
  workflow on the left side: 

  @image html AMR-logic.png
  
  The image is the time axis and the bullets are the individual mesh 
  traversals. On the left, we see the user input which eventually triggers a 
  cascade of actions on the bookkeeping side (right). The user alters the 
  particle position and hence might invalidate the particle-grid association.
  We have to re-sort. Some sorting can happen right in the same mesh traversal
  in which we alter the particles' properties in the touchCellLastTime or
  touchVertexLastTime actions. The green colour denotes stuff that's happening
  in the last data access - which might also introduce some MPI sends.
  
  In the subsequent mesh traversal, we have to receive MPI data, move particles
  down in their tree and associate that to the correct vertex. This happens 
  prior to any operation (denoted blue). Now that particles are in the right 
  place, we can update their parallel state.
  
  A straightforward implementation now might evaluate the AMR criterion, as 
  all particles are in place and we can do so. Once we have collected all AMR
  info, we can distribute it globally and hand it out in the subsequent mesh
  traversal in which we consequently trigger refinements. Peano does not
  realise refinements immediately, but waits for the next mesh traversal to 
  do so. 
  
  WHen we have done the refinement, we have to resort. Again, the resort has
  to spread over two mesh traversals, as the first half of the sorting might
  trigger MPI exchanges and multilevel traversals.
  
  In this native data flow scheme, the user action affects the behaviour of
  five follow-up mesh traversals.
  
  
  ## Optimised data workflow
  
  The optimised workflow accepts that we we are fine to deal with a suboptimal 
  mesh layout, as long as we minimise the number of particle movements. This
  is a different cost objective than in the description above. We realise that
  we will always need a resort when we have updated a mesh.
  
  The idea is that we know that a user interaction forces us to update the 
  particle-mesh association, while AMR does not: We have to lift particles
  from areas of the tree which are coarsened, but otherwise AMR does not 
  invalidate a valid mesh association. Particles might reside on too coarse
  levels, but still all the kernel evaluations remain correct. The lift is the
  only thing that causes issues, as it might mean that particles literally 
  travel.
  
  To avoid that we have to do two sorts per particle position update, we 
  evaluate the AMR criterion two grid sweeps ahead of the actual particle
  position update. This way, the AMR realisation and the actual position 
  update coincide in the same mesh sweep. No need for an additional sorting
  step. 
  
  The disadvantage of this scheme is that the mesh always lags behind. The 
  AMR just is about the realise when the particles move again. So it might 
  have been optimal before, but now it is not good anymore. In return, we 
  however get a scheme which does, amortised, need only one resorting step
  per mesh update plus AMR update.


"""

Ist das Refine-Event static, d.h. wirklich nur einmal da?
Oder sind das verschiedene Klassen?
Sollten es nicht sein.

Tut das delete wirklich liften?
