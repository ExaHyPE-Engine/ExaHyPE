# This file is part of the ExaHyPE2 project. For conditions of distribution and 
# use, please see the copyright notice at www.peano-framework.org
import peano4
import peano4.output.Jinja2TemplatedHeaderImplementationFilePair

import dastgen2

import os
import sys

from swift2.particle.Particle         import Particle
from swift2.actionsets.DynamicMeshRefinementAnalysis import DynamicMeshRefinementAnalysis
from swift2.graphcompiler.Sequential  import serialise_and_map_onto_separate_mesh_traversals
from swift2.output.SWIFTMain          import SWIFTMain


from abc import abstractmethod


class Project(object):
  """
   
  Represents an abstract SWIFT 2 project. An SWIFT 2 project is a Peano 4 
  project with a particular set of actions (algorithmic phases) 
  that you can choose from and with particular solver types. It 
  realises a builder mechanism, i.e. you build up your SWIFT 2
  project and then you finally tell the project "give me the Peano 4
  project". From hereon, you can use this Peano 4 project to actually
  set up the Peano 4 application.
  
  Please do not use this class directly. Use one of its subclasses.
  
  @see generate_Peano4_project()
  
  """
  def __init__(self, 
               namespace, 
               project_name, 
               directory  = ".", 
               executable = "swift2"):
    self._project = peano4.Project(namespace, project_name, directory)
    
    self._domain_offset = [0.0, 0.0, 0.0]
    self._domain_size   = [1.0, 1.0, 1.0]
    self._dimensions    = 2
    self._min_terminal_time = 1.0
    self._max_terminal_time = 0.0
    self._first_plot_time_stamp   = 0.0
    self._time_in_between_plots   = 0.1
    self._load_balancer_name      = ""
    self._load_balancer_arguments = ""
    self._Peano_src_directory = "."
    self._build_mode          = peano4.output.CompileMode.Asserts
    self._executable_name = executable
    self._periodic_BC   = [False, False, False]
    self._output_path   = "./"

    self._particle_species     = []
    self._particle_species_set = []
 
    self._algorithm_step_create_grid   = peano4.solversteps.Step( "CreateGrid", False )
    self.algorithm_step_init           = peano4.solversteps.Step( "Init",       False )
    self.algorithm_step_sort           = peano4.solversteps.Step( "Sort",       False )
    self.algorithm_step_plot           = peano4.solversteps.Step( "Plot",       False )
                
    self.algorithm_step_plot.add_action_set( peano4.toolbox.PlotGridInPeanoBlockFormat( 
      filename              = "grid", 
      time_stamp_evaluation = "repositories::getMinTimeStamp()",
      additional_includes   = """
#include "repositories/GlobalState.h"
"""      
    ))
    
    self.task_graph_compiler = serialise_and_map_onto_separate_mesh_traversals


    
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


  def __compute_global_max_h(self):
    self._global_max_h = 0.0
    for current_species_set in self._particle_species_set:
      self._global_max_h = max( self._global_max_h, current_species_set.particle_model.max_h )
  

  def __export_constants(self):
    """
    
    We export SWIFT's constants. Besides the constants from SWIFT,
    I also export some parameters from Peano onto the SWIFT constants
    file. Therefore, it is important that you parse the configure output
    before we export the constants.
    
    """
    self._project.constants.clear()
    offset_string = "{" + str(self._domain_offset[0])
    size_string   = "{" + str(self._domain_size[0])
    for i in range(1,self._dimensions):
      offset_string += ","
      size_string   += ","
      offset_string += str(self._domain_offset[i])
      size_string   += str(self._domain_size[i])
    offset_string += "}"
    size_string   += "}"
    self._project.constants.add_include( """#include <bitset>""") 
    self._project.constants.export_const_with_type( "DomainOffset", offset_string, "std::initializer_list<double>" )
    self._project.constants.export_const_with_type( "DomainSize", size_string, "std::initializer_list<double>" )
    self._project.constants.export( "MinTerminalTime", str(self._min_terminal_time) )
    if self._max_terminal_time>=self._min_terminal_time:
      self._project.constants.export( "MaxTerminalTime", str(self._max_terminal_time) )
    else:
      self._project.constants.export( "MaxTerminalTime", "std::numeric_limits<double>::max()" )
    self._project.constants.export( "FirstPlotTimeStamp", str(self._first_plot_time_stamp) )
    self._project.constants.export( "TimeInBetweenPlots", str(self._time_in_between_plots) )
    self._project.constants.export( "PlotterPrecision", str(self._plotter_precision) )
    self._project.constants.export_boolean_sequence( "PeriodicBC", self._periodic_BC )
    self._project.constants.export( "GlobalMaxH", str(self._global_max_h) )

    build_string = "python3 "
    for i in sys.argv:
      build_string += " "
      build_string += i
    self._project.constants.export_string( "BuildInformation",     build_string )
    self._project.constants.export_string( "ConfigureInformation", self._project.output.makefile.configure_call )
    
    self._project.output.readme.add_package_description( """

### SWIFT 2 

This code uses the second generation of the SWIFT code controlled through 
its Python API. Under the hood it uses Peano 4. We do not yet have a release 
paper for this second generation of SWIFT yet, and  thus appreciate any 
citation of the original SWIFT paper
                                                                            
       @article{Schaller:2020:SWIFT,   
         title = {t.b.d.}   
       } 


""" )


  def __configure_makefile(self):
    self._project.output.makefile.set_dimension(self._dimensions)
    self._project.output.makefile.set_executable_name(self._executable_name)


  def set_global_simulation_parameters(self,
                                       dimensions,
                                       offset,
                                       domain_size,
                                       min_end_time,
                                       max_end_time,
                                       first_plot_time_stamp,
                                       time_in_between_plots, 
                                       periodic_BC       = [False, False, False], 
                                       plotter_precision = 5,
                                       ):
    """
    
      offset and size should be lists with dimensions double entries.
      
      first_plot_time_stamp: Float
        Is irrelevant if time_in_between_plots equals zero
  
      time_in_between_plots: Float 
        Set to zero if you don't want to have any plots
        
      max_end_time: Float
        If you set it zero (or actually any value msmaller than min_end_time), then 
        the code will run until the cell that lags behind the most hits the min time.
        If you specify a valid max time however, you can stop the sim as soon as the
        most advanced cell exceeds this threshold.
      
    """
    self._domain_offset = offset
    self._domain_size   = domain_size
    self._dimensions    = dimensions
    self._max_terminal_time = max_end_time
    self._min_terminal_time = min_end_time
    self._first_plot_time_stamp = first_plot_time_stamp
    self._time_in_between_plots = time_in_between_plots
    self._periodic_BC = []
    self._plotter_precision = plotter_precision
    for d in range(0,dimensions):
      self._periodic_BC.append( periodic_BC[d] )
    if plotter_precision<=0:
      raise Exception( "Plotter precision has to be bigger than 0" ) 
    
    
  def  set_output_path(self,path):
    self._output_path = path
    if not self._output_path.endswith( "/" ):
      self._output_path += "/"
    
    
  def __generate_global_state_files(self):
    solverRepositoryDictionary = {
      "PARTICLE_SPECIES" : [x.name for x in self._particle_species],
      "LOAD_BALANCER" : self._load_balancer_name,
      "LOAD_BALANCER_ARGUMENTS" : self._load_balancer_arguments,
    }

    templatefile_prefix = os.path.dirname(__file__)
    generated_solver_files = peano4.output.Jinja2TemplatedHeaderImplementationFilePair(
      templatefile_prefix + "/output/GlobalState.template.h",
      templatefile_prefix + "/output/GlobalState.template.cpp",
      "GlobalState", 
      self._project.namespace + ["repositories"],
      "repositories", 
      solverRepositoryDictionary,
      True)

    self._project.output.add( generated_solver_files )
    self._project.output.makefile.add_cpp_file( "repositories/GlobalState.cpp" )
   

  def add_particle_species(self, particle: Particle ):
    """
    
    Add a new particle species (type) to the project. You get the container
    back in which the particles are handled on the rank. Use this one where
    another action set requires a particle set.
    
    """
    self._particle_species.append( particle )
    self._particle_species_set.append( peano4.toolbox.particles.ParticleSet(particle) )    
    return self._particle_species_set[-1]
      
        
  def generate_Peano4_project(self, verbose=False):
    """
    
     Build the Peano4 project, i.e. all the action sets et al that you require
     to run this ExaHyPE2 application. 
     
     This routine generates a Peano project, i.e. the domain-specific ExaHyPE 
     view is translated into a Peano model. Once you have called this routine,
     any changes to the ExaHyPE 2 configuration do not propagate into the Peano
     setup anymore. If you alter the ExaHyPE setup, you have to call 
     generate_Peano4_project() again to get a new snapshot/version of the
     Peano setup.
     
     
     ## Task graph compiler
     
     The core contribution of the generation is the task graph compiler which 
     really is a mapping of algorithm steps per particle species onto grid 
     sweeps. The actual mapping is outsourced into the function represented
     by self.task_graph_compiler. This way, users can switch the translator's
     behaviour. This function returns a sequence of mesh traversals. On top 
     of that, we have the default traversals to create a grid, to plot it, and
     to initialise the setup.
     
     Once we have our three default steps plus a sequence of algorithmic steps
     per time step, we run through the following steps:
     
     - Create a particle set around each species and add it to the project as
       global variable. This is the global container administering these guys.
     - Tell each algorithmic step to use this particle set. An exception could be
       the grid creation. At this point, we don't have particles yet. We add
       the data structure nevertheless. It ensures that we have all the data
       in place, and it we also then can be sure that everything is properly
       initialised. The actual particle set will be empty at this point of the
       simulation.
     - Ensure that plotting and initialisation use the update particle-grid
       association properly. The plot has to update it, as previous steps 
       might have started a resort yet might not have finished it.
     - Add all the algorithmic steps, including the default ones, to
       the project.

       
    ## Particle initialisation and proper sorting
    
    The particle initialisation might end up with an invalid association of 
    particles to vertices. The graph compiler might make the first step of a
    time step sequence sort if and only if the last one has altered the 
    particles' position. Consequently, we might end up with an initialisation
    which yields an inconsistent data association. We therefore make it sort
    the particles, but we need another grid sweep to finalise this sort in 
    case we have to do some global rearrangements. This is our rationale why
    we realise the initialisation in two steps.
     
    """
    self.__generate_global_state_files()
    self.__compute_global_max_h()

    ## Make pre-defined mesh traversals resort the particles. For this, they need 
    ## species set into
    for current_species_set in self._particle_species_set:
      self.algorithm_step_plot.add_action_set(peano4.toolbox.particles.UpdateParticleGridAssociation_LiftDrop(current_species_set),False)
      self.algorithm_step_init.add_action_set(peano4.toolbox.particles.UpdateParticleGridAssociation_LiftDrop(current_species_set),False)
      self.algorithm_step_sort.add_action_set(peano4.toolbox.particles.UpdateParticleGridAssociation_LiftDrop(current_species_set),False)
      
      self.algorithm_step_plot.add_action_set(peano4.toolbox.particles.UpdateParallelState(current_species_set),False)
      self.algorithm_step_init.add_action_set(peano4.toolbox.particles.UpdateParallelState(current_species_set),False)
      self.algorithm_step_sort.add_action_set(peano4.toolbox.particles.UpdateParallelState(current_species_set),False)

    ## Create mesh traversals aka solver steps
    solversteps = self.task_graph_compiler(self._particle_species_set)

    ## Make each solver step (incl predefined ones) use the particles and particle sets
    for current_species_set in self._particle_species_set:
      cell_marker_for_statistics = DynamicMeshRefinementAnalysis.create_cell_marker(current_species_set.name)
      
      self._project.datamodel.add_global_object(current_species_set.particle_model)
      self._project.datamodel.add_vertex(current_species_set)

      self._algorithm_step_create_grid.use_vertex(current_species_set)
      self.algorithm_step_init.use_vertex(current_species_set)
      self.algorithm_step_sort.use_vertex(current_species_set)
      self.algorithm_step_plot.use_vertex(current_species_set)

      # Enable cell statistics for this species
      self._project.datamodel.add_cell(cell_marker_for_statistics)   
      
      # Add cell statistics to these action steps
      self._algorithm_step_create_grid.use_cell(cell_marker_for_statistics)
      self.algorithm_step_init.use_cell(cell_marker_for_statistics)
      self.algorithm_step_sort.use_cell(cell_marker_for_statistics)
      self.algorithm_step_plot.use_cell(cell_marker_for_statistics)
      for solverstep in solversteps:
        solverstep.use_vertex(current_species_set)
        solverstep.use_cell(cell_marker_for_statistics)

    self._algorithm_step_create_grid.add_action_set(peano4.toolbox.CreateRegularGrid(self._global_max_h),False)
     
    self._project.solversteps.add_step(self._algorithm_step_create_grid)
    self._project.solversteps.add_step(self.algorithm_step_init)
    self._project.solversteps.add_step(self.algorithm_step_sort)
    self._project.solversteps.add_step(self.algorithm_step_plot)
    for solverstep in solversteps:
      self._project.solversteps.add_step(solverstep)
    
    self._project.main = SWIFTMain( self._project, solversteps )

    self.__configure_makefile()
    self._project.output.makefile.parse_configure_script_outcome( self._Peano_src_directory )
    self.__export_constants()

    self._project.output.makefile.set_mode(self._build_mode)

    return self._project

    
  @abstractmethod
  def main_file_template(self):
    assert False, "should be implemented by subclass"
    pass
