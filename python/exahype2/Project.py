# This file is part of the ExaHyPE2 project. For conditions of distribution and 
# use, please see the copyright notice at www.peano-framework.org
import peano4
import peano4.output.Jinja2TemplatedHeaderImplementationFilePair


import os

import exahype2.grid
import exahype2.solvers



class Project(object):
  """
   
  Represents an ExaHyPE2 project. An ExaHyPE2 project is a Peano4 
  project with a particular set of actions (algorithmic phases) 
  that you can choose from and with particular solver types. It 
  realises a builder mechanism, i.e. you build up your ExaHyPE2
  project and then you finally tell the project "give me the Peano4
  project". From hereon, you can use this Peano4 project to actually
  set up the Peano4 application.
  
  The project will have a marker per cell that encodes stuff alike
  a boundary marker. But it is also used to coordinate different 
  solver types.
  
  @see generate_Peano4_project()
  
  """
  def __init__(self, namespace, project_name, directory = "."):
    self._project = peano4.Project(namespace, project_name, directory)
    self._solvers = []
    self._domain_offset = [0.0, 0.0, 0.0]
    self._domain_size   = [1.0, 1.0, 1.0]
    self._dimensions    = 2
    self._terminal_time = 1.0
    self._first_plot_time_stamp   = 0.0
    self._time_in_between_plots   = 0.1
    self._load_balancer_name      = ""
    self._load_balancer_arguments = ""
    self._Peano_src_directory = "."
    self._build_mode          = peano4.output.CompileMode.Asserts
    
    
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
    
    
  LibraryDebug   = "_debug"
  LibraryRelease = ""
  LibraryTrace   = "_trace"
  LibraryAsserts = "_asserts"
    
    
  def set_Peano4_installation(self, src_path, mode ):
    """
      src_path: string
        Path (relative or absolute) to the src directory of Peano. This path 
        should hold both the headers (in subdirectories) and all the static
        libraries.
        
      mode: peano4.output.CompileMode
    """
    self._Peano_src_directory = src_path
    self._build_mode          = mode


  def __get_library_postfix( self ):
    """
    
    Libraries in ExaHyPE 2 always end with 2d_debug or 3d_trace. This 
    operation gives you back the respective postfix. There are two 
    variants of this routine. One accepts a dimension, the other one
    does not, i.e. accepts None. This is important as some libraries are dimension-generic
    while others are not. Therefore, some have the 2d/3d postfix and 
    others lack it.

    mode is from peano4.output.CompileMode
    dimensions either None or something smaller zero (takes project's
      dimension) or a particular dimension
    
    """
    result = ""
    result += str(self._dimensions) + "d"

    if self._build_mode==peano4.output.CompileMode.Debug:
        result += self.LibraryDebug
    if self._build_mode==peano4.output.CompileMode.Trace:
        result += self.LibraryTrace
    if self._build_mode==peano4.output.CompileMode.Asserts:
        result += self.LibraryAsserts
    if self._build_mode==peano4.output.CompileMode.Release:
        result += self.LibraryRelease
    return result


  def __get_ExaHyPE_library( self ):
    """

    Returns the core ExaHyPE library against which to link
         
    mode is from peano4.output.CompileMode
    
    """
    return "ExaHyPE2Core" + self.__get_library_postfix()


  def __get_load_balancing_library( self ):
    """

    Returns the core ExaHyPE library against which to link
         
    mode is from peano4.output.CompileMode
    
    """
    return "ToolboxLoadBalancing" + self.__get_library_postfix()


  def add_solver(self,solver):
    self._solvers.append( solver )


  def __export_constants(self):
    offset_string = "{" + str(self._domain_offset[0])
    size_string   = "{" + str(self._domain_size[0])
    for i in range(1,self._dimensions):
      offset_string += ","
      size_string   += ","
      offset_string += str(self._domain_offset[i])
      size_string   += str(self._domain_size[i])
    offset_string += "}"
    size_string   += "}"
    self._project.constants.export( "DomainOffset", offset_string )
    self._project.constants.export( "DomainSize", size_string )
    self._project.constants.export( "TerminalTime", str(self._terminal_time) )
    self._project.constants.export( "FirstPlotTimeStamp", str(self._first_plot_time_stamp) )
    self._project.constants.export( "TimeInBetweenPlots", str(self._time_in_between_plots) )


  def __configure_makefile(self):
    self._project.output.makefile.set_dimension(self._dimensions)
    

  def set_global_simulation_parameters(self,dimensions,offset,size,end_time,first_plot_time_stamp,time_in_between_plots):
    """
    
      offset and size should be lists with dimensions double entries.
      
      first_plot_time_stamp Is irrelevant if time_in_between_plots equals zero
      time_in_between_plots Set to zero if you don't want to have any plots
      
    """
    self._domain_offset = offset
    self._domain_size   = size
    self._dimensions    = dimensions
    self._terminal_time = end_time
    self._first_plot_time_stamp = first_plot_time_stamp
    self._time_in_between_plots = time_in_between_plots
    
    
  def __generate_solver_repository(self):
    solverRepositoryDictionary = {
      "SOLVERS" : [],
      "LOAD_BALANCER" : self._load_balancer_name,
      "LOAD_BALANCER_ARGUMENTS" : self._load_balancer_arguments
    }

    for solver in self._solvers:
      solverRepositoryDictionary[ "SOLVERS" ].append( (solver._name,solver.get_name_of_global_instance()) )
      

    templatefile_prefix = os.path.realpath(__file__).replace( ".pyc", "" ).replace( ".py", "" )
    generated_solver_files = peano4.output.Jinja2TemplatedHeaderImplementationFilePair(
      templatefile_prefix + "SolverRepository.template.h",
      templatefile_prefix + "SolverRepository.template.cpp",
      "SolverRepository", 
      self._project.namespace + ["observers"],
      "observers", 
      solverRepositoryDictionary,
      True)

    self._project.output.add( generated_solver_files )
    self._project.output.makefile.add_cpp_file( "observers/SolverRepository.cpp" )
    
    
  def generate_Peano4_project(self):
    """
    
     Build the Peano4 project, i.e. all the action sets et al that you require
     to run this ExaHyPE2 application. 
     
     
     You will have to add the ExaHyPE2 library later on manually. I would like 
     to do this here, but most codes first want to parse the config file before
     they add the library. And, obviously, you still have to make the choice 
     which variant of the library (release, trace, debug, ...) you want to use.

     So most codes invoke something alike
     
     
     on the result of this routine.     
     
    """
    self.__export_constants()
    self.__configure_makefile()
    
    create_grid                   = peano4.solversteps.Step( "CreateGrid", False )
    init_grid                     = peano4.solversteps.Step( "InitGrid", False )
    create_grid_but_postpone_refinement = peano4.solversteps.Step( "CreateGridButPostponeRefinement", False )
    plot_solution                 = peano4.solversteps.Step( "PlotSolution", False )
    perform_time_step             = peano4.solversteps.Step( "TimeStep", False )
    
    self._project.solversteps.add_step(create_grid)
    self._project.solversteps.add_step(init_grid)
    self._project.solversteps.add_step(create_grid_but_postpone_refinement)
    self._project.solversteps.add_step(plot_solution)
    self._project.solversteps.add_step(perform_time_step)
    
    for solver in self._solvers:
      solver.add_to_Peano4_datamodel( self._project.datamodel )
      
      solver.add_use_data_statements_to_Peano4_solver_step( create_grid )
      solver.add_use_data_statements_to_Peano4_solver_step( plot_solution )
      solver.add_use_data_statements_to_Peano4_solver_step( perform_time_step )
      solver.add_use_data_statements_to_Peano4_solver_step( init_grid )
      solver.add_use_data_statements_to_Peano4_solver_step( create_grid_but_postpone_refinement )
      
      solver.add_actions_to_create_grid( create_grid, evaluate_refinement_criterion=True )
      solver.add_actions_to_create_grid( init_grid, evaluate_refinement_criterion=False )
      solver.add_actions_to_create_grid( create_grid_but_postpone_refinement, evaluate_refinement_criterion=False )
      solver.add_actions_to_plot_solution( plot_solution )
      solver.add_actions_to_perform_time_step( perform_time_step )
      
      solver.add_implementation_files_to_project( self._project.namespace, self._project.output )

    self.__generate_solver_repository();

    face_label = exahype2.grid.create_face_label()  
    self._project.datamodel.add_face(face_label)
    create_grid.use_face(face_label)
    init_grid.use_face(face_label)
    create_grid_but_postpone_refinement.use_face(face_label)
    plot_solution.use_face(face_label)
    perform_time_step.use_face(face_label)
    
    set_labels_action_set = exahype2.grid.SetLabels()
    create_grid.add_action_set( set_labels_action_set )
    init_grid.add_action_set( set_labels_action_set )
    create_grid_but_postpone_refinement.add_action_set( set_labels_action_set )
    plot_solution.add_action_set( set_labels_action_set )
    perform_time_step.add_action_set( set_labels_action_set )
    
    self._project.main = exahype2.ExaHyPEMain(self._project)

    # maybe use ..
    self._project.output.makefile.parse_configure_script_outcome( self._Peano_src_directory )
    self._project.output.makefile.add_library( self.__get_ExaHyPE_library(),        self._Peano_src_directory + "/src/exahype2" )
    self._project.output.makefile.add_library( self.__get_load_balancing_library(), self._Peano_src_directory + "/src/toolbox/loadbalancing" )
    self._project.output.makefile.set_mode(self._build_mode)

    return self._project

    
