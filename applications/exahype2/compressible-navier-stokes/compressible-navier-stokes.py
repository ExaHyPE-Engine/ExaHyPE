import os
import sys

sys.path.insert(0, os.path.abspath('../../../python'))

import peano4
import exahype2

dimensions                = 2
unknowns                  = dimensions + 2 # [rho, u, v, (w), e]
cell_size                 = 0.005
amr_levels                = 1 # Set to 1 to disable adaptive mesh refinement (AMR)
patch_size                = 4
end_time                  = 1.0
plot_interval             = 0.05 # Set to 0 to disable I/O
mode                      = peano4.output.CompileMode.Release # Switch here between 'Release', 'Debug', 'Asserts', 'Trace', 'Stats'

if dimensions == 2:
  size                    = [1.0, 1.0]
  offset                  = [0.0, 0.0]
else:
  size                    = [1.0, 1.0, 1.0]
  offset                  = [0.0, 0.0, 0.0]

gamma                     = 1.4
fluid_viscosity           = 0.0
prandtl_number            = 0.7
lid_drive_cavity_velocity = 1.0 # Wall velocity for lid-driven cavity

my_project = exahype2.Project(
  ["applications", "exahype2", "CompressibleNavierStokes"],
  "CompressibleNavierStokesApplication", ".",
  executable="compressible-navier-stokes-" + str(dimensions) + "d"
)

my_solver = exahype2.solvers.fv.rusanov.GlobalAdaptiveTimeStep(
  name                  = "NavierStokesSolver",
  patch_size            = patch_size,
  unknowns              = unknowns,
  auxiliary_variables   = 0,
  max_volume_h          = cell_size,
  min_volume_h          = cell_size * 3.0**(-amr_levels),
  time_step_relaxation  = 0.5,
)

if dimensions == 2:
  my_solver.plot_description = "rho, u, v, e"
else:
  my_solver.plot_description = "rho, u, v, w, e"

my_solver.set_implementation(
  ncp                   = exahype2.solvers.fv.PDETerms.User_Defined_Implementation,
  flux                  = exahype2.solvers.fv.PDETerms.User_Defined_Implementation,
  eigenvalues           = exahype2.solvers.fv.PDETerms.User_Defined_Implementation,
  #source_term          = exahype2.solvers.fv.PDETerms.User_Defined_Implementation,
  refinement_criterion = exahype2.solvers.fv.PDETerms.User_Defined_Implementation
)

my_solver.add_solver_constants("static constexpr double Gamma = {};".format(gamma))
my_solver.add_solver_constants("static constexpr double Viscosity = {};".format(fluid_viscosity))
my_solver.add_solver_constants("static constexpr double PrandtlNumber = {};".format(prandtl_number))
my_solver.add_solver_constants("static constexpr double LidDrivenCavityVelocity = {};".format(lid_drive_cavity_velocity))
my_solver.add_solver_constants("static constexpr double DomainSize[] = {%s};" % str(size).strip('[]'))
my_solver.add_solver_constants("static constexpr double DomainOffset[] = {%s};" % str(offset).strip('[]'))

my_project.add_solver( my_solver )

my_project.set_global_simulation_parameters(
  dimensions            = dimensions,
  size                  = size,
  offset                = offset,
  min_end_time          = end_time,
  max_end_time          = end_time,
  first_plot_time_stamp = 0.0,
  time_in_between_plots = plot_interval,
  periodic_BC           = [False, False, False]
)

my_project.set_output_path("solution")
if not os.path.exists("solution"):
  os.makedirs("solution")

my_project.set_load_balancing( "toolbox::loadbalancing::RecursiveBipartition", "new ::exahype2::LoadBalancingConfiguration()" )
my_project.set_Peano4_installation("../../../", mode )
my_project = my_project.generate_Peano4_project(verbose=True)
my_project.output.makefile.add_cpp_file( "NavierStokesSolver.cpp", generated=False )
my_project.build(make_clean_first=True, throw_away_data_after_build=True)

print("Executable is 'compressible-navier-stokes-<dimensions>d'")
print("Clean object files via 'make clean'")
print("Recompile the generated code via 'make -j'")
print("Remove all generated code via 'make distclean'")
print("Regenerate all code by running 'python3 compressible-navier-stokes.py' again")
print("Convert any output via python3 ../../../python/peano4/visualisation/render.py solution/solution-NavierStokesSolver.peano-patch-file")
print("Then you can open the generated .pvd file in a tool of your choice (e.g., ParaView)")
