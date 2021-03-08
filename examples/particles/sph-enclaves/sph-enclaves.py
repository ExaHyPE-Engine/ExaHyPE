# To add a new cell, type '# %%'
# To add a new markdown cell, type '# %% [markdown]'
# %%
from IPython import get_ipython

# %% [markdown]
# # SPH with Enclave Tasks
#
# Enclave tasking with particles, demonstrated by simulating a 2D Sedov blast using SPH.
#
# As each timestep is already split into two traversals for SPH (the density calculation followed by the acceleration calculation), we can easily turn this into an enclave tasking scheme:
#
# - In the primary traversal, we spawn a task for each cell which will calculate the density for each particle within the cell - when such a task completes, it lets the vertices of its cell know.
#
# - In the secondary traversal, at each cell we must have that all particles in the corresponding ghost/dual cell must have had their densities calculated. We do this by checking the vertices of the cell to see whether all cells adjacent to each vertex have finished their density tasks. If yes, we calculate the particle accelerations. If no, we yield.
# %% [markdown]
# Create a project and configure it to end up in a subnamespace (and thus subdirectory).

# %%
import os

import peano4
import peano4.dastgen2
import peano4.toolbox
import peano4.toolbox.particles
import dastgen2

from peano4.solversteps.ActionSet import ActionSet


# %%
project = peano4.Project(["peanosph", "enclavesph"], "sphEnclaves", ".")

# %% [markdown]
# ## Prepare code
#
# Peano's API does not know which settings to use on the present system. To make it copy/clone the settings identified by ./configure, we ask it to parse the generated configuration scripts.
#

# %%
project.output.makefile.parse_configure_script_outcome("../../..")

# %% [markdown]
# ## Model the particle
#
# We create the particle through the toolbox. A particle by definition has a position, so we just need to add everything else for SPH:
# - Velocity array
# - Acceleration array
# - Mass
# - Density
# - Pressure
# - Internal Energy (U)
# - Time derivative of internal energy (U')
# - Kernel smoothing length
#
# We add a marker to indicate the next calculation step required for a particle so we don't perform the same step on a particle twice in a row.
#
# We also add an array to store the the new position of a particle after a timestep is completed. We will use this to move the particle at the start of the subsequent timestep.

# %%
particle = peano4.toolbox.particles.Particle("Particle")

v_attr = peano4.dastgen2.Peano4DoubleArray("v","Dimensions")
a_attr = peano4.dastgen2.Peano4DoubleArray("a","Dimensions")
mass_attr = dastgen2.attributes.Double("mass")
density_attr = dastgen2.attributes.Double("density")
pressure_attr = dastgen2.attributes.Double("pressure")
u_attr = dastgen2.attributes.Double("u")
uDash_attr = dastgen2.attributes.Double("uDash")
smoothingLength_attr = dastgen2.attributes.Double("smoothingLength")
step_attr = dastgen2.attributes.Enumeration("step", ["calcDensity", "calcAccel", "move"])
newX_attr = peano4.dastgen2.Peano4DoubleArray("newX", "Dimensions")

particle.data.add_attribute(v_attr)
particle.data.add_attribute(a_attr)
particle.data.add_attribute(mass_attr)
particle.data.add_attribute(density_attr)
particle.data.add_attribute(pressure_attr)
particle.data.add_attribute(u_attr)
particle.data.add_attribute(uDash_attr)
particle.data.add_attribute(smoothingLength_attr)
particle.data.add_attribute(step_attr)
particle.data.add_attribute(newX_attr)

particles = peano4.toolbox.particles.ParticleSet(particle)

# %% [markdown]
# We create the plotter for the particle, providing it with the attributes we want to plot per particle.

# %%
particle_plotter = peano4.toolbox.particles.PlotParticlesInVTKFormat( "particles-enclave", particles )
particle_plotter.add_attribute_to_plot(density_attr, 1)
particle_plotter.add_attribute_to_plot(pressure_attr, 1)
particle_plotter.add_attribute_to_plot(u_attr, 1)

# %% [markdown]
# This particle has to be added to the project. This way the project knows that we are working with this guy. We associate the particle with the vertices, as we work with the pidt (particle in dual tree) algorithm.

# %%
project.datamodel.add_global_object(particle)
project.datamodel.add_vertex(particles)

# %% [markdown]
# ## Task Counter
# I have implemented Enclave Tasks inspired by the ExaHyPe2 version which uses a Task class and a Bookkeeping class.
#
# For particles, the functor of an Enclave task should take a pointer to the local and active particle sets of its corresponding cell as well as the vertex task counters.

# %%
task_counter = peano4.toolbox.particles.VertexTaskCounter("VertexTaskCounter")

project.datamodel.add_vertex(task_counter)

