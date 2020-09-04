#!/usr/bin/env python3
"""

 A very simple example which demonstrates how to configure a patch-based
 Finite Volume solver in Peano4. The code relies on snippets from ExaHyPE2.
 However, it relies only on ExaHyPE's C/FORTRAN compute kernels, i.e. the 
 sophisticated build environment of this H2020 project is not used. The 
 solver simulates the simple Euler equations.

"""


#
# We import Peano4 as project. If this step fails, ensure that your environment
# variable PYTHONPATH points to Peano4's python directory.
#
import os
import peano4
import peano4.datamodel
import peano4.solversteps
import peano4.output
import peano4.visualisation
import peano4.toolbox.blockstructured


import exahype2



#
# Lets first clean up all plot files, so we don't get a mismatch
#
output_files = [ f for f in os.listdir(".") if f.endswith(".peano-patch-file") or f.endswith(".vtu") ]
for f in output_files:
  os.remove(f)


#
# Create a project and configure it to end up in a subnamespace (and thus
# subdirectory). 
#
project = exahype2.Project( ["examples", "exahype2", "euler"], "finitevolumes", "." )


#
# Lets configure some global parameters
#
dimensions = 3
if dimensions==2:
  project.set_global_simulation_parameters(
    dimensions,  [0.0,0.0],  [1.0,1.0],
    0.001,          # end time
    0.0, 0.01     # snapshots
  )
else:
  project.set_global_simulation_parameters(
    dimensions, [0.0,0.0,0.0], [1.0,1.0,1.0],
    0.1,          # end time
    0.0, 0.01
  )


#
# Include the new packages that we need to use CAS
#
import sympy
import exahype2.solvers.sympy.FirstOrderConservativePDEFormulation


#
# Create a new instance of Peano's symbolic ExaHyPE PDE interface.
#
pde = exahype2.solvers.sympy.FirstOrderConservativePDEFormulation(unknowns = 5,dimensions = 3)
#print( pde )


#
# Give entries in input vector symbolic names. We first declare some
# helpers/constants. Then we tell the solver how we would like to name the Q 
# entries
#
gamma = sympy.symbols( "gamma")
rho   = pde.name_Q_entry( 0, "rho" )
j     = pde.name_Q_entries( 1, 3, "j" )
E     = pde.name_Q_entry( 4, "E" )
#print( pde )


#
# Finally, define the equation system
#
p = (gamma - 1 ) * (E-1/2 * exahype2.solvers.sympy.dot(j,j) / rho)

pde.F[0,:]   = j
pde.F[1:4,:] = 1/rho * exahype2.solvers.sympy.outer(j,j) + p * sympy.eye(3)
pde.F[4,:]   = 1/rho * j * (E+p)
#print( pde )



c = sympy.sqrt( gamma * p /rho )
pde.eigenvalue[0] = [ j[0]/rho - c, j[1]/rho - c, j[2]/rho - c ]
pde.eigenvalue[1] = [ j[0]/rho, j[1]/rho, j[2]/rho ]
pde.eigenvalue[2] = [ j[0]/rho, j[1]/rho, j[2]/rho ]
pde.eigenvalue[3] = [ j[0]/rho, j[1]/rho, j[2]/rho ]
pde.eigenvalue[4] = [ j[0]/rho + c, j[1]/rho + c, j[2]/rho + c ]
#print( pde )


#
# Won't compile/work
#
#print( pde.implementation_of_flux() )


pde.substitute_expression( gamma, 1.4 )


#print( pde.implementation_of_flux() )
#print( pde.implementation_of_eigenvalues() )



#
# Add the Finite Volumes solver. This is the same as before
#
patch_size     = 23
unknowns       = 5
time_step_size = 0.0001
my_solver = exahype2.solvers.GenericRusanovFVFixedTimeStepSize("Euler", patch_size, unknowns, 0.0001)
project.add_solver( my_solver )


#
# But now set some implementations. The arguments are strings, so I can 
# inject plain C code into the routines. And plain C code is exactly what
# the symbolic PDE interface gives me.
#
my_solver.set_implementation(flux=pde.implementation_of_flux(),eigenvalues=pde.implementation_of_eigenvalues())


#
# Now build code as we did before
# 
build_mode = peano4.output.CompileMode.Asserts


peano4_project = project.generate_Peano4_project()
peano4_project.constants.export( "MaxHOfVolume", 0.1 )
peano4_project.constants.define( "WeUsedSymPy" )
peano4_project.output.makefile.parse_configure_script_outcome( "../../../.." )
peano4_project.output.makefile.add_library( project.get_core_library(build_mode), "../../../../src/exahype2" )
peano4_project.output.makefile.set_mode(build_mode)
peano4_project.generate(peano4.output.Overwrite.Default)
peano4_project.build()
#success = peano4_project.run( [] )


success = False


if success:
  convert = peano4.visualisation.Convert( "solutionEuler", True )
  convert.set_visualisation_tools_path( "../../../../src/visualisation" )
  convert.extract_fine_grid()
  convert.convert_to_vtk()
