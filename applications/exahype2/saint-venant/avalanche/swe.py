import os
import sys

#
# We use our code Python generator to generate additional glue code.
# The generator is located in the /Peano/python folder and needs to therefore added to PYTHONPATH.
# You can either set PYTHONPATH on the command line yourself or insert it here in the script.
#
sys.path.insert(0, os.path.abspath("../../../../python"))

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
my_project = exahype2.Project( ["applications", "exahype2", "avalanche"], "avalanche", ".", executable="avalanche" )

#
# Add the Finite Volumes solver using named arguments.
# This is the way you can add further PDE terms.
# This requires the 'BlockStructured' toolbox and 'ExaHyPE' to be built.
#
my_solver = exahype2.solvers.fv.rusanov.GlobalAdaptiveTimeStep(
  name                  = "SWESolver",
  patch_size            = 3,
  unknowns              = 3,
  auxiliary_variables   = 11,
  min_volume_h          = 0.008,
  max_volume_h          = 0.08,
  time_step_relaxation  = 0.5
)

#
# To see which variables (unknowns + auxiliary variables) we can visualise,
# let's add a plot description for the variables to our solver.
#
my_solver.plot_description = "h, hu, hv, b, u_x, u_y, u_xx, u_yy, u_xy, v_x, v_y, v_xx, v_yy, v_xy"

my_solver._preprocess_reconstructed_patch = """
  SWESolver::extrapolateHalo(oldQWithHalo);
  SWESolver::calculateDerivatives(oldQWithHalo);
        

    """


my_solver.set_implementation(
  ncp                   = exahype2.solvers.fv.PDETerms.User_Defined_Implementation,
  flux                  = exahype2.solvers.fv.PDETerms.User_Defined_Implementation,
  refinement_criterion  = exahype2.solvers.fv.PDETerms.User_Defined_Implementation,
  source_term           = exahype2.solvers.fv.PDETerms.User_Defined_Implementation
)

#
# We can add additional variables to our solver instance, such as constexpr values
# we want to use in our solver implementations multiple times.
#
## Independent Constants
my_solver.add_solver_constants("static constexpr double Gravity = {}; // [m/s^2]".format(9.81))
my_solver.add_solver_constants("static constexpr double Pi = {};".format(3.14159))
my_solver.add_solver_constants("static constexpr double Zeta = 23*Pi/180;") # Pouliquen and Forterre
my_solver.add_solver_constants("static constexpr double Zeta1 = 21*Pi/180;") # Pouliquen and Forterre
my_solver.add_solver_constants("static constexpr double Zeta2 = 30.7*Pi/180;") # Pouliquen and Forterre
my_solver.add_solver_constants("static constexpr double Zeta3 = 22.2*Pi/180;") # Pouliquen and Forterre
my_solver.add_solver_constants("static constexpr double Beta = {};".format(0.136)) # Pouliquen main paper
my_solver.add_solver_constants("static constexpr double FrictionLengthScale = {};".format(0.00065)) # Pouliquen and Forterre

## Dependent Constants
my_solver.add_solver_constants("static constexpr double Mu1 = tan(Zeta1);")
my_solver.add_solver_constants("static constexpr double Mu2 = tan(Zeta2);")
my_solver.add_solver_constants("static constexpr double Mu3 = tan(Zeta3);")
my_solver.add_solver_constants("static constexpr double Nu = (2.0/9.0)*(FrictionLengthScale/Beta)*(Gravity*sin(Zeta)/sqrt(Gravity*cos(Zeta)))*(((Mu2-Mu1)/(tan(Zeta)-Mu1))-1); //Granular Viscosity Coefficient")
# my_solver.add_solver_constants("static constexpr double Nu = 0.0;")
my_solver.add_solver_constants("static constexpr double Hstart = FrictionLengthScale*(((Mu2-Mu1)/(tan(Zeta)-Mu3))-1) ;")
my_solver.add_solver_constants("static constexpr double Hstop = FrictionLengthScale*(((Mu2-Mu1)/(tan(Zeta)-Mu1))-1) ;")
#

#
# Add the solver to our project
#
my_project.add_solver( my_solver )

#
# Configure some global parameters
#
my_project.set_global_simulation_parameters(
  dimensions            = 2,
  size                  = [2.0, 2.0],
  offset                = [0.0, 0.0],
  min_end_time          = 2.0,
  max_end_time          = 2.0,
  first_plot_time_stamp = 0.0,
  time_in_between_plots = 0.005, # snapshot interval
  periodic_BC           = [False, False, False]
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
my_project.set_load_balancing( "toolbox::loadbalancing::RecursiveBipartition", "new ::exahype2::LoadBalancingConfiguration()" )

#
# We need to set the location of our core libraries ('Peano4').
# This helps us to resolve any dependencies.
# Additionally, we specify the build mode which you can also change to a different mode.
# Switch between 'Release', 'Debug', 'Asserts', 'Trace', 'Stats'.
#
my_project.set_Peano4_installation( "../../../../", mode=peano4.output.CompileMode.Release )

#
# We generate and grab the underlying core project of 'Peano4'.
# This gives us access to some functions we want to use to finalise and build this project.
#
my_project = my_project.generate_Peano4_project(verbose=True)

#
# We add our custom C++ solver file here.
# This allows us to call 'make distclean' without deleting this file as it is not generated.
# This step is optional and only required if 'make distclean' is used.
# Furthermore, this ensures that the C++ file is not ignored by the generated .gitignore (which ignores all generated files).
# As you want to have your own custom C++ file tracked by Git.
#
my_project.output.makefile.add_cpp_file( "SWESolver.cpp" )
my_project.output.makefile.add_h_file( "SWESolver.h" )

#
# Finally, we want to build our project.
# First, all of the necessary glue code is generated in the application folder,
# then 'make' is invoked automatically which compiles the generated code and links against our core libraries
# and toolboxes which have been built before.
# You can also always invoke 'make' yourself to compile, or cleanup with 'make clean'.
#
my_project.build(make_clean_first=True, throw_away_data_after_build=True)

print("Executable is 'avalanche'")
print("Clean object files via 'make clean'")
print("Recompile the generated code via 'make -j'")
print("Remove all generated code via 'make distclean'")
print("Regenerate all code by running 'python3 swe.py' again")
print("Convert any output via python3 ../../../../python/peano4/visualisation/render.py solution/solution-SWESolver.peano-patch-file")
print("Then you can open the generated .pvd file in a tool of your choice (e.g., ParaView)")