# %% [markdown]
# **IT'S VITAL THAT AFTER GENERATING THE FILES WE EDIT `vertexdata/TaskCounter.h` SO THAT `_numTasksRemaining` IS ZERO INITIALIZED BY DEFAULT.**
# %% [markdown]
# ## Export global constants
#
# There are plenty of constant simulation paramaters to specify for our SPH simulation - we do that here.

# %%
from numpy import pi

GAMMA = str(5/3)
DYNAMIC_VISCOSITY = "0.00001"
CUBIC_SPLINE_SIGMA = str(10/(7*pi))
TARGET_PARTICLE_SIZE = "0.01"
ENERGY_SPIKE = "0.1"
SPIKE_RADIUS = "0.08"
REST_DENSITY = "1"
INITIAL_H = "0.02"

DOMAIN_OFFSET = "{0.0, 0.0}"
DOMAIN_SIZE = "{1.0, 1.0}"
EXPLOSION_CENTRE = "{0.5, 0.5}"

project.constants.export_const_with_type("GAMMA", GAMMA, "double")
project.constants.export_const_with_type("DYNAMIC_VISCOSITY", DYNAMIC_VISCOSITY, "double")
project.constants.export_const_with_type("CUBIC_SPLINE_SIGMA", CUBIC_SPLINE_SIGMA, "double")
project.constants.export_const_with_type("TARGET_PARTICLE_SIZE", TARGET_PARTICLE_SIZE, "double")
project.constants.export_const_with_type("ENERGY_SPIKE", ENERGY_SPIKE, "double")
project.constants.export_const_with_type("SPIKE_RADIUS", SPIKE_RADIUS, "double")
project.constants.export_const_with_type("REST_DENSITY", REST_DENSITY, "double")
project.constants.export_const_with_type("INITIAL_H", INITIAL_H, "double")

project.constants.export_const_with_type("DOMAIN_OFFSET", DOMAIN_OFFSET, "auto")
project.constants.export_const_with_type("DOMAIN_SIZE", DOMAIN_SIZE, "auto")
project.constants.export_const_with_type("EXPLOSION_CENTRE", EXPLOSION_CENTRE, "auto")

# %% [markdown]
# ## Model the algorithmic steps we rely on
#
# When we model the algorithmic steps we rely on, we use a number of pre-defined action sets from the particle toolbox. First and foremost, we use an analysed tree grammar, i.e. some marking mechanism for the tree nodes. This helps us to guide the AMR - which is the second ingredient that we use. Finally we use the plotting routines for particles from the toolbox.

# %%
particle_tree_analysis = peano4.toolbox.particles.ParticleTreeAnalysis(particles)
project.datamodel.add_cell(particle_tree_analysis.cell_marker)   # read docu of ParticleTreeAnalysis

# %% [markdown]
# ### Grid/experiment setup
#
# The first thing we will do is to create a set of particles and add them to the grid.

# %%
create_grid = peano4.solversteps.Step("CreateGrid")
create_grid.use_vertex(particles)
create_grid.use_vertex(task_counter)
create_grid.use_cell(particle_tree_analysis.cell_marker)
create_grid.add_action_set(peano4.toolbox.particles.UpdateParticleGridAssociation(particles))
create_grid.add_action_set(peano4.toolbox.particles.ParticleAMR(particles,particle_tree_analysis))
create_grid.add_action_set(peano4.toolbox.CreateRegularGrid(0.2))
create_grid.add_action_set(particle_tree_analysis)
project.solversteps.add_step(create_grid)

# %% [markdown]
# ## Calculate densities at particles
#
# In the first phase of a timestep we move particles to their new positions calculated in the previous timestep.
#
# For each particle we also calculate the fluid density at its position based on the positions and masses of the particles in its local neighbourhood.
#
# From the density and the internal energy we calculate the fluid pressure at the particle's position.
#
# We **don't** do the particle AMR in this primary traversal since we need the grid to stay the way it is for the secondary traversal.

# %%
calc_densities = peano4.solversteps.Step("PrimaryTraversal")
calc_densities.use_vertex(particles)
calc_densities.use_vertex(task_counter)
calc_densities.use_cell(particle_tree_analysis.cell_marker)
calc_densities.add_action_set(peano4.toolbox.particles.UpdateParticleGridAssociation(particles))
#calc_densities.add_action_set(peano4.toolbox.particles.ParticleAMR(particles,particle_tree_analysis))
calc_densities.add_action_set(particle_tree_analysis)

