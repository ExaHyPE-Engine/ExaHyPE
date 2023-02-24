import os
import argparse
import numpy as np

import peano4
import swift2
import dastgen2

import shutil


########################################################################################
# Parse user input
########################################################################################

parser = argparse.ArgumentParser(description='SPH benchmarking script')
parser.add_argument("-j",     "--parallel-builds", dest="j",              type=int, default=-1,                 help="Parallel builds" )
parser.add_argument("-v",     "--verbose",         dest="verbose",        action="store_true", default=False,   help="Verbose")
parser.add_argument("-c",     "--clean",           dest="clean_temporary_directories",  action="store_true",    help="Remove temporary directories before script starts" )
parser.add_argument("-d",     "--dimensions",      dest="dimensions",     type=int,   default=2,                help="Spatial dimensions (default 2)" )
parser.add_argument("-np",    "--particle-number", dest="particle_number",   required=True,  type=int,          help="Particle number (1D)" )
parser.add_argument("-et",    "--end-time",        dest="end_time",       type=float, default=0.2,              help="End of simulation" )
parser.add_argument("-plot",  "--plot-delta",      dest="plot_delta",     type=float, default=0.01,             help="Plot delta (0 switches off)" )
parser.add_argument("-dt",    "--timestep-size",   dest="timestep_size",  type=float, default=1e-4,             help="Global timestep size (initial value)" )
parser.add_argument("-cfl",   "--cfl-factor",      dest="cfl_factor",     type=float, default=0.1,              help="Value of the CFL constant" )
parser.add_argument("-cs",    "--cell-size",       dest="cell_size",      type=float, default=0.3,              help="Cell size (default 0.3)" )
parser.add_argument("-pd",    "--peano-dir",       dest="peanodir",       default="../../../..",                help="Peano4 directory" )
parser.add_argument("-asserts",  "--asserts",      dest="asserts",        action="store_true", default=False,   help="Switch on assertions" )
args = parser.parse_args()


########################################################################################
# Generate SWIFT project
########################################################################################

project_namespace = ["benchmarks", "swift2", "noh"]

project = swift2.Project( namespace=project_namespace,
                          project_name="Noh", 
                          executable="noh2D" )


# Initialise the SPH particle --------------------------------------------------
# If multiple particle species are present, we can specify the parameters
# independently for each case.

name = "hydroPart"
dimensions  = args.dimensions

particle = swift2.particle.SPHLeapfrogFixedInteractionRadius(name, 
                                                             dimensions_hydro       = dimensions,
                                                             cfl_factor             = args.cfl_factor, 
                                                             initial_time_step_size = args.timestep_size,
                                                             constant_time_step_size= True,
                                                             swift_project_namespace= project_namespace,
                                                             is_sph                 = True,
                                                             particles_per_cell     = 20,
                                                             min_h                  = args.cell_size/100,
                                                             max_h                  = args.cell_size
                                                             )


########################################################################################
# Set parameters for the SPH simulation
# If no parameters are passed to the swift2 project, it initializes with
# default values. The values of the parameters can be modified or retrieved
# using the set/get methods.
########################################################################################


# HYDRO_DIMENSION can be smaller than the Peano4 dimensions
HYDRO_DIMENSIONS = dimensions

# Particle number along 1 dimension
HYDRO_PART_NUMBER = args.particle_number

# Time integration parameters 
GLOBAL_TIME_STEP_SIZE = args.timestep_size
CFL_CONSTANT = args.cfl_factor

# EoS parameter ---------------------------------------------------------------
gamma_hydro_list = [str(5/3), str(7/5), str(4/3), str(2/1)]
gamma_hydro_list_symbols = ['HYDRO_GAMMA_5_3','HYDRO_GAMMA_7_5','HYDRO_GAMMA_4_3','HYDRO_GAMMA_2_1']
GAMMA = gamma_hydro_list[0]
GAMMA_HYDRO_SYMBOL = gamma_hydro_list_symbols[0]

# Artificial viscosity parameters ---------------------------------------------
ALPHA_AV= particle.alpha_av
BETA_AV = particle.beta_av

# Newton-Raphson parameters --------------------------------------------------
# Maximum allowed number of iterations and convergence tolerance
HYDRO_DEFAULT_MAX_ITERATIONS = particle.h_max_iterations
H_TOLERANCE = particle.h_tolerance

# SPH kernel -------------------------------------------------------
kernel_list = ['M5']
kernel_list_symbols = ['QUARTIC_SPLINE_KERNEL']
KERNEL = kernel_list[0]
KERNEL_SYMBOL = kernel_list_symbols[0]

# Smoothing-length free parameter (resolution "eta_neighbours"). 
SMOOTH_ETA_FAC = particle.eta_factor

if(KERNEL=='M5' and HYDRO_DIMENSIONS ==1):
  KERNEL_SUPPORT_RAD = "1.936492"  #  1D SWIFT M5 Kernel
