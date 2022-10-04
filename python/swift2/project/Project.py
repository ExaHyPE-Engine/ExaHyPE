# This file is part of the ExaHyPE2 project. For conditions of distribution and 
# use, please see the copyright notice at www.peano-framework.org
import peano4
import peano4.output.Jinja2TemplatedHeaderImplementationFilePair

import dastgen2

import os
import sys
import swift2


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
               directory, 
               executable):
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
    self._plot_filters  = []
    self._output_path   = "./"
    self._plotter_precision = 5
    self._min_h         = 0.01
    self._max_h         = 0.1

    #self._particle_species = []
 
    self._algorithm_step_create_grid    = peano4.solversteps.Step( "CreateGrid", False )
    self._algorithm_step_init           = peano4.solversteps.Step( "Init",       False )
    self._algorithm_step_plot           = peano4.solversteps.Step( "Plot",       False )
        
    
  def  set_load_balancing(self, load_balancer_name, load_balancer_arguments = ""):
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


  #def add_solver(self,solver):
  #  self._solvers.append( solver )


  def add_plot_filter(self,offset,size,frequency=1):
    """
    
    Add a new filter to Peano/ExaHyPE
    
    offset: (float,float,float)
    
    size: (float,float,float)
    
    frequency: int
      A positive value. Peano makes snapshots every dt simulation
      units. This is something you specify once per simulation. But
      you might decide only to splot every k of these snapshots.
    
    """  
    new_entry = "{{"
    new_entry += str(offset[0])
    for i in offset[1:]:
      new_entry += ","
      new_entry += str(i)
    new_entry += "},{"
    new_entry += str(size[0])
    for i in size[1:]:
      new_entry += ","
      new_entry += str(i)
    new_entry += "},"
    new_entry += str(frequency)
    new_entry += "}"
    
    self._plot_filters.append( new_entry )

  
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
    
    self._project.constants.export( "MaxH", str(self._max_h) )
    self._project.constants.export( "MinH", str(self._min_h) )

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

    #for i in self._solvers:
    #  self._project.output.readme.add_entry( i.create_readme_descriptor(self._domain_offset[0],self._domain_size[0]) ) 


  def __configure_makefile(self):
    self._project.output.makefile.set_dimension(self._dimensions)
    self._project.output.makefile.set_executable_name(self._executable_name)


  def set_global_simulation_parameters(self,
                                       dimensions,
                                       offset,size,
                                       max_h,min_h,
                                       min_end_time,
                                       first_plot_time_stamp,
                                       time_in_between_plots, 
                                       periodic_BC = [False, False, False], 
                                       plotter_precision=5,
                                       max_end_time=0.0,
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
    assert max_h>=min_h
    self._max_h         = max_h
    self._min_h         = min_h
    self._domain_offset = offset
    self._domain_size   = size
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
    
    
  def __generate_solver_repository(self):
    solverRepositoryDictionary = {
      "SOLVERS" : [],
      "LOAD_BALANCER" : self._load_balancer_name,
      "LOAD_BALANCER_ARGUMENTS" : self._load_balancer_arguments,
      "PLOT_FILTER": self._plot_filters
    }

    #for solver in self._solvers:
    #  solverRepositoryDictionary[ "SOLVERS" ].append( (solver._name,solver.get_name_of_global_instance()) )
      

    #templatefile_prefix = os.path.realpath(__file__).replace( ".pyc", "" ).replace( ".py", "" )
    #generated_solver_files = peano4.output.Jinja2TemplatedHeaderImplementationFilePair(
    #  templatefile_prefix + "SolverRepository.template.h",
    #  templatefile_prefix + "SolverRepository.template.cpp",
    #  "SolverRepository", 
    #  self._project.namespace + ["repositories"],
    #  "repositories", 
    #  solverRepositoryDictionary,
    #  True)

    #self._project.output.add( generated_solver_files )
    #self._project.output.makefile.add_cpp_file( "repositories/SolverRepository.cpp" )
   


  #def add_action_set_to_timestepping(self, action_set):
  #  self.perform_time_step.add_action_set( action_set )


  def add_action_set_to_initialisation(self, action_set):
    self._algorithm_step_init.add_action_set( action_set )

        
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
     
    """
    self._project.solversteps.add_step(self._algorithm_step_create_grid)
    self._project.solversteps.add_step(self._algorithm_step_init)
    self._project.solversteps.add_step(self._algorithm_step_plot)
   
    self.__generate_solver_repository();
    
    self._project.main = swift2.project.SWIFTMain(self._project, self.main_file_template() )

    self.__configure_makefile()
    self._project.output.makefile.parse_configure_script_outcome( self._Peano_src_directory )
    self.__export_constants()

    self._project.output.makefile.set_mode(self._build_mode)

    return self._project

    
  @abstractmethod
  def main_file_template(self):
    assert False, "should be implemented by subclass"
    pass
