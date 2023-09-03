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
# Since we are creating an 'ExaHyPE' application, we additionally
# need to import the 'exahype2' module.
#
import peano4
import exahype2

#
# We first create a new ExaHyPE2 project.
# For this, we specify the (nested) namespaces, the name of our main file and our executable name.
#
my_project = exahype2.Project(
    ["tutorials", "exahype2", "swe"], "SWETutorial", ".", executable="swe"
)

#
# Add the Finite Volumes solver using named arguments.
# This is the way you can add further PDE terms.
# This requires the 'BlockStructured' toolbox and 'ExaHyPE' to be built.
#
my_solver = exahype2.solvers.fv.rusanov.GlobalAdaptiveTimeStep(
    name="SWESolver",
    patch_size=3,
    unknowns=3,
    auxiliary_variables=1,
    min_volume_h=0.005,
    max_volume_h=0.05,
    time_step_relaxation=0.5,
)

#
# To see which variables (unknowns + auxiliary variables) we can visualise,
# let's add a plot description for the variables to our solver.
#
my_solver.plot_description = "h, hu, hv, b"

#
# Specify which implementation our solvers uses.
# Here we want to use a user-defined C++ implementation.
# For this, we create a C++ file "SWESolver.cpp" (same as our solver name from above),
# where we can use ExaHyPE's API to fill out the necessary implementations.
# The code generator recognises that we have a custom-made implementation
# of this solver (the name of the file is important here) and therefore does not
# generate a solver on its own (which would be empty as we specify a user-defined implementation here).
#
my_solver.set_implementation(
    ncp=exahype2.solvers.fv.PDETerms.User_Defined_Implementation,
    flux=exahype2.solvers.fv.PDETerms.User_Defined_Implementation,
    refinement_criterion=exahype2.solvers.fv.PDETerms.User_Defined_Implementation,
)

#
# We can add additional variables to our solver instance, such as constexpr values
# we want to use in our solver implementations multiple times.
#
my_solver.add_solver_constants(
    "static constexpr double Gravity = {}; // [m/s^2]".format(9.81)
)

#
# Add the solver to our project
#
my_project.add_solver(my_solver)

#
# Configure some global parameters
#
my_project.set_global_simulation_parameters(
    dimensions=2,
    size=[2.0, 2.0],
    offset=[-1.0, -1.0],
    min_end_time=0.20,
    max_end_time=0.20,
    first_plot_time_stamp=0.0,
    time_in_between_plots=0.005,  # snapshot interval
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
#
my_project.set_load_balancing(
    "toolbox::loadbalancing::RecursiveBipartition",
    "new ::exahype2::LoadBalancingConfiguration()",
)

#
# We need to set the location of our core libraries ('Peano4').
# This helps us to resolve any dependencies.
# Additionally, we specify the build mode which you can also change to a different mode.
# Switch between 'Release', 'Debug', 'Asserts', 'Trace', 'Stats'.
#
my_project.set_Peano4_installation("../../../", mode=peano4.output.CompileMode.Release)

#
# We generate and grab the underlying core project of 'Peano4'.
# This gives us access to some functions we want to use to finalise and build this project.
#
my_project = my_project.generate_Peano4_project(verbose=True)

#
# Optional: Set an floating-point exception handler to the Peano4 project.
#
my_project.set_fenv_handler("FE_DIVBYZERO | FE_INVALID | FE_OVERFLOW")

#
# We add our custom C++ solver file here.
# This allows us to call 'make distclean' without deleting this file as it is not generated.
# This step is optional and only required if 'make distclean' is used.
# Furthermore, this ensures that the C++ file is not ignored by the generated .gitignore (which ignores all generated files).
# As you want to have your own custom C++ file tracked by Git.
#
my_project.output.makefile.add_cpp_file("SWESolver.cpp")

#
# Finally, we want to build our project.
# First, all of the necessary glue code is generated in the application folder,
# then 'make' is invoked automatically which compiles the generated code and links against our core libraries
# and toolboxes which have been built before.
# You can also always invoke 'make' yourself to compile, or cleanup with 'make clean'.
#
my_project.build(make_clean_first=True, throw_away_data_after_build=True)

print("Executable is 'swe'")
print("Clean object files via 'make clean'")
print("Recompile the generated code via 'make -j'")
print("Remove all generated code via 'make distclean'")
print("Regenerate all code by running 'python3 swe.py' again")
print(
    "Convert any output via python3 ../../../python/peano4/visualisation/render.py solution/solution-SWESolver.peano-patch-file"
)
print(
    "Then you can open the generated .pvd file in a tool of your choice (e.g., ParaView)"
)
