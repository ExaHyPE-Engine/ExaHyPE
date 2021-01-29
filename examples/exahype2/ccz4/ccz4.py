import os
import peano4
import exahype2

project = exahype2.Project( ["examples", "exahype2", "ccz4"], "ccz4" )

unknowns = {
  "G":6,
  "K":6,
  "theta":1,
  "Z":3,
  "lapse":1,
  "shift":3,
  "b":3,
  "dLapse":3,
  "dxShift":3,
  "dyShift":3,
  "dzShift":3,
  "dxG":6,
  "dyG":6,
  "dzG":6,
  "traceK":1,
  "phi":1,
  "P":3,
  "K0":1,
}

number_of_unknowns = 0
for i in unknowns:
  number_of_unknowns += unknowns[i]
print( "number of unknowns=", number_of_unknowns )

patch_size          = 6
auxiliary_variables = 0
time_step_size      = 0.001
max_h               = 0.4
min_h               = 0.4

my_solver = exahype2.solvers.fv.GenericRusanovFixedTimeStepSizeWithEnclaves(
  "CCZ4", patch_size, number_of_unknowns, auxiliary_variables, min_h, max_h, time_step_size
)

project.add_solver(my_solver)

import exahype2.sympy

pde = exahype2.sympy.PDE(unknowns=number_of_unknowns,auxiliary_variables=0,dimensions=3)

my_solver.set_implementation(
  boundary_conditions=exahype2.solvers.fv.PDETerms.User_Defined_Implementation,
  flux=exahype2.solvers.fv.PDETerms.None_Implementation,
  ncp=exahype2.solvers.fv.PDETerms.User_Defined_Implementation,
  source_term=exahype2.solvers.fv.PDETerms.User_Defined_Implementation
)   

build_mode = peano4.output.CompileMode.Asserts
#build_mode = peano4.output.CompileMode.Release

dimensions = 3
end_time = 1.0
snapshots = time_step_size*40
    
project.set_global_simulation_parameters(
  dimensions,               # dimensions
  [-0.5, -0.5, -0.5],  [1.0, 1.0, 1.0],
  end_time,                 # end time
  0.0, snapshots,   # snapshots
  [True,True,True]          # Periodic BC
)

project.set_Peano4_installation("../../..", build_mode)

peano4_project = project.generate_Peano4_project()

peano4_project.output.makefile.add_Fortran_flag( "-DCCZ4EINSTEIN -DDim3" )

# This is for GNU
peano4_project.output.makefile.add_Fortran_flag( "-lstdc++ -fdefault-real-8 -fdefault-double-8 -cpp -std=legacy -ffree-line-length-512 -fPIC" )
peano4_project.output.makefile.add_linker_flag( "-lstdc++ -fPIC -lgfortran" )

# This might work for Intel (not tested)
#peano4_project.output.makefile.add_Fortran_flag( "-r8 -cpp -auto -qopenmp-simd -O2" )
#peano4_project.output.makefile.add_linker_flag( "-lstdc++ -fPIC" )
# you might need -lifcore

peano4_project.output.makefile.add_Fortran_module( "MainVariables.f90" )

peano4_project.output.makefile.add_Fortran_files( 
  ["PDE.f90 ", "EinsteinConstraints.f90 ", "Properties.f90",
    "Metric.f90 ", "C2P-FOCCZ4.f90 "] 
)

peano4_project.constants.export_string( "Scenario", "gaugewave-c++" )

peano4_project.generate( throw_away_data_after_generation=False )

parallel_builds = 16   # I don't use a massively parallel build here as my laptop otherwise becomes too hot.
                      # Without any arguments, the build process will grab all of your cores.
peano4_project.build( make_clean_first = False, number_of_parallel_builds = parallel_builds )
#!make -j4







