if(KERNEL=='M5' and HYDRO_DIMENSIONS ==2):
  KERNEL_SUPPORT_RAD = "1.977173"
else:
  print(f'Unable to determine the kernel support radius. Please check.'  )

# Max/min allowed values for smoothing length h 
HYDRO_h_MAX = args.cell_size / 2
HYDRO_h_MIN = particle.h_hydro_min

particle.h_hydro_max = HYDRO_h_MAX

# Optional: Reduce the mantissa size for particle attributes -------------------
# Native size is 53 (double precision)
#particle.mantissa_size = 23

# Finally, we have to declare the sph particle attributes, in this case for MinimalSPH.
particle.declare_sph_particle_attributes()

# Add particle to the SWIFT project --------------------------------------------
# Notice that you any change to the particle will not take effect after adding
# the particle_set to the project
particle_set = project.add_particle_species( particle )

if args.asserts:
  build_mode = peano4.output.CompileMode.Asserts
else:
  build_mode = peano4.output.CompileMode.Release

# Set Peano4 simulation parameters ---------------------------------------------
offset      = [ 0, 0, 0 ] 
domain_size = [ 1, 1, 1 ]
periodic_boundary_conditions = [False,False,False]

project.set_global_simulation_parameters(
  dimensions            = dimensions,
  offset                = offset,  
  domain_size           = domain_size,
  min_end_time          = args.end_time,
  max_end_time          = 0.0,
  first_plot_time_stamp = 0.0, 
  time_in_between_plots = args.plot_delta, 
  periodic_BC           = periodic_boundary_conditions,
  plotter_precision     = 8
)

project.set_Peano4_installation(args.peanodir, build_mode)

project.set_load_balancing("toolbox::loadbalancing::RecursiveSubdivision", "new toolbox::loadbalancing::RecursiveSubdivision::DefaultConfiguration()")


###############################################################################
# Initialisation snippet for the particle
###############################################################################

# Set up the coordinates for the particles
# We construct a regular (lattice) configuration.
Npart = args.particle_number
offset = 1/Npart

coords = np.zeros((Npart**HYDRO_DIMENSIONS, 3))

x = np.linspace(offset, 1, Npart, endpoint=False)
y = np.linspace(offset, 1, Npart, endpoint=False)

xp, yp = np.meshgrid(x, y)
xp= xp.reshape((np.prod(xp.shape),))
yp= yp.reshape((np.prod(yp.shape),))

coords[:,0] = xp
coords[:,1] = yp

#  Noh problem (2D) snippet  --------------------------------------------------
#  The particle attributes are set via an initalisation snippet that sets
#  the initial conditions specific for this problem, i.e. the velocity field
#  that converges towards the center of the domain and that generates the
#  collapse.

ICs = swift2.scenario.NohProblemIC( dimensions=HYDRO_DIMENSIONS )
initialisation_snippet = ICs.initialisation_snippet

# Finally, pass this configuration to the initialisation script
project.algorithm_step_init.add_action_set( swift2.input.InsertParticlesByCoordinates(
  particle_set        = particle_set,
  initialisation_call = initialisation_snippet,
  coordinates         = coords,
  additional_includes = """
#include "swift2/kernels/equation_of_state.h"
"""  
  )
)


###############################################################################
# Set up the plotter
###############################################################################
#
# A plotter has to be told explicitly which data to visualisation. We visualise
# the velocity here as scalar attribute. To some degree, this is redundant info
# as we implicitly have the position of the particle and can "see" its 
# velocity when we play through a movie. But the explicit visualisation shows
# how to handle attributes per particle.
#
# When we create the plotter, we have to inform it how to enumerate the 
# individual snapshots. Obviously, we can just count through the snapshots.
# In this case, you have to pass the constant peano4.toolbox.particles.PlotParticlesInVTKFormat.CountTimeSteps
# into the field time_stamp_evaluation. You can also define your own time
# stamp. For this, it is useful to know that each particle definition in the 
# Python script will yield a particle class with the same name in the 
# subnamespace globaldata. Every particle class in return has a class attribute
#      
#            getSpecies()
#
# which returns an object that holds global information about this particle 
# type. 
###############################################################################
#particle_plotter = peano4.toolbox.particles.PlotParticlesInVTKFormat( "particles", particle_set, time_stamp_evaluation=peano4.toolbox.particles.PlotParticlesInVTKFormat.CountTimeSteps )
particle_plotter = peano4.toolbox.particles.PlotParticlesInVTKFormat( "particles", 
                                                                      particle_set, 
                                                                      "globaldata::{}::getSpecies().getMinTimeStamp()".format(particle.name) )
