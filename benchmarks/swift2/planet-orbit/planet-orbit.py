import os
import argparse

import peano4
import swift2
import dastgen2



########################################################################################
# Parse user input
########################################################################################

parser = argparse.ArgumentParser(description='ExaHyPE 2 - Euler benchmarking script')
parser.add_argument("-j",   "--parallel-builds",   dest="j",              type=int, default=-1, help="Parallel builds" )
parser.add_argument("-pd",  "--peano-dir",         dest="peanodir",       default="../../..", help="Peano4 directory" )
parser.add_argument("-v",   "--verbose",           dest="verbose",        action="store_true", default=False, help="Verbose")
parser.add_argument("-et",  "--end-time",          dest="end_time",       type=float, default=1.0, help="End of simulation" )
parser.add_argument("-cs",  "--cell-size",         dest="cell_size",      type=float, default=0.3, help="Cell size (default 0.3)" )
parser.add_argument("-asserts",  "--asserts",      dest="asserts",        action="store_true", default=False, help="Switch on assertions" )
args = parser.parse_args()



dimensions  = 3



########################################################################################
# Generate plain SWIFT project
########################################################################################

project = swift2.Project( namespace=["benchmarks", "swift2", "planetorbit"], 
                          project_name="planetorbit", 
                          executable="orbit-" + str(args.cell_size) )


particle   = swift2.particle.ExplicitEulerFixedInteractionRadius( "Planet" )

particle_set = project.add_particle_species( particle )

cut_off_radius = 0.1
initialisation_snippet = """

  /*
   * In this problem we will initialize just a single particle (Earth)
   * This will orbit around a point where the hypothetical Sun is (no particle needed here)
   */

  /* Internal cutoff radious used by Peano. 
   * Should always be larger than actual SPH cutoff radious (2h). 
   * But no really used here. */
  particle->setCutOffRadius(""" + str(cut_off_radius) + """);
  // particle->setX( {0.0, 0.0, 0.0} ); is set by injection
  particle->setV( {0.0, 0.0, 0.0} );
  particle->setA( {0.1, 0.0, 0.0} );

"""


project.algorithm_step_init.add_action_set( peano4.toolbox.particles.InsertParticlesAlongCartesianLayoutIntoUnrefinedCells(
  particle_set                = particle_set,
  distance_between_particles  = 0.45,               # this will initialize just one particle in the domain at (0,0)
  initialisation_call         = initialisation_snippet,
  noise                       = False,
#  additional_includes="""
#    #include "Constants.h"
#    """,
  computational_domain_offset = [0.5, 0.55])
  )


particle_plotter = peano4.toolbox.particles.PlotParticlesInVTKFormat( "particles", particle_set, time_stamp_evaluation=peano4.toolbox.particles.PlotParticlesInVTKFormat.CountTimeSteps )
particle_plotter.add_attribute_to_plot(particle.velocity, dimensions)
#particle_plotter.add_attribute_to_plot(a_attr       , Dimensions)
#particle_plotter.add_attribute_to_plot(energy_kin_attr  , 1)
#particle_plotter.add_attribute_to_plot(energy_pot_attr  , 1)
#particle_plotter.add_attribute_to_plot(energy_tot_attr  , 1)
project.algorithm_step_plot.add_action_set( particle_plotter )


########################################################################################
# Set up the SWIFT 2 project
########################################################################################

if args.asserts:
  build_mode = peano4.output.CompileMode.Asserts
else:
  build_mode = peano4.output.CompileMode.Release
    
#offset      = [-0.5, -0.5, -0,5] 
offset      = [ 0, 0, 0 ] 
domain_size = [ 1, 1, 1 ]
periodic_boundary_conditions = [False,False,False]          # Periodic BC

project.set_global_simulation_parameters(
  dimensions,               # dimensions
  offset,  domain_size,
  0.1,     0.001,
  args.end_time,
  0.0, 
  args.end_time/20,                      #args.plot_step_size,   # snapshots
  periodic_boundary_conditions,
  8  # plotter precision
)

project.set_Peano4_installation(args.peanodir, build_mode)


project.set_load_balancing("toolbox::loadbalancing::RecursiveSubdivision", "new toolbox::loadbalancing::RecursiveSubdivision::DefaultConfiguration()")


########################################################################################
# Generate plain Peano 4 project
########################################################################################
peano4_project = project.generate_Peano4_project(verbose=args.verbose)


#peano4_project.output.makefile.add_cpp_file( "../../../../applications/exahype2/ccz4/InitialValues.cpp" )
#peano4_project.output.makefile.add_cpp_file( "../../../../applications/exahype2/ccz4/CCZ4Kernels.cpp" )
#peano4_project.output.makefile.add_header_search_path( "../../../../applications/exahype2/ccz4" )

peano4_project.generate( throw_away_data_after_generation=False )
peano4_project.build( make_clean_first = True, number_of_parallel_builds=args.j )

