# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import jinja2





def perserveCartesianTracerLayout(solver,particle_spacing):
  """
  
    This constraint is used in combination particle-particle interaction
    for tracers. A typical invocation resembles
        
project.add_action_set_to_timestepping( peano4.toolbox.particles.ParticleParticleInteraction(
  particle_set = tracer_particles,
  cell_compute_kernel = exahype2.tracer.perserveCartesianTracerLayout(thesolver,particle_spacing),
  additional_includes = "#include "toolbox/particles/potentials/Springs.h"
))

  """
  return """
//Run over local set
for (auto& p: localParticles) {
  if ( marker.isContained( p->getX() ) ) {
    tarch::la::Vector<Dimensions,double> updatedX = p->getX();
    for (auto& pp: activeParticles) {
      if (p->getNumber()!=pp->getNumber()) {
        updatedX += repositories::""" + solver.get_name_of_global_instance() + """.getMaxTimeStepSize()
                  * toolbox::particles::potentials::springPotential(p->getX(), pp->getX(), """ + str(particle_spacing) + """ );
      }
    }
    p->setX(updatedX);
  }
}
"""
  