particle_plotter.add_attribute_to_plot(particle.part_id , 1)
particle_plotter.add_attribute_to_plot(particle.velocity     , dimensions)
particle_plotter.add_attribute_to_plot(particle.acceleration , dimensions)
particle_plotter.add_attribute_to_plot(particle.mass , 1)
particle_plotter.add_attribute_to_plot(particle.density , 1)
particle_plotter.add_attribute_to_plot(particle.pressure, 1)
particle_plotter.add_attribute_to_plot(particle.u       , 1)
particle_plotter.add_attribute_to_plot(particle.smoothL , 1)
particle_plotter.add_attribute_to_plot(particle.f , 1)
particle_plotter.add_attribute_to_plot(particle.Ncount, 1)
particle_plotter.add_attribute_to_plot(particle.is_boundary_part, 1)
project.algorithm_step_plot.add_action_set( particle_plotter )


###############################################################################
# Generate plain Peano 4 project
###############################################################################
#
# The value of some constants such as the adiabatic index cannot be arbitrary
# but rather we have a set of pre-defined options. The same happens for some
# functions, e.g. the SPH smoothing kernel. This allows us to define some
# symbols and fix the values at compile time.
# 
#
###############################################################################

peano4_project = project.generate_Peano4_project(verbose=args.verbose)

# Some sanity checks ----------------------------------------------------------
if GAMMA not in gamma_hydro_list:
  print(f'Please check the value of GAMMA. You have chosen: {GAMMA}'  )

if KERNEL not in kernel_list:
  print(f'Please check the kernel. You have chosen: {KERNEL}'  )

print('-----------------------------------------------------------------------')
print('SPH parameters summary:')
print(f'Effective dimensions of the Simulation: {HYDRO_DIMENSIONS}'  )
print(f'Global time-step size: {GLOBAL_TIME_STEP_SIZE}'  )
print(f'Number of particles (1D): {HYDRO_PART_NUMBER}'  )
print(f'Adiabatic index: {GAMMA}'  )
print(f'Smoothing length free factor: {SMOOTH_ETA_FAC}'   )
print('-----------------------------------------------------------------------')


# Export Constants.h file ------------------------------------------------------
peano4_project.constants.export_const_with_type("HYDRO_DIMENSIONS"     , str(HYDRO_DIMENSIONS) , "double")
peano4_project.constants.export_const_with_type("HYDRO_PART_NUMBER"    , str(HYDRO_PART_NUMBER), "double")
peano4_project.constants.export_const_with_type("GLOBAL_TIME_STEP_SIZE", str(GLOBAL_TIME_STEP_SIZE), "double")
peano4_project.constants.export_const_with_type("CFL_CONSTANT"         , str(CFL_CONSTANT)    , "double")
peano4_project.constants.export_const_with_type("GAMMA"                , GAMMA                , "double")
peano4_project.constants.export_const_with_type("ALPHA_AV"             , str(ALPHA_AV)        , "double")
peano4_project.constants.export_const_with_type("BETA_AV"              , str(BETA_AV )        , "double")
peano4_project.constants.export_const_with_type("HYDRO_DEFAULT_MAX_ITERATIONS", str(HYDRO_DEFAULT_MAX_ITERATIONS), "int")
peano4_project.constants.export_const_with_type("H_TOLERANCE"          , str(H_TOLERANCE)     , "double")
peano4_project.constants.export_const_with_type("KERNEL_SUPPORT_RAD"   , KERNEL_SUPPORT_RAD   , "double")
peano4_project.constants.export_const_with_type("HYDRO_h_MAX"          , str(HYDRO_h_MAX)     , "double")
peano4_project.constants.export_const_with_type("HYDRO_h_MIN"          , str(HYDRO_h_MIN)     , "double")
peano4_project.constants.export_const_with_type("SMOOTH_ETA_FAC"       , str(SMOOTH_ETA_FAC)  , "double")

# Export symbols for compilation (e.g. for SPH kernel) -----------------------
peano4_project.output.makefile.add_CXX_flag("-D" + "HYDRO_DIMENSION=" + str(HYDRO_DIMENSIONS) )
peano4_project.output.makefile.add_CXX_flag("-D" + GAMMA_HYDRO_SYMBOL )
peano4_project.output.makefile.add_CXX_flag("-D" + KERNEL_SYMBOL )


###############################################################################
# Cleanup and compilation
###############################################################################

# If we want to start a project from scratch, we may want to remove some of the generated code
if args.clean_temporary_directories:
  dirs = [ 'vertexdata/', 'repositories/', 'globaldata/', 'observers/' ]
  for dir in dirs:
    shutil.rmtree( dir )

# It is safe (and recommended) to overwrite any existing main file, since this
# is generated automatically by the Swift2 graph compiler.
peano4_project.generate( overwrite=peano4.output.Overwrite.Always, throw_away_data_after_generation=True )
peano4_project.build( make_clean_first = True, number_of_parallel_builds=args.j )

# remove output files from previous runs
output_files = [ f for f in os.listdir(".") if f.endswith(".peano-patch-file") or f.endswith(".vtu") or f.endswith(".pvd") ]
for f in output_files:
  os.remove(f)