cell_compute_kernel = f"""
const int numLocalParticles = std::distance(localParticles.begin(), localParticles.end());
if (numLocalParticles > 0) {{
  std::cout << "[ppi::PrimaryTraversal::EnterCell] cell centre: " << marker.x()(0) << ", " << marker.x()(1) << '\\n';
  for (int i=0; i<TwoPowerD; i++) {{
    peanosph::enclavesph::vertexdata::TaskCounter& tc = fineGridVerticesTaskCounter(i);
    std::cout << "[ppi::PrimaryTraversal::EnterCell] taskcounter " << i << " = " << tc.getNumTasksRemaining() << '\\n';
    tc.setNumTasksRemaining(tc.getNumTasksRemaining() + 1);
  }}
  std::cout << std::endl;
}}

//Run over local set
for (auto& p: localParticles) {{
  // Pick only those particles that reside within the current cell, otherwise
  // we would update each particle up to 2^d times. I do not exploit the symmetry
  // of forces here
  if ( marker.isContained( p->getNewX() ) ) {{
    if (p->getStep() == peanosph::enclavesph::globaldata::Particle::Step::calcDensity) {{

      const double h = p->getSmoothingLength();
      double densitySum = 0.0;
      // int numNbrs = 0;
      for (auto& pp: activeParticles) {{
        const double dist = tarch::la::norm2(p->getNewX() - pp->getNewX());
        if (dist < p->getCutOffRadius()) {{
          // if (p != pp) {{ numNbrs++; }}
          const double q = dist / h;
          double W = {CUBIC_SPLINE_SIGMA} / (h*h);
          if (q >= 2) {{
            W *= 0;
          }} else if (q >= 1) {{
            W *= (0.25 * (2 - q)*(2 - q)*(2 - q));
          }} else {{
            W *= (1 - 1.5*q*q*(1 - q/2));
          }}

          densitySum += pp->getMass() * W;
        }}
      }}

      // p->setNumNbrs(numNbrs);

      p->setDensity(densitySum);
      double eosPressure = p->getDensity() * p->getU() * ({GAMMA} - 1);
      p->setPressure((eosPressure < 0) ? 0 : eosPressure);
      p->setStep(peanosph::enclavesph::globaldata::Particle::Step::calcAccel);

    }} else {{
      // This means a particle is being touched for the second time
    }}
  }}
}}
"""

# touch_vertex_first_time_compute_kernel = """
# for (auto& p: localParticles) {
#   // p is a pointer to a local particle.
#   p->setX( p->getNewX() );
# }
# """

touch_vertex_first_time_compute_kernel = """
"""

calc_densities.add_action_set( peano4.toolbox.particles.ParticleParticleInteraction(particles,cell_compute_kernel,touch_vertex_first_time_compute_kernel))
project.solversteps.add_step(calc_densities)

# %% [markdown]
# We edit the generated C++ CalcDensities class to update the particle positions from the previous timestep.
# %% [markdown]
# ## Calculate particle accelerations
#
# Each particle has its acceleration calculated using the densities, pressures and velocities of its neighbours.
#
# We also calculate the time derivative of the internal energy.
#
# In this secondary traversal we finally use the particle AMR action set.

# %%
calc_accels = peano4.solversteps.Step("SecondaryTraversal")
calc_accels.use_vertex(particles)
calc_accels.use_vertex(task_counter)
calc_accels.use_cell(particle_tree_analysis.cell_marker)
calc_accels.add_action_set(peano4.toolbox.particles.UpdateParticleGridAssociation(particles))
calc_accels.add_action_set(peano4.toolbox.particles.ParticleAMR(particles,particle_tree_analysis))
calc_accels.add_action_set(particle_tree_analysis)

