# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import os
import sys
import sympy
import subprocess

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
# We specify the space dimensions here.
# We support either 2- or 3-dimensional problems.
#
dimensions = 2

#
# The number of finite volumes per axis in one patch.
#
patch_size = 16

#
# The size of a finite volume/cell per axis.
#
volume_size = 0.01

#
# The simulation end time.
#
end_time = 1

#
# Choose domain size and offset.
#
size = [1.0, 1.0, 1.0]
offset = [0.0, 0.0, 0.0]

#
# If set to False, the project will not compile the project after generation (i.e., you need to invoke the Makefile yourself).
#
use_make = True

#
# Set to true, to visualise/convert the output data using a postprocessor.
#
visualise = False

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
# We amend the executable name by the space dimensions.
#
my_project = exahype2.Project(
    namespace=["tutorials", "exahype2", "euler"],
    project_name="EulerTutorial",
    directory=".",
    executable="euler-" + str(dimensions) + "d",
)

#
# Add the Finite Volumes solver using named arguments.
# This is the way you can add further PDE terms.
# This requires the 'BlockStructured' toolbox and 'ExaHyPE' to be built.
# Rusanov is the type of flux that is used to solve the Riemann problem at boundaries between cells.
#
my_solver = exahype2.solvers.fv.rusanov.GlobalAdaptiveTimeStep(
    name="EulerSolver",
    patch_size=patch_size,
    unknowns=dimensions + 2, # [rho, u, v, (w), e]
    auxiliary_variables=0, # This could be something alike material parameters. Not required for Euler.
    min_volume_h=volume_size,
    max_volume_h=volume_size,
    time_step_relaxation=0.5,
)

#
# We want to define our PDE symbolically.
# For this we use the 'symhype' package (not to be confused with 'sympy') from 'ExaHyPE'.
#
my_pde = exahype2.symhype.FirstOrderConservativePDEFormulation(unknowns=dimensions+2, auxiliary_variables=0, dimensions=dimensions)

#
# Give entries in input vector symbolic names. We first declare the constant
# gamma. Then we tell the solver how we would like to name the Q entries.
#
gamma = sympy.symbols("gamma")
# First scalar is density
rho   = my_pde.name_Q_entry(0, "rho")
# Momentum: Entries 1-dimensions (C counting style) holds j vector
# (Note: To get the velocities we need to divide with the density)
j     = my_pde.name_Q_entries(1, dimensions, "j")
# Energy
E     = my_pde.name_Q_entry(dimensions + 1, "E")
# Pressure
p = (gamma - 1) * (E-1/2 * exahype2.symhype.dot(j, j) / rho)

#
# Define the equation system
#
# Flux [unknowns, dimensions]
my_pde.F[0, :] = j
my_pde.F[1:dimensions + 1, :] = 1 / rho * exahype2.symhype.outer(j, j) + p * sympy.eye(dimensions)
my_pde.F[dimensions + 1, :]   = 1 / rho * j * (E + p)

# Eigenvalues [unknowns, dimensions]
c = sympy.sqrt(gamma * p / rho)
u = j / rho # Velocities
if dimensions == 3:
  my_pde.eigenvalues[0] = [u[0] - c, u[1] - c, u[2] - c]
  my_pde.eigenvalues[1] = [u[0],     u[1],     u[2]]
  my_pde.eigenvalues[2] = [u[0],     u[1],     u[2]]
  my_pde.eigenvalues[3] = [u[0],     u[1],     u[2]]
  my_pde.eigenvalues[4] = [u[0] + c, u[1] + c, u[2] + c]
else:
  my_pde.eigenvalues[0] = [u[0] - c, u[1] - c]
  my_pde.eigenvalues[1] = [u[0],     u[1]]
  my_pde.eigenvalues[2] = [u[0],     u[1]]
  my_pde.eigenvalues[3] = [u[0] + c, u[1] + c]

my_pde.substitute_expression(gamma, 1.4)

