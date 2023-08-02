import os
import sys

#
# We use our code Python generator to generate additional glue code.
# The generator is located in the /Peano/python folder and needs to therefore added to PYTHONPATH.
# You can either set PYTHONPATH on the command line yourself or insert it here in the script.
#
sys.path.insert(0, os.path.abspath("../../../python"))

#
# We import the required modules for this project.
# We always need the 'peano4' module as this is our core project.
# Since we are creating an ExaHyPE 2 application, we additionally
# need to import the 'exahype2' module.
#
import peano4
import exahype2

#
# The number of finite volumes per axis in one patch.
#
patch_size = 16

#
# The size of a finite volume/cell per axis.
#
volume_size = 0.05

#
# The simulation end time.
#
end_time = 3.0

#
# Choose domain size and offset.
#
size=[2.0, 2.0]
offset=[-1.0, -1.0]

#
# If set to False, the project will not compile the project after generation (i.e., you need to invoke the Makefile yourself).
#
use_make = True

#
# Choose how often a snapshot is written.
#
time_in_between_two_snapshots = end_time/100
#time_in_between_two_snapshots = 0 # Set to 0 to disable I/O

#
# Switch between 'Release', 'Debug', 'Asserts', 'Trace', 'Stats'.
#
compile_mode=peano4.output.CompileMode.Release

#
# We first create a new ExaHyPE 2 project.
# For this, we specify the (nested) namespaces, the name of our main file and our executable name.
#
my_project = exahype2.Project(
    namespace=["tutorials", "exahype2", "swe"],
    project_name="SWETutorial",
    directory=".",
    executable="swe",
)

#
# Add the Finite Volumes solver using named arguments.
# This is the way you can add further PDE terms.
# This requires the 'BlockStructured' toolbox and 'ExaHyPE' to be built.
# Rusanov is the type of flux that is used to solve the Riemann problem at boundaries between cells.
#
my_solver = exahype2.solvers.fv.rusanov.GlobalAdaptiveTimeStep(
    name="SWESolver",
    patch_size=patch_size,
    unknowns=2 + 1, # [h, hu, hv]
    auxiliary_variables=1, # bathymetry
    min_volume_h=volume_size,
    max_volume_h=volume_size,
    time_step_relaxation=0.5,
)

my_solver.add_user_solver_includes("""
#include "SWEKernels.h"
""")
my_solver.set_implementation(
    initial_conditions="""
  Q[0] = 1.0;                      // Water height (h)
  Q[1] = 0;                        // x-momentum (hu)
  Q[2] = 0;                        // y-momentum (hv)
  const bool nearCentre = tarch::la::norm2(volumeCentre) < 0.40;
  Q[3] = (nearCentre ? 0.0 : 0.1); // Bathymetry (b)
""",

    boundary_conditions="""
  ::exahype2::swe::reflectiveBoundaryConditions(Qinside, Qoutside, faceCentre, volumeH, t, normal, NumberOfUnknowns, NumberOfAuxiliaryVariables);
""",

    flux="""
  ::exahype2::swe::flux(Q, faceCentre, volumeH, t, dt, normal, NumberOfUnknowns, NumberOfAuxiliaryVariables, F);
""",

    eigenvalues="""
  return ::exahype2::swe::maxEigenvalue(Q, faceCentre, volumeH, t, dt, normal, NumberOfUnknowns, NumberOfAuxiliaryVariables);
""",

    ncp="""
  ::exahype2::swe::nonconservativeProduct(Q, deltaQ, faceCentre, volumeH, t, dt, normal, NumberOfUnknowns, NumberOfAuxiliaryVariables, BTimesDeltaQ);
"""
)

my_solver.plot_description = "h, hu, hv, b"

#
# Add the solver to our project
#
my_project.add_solver(my_solver)

#
# Configure some global parameters
#
my_project.set_global_simulation_parameters(
    dimensions=2, # The dimensionality is fixed for SWE
    size=size,
    offset=offset,
    min_end_time=end_time,
    max_end_time=end_time,
    first_plot_time_stamp=0.0,
    time_in_between_plots=time_in_between_two_snapshots,
    periodic_BC=[False, False, False],
)

#
# This defines where the output files should go.
# If you omit this, output files are automatically put into the application's folder
#
my_project.set_output_path("solution")
if not os.path.exists("solution"):
    os.makedirs("solution")

#
# If you only target a sequential execution, you can omit the load balancing.
# However, for a parallel build and execution you need to set load balancing.
# This requires the 'LoadBalancing' toolbox to be built.
# The type of load balancer can greatly impact the speedup and overall performance.
# For an overview of available load balancer refer to the documentation.
#
my_project.set_load_balancing(
    "toolbox::loadbalancing::SpreadOutHierarchically",
    "new ::exahype2::LoadBalancingConfiguration()",
)

#
# We need to set the location of our core libraries ('Peano4').
# This helps us to resolve any dependencies.
# Additionally, we specify the build mode which you can also change to a different mode.
#
my_project.set_Peano4_installation("../../../", mode=compile_mode)

#
# We generate and grab the underlying core project of 'Peano4'.
# This gives us access to some functions we want to use to finalise and build this project.
#
my_project = my_project.generate_Peano4_project(verbose=True)

#
# Finally, we want to build our project.
# First, all of the necessary glue code is generated in the application folder,
# then 'make' is invoked automatically which compiles the generated code and links against our core libraries
# and toolboxes which have been built before.
# You can also always invoke 'make' yourself to compile, or cleanup with 'make clean'.
#
#my_project.build(make=use_make, make_clean_first=True, throw_away_data_after_build=True)
my_project.build(make_clean_first=True, throw_away_data_after_build=True)

#
# Create a postprocessing script which converts the output patch files of the simulation
# to VTU files which can be read by ParaView e.g.
#
with open("postprocess.py", "w") as file:
    file.write('''
import os
import sys
import subprocess

solution_dir = "solution"
if not os.path.exists(solution_dir):
    os.makedirs(solution_dir)

output_patch_file = solution_dir + "/solution-SWESolver.peano-patch-file"
if os.path.isfile(output_patch_file):
    subprocess.call([
        "python3", "../../../python/peano4/visualisation/render.py",
        str(output_patch_file)
    ])
''')

print("Executable is swe")
print("Clean object files via 'make clean'")
print("(Re)compile the generated code via 'make -j'")
print("Remove all generated code via 'make distclean'")
print("Regenerate all code by running 'python3 swe.py' again")
print("Generate (and convert) any postprocessing data by calling 'python3 postprocess.py'")
