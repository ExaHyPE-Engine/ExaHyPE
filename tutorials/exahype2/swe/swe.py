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
end_time = 1

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

#
# We want to define our PDE symbolically.
# For this we use the 'symhype' package (not to be confused with 'sympy') from 'ExaHyPE'.
#
my_pde = exahype2.symhype.FirstOrderConservativePDEFormulation(unknowns=2+1, auxiliary_variables=1, dimensions=2)
h = my_pde.name_Q_entry(0, "h")
hu = my_pde.name_Q_entry(1, "hu")
hv = my_pde.name_Q_entry(2, "hv")
b = my_pde.name_auxiliary_variable(0, "b")

# Gravitational constant
g = sympy.symbols('g')

# Speed in x-direction
u = sympy.Piecewise((hu / h, h > 0.0), (0.0, True))

# Speed in y-direction
v = sympy.Piecewise((hv / h, h > 0.0), (0.0, True))

#
# Define the equation system
#
# Flux [unknowns, dimensions]
my_pde.F[0, 0] = sympy.Piecewise((hu, h > 0.0), (0.0, True))
my_pde.F[1, 0] = sympy.Piecewise((hu * u + (g * h**2) / 2, h > 0.0), (0.0, True))
my_pde.F[2, 0] = sympy.Piecewise((h * u * v, h > 0.0), (0.0, True))

my_pde.F[0, 1] = sympy.Piecewise((hv, h > 0.0), (0.0, True))
my_pde.F[1, 1] = sympy.Piecewise((h * u * v, h > 0.0), (0.0, True))
my_pde.F[2, 1] = sympy.Piecewise((hv * v + (g * h**2)/2, h > 0.0), (0.0, True))

# Eigenvalues [unknowns, dimensions]
my_pde.eigenvalues[0, 0] = u
my_pde.eigenvalues[0, 1] = v

my_pde.eigenvalues[1, 0] = sympy.Piecewise((u + sympy.sqrt(g * h), h > 0.0), (0.0, True))
my_pde.eigenvalues[1, 1] = sympy.Piecewise((v + sympy.sqrt(g * h), h > 0.0), (0.0, True))

my_pde.eigenvalues[2, 0] = sympy.Piecewise((u - sympy.sqrt(g * h), h > 0.0), (0.0, True))
my_pde.eigenvalues[2, 1] = sympy.Piecewise((v - sympy.sqrt(g * h), h > 0.0), (0.0, True))

# NCP [unknowns, dimensions]
grad_b = my_pde.grad(b)

my_pde.ncp[0, 0] = 0.0
my_pde.ncp[1, 0] = sympy.Piecewise((g * h * grad_b, h > 0.0), (0.0, True))
my_pde.ncp[2, 0] = 0.0

my_pde.ncp[0, 1] = 0.0
my_pde.ncp[1, 1] = 0.0
my_pde.ncp[2, 1] = sympy.Piecewise((g * h * grad_b, h > 0.0), (0.0, True))

#
# Since 'my_pde' only holds the PDE for the Euler equations without initial- or boundary conditions,
# we still need to properly define initial- and boundary conditions.
# This gives us then a complete description of a 'scenario'.
#
#
# Initial conditions as specified in the README.md.
#
my_pde.initial_values[0] = 1 # Water height (h)
my_pde.initial_values[1] = 0 # x-momentum (hu)
my_pde.initial_values[2] = 0 # y-momentum (hv)

volume_centre = sympy.sqrt((0.5 - my_pde.x[0])**2 + (0.5 - my_pde.x[1])**2)
my_pde.initial_values[3] = sympy.Piecewise((0.0, volume_centre < 0.4), (0.1, True)) # Bathymetry (b)

my_pde.boundary_values[0] = h
my_pde.boundary_values[1] = -hu
my_pde.boundary_values[2] = -hv
my_pde.boundary_values[3] = b

my_pde.substitute_expression(g, 9.81)

#
# Specify which implementation our solvers uses.
# Here we want to set the implementation we get from our symbolically defined PDE,
# i.e., we get the C++ implementation which is generated by ExaHyPE's 'symhype' package.
#
my_solver.set_implementation(
    initial_conditions=my_pde.implementation_of_initial_conditions(),
    boundary_conditions=my_pde.implementation_of_boundary_conditions(),
    flux=my_pde.implementation_of_flux(),
    eigenvalues=my_pde.implementation_of_max_eigenvalue(),
    ncp = my_pde.implementation_of_ncp()
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

if visualise:
  output_patch_file = "solution/solution-SWESolver.peano-patch-file"
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
print("Executable is 'swe'")
print("Clean object files via 'make clean'")
print("(Re)compile the generated code via 'make -j'")
print("Remove all generated code via 'make distclean'")
print("Regenerate all code by running 'python3 swe.py' again")