#
# Since 'my_pde' only holds the PDE for the Euler equations without initial- or boundary conditions,
# we still need to properly define initial- and boundary conditions.
# This gives us then a complete description of a 'scenario'.
#
#
# Initial conditions as specified in the README.md.
#
my_pde.initial_values[0] = 1 # rho
my_pde.initial_values[1] = 0 # u
my_pde.initial_values[2] = 0 # v

if dimensions == 2:
  volume_centre = sympy.sqrt((0.5 - my_pde.x[0])**2 + (0.5 - my_pde.x[1])**2)
  my_pde.initial_values[3] = sympy.Piecewise((1.0, volume_centre < 0.2), (1.01, True)) # e
else:
  volume_centre = sympy.sqrt((0.5 - my_pde.x[0])**2 + (0.5 - my_pde.x[1])**2 + (0.5 - my_pde.x[2])**2)
  my_pde.initial_values[3] = 0 # w
  my_pde.initial_values[4] = sympy.Piecewise((1.0, volume_centre < 0.2), (1.01, True)) # e

#
# Specify which implementation our solvers uses.
# Here we want to set the implementation we get from our symbolically defined PDE,
# i.e., we get the C++ implementation which is generated by ExaHyPE's 'symhype' package.
#
my_solver.set_implementation(
    initial_conditions=my_pde.implementation_of_initial_conditions(),
    boundary_conditions=my_pde.implementation_of_homogeneous_Neumann_BC(),
    flux=my_pde.implementation_of_flux(),
    eigenvalues=my_pde.implementation_of_max_eigenvalue(),
)

#
# To see which variables (unknowns + auxiliary variables) we can visualise,
# let's add a plot description for the variables to our solver.
#
my_solver.plot_description = my_pde.unknown_identifier_for_plotter()

#
# Add the solver to our project
#
my_project.add_solver(my_solver)

#
# Configure some global parameters
#
my_project.set_global_simulation_parameters(
    dimensions=dimensions,
    size=size[0:dimensions],
    offset=offset[0:dimensions],
    min_end_time=end_time,
    max_end_time=end_time,
    first_plot_time_stamp=0.0,
    time_in_between_plots=time_in_between_two_snapshots,
    periodic_BC=[False, False, False],
)

#
# This defines where the output files should go.
# If you omit this, output files are automatically put into the application's folder.
#
my_project.set_output_path("solution")
if not os.path.exists("solution"):
    os.makedirs("solution")

if visualise:
  output_patch_file = "solution/solution-EulerSolver.peano-patch-file"
  if os.path.isfile(output_patch_file):
    subprocess.call([
        "python3", "../../../python/peano4/visualisation/render.py",
        str(output_patch_file),
    ])
  sys.exit(0)

#
# If you only target a sequential execution, you can omit the load balancing.
# However, for a parallel build and execution you need to set load balancing.
# This requires the 'LoadBalancing' toolbox to be built.
# The type of load balancer can greatly impact the speedup and overall performance.
# For an overview of available load balancer refer to the documentation.
#
my_project.set_load_balancing(
    "toolbox::loadbalancing::strategies::SpreadOutHierarchically",
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
# Optional: Set an floating-point exception handler to the Peano 4 project.
#
my_project.set_fenv_handler("FE_DIVBYZERO | FE_INVALID | FE_OVERFLOW")

#
# Finally, we want to build our project.
# First, all of the necessary glue code is generated in the application folder,
# then 'make' is invoked automatically which compiles the generated code and links against our core libraries
# and toolboxes which have been built before.
# You can also always invoke 'make' yourself to compile, or cleanup with 'make clean'.
#
my_project.build(make=use_make, make_clean_first=True, throw_away_data_after_build=True)

print("\nPDE is: ")
print(my_pde.__str__())
print("Executable is euler-" + str(dimensions) + "d")
print("Clean object files via 'make clean'")
print("(Re)compile the generated code via 'make -j'")
print("Remove all generated code via 'make distclean'")
print("Regenerate all code by running 'python3 euler.py' again")
