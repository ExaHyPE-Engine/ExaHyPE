# This file is part of the ExaHyPE2 project. For conditions of distribution and 
# use, please see the copyright notice at www.peano-framework.org
import peano4
import peano4.output.Jinja2TemplatedHeaderImplementationFilePair

import dastgen2

import os
import sys

from petsc.PETScMain import PETScMain




class Project(object):
  """!
   
Multigrid solver project

A project hosts a set of multigrid solvers for different PDEs (which you might 
eventually want to couple). Other than that, each project generates exactly 
one main file and a solver repository. The main file is represented by 
petsc.PETScMain, while the repository is generated directly by the project.  
Consult generate_Peano4_project() for the latter.
  
  
## Algorithmic steps
  
Every PETSc project runs through the following steps:
 
  - algorithm_step_create_grid
  - algorithm_step_enumerate_and_init_solution
  - _algorithm_step_init_petsc
  - algorithm_step_assemble
  - _algorithm_step_solve
  - algorithm_step_map_solution_onto_mesh
  - algorithm_step_plot
  
Each of these steps is mapped onto one observer. It is clear that not all
steps actually lead to mesh traversals and, therefore, are not to be modified
by users. They are simply there to allow the main function to pick a step and
to tell all MPI ranks to run the same step on all local trees. I therefore 
mark these steps as private via a leading underscore.

  
## Attributes
  
  - _solver_instances: [String,String]
      List of tuples of typename and names of solvers that this project should use simultaneously.
  
  
  """
  def __init__(self, 
               namespace, 
               project_name, 
               directory  = ".", 
               executable = "peano4petsc",
               ):
    self._project = peano4.Project(namespace, project_name, directory)
    self._project.output.makefile.add_cmake_core_library("petsc")
    
    self._domain_offset = [0.0, 0.0, 0.0]
    self._domain_size   = [1.0, 1.0, 1.0]
    self._dimensions    = 2
    self._load_balancer_name      = ""
    self._load_balancer_arguments = ""
    self._Peano_src_directory = "."
    self._build_mode          = peano4.output.CompileMode.Asserts
    self._executable_name = executable
    self._output_path   = "./"
    
    self._solver_instances = []

    self.algorithm_step_create_grid                  = peano4.solversteps.Step( "CreateGrid",                False )
    self.algorithm_step_enumerate_and_init_solution  = peano4.solversteps.Step( "EnumerateAndInitSolution",  False )
    self._algorithm_step_init_petsc                  = peano4.solversteps.Step( "InitPETSc",                 False )
    self.algorithm_step_assemble                     = peano4.solversteps.Step( "Assemble",                  False )
    self._algorithm_step_solve                       = peano4.solversteps.Step( "Solve",                     False )
    self.algorithm_step_map_solution_onto_mesh       = peano4.solversteps.Step( "MapSolutionOntoMesh",       False )
    self.algorithm_step_plot                         = peano4.solversteps.Step( "Plot",                      False )
                
    
  def  set_load_balancing(self, load_balancer_name, load_balancer_arguments):
    """
    
      load_balancer_name: string
        Should be full-qualified name of the load balancer. 
        By default, I recommend to pass "toolbox::loadbalancing::RecursiveSubdivision"
        
      load_balancer_arguments: string
        If your load balancing requires parameters, add them
        here. It is a string that will be copied into the C++ class instantiation. 
        Please add the brackets yourself, i.e. "(3,4,5)" is fine, but "3,4,5" is not. 
        The only exception is the empty parameter list. Here, you can/should simply
        add the empty string.
        
    """
    self._load_balancer_name      = load_balancer_name
    self._load_balancer_arguments = load_balancer_arguments
    
    
  """
   The standard extensions that I use for both Peano and ExaHyPE.
  """
  LibraryDebug   = "_debug"
  LibraryRelease = ""
  LibraryTrace   = "_trace"
  LibraryAsserts = "_asserts"
  LibraryStats   = "_stats"
    
    
  def set_Peano4_installation(self, src_path, mode=peano4.output.CompileMode.Release ):
    """!

src_path: string
  Path (relative or absolute) to the src directory of Peano. This path 
  should hold both the headers (in subdirectories) and all the static
  libraries.
        
mode: peano4.output.CompileMode

    """
    self._Peano_src_directory = src_path
    self._build_mode          = mode


  def __configure_makefile(self):
    """!

Configure the makefile of the PETSc project

We parse the configure script's/CMAKE outcome first via parse_configure_script_outcome().
After that, we set the dimension manually as well as the executable name and 
the build mode. The values for these parameters step from set_Peano4_installation().
    
    """
    self._project.output.makefile.parse_configure_script_outcome( self._Peano_src_directory )
    self._project.output.makefile.set_dimension(self._dimensions)
    self._project.output.makefile.set_executable_name(self._executable_name)

    self._project.output.makefile.set_mode(self._build_mode)


  def set_global_simulation_parameters(self,
                                       dimensions,
                                       offset,
                                       domain_size,
                                       plotter_precision = 5,
                                       ):
    """
    
      offset and size should be lists with dimensions double entries.
            
    """
    self._domain_offset = offset
    self._domain_size   = domain_size
    self._dimensions    = dimensions
    self._plotter_precision = plotter_precision
    if plotter_precision<=0:
      raise Exception( "Plotter precision has to be bigger than 0" ) 
    
    
  def  set_output_path(self,path):
    self._output_path = path
    if not self._output_path.endswith( "/" ):
      self._output_path += "/"
      
        
  def generate_Peano4_project(self, verbose=False):
    """!

Build project
    
Construct and return the Peano4 project, i.e. all the action sets et al that 
you require to run this Peano4 application with PETSc. 
     
This routine generates a Peano project, i.e. the domain-specific 
view is translated into a Peano model. Once you have called this routine,
any changes to the PETSc project configuration do not propagate into the Peano
setup anymore. If you alter the project setup, you have to call 
generate_Peano4_project() again to get a new snapshot/version of the
Peano setup.

The function runs through various steps:

1. We create an instance of PETScMain that represents the C++ main file.

2. We add the essential algorithmic steps to the result project. At this
   point, they should all be configured properly
   
3. We configure the makefile via __configure_makefile().

4. Add the repository holding the instances of the solver.

5. Ensure that the generated readme file holds all relevant literature and 
   links.
          
    """
    self._project.main = PETScMain(self._project,
                                   self._domain_offset,
                                   self._domain_size,
                                   )

    self._project.solversteps.add_step(self.algorithm_step_create_grid)
    self._project.solversteps.add_step(self.algorithm_step_enumerate_and_init_solution)
    self._project.solversteps.add_step(self._algorithm_step_init_petsc)
    self._project.solversteps.add_step(self.algorithm_step_assemble)
    self._project.solversteps.add_step(self._algorithm_step_solve)
    self._project.solversteps.add_step(self.algorithm_step_map_solution_onto_mesh)
    self._project.solversteps.add_step(self.algorithm_step_plot)

    self.__configure_makefile()
    self.__generate_solver_repository()

    self._project.output.readme.add_package_description( """

### Peano4 PETSc backend

This yet has to be written.

""" )

    return self._project


  def __generate_solver_repository(self):
    """!

     type_name_of_solver_instances: [String]
       Name of the solvers that we should instantiate and use in the main.
    
    
    """
    dictionary = {}
    dictionary[ "SOLVER_INSTANCES"] = self._solver_instances
    
    templatefile_prefix = os.path.realpath(__file__).replace( ".pyc", "" ).replace( ".py", "" )
    template_name       = templatefile_prefix + ".SolverRepository.template"  
    generated_repository_files = peano4.output.Jinja2TemplatedHeaderImplementationFilePair(
      template_name + ".h",
      template_name + ".cpp",
      "SolverRepository", 
      self._project.namespace,
      "repositories", 
      dictionary,
      True,
      True)

    self._project.output.add( generated_repository_files )
    self._project.output.makefile.add_h_file(   "repositories/SolverRepository.h", generated=True )
    self._project.output.makefile.add_cpp_file( "repositories/SolverRepository.cpp", generated=True )
      

  def add_solver( self, solver ):
    """
    
    Takes the solver and runs through the individual steps of the algorithm.
    Per step, it tells the solver to inject its particular stuff to the 
    underlying observers.
    
    """  
    print( "---------------------------------------")
    print( "Create data for solver " +solver._name  )
    print( "---------------------------------------")
    print( str(solver) )
      
    solver.add_to_Peano4_datamodel( self._project.datamodel, verbose=False )
    
    solver.add_use_statements(self.algorithm_step_create_grid)
    solver.add_use_statements(self.algorithm_step_enumerate_and_init_solution)
    solver.add_use_statements(self._algorithm_step_init_petsc)
    solver.add_use_statements(self.algorithm_step_assemble)
    solver.add_use_statements(self.algorithm_step_map_solution_onto_mesh)
    solver.add_use_statements(self.algorithm_step_plot)

    solver.add_to_create_grid                (self.algorithm_step_create_grid)
    solver.add_to_enumerate_and_init_solution(self.algorithm_step_enumerate_and_init_solution)
    solver.add_to_assemble                   (self.algorithm_step_assemble)
    solver.add_to_map_solution_onto_mesh     (self.algorithm_step_map_solution_onto_mesh)
    solver.add_to_plot                       (self.algorithm_step_plot)

    solver.add_implementation_files_to_project( self._project.namespace, self._project.output )
    
    self._solver_instances.append( (solver.typename(), solver.instance_name()) )
    pass


