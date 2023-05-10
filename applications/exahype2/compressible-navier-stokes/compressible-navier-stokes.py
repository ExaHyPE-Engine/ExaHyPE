import os
import sys

sys.path.insert(0, os.path.abspath('../../../python'))

import peano4
import exahype2

dimensions              = 2
unknowns                = dimensions + 2
auxiliary_variables     = 2
mesh_size               = 0.4
patch_size              = 17

my_project = exahype2.Project(
  ["applications", "exahype2", "CompressibleNavierStokes"],
  "CompressibleNavierStokesApplication", ".",
  executable="compressible-navier-stokes-" + str(dimensions) + "d"
)

my_solver = exahype2.solvers.fv.rusanov.GlobalAdaptiveTimeStep(
  name                  = "NavierStokesSolver",
  patch_size            = patch_size,
  unknowns              = unknowns,
  auxiliary_variables   = auxiliary_variables,
  min_volume_h          = mesh_size,
  max_volume_h          = mesh_size,
  time_step_relaxation  = 0.5,
)

my_solver.set_implementation(
  ncp                   = exahype2.solvers.fv.PDETerms.User_Defined_Implementation,
  flux                  = exahype2.solvers.fv.PDETerms.User_Defined_Implementation,
  eigenvalues           = exahype2.solvers.fv.PDETerms.User_Defined_Implementation,
  #source_term          = exahype2.solvers.fv.PDETerms.User_Defined_Implementation,
  #refinement_criterion = exahype2.solvers.fv.PDETerms.User_Defined_Implementation
)

my_project.add_solver( my_solver )

my_project.set_global_simulation_parameters(
  dimensions            = dimensions,
  offset                = [0.0,0.0],
  size                  = [1.0,1.0],
  min_end_time          = 2.0,
  max_end_time          = 2.0,
  first_plot_time_stamp = 0.0,
  time_in_between_plots = 0.01,
  periodic_BC           = [False, False, False]
)

my_project.set_output_path("solution")
if not os.path.exists("solution"):
  os.makedirs("solution")

my_project.set_load_balancing( "toolbox::loadbalancing::RecursiveBipartition", "new ::exahype2::LoadBalancingConfiguration()" )
my_project.set_Peano4_installation("../../../", peano4.output.CompileMode.Release )
my_project = my_project.generate_Peano4_project(True)
#my_project.output.makefile.add_cpp_file( "NavierStokesSolver.cpp", generated=False )
my_project.build(make_clean_first=True)

print("Executable is 'compressible-navier-stokes-<dimensions>d'")
print("Clean object files via 'make clean'")
print("Recompile the generated code via 'make -j'")
print("Remove all generated code via 'make distclean'")
print("Regenerate all code by running 'python3 compressible-navier-stokes.py' again")
print("Convert any output via python3 ../../../python/peano4/visualisation/render.py solution/solution-NavierStokesSolver.peano-patch-file")
print("Then you can open the generated .pvd file in a tool of your choice (e.g., ParaView)")
