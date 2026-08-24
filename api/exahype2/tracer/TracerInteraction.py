# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org


def preserveCartesianTracerLayout(solver, particle_spacing):
    """Create a cell kernel that relaxes tracers towards Cartesian spacing.

    The kernel uses the time step of the owning finite-volume cell and is
    intended for a particle-particle interaction action set.
    """
    return """
const auto timeStepSize = fineGridCell{cell_label}.getTimeStepSize();
for (auto* const particle : localParticles) {{
  if (marker.isContained(particle->getX())) {{
    auto updatedPosition = particle->getX();
    for (const auto* const interactingParticle : activeParticles) {{
      if (particle->getNumber() != interactingParticle->getNumber()) {{
        updatedPosition += timeStepSize
          * toolbox::particles::potentials::springPotential(
            particle->getX(),
            interactingParticle->getX(),
            {particle_spacing}
          );
      }}
    }}
    particle->setX(updatedPosition);
  }}
}}
""".format(
        cell_label=solver._cell_label.name,
        particle_spacing=particle_spacing,
    )
