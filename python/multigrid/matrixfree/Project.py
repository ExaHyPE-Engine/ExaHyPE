import peano4
import peano4.output.Jinja2TemplatedHeaderImplementationFilePair

import dastgen2

from .Main import Main


class Project:
  """!

  Here we recreate a multigrid project, without using PETSc. 

  We keep some of the basic steps:
  - algorithm_step_create_grid
  - algorithm_step_enumerate_and_init_solution
  - _algorithm_step_solve
  - algorithm_step_plot

  The rest is yet to be fleshed out.

  """

  """
   The standard extensions that are used for Peano and ExaHyPE.
  """
  LibraryDebug   = "_debug"
  LibraryRelease = ""
  LibraryTrace   = "_trace"
  LibraryAsserts = "_asserts"
  LibraryStats   = "_stats"

  def __init__(self, 
               namespace, 
               project_name, 
               directory  = ".", 
               executable = "peano4matrixfree",
               ):
    self._project = peano4.Project(namespace, project_name, directory)
    
    # what to do about this line?
    # self._project.output.makefile.add_cmake_core_library("petsc")

    # does this get overwritten later?
    self._domain_offset = [0.0, 0.0, 0.0]
    self._domain_size   = [1.0, 1.0, 1.0]
    self._dimensions    = 2

    self._load_balancer_name      = ""
    self._load_balancer_arguments = ""
    self._Peano_src_directory = "."

    # does this get overwritten later?
    self._build_mode          = peano4.output.CompileMode.Asserts
    self._executable_name = executable
    self._output_path   = "./" 

    self._solvers = []

    self.algorithm_step_create_grid                  = peano4.solversteps.Step( "CreateGrid",                False )
    self.algorithm_step_enumerate_and_init_solution  = peano4.solversteps.Step( "EnumerateAndInitSolution",  False )
    self.algorithm_step_solve                        = peano4.solversteps.Step( "Solve",                     False )
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

    """
    self._project.main = Main(self._project,
                              self._domain_offset,
                              self._domain_size,
                              )

    self._project.solversteps.add_step(self.algorithm_step_create_grid)
    self._project.solversteps.add_step(self.algorithm_step_enumerate_and_init_solution)
    self._project.solversteps.add_step(self.algorithm_step_solve)
    self._project.solversteps.add_step(self.algorithm_step_plot)

    self.__configure_makefile()
    self.__generate_solver_repository()

    self._project.output.readme.add_package_description( """


""" )

    return self._project

