import os
import argparse
import numpy as np

import peano4
import swift2
import dastgen2

import shutil

# HDF5 Reader
import peano4.toolbox.particles.sph.fileReader

# SPH initialisation snippets library
import peano4.toolbox.particles.sph.initSnippets

########################################################################################
# Parse user input
########################################################################################

parser = argparse.ArgumentParser(description='SPH benchmarking script')
parser.add_argument("-j",   "--parallel-builds",   dest="j",              type=int, default=-1,                     help="Parallel builds" )
parser.add_argument("-v",   "--verbose",           dest="verbose",        action="store_true", default=False,       help="Verbose")
parser.add_argument("-c",   "--clean",             dest="clean_temporary_directories",  action="store_true",        help="Remove temporary directories before script starts" )
parser.add_argument("-d",   "--dimensions",        dest="dimensions",     type=int,   default=2,                    help="Spatial dimensions (default 2)" )
parser.add_argument("-et",  "--end-time",          dest="end_time",       type=float, default=1.0,                  help="End of simulation" )
parser.add_argument("-dt",  "--timestep-size",     dest="timestep_size",  type=float, default=1e-3,                 help="Global timestep size (initial value)" )
parser.add_argument("-cs",  "--cell-size",         dest="cell_size",      type=float, default=0.3,                  help="Cell size (default 0.3)" )
parser.add_argument("-pd",  "--peano-dir",         dest="peanodir",       default="../../..",                       help="Peano4 directory" )
parser.add_argument("-asserts",  "--asserts",      dest="asserts",        action="store_true", default=False,       help="Switch on assertions" )
args = parser.parse_args()


########################################################################################
# Generate SWIFT project
########################################################################################

project_namespace = ["benchmarks", "swift2", "densityTest"]

dimensions  = args.dimensions

project = swift2.Project( namespace=project_namespace,
                          project_name="test_density", 
                          executable="test_density" )

# Declare the particle type
particle = swift2.particle.testDensityCalculation(name =
                                                  "testDensityCalculation",
                                                  cfl_factor=0.1,
                                                  initial_time_step_size=1e-4)

# Add particle to the SWIFT project
particle_set = project.add_particle_species( particle )

if args.asserts:
  build_mode = peano4.output.CompileMode.Asserts
else:
  build_mode = peano4.output.CompileMode.Debug
    
offset      = [ 0, 0, 0 ] 
domain_size = [ 1, 1, 1 ]
periodic_boundary_conditions = [False,False,False]          # Periodic BC

project.set_global_simulation_parameters(
  dimensions,               # dimensions
  offset,  domain_size,
  0.1, 
  args.timestep_size,
  args.end_time,
  0.0, 
  args.end_time/50,                      #args.plot_step_size,   # snapshots
  periodic_boundary_conditions,
  8  # plotter precision
)


project.set_Peano4_installation(args.peanodir, build_mode)

project.set_load_balancing("toolbox::loadbalancing::RecursiveSubdivision", "new toolbox::loadbalancing::RecursiveSubdivision::DefaultConfiguration()")



###############################################################################
# Initialisation snippet for the particle
###############################################################################
#
# If we work with only one particle, then we can simply set its velocity. 
# However, our script works both if we only module the earth but also when we
# model earth and sun explicitly. In the latter case, we have to ensure that 
# only the earth is given an initial velocity.
#
###############################################################################

# Snippet from library
# Just to fill in some dummy values for this case
ICs = peano4.toolbox.particles.sph.initSnippets.SodShockIC( dimensions=1 )
initialisation_snippet = ICs.initialisation_snippet

#
# Insert particles by coordinate
#
coords = np.zeros((10,3))
coords[:,1] = 0.5
for i in range(10):
  coords[i,0] = 0.1 * i

print('inserting particles at:', coords)

project.algorithm_step_init.add_action_set( peano4.toolbox.particles.InsertParticlesbyCoordinates(
  particle_set         = particle_set,
  initialisation_call  = initialisation_snippet,
  #  coordinates          = [ (0.5,0.5,0), (0.51,0.5,0), (0.52,0.5,0), (0.49,0.5,0), (0.48,0.5,0) ]
    coordinates          = coords,
#  coordinates          = [ (0,0.5,0) ] 
  ))

#
#  Insert particles from file
#
#project.algorithm_step_init.add_action_set( peano4.toolbox.particles.sph.fileReader.InsertParticlesFromHDF5File( 
#  particle_set         = particle_set,
#  filename='sodShock.hdf5' 
#  ))


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
particle_plotter.add_attribute_to_plot(particle.smoothL , 1)
particle_plotter.add_attribute_to_plot(particle.Ncount , 1)
project.algorithm_step_plot.add_action_set( particle_plotter )


###############################################################################
# Generate plain Peano 4 project
###############################################################################
#
# See commments above (for the force interaction kernel) why we might want to 
# have some of the constants from the Python script within the C++ code base
# that's generated.
#
###############################################################################
peano4_project = project.generate_Peano4_project(verbose=args.verbose)

# -----------------------------------------------------------------------------
# Export global SPH constants and parameters
# -----------------------------------------------------------------------------
 
# There are plenty of constant simulation paramaters to specify for our SPH
# simulation - we do that here. 

# Effective SPH dimensions ----------------------------------------------------
HYDRO_DIMENSIONS = 1

HYDRO_PART_NUMBER = str(len(coords))

# Time-integration parameters -------------------------------------------------
GLOBAL_TIME_STEP_SIZE = "1e-4"
CFL_CONSTANT = "0.1"

