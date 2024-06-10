# This file is part of the ExaHyPE2 project. For conditions of distribution and 
# use, please see the copyright notice at www.peano-framework.org
import peano4
import peano4.output.Jinja2TemplatedHeaderImplementationFilePair

import dastgen2

import os
import sys
import itertools

from .PETScMain import PETScMain




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
  
  \param _solvers: [Solver]
      List of tuples of typename and names of solvers that this project should use simultaneously.
  \param preconditioner_type: String
      Preconditioner type to be used. This part of the string should match the final 
      part of the enum class in src/petsc/LinearEquationSystem.h. The namespaceing is 
      taken care of downstream.
  \param solver_type: String
      Solver type to be used. This part of the string should match the final 
      part of the enum class in src/petsc/LinearEquationSystem.h. The namespaceing is 
      taken care of downstream.
  
  """
  def __init__(self, 
               namespace, 
               project_name, 
               directory  = ".", 
               executable = "peano4petsc",
               preconditioner_type = "None",
               solver_type         = "GMRES"
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
    
    self._solvers = []

    self.algorithm_step_create_grid                  = peano4.solversteps.Step( "CreateGrid",                False )
    self.algorithm_step_enumerate_and_init_solution  = peano4.solversteps.Step( "EnumerateAndInitSolution",  False )
    self._algorithm_step_init_petsc                  = peano4.solversteps.Step( "InitPETSc",                 False )
    self.algorithm_step_assemble                     = peano4.solversteps.Step( "Assemble",                  False )
    self._algorithm_step_solve                       = peano4.solversteps.Step( "Solve",                     False )
    self.algorithm_step_map_solution_onto_mesh       = peano4.solversteps.Step( "MapSolutionOntoMesh",       False )
    self.algorithm_step_plot                         = peano4.solversteps.Step( "Plot",                      False )

    self.preconditioner_type = preconditioner_type
    self.solver_type         = solver_type
    
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

    self.__export_constants()


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

     type_name_of_solvers: [String]
       Name of the solvers that we should instantiate and use in the main.
    
    
    """
    dictionary = {}
    dictionary[ "SOLVER_INSTANCES"] = [(x.typename(), x.instance_name()) for x in self._solvers ]
    dictionary[ "PRECONDITIONER_TYPE" ] = "::petsc::LinearEquationSystem::PreconditionerType::" + self.preconditioner_type
    dictionary[ "SOLVER_TYPE" ]         = "::petsc::LinearEquationSystem::KSPSolverType::" + self.solver_type
    
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
    solver.add_to_init_petsc                 (self._algorithm_step_init_petsc)
    solver.add_to_assemble                   (self.algorithm_step_assemble)
    solver.add_to_map_solution_onto_mesh     (self.algorithm_step_map_solution_onto_mesh)
    solver.add_to_plot                       (self.algorithm_step_plot)

    solver.add_implementation_files_to_project( self._project.namespace, self._project.output )
    
    self._solvers.append( solver )

    pass


  def __export_constants(self):
        """
        We export ExaHyPE's constants. Besides the constants from ExaHyPE,
        I also export some parameters from Peano onto the ExaHyPE constants
        file. Therefore, it is important that you parse the configure output
        before we export the constants.
        """
        self._project.constants.clear()
        offset_string = "{" + str(self._domain_offset[0])
        size_string = "{" + str(self._domain_size[0])
        for i in range(1, self._dimensions):
            offset_string += ","
            size_string += ","
            offset_string += str(self._domain_offset[i])
            size_string += str(self._domain_size[i])
        offset_string += "}"
        size_string += "}"
        self._project.constants.add_include("""#include <bitset>""")
        self._project.constants.add_include("""#include "tarch/la/Vector.h" """)
        self._project.constants.add_include("""#include "peano4/utils/Globals.h" """)
        self._project.constants.export_const_with_type(
            "DomainOffset", offset_string, "tarch::la::Vector<Dimensions,double>"
        )
        self._project.constants.export_const_with_type(
            "DomainSize", size_string, "tarch::la::Vector<Dimensions,double>"
        )
        self._project.constants.export("PlotterPrecision", str(self._plotter_precision))

        build_string = "python3 "
        for i in sys.argv:
            build_string += " "
            build_string += i
        self._project.constants.export_string("BuildInformation", build_string)
        self._project.constants.export_string(
            "ConfigureInformation", self._project.output.makefile.configure_call
        )

        self._project.output.readme.add_package_description(
            """

### Peano's multigrid extension

This is yet to be written, and we need to have a paper on our work.

       @article{Reinarz:2020:ExaHyPE,
         title = {ExaHyPE: An engine for parallel dynamically adaptive simulations of wave problems},
         journal = {Computer Physics Communications},
         volume = {254},
         pages = {107251},
         year = {2020},
         issn = {0010-4655},
         doi = {https://doi.org/10.1016/j.cpc.2020.107251},
         url = {https://www.sciencedirect.com/science/article/pii/S001046552030076X},
         author = {Anne Reinarz and Dominic E. Charrier and Michael Bader and Luke Bovard and Michael Dumbser and Kenneth Duru and Francesco Fambri and Alice-Agnes Gabriel and Jean-Matthieu Gallard and Sven K\"oppel and Lukas Krenz and Leonhard Rannabauer and Luciano Rezzolla and Philipp Samfass and Maurizio Tavelli and Tobias Weinzierl},
         keywords = {Hyperbolic, PDE, ADER-DG, Finite volumes, AMR, MPI, TBB, MPI+X},
         abstract = {ExaHyPE (An Exascale Hyperbolic PDE Engine) is a software engine for solving systems of first-order hyperbolic partial differential equations (PDEs). Hyperbolic PDEs are typically derived from the conservation laws of physics and are useful in a wide range of application areas. Applications powered by ExaHyPE can be run on a student's laptop, but are also able to exploit thousands of processor cores on state-of-the-art supercomputers. The engine is able to dynamically increase the accuracy of the simulation using adaptive mesh refinement where required. Due to the robustness and shock capturing abilities of ExaHyPE's numerical methods, users of the engine can simulate linear and non-linear hyperbolic PDEs with very high accuracy. Users can tailor the engine to their particular PDE by specifying evolved quantities, fluxes, and source terms. A complete simulation code for a new hyperbolic PDE can often be realised within a few hours - a task that, traditionally, can take weeks, months, often years for researchers starting from scratch. In this paper, we showcase ExaHyPE's workflow and capabilities through real-world scenarios from our two main application areas: seismology and astrophysics.
           Program summary
           Program title: ExaHyPE-Engine Program Files doi: http://dx.doi.org/10.17632/6sz8h6hnpz.1 Licensing provisions: BSD 3-clause Programming languages: C++, Python, Fortran Nature of Problem: The ExaHyPE PDE engine offers robust algorithms to solve linear and non-linear hyperbolic systems of PDEs written in first order form. The systems may contain both conservative and non-conservative terms. Solution method: ExaHyPE employs the discontinuous Galerkin (DG) method combined with explicit one-step ADER (arbitrary high-order derivative) time-stepping. An a-posteriori limiting approach is applied to the ADER-DG solution, whereby spurious solutions are discarded and recomputed with a robust, patch-based finite volume scheme. ExaHyPE uses dynamical adaptive mesh refinement to enhance the accuracy of the solution around shock waves, complex geometries, and interesting features.
         }
       }
"""
        )

        for i in self._solvers:
            self._project.output.readme.add_entry(
                i.create_readme_descriptor()
            )