cell_compute_kernel = f"""
const int numLocalParticles = std::distance(localParticles.begin(), localParticles.end());
if (numLocalParticles > 0) {{
  std::cout << "[ppi::SecondaryTraversal::EnterCell] cell centre: " << marker.x()(0) << ", " << marker.x()(1) << '\\n';
  for (int i=0; i<TwoPowerD; i++) {{
    peanosph::enclavesph::vertexdata::TaskCounter& tc = fineGridVerticesTaskCounter(i);
    std::cout << "[ppi::SecondaryTraversal::EnterCell] taskcounter " << i << " = " << tc.getNumTasksRemaining() << '\\n';
    tc.setNumTasksRemaining(tc.getNumTasksRemaining() - 1);
  }}
  std::cout << std::endl;
}}

//Run over local set
for (auto& p: localParticles) {{
  // Pick only those particles that reside within the current cell, otherwise
  // we would update each particle up to 2^d times. I do not exploit the symmetry
  // of forces here
  if ( marker.isContained( p->getX() ) ) {{
    if (p->getStep() == peanosph::enclavesph::globaldata::Particle::Step::calcAccel) {{

      const double h = p->getSmoothingLength();
      tarch::la::Vector<Dimensions,double> accel(0.0);
      double uDash = 0.0;
      const double pPOverRhoSquared = p->getPressure() / (p->getDensity() * p->getDensity());

      int numNbrs = 0;
      for (auto& pp: activeParticles) {{
        if (p!=pp) {{
          tarch::la::Vector<Dimensions,double> distVec = p->getX() - pp->getX();
          const double dist = tarch::la::norm2(distVec);
          if (dist < p->getCutOffRadius()) {{
            // numNbrs++;
            const double mass = pp->getMass();
            const double ppPOverRhoSquared = pp->getPressure() / (pp->getDensity() * pp->getDensity());

            const double q = dist / h;
            double delW = {CUBIC_SPLINE_SIGMA} / (h*h);
            if (q >= 2) {{
              delW *= 0;
            }} else if (q >= 1) {{
              delW *= (-0.75 * ((2 - q)*(2 - q)));
            }} else {{
              delW *= (0.75*q * (3*q - 4));
            }}

            tarch::la::Vector<Dimensions,double> gradW = (delW / (dist*h)) * distVec;

            // Add acceleration from pressure
            accel -= mass * (pPOverRhoSquared + ppPOverRhoSquared) * gradW;
            assertion2( std::isfinite(accel(0)), pp->toString(), p->toString() );
            assertion2( std::isfinite(accel(1)), pp->toString(), p->toString() );

            // Add acceleration from viscosity
            assertion2( std::isfinite(dist), pp->toString(), p->toString() );
            accel += (mass * 2 * tarch::la::norm2(gradW) * {DYNAMIC_VISCOSITY}) * (pp->getV() - p->getV()) / (p->getDensity() * pp->getDensity() * dist);

            // Update energy per mass derivative (uDash)
            uDash += mass * (pPOverRhoSquared + ppPOverRhoSquared) * tarch::la::dot((p->getV() - pp->getV()), gradW) / 2;
          }}
        }}
      }}

      // p->setNumNbrs(numNbrs);

      assertion1( std::isfinite(accel(0)), p->toString() );
      assertion1( std::isfinite(accel(1)), p->toString() );
      p->setA(accel);
      p->setUDash(uDash);
      p->setStep(peanosph::enclavesph::globaldata::Particle::Step::move);

    }} else {{
      // This means the particle is being touched for the second time.
    }}
  }}
}}
"""

touch_vertex_first_time_compute_kernel = """
for (auto& p: localParticles) {
 // p is a pointer to a local particle. I don't do anything with it in this example.
}
"""

calc_accels.add_action_set( peano4.toolbox.particles.ParticleParticleInteraction(particles,cell_compute_kernel,touch_vertex_first_time_compute_kernel))
project.solversteps.add_step(calc_accels)

# %% [markdown]
# We finally have to update the particle velocities and internal energies as well as calculate the new particle positions. For this, I don't rely on the injected code (I could so). Instead, I wait for the Python script to run once and the implement the updates manually in the class within actionsets.
# %% [markdown]
# ### Plots setup
#
# I will have to plot the outcome (or any snapshot). With particles, each of my dumps produces two types of files: particle data and mesh data. For the mesh, I rely on the toolbox's grid plotting. The plot is the step where we don't need the AMR criterion - the particles don't move here after all. We however run the tree analysis, so it is up-to-date for the next step.

# %%
print_solution = peano4.solversteps.Step( "Plot" )
print_solution.use_vertex(particles)
print_solution.use_vertex(task_counter)
print_solution.use_cell(particle_tree_analysis.cell_marker)
#print_solution.remove_all_actions()
print_solution.add_action_set( peano4.toolbox.particles.UpdateParticleGridAssociation(particles) )
print_solution.add_action_set( particle_tree_analysis )
print_solution.add_action_set( peano4.toolbox.PlotGridInPeanoBlockFormat( "grid-enclave", None ) )
print_solution.add_action_set( particle_plotter )
project.solversteps.add_step(print_solution)

# %% [markdown]
# ## Generate the actual C++ code
#
# Standard triad of operations. You can skip the first two steps if you want as the script then will automatically invoke the previous steps. The other way round, it is always admissible to only generate stuff, e.g., but to build and run the project through a command line.

# %%
project.generate()

# %% [markdown]
# **IT'S VITAL THAT AFTER GENERATING THE FILES WE EDIT `vertexdata/TaskCounter.h` SO THAT `_numTasksRemaining` IS ZERO INITIALIZED BY DEFAULT.**

# %%
project.build()
# !make -j4

# %% [markdown]
# ## Implement the actual code
#
# Don't forget to insert
#
# <pre>
# #include "../globaldata/Particle.h"
# </pre>
# %% [markdown]
# ## Run code
#
# Before we run any code, we clean up, i.e. remove all old output files.

# %%
output_files = [ f for f in os.listdir(".") if f.endswith(".peano-patch-file") or f.endswith(".vtu") or f.endswith(".pvd") ]
for f in output_files:
  os.remove(f)
