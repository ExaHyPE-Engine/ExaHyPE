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
  def __init__(self, namespace, project_name, directory = ".", executable="peano4"):
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
    self._executable_name = executable
    self._periodic_BC   = [False, False, False]
    self._plot_filters  = []
    self._output_path   = "./"
    
    
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


  def add_solver(self,solver):
    self._solvers.append( solver )


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

  
  def remove_all_solvers(self):
    self._solvers = []
    self._project.cleanup()
    

  def __export_constants(self):
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
    self._project.constants.export( "TerminalTime", str(self._terminal_time) )
    self._project.constants.export( "FirstPlotTimeStamp", str(self._first_plot_time_stamp) )
    self._project.constants.export( "TimeInBetweenPlots", str(self._time_in_between_plots) )
    
    self._project.constants.export_boolean_sequence( "PeriodicBC", self._periodic_BC )
    

  def __configure_makefile(self):
    self._project.output.makefile.set_dimension(self._dimensions)
    self._project.output.makefile.set_executable_name(self._executable_name)


  def set_global_simulation_parameters(self,dimensions,offset,size,end_time,first_plot_time_stamp,time_in_between_plots, periodic_BC = [False, False, False]):
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
    self._periodic_BC = []
    for d in range(0,dimensions):
      self._periodic_BC.append( periodic_BC[d] )
    
    
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

    for solver in self._solvers:
      solverRepositoryDictionary[ "SOLVERS" ].append( (solver._name,solver.get_name_of_global_instance()) )
      

    templatefile_prefix = os.path.realpath(__file__).replace( ".pyc", "" ).replace( ".py", "" )
    generated_solver_files = peano4.output.Jinja2TemplatedHeaderImplementationFilePair(
      templatefile_prefix + "SolverRepository.template.h",
      templatefile_prefix + "SolverRepository.template.cpp",
      "SolverRepository", 
      self._project.namespace + ["repositories"],
      "repositories", 
      solverRepositoryDictionary,
      True)

    self._project.output.add( generated_solver_files )
    self._project.output.makefile.add_cpp_file( "repositories/SolverRepository.cpp" )
    
    
  def generate_Peano4_project(self, verbose=False):
    """
    
     Build the Peano4 project, i.e. all the action sets et al that you require
     to run this ExaHyPE2 application. 
     
     To get all the pathes right, please invoke set_Peano4_installation() prior
     to this call.
     
     
     !!! create_grid
     
     
     !!! create_grid_but_postpone_refinement
     
     The same as create_grid, but this traversal type does not evaluate the AMR 
     criteria. The rationale is that a create_grid call might add quite a lot
     of mesh elements and consequently require some rebalancing. This is 
     expensive memory- and time-wisely and might trigger follow-up rebalancing.
     If we ran the next mesh refinement immediately afterwards, we would likely
     run out of memory at one point and the mesh construction would last 
     forever. With create_grid_but_postpone_refinement, we give the main code
     the opportunity to insert a few "empty"ish traversals in-between.
     
    """
    self._project.cleanup()

    self.__export_constants()
    self.__configure_makefile()
    
    create_grid                         = peano4.solversteps.Step( "CreateGrid", False )
    init_grid                           = peano4.solversteps.Step( "InitGrid",   False )
    create_grid_but_postpone_refinement = peano4.solversteps.Step( "CreateGridButPostponeRefinement", False )
    create_grid_and_converge_lb         = peano4.solversteps.Step( "CreateGridAndConvergeLoadBalancing", False )
    plot_solution                       = peano4.solversteps.Step( "PlotSolution", False )
    perform_time_step                   = peano4.solversteps.Step( "TimeStep",     False )
        
    self._project.solversteps.add_step(create_grid)
    self._project.solversteps.add_step(init_grid)
    self._project.solversteps.add_step(create_grid_but_postpone_refinement)
    self._project.solversteps.add_step(create_grid_and_converge_lb)
    self._project.solversteps.add_step(plot_solution)
    self._project.solversteps.add_step(perform_time_step)
   
    for solver in self._solvers:
      print( "---------------------------------------")
      print( "Create data for solver " +solver._name  )
      print( "---------------------------------------")
      print( str(solver) )
      
      solver.add_to_Peano4_datamodel( self._project.datamodel, verbose )
      
      solver.add_use_data_statements_to_Peano4_solver_step( create_grid )
      solver.add_use_data_statements_to_Peano4_solver_step( plot_solution )
      solver.add_use_data_statements_to_Peano4_solver_step( perform_time_step )
      solver.add_use_data_statements_to_Peano4_solver_step( init_grid )
      solver.add_use_data_statements_to_Peano4_solver_step( create_grid_but_postpone_refinement )
      solver.add_use_data_statements_to_Peano4_solver_step( create_grid_and_converge_lb )
      
      solver.add_actions_to_create_grid( create_grid,                         evaluate_refinement_criterion=True  )
      solver.add_actions_to_init_grid( init_grid )
      solver.add_actions_to_create_grid( create_grid_but_postpone_refinement, evaluate_refinement_criterion=False )
      solver.add_actions_to_create_grid( create_grid_and_converge_lb,         evaluate_refinement_criterion=False )
      solver.add_actions_to_plot_solution( plot_solution, self._output_path )
      solver.add_actions_to_perform_time_step( perform_time_step )
      
      solver.add_implementation_files_to_project( self._project.namespace, self._project.output )
      

    self.__generate_solver_repository();

    face_label = exahype2.grid.create_face_label()  
    self._project.datamodel.add_face(face_label)
    
    create_grid.use_face(face_label)
    init_grid.use_face(face_label)
    create_grid_but_postpone_refinement.use_face(face_label)
    create_grid_and_converge_lb.use_face(face_label)
    plot_solution.use_face(face_label)
    perform_time_step.use_face(face_label)
    
    set_labels_action_set = exahype2.grid.SetLabels()

    create_grid.add_action_set( set_labels_action_set )
    init_grid.add_action_set( set_labels_action_set )
    create_grid_but_postpone_refinement.add_action_set( set_labels_action_set )
    create_grid_and_converge_lb.add_action_set( set_labels_action_set )
    plot_solution.add_action_set( set_labels_action_set )
    perform_time_step.add_action_set( set_labels_action_set )
    
    self._project.main = exahype2.ExaHyPEMain(self._project,self._periodic_BC,self._dimensions)

    # maybe use ..
    self._project.output.makefile.parse_configure_script_outcome( self._Peano_src_directory )
    self._project.output.makefile.add_library( "ExaHyPE2Core$(DIMENSIONS)d$(LIBRARY_POSTFIX)",          self._Peano_src_directory + "/src/exahype2" )
    self._project.output.makefile.add_library( "ToolboxLoadBalancing$(DIMENSIONS)d$(LIBRARY_POSTFIX)",  self._Peano_src_directory + "/src/toolbox/loadbalancing" )
    self._project.output.makefile.set_mode(self._build_mode)

    return self._project

    