# EoS parameter ---------------------------------------------------------------
GAMMA = str(5./3.)

# Artificial viscosity parameters ---------------------------------------------
ALPHA_AV= "0.8"     # =0.8 in SWIFT
BETA_AV = "3"       # =3 in SWIFT/SPHENIX

# Newton-Raphson parameters --------------------------------------------------
# Maximum allowed number of Newton-Raphson iterations
HYDRO_DEFAULT_MAX_ITERATIONS = "10"

# Tolerance for Newton-Raphson convergence
H_TOLERANCE = "1e-4"

# SPH kernel parameters -------------------------------------------------------
#KERNEL_SUPPORT_RAD = "1.732051"  #  1D SWIFT M4 Kernel
KERNEL_SUPPORT_RAD = "1.936492"  #  1D SWIFT M5 Kernel - DOUBLE CHECKED CORRECT

# Max/min allowed values for smoothing length h
HYDRO_h_MAX = str( 1.0 / 3.0 / float(KERNEL_SUPPORT_RAD) ) 
HYDRO_h_MIN = "1e-6"

# Smoothing-length free parameter (resolution "eta_neighbours"). 
# Can be set by specifying a desired number of neighbours.
SMOOTH_L_FAC = str( 1.235 ) 

if(HYDRO_DIMENSIONS==1):
    N_MEAN_TARGET = str( 2.0 *           (float(KERNEL_SUPPORT_RAD) * float(SMOOTH_L_FAC) )    )
elif(HYDRO_DIMENSIONS==2):
    N_MEAN_TARGET = str(         np.pi * (float(KERNEL_SUPPORT_RAD) * float(SMOOTH_L_FAC) )**2 )
elif(HYDRO_DIMENSIONS==3):
    N_MEAN_TARGET = str( 4./3. * np.pi * (float(KERNEL_SUPPORT_RAD) * float(SMOOTH_L_FAC) )**2 )

# Kernel root value 
if(HYDRO_DIMENSIONS==1):
  #    CUBIC_SPLINE_SIGMA = str(8./3.)       # 1D M4 SWIFT
    CUBIC_SPLINE_SIGMA = str(3125./768.)     # 1D M5 SWIFT DOUBLE CHECKED CORRECT
elif(HYDRO_DIMENSIONS==2):
    CUBIC_SPLINE_SIGMA = str(80/(7*np.pi))    # 2D SWIFT Kernel
#    CUBIC_SPLINE_SIGMA = str(10/(7*pi))
elif(HYDRO_DIMENSIONS==3):
    CUBIC_SPLINE_SIGMA = str(1/np.pi)


print('-----------------------------------------------------------------------')
print('SPH parameters summary:')
print(f'Effective dimensions of the Simulation: {HYDRO_DIMENSIONS}'  )
print(f'Global time-step size: {GLOBAL_TIME_STEP_SIZE}'  )
print(f'Number of particles (1D): {HYDRO_PART_NUMBER}'  )
print(f'Smoothing length free factor: {SMOOTH_L_FAC}'   )
print(f'Kernel Root value (sigma): {CUBIC_SPLINE_SIGMA}')
print('-----------------------------------------------------------------------')

# Export Constants.h file
peano4_project.constants.export_const_with_type("HYDRO_DIMENSIONS"     , str(HYDRO_DIMENSIONS), "double")
peano4_project.constants.export_const_with_type("HYDRO_PART_NUMBER"    , HYDRO_PART_NUMBER    , "double")
peano4_project.constants.export_const_with_type("GLOBAL_TIME_STEP_SIZE", GLOBAL_TIME_STEP_SIZE, "double")
peano4_project.constants.export_const_with_type("CFL_CONSTANT"         , CFL_CONSTANT         , "double")
peano4_project.constants.export_const_with_type("GAMMA"                , GAMMA                , "double")
peano4_project.constants.export_const_with_type("ALPHA_AV"             , ALPHA_AV             , "double")
peano4_project.constants.export_const_with_type("BETA_AV"              , BETA_AV              , "double")
peano4_project.constants.export_const_with_type("HYDRO_DEFAULT_MAX_ITERATIONS", HYDRO_DEFAULT_MAX_ITERATIONS, "int")
peano4_project.constants.export_const_with_type("H_TOLERANCE"          , H_TOLERANCE          , "double")
peano4_project.constants.export_const_with_type("KERNEL_SUPPORT_RAD"   , KERNEL_SUPPORT_RAD   , "double")
peano4_project.constants.export_const_with_type("HYDRO_h_MAX"          , HYDRO_h_MAX          , "double")
peano4_project.constants.export_const_with_type("HYDRO_h_MIN"          , HYDRO_h_MIN          , "double")
peano4_project.constants.export_const_with_type("SMOOTH_L_FAC"         , SMOOTH_L_FAC         , "double")
peano4_project.constants.export_const_with_type("CUBIC_SPLINE_SIGMA"   , CUBIC_SPLINE_SIGMA   , "double")

# If we want to start a project from scratch, we may want to remove some of the generated code
if args.clean_temporary_directories:
  dirs = [ 'vertexdata/', 'repositories/', 'globaldata/', 'observers/' ]
  for dir in dirs:
    shutil.rmtree( dir )

peano4_project.generate( throw_away_data_after_generation=True )
peano4_project.build( make_clean_first = True, number_of_parallel_builds=args.j )

# remove output files from previous runs
output_files = [ f for f in os.listdir(".") if f.endswith(".peano-patch-file") or f.endswith(".vtu") or f.endswith(".pvd") ]
for f in output_files:
  os.remove(f)

