# This file is part of the ExaHyPE2 project. For conditions of distribution and 
# use, please see the copyright notice at www.peano-framework.org
import peano4
import peano4.output.Jinja2TemplatedHeaderImplementationFilePair

import dastgen2

import os
import sys

from petsc.PETScMain import PETScMain




class Project(object):
  """
   
  Represents a multigrid solver project
  
  
  
  ## Algorithmic steps
  
  Every PETSc project runs through the following steps:
 
  - algorithm_step_create_grid
  - algorithm_step_enumerate
  - _algorithm_step_init
  - algorithm_step_assemble
  - _algorithm_step_solve
  - algorithm_step_map_solution_onto_mesh
  - algorithm_step_plot
  
  Each of these steps is mapped onto one observer. It is clear that not all
  steps actually lead to mesh traversals and, therefore, are not to be modified
  by users. They are simply there to allow the main function to pick a step and
  to tell all MPI ranks to run the same step on all local trees. I therefore 
  mark these steps as private via a leading underscore.
  
  
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

    self.algorithm_step_create_grid             = peano4.solversteps.Step( "CreateGrid",          False )
    self.algorithm_step_enumerate               = peano4.solversteps.Step( "Enumerate",           False )
    self._algorithm_step_init                   = peano4.solversteps.Step( "Init",                False )
    self.algorithm_step_assemble                = peano4.solversteps.Step( "Assemble",            False )
    self._algorithm_step_solve                  = peano4.solversteps.Step( "Solve",               False )
    self.algorithm_step_map_solution_onto_mesh  = peano4.solversteps.Step( "MapSolutionOntoMesh", False )
    self.algorithm_step_plot                    = peano4.solversteps.Step( "Plot",                False )
                
#    self.algorithm_step_plot.add_action_set( peano4.toolbox.PlotGridInPeanoBlockFormat( 
#      filename              = "grid", 
#      time_stamp_evaluation = "repositories::getMinTimeStamp()",
#      additional_includes   = """
##include "repositories/GlobalState.h"
#"""      
#    ))


    
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
    """
      src_path: string
        Path (relative or absolute) to the src directory of Peano. This path 
        should hold both the headers (in subdirectories) and all the static
        libraries.
        
      mode: peano4.output.CompileMode
    """
    self._Peano_src_directory = src_path
    self._build_mode          = mode


  def __configure_makefile(self):
    self._project.output.makefile.set_dimension(self._dimensions)
    self._project.output.makefile.set_executable_name(self._executable_name)


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
    """
    
     Build the Peano4 project, i.e. all the action sets et al that you require
     to run this Peano4 application with PETSc. 
     
     This routine generates a Peano project, i.e. the domain-specific 
     view is translated into a Peano model. Once you have called this routine,
     any changes to the PETSc project configuration do not propagate into the Peano
     setup anymore. If you alter the project etup, you have to call 
     generate_Peano4_project() again to get a new snapshot/version of the
     Peano setup.
          
    """
    self._project.main = PETScMain(self._project,
                                   self._domain_offset,
                                   self._domain_size
                                   )


    #
    #  
    #  self._project.datamodel.add_global_object(current_species_set.particle_model)
    #  self._project.datamodel.add_vertex(current_species_set)#

    #  self._algorithm_step_create_grid.use_vertex(current_species_set)
    #  self.algorithm_step_init.use_vertex(current_species_set)
    #  self.algorithm_step_sort.use_vertex(current_species_set)
    #  self.algorithm_step_plot.use_vertex(current_species_set)

      # Enable cell statistics for this species
    #  self._project.datamodel.add_cell(cell_marker_for_statistics)   
      
      # Add cell statistics to these action steps
    #  self._algorithm_step_create_grid.use_cell(cell_marker_for_statistics)
    #  self.algorithm_step_init.use_cell(cell_marker_for_statistics)
    #  self.algorithm_step_sort.use_cell(cell_marker_for_statistics)
    #  self.algorithm_step_plot.use_cell(cell_marker_for_statistics)
    #  for solverstep in solversteps:
    #    solverstep.use_vertex(current_species_set)
    #    solverstep.use_cell(cell_marker_for_statistics)

    self._project.solversteps.add_step(self.algorithm_step_create_grid)
    self._project.solversteps.add_step(self.algorithm_step_enumerate)
    self._project.solversteps.add_step(self._algorithm_step_init)
    self._project.solversteps.add_step(self.algorithm_step_assemble)
    self._project.solversteps.add_step(self._algorithm_step_solve)
    self._project.solversteps.add_step(self.algorithm_step_map_solution_onto_mesh)
    self._project.solversteps.add_step(self.algorithm_step_plot)

    self.__configure_makefile()
    self._project.output.makefile.parse_configure_script_outcome( self._Peano_src_directory )

    self._project.output.makefile.set_mode(self._build_mode)

    self._project.output.readme.add_package_description( """

### Peano4 PETSc backend

This yet has to be written.

""" )

    return self._project


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
    solver.add_use_statements(self.algorithm_step_enumerate)
    solver.add_use_statements(self._algorithm_step_init)
    solver.add_use_statements(self.algorithm_step_assemble)
    solver.add_use_statements(self.algorithm_step_map_solution_onto_mesh)
    solver.add_use_statements(self.algorithm_step_plot)

    solver.add_to_create_grid           (self.algorithm_step_create_grid)
    solver.add_to_enumerate             (self.algorithm_step_enumerate)
    solver.add_to_init                  (self._algorithm_step_init)
    solver.add_to_assemble              (self.algorithm_step_assemble)
    solver.add_to_map_solution_onto_mesh(self.algorithm_step_map_solution_onto_mesh)
    solver.add_to_plot                  (self.algorithm_step_plot)

    solver.add_implementation_files_to_project( self._project.namespace, self._project.output )
    pass


