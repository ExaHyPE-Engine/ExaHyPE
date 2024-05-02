# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import peano4
import peano4.output.Jinja2TemplatedHeaderImplementationFilePair

import dastgen2

import os
import sys

import exahype2.grid
import exahype2.solvers

from exahype2.solvers.aderdg.ADERDG import ADERDG
from exahype2.solvers.aderdg.kernelgenerator import Configuration


class Project(object):
    """!

    ExaHyPE 2 project

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

    def __init__(self, namespace, project_name, executable, directory=".", subdirectory=""):
        self._project = peano4.Project(namespace, project_name, directory, subdirectory)
        self._project.output.makefile.add_cmake_core_library("ExaHyPE2Core")
        self._subdirectory = subdirectory

        self._solvers = []
        self._domain_offset = [0.0, 0.0, 0.0]
        self._domain_size = [1.0, 1.0, 1.0]
        self._dimensions = 2
        self._min_terminal_time = 1.0
        self._max_terminal_time = 0.0
        self._first_plot_time_stamp = 0.0
        self._time_in_between_plots = 0.1
        self._load_balancer_name      = ""
        self._load_balancer_arguments = ""
        self._log_filter_file         = "exahype.log-filter"
        self._number_of_threads       = "tarch::multicore::Core::UseDefaultNumberOfThreads"
        self._multicore_orchestration = "tarch::multicore::orchestration::createDefaultStrategy()"
        self._timeout                 = 3600
        self._gpus                    = []
        self._additional_includes     = [ "tarch/logging/LogFilterFileReader.h", 
                                          "tarch/multicore/orchestration/StrategyFactory.h",
                                          "tarch/accelerator/Device.h"
                                        ]
        self._Peano_src_directory     = "."
        self._build_mode = peano4.output.CompileMode.Asserts
        self._executable_name = executable
        self._periodic_BC = [False, False, False]
        self._plot_filters = []
        self._output_path = "./"
        self._plotter_precision = 5
        self._tracer_particles = []
        self._tracer_particle_sets = []

        self.create_grid = peano4.solversteps.Step("CreateGrid", False)
        self.init_grid = peano4.solversteps.Step("InitGrid", False)
        self.create_grid_but_postpone_refinement = peano4.solversteps.Step("CreateGridButPostponeRefinement", False)
        self.create_grid_and_converge_lb = peano4.solversteps.Step("CreateGridAndConvergeLoadBalancing", False)
        self.plot_solution = peano4.solversteps.Step("PlotSolution", False)
        self.perform_time_step = peano4.solversteps.Step("TimeStep", False)


    def set_load_balancing(self, load_balancer_name, load_balancer_arguments=""):
        """
        load_balancer_name: string
          Should be full-qualified name of the load balancer.
          By default, I recommend to pass "toolbox::loadbalancing::strategies::RecursiveSubdivision".

        load_balancer_arguments: string
          If your load balancing requires parameters, add them
          here. It is a string that will be copied into the C++ class instantiation.
          Please add the brackets yourself, i.e. "(3,4,5)" is fine, but "3,4,5" is not.
          The only exception is the empty parameter list. Here, you can/should simply
          add the empty string.
        """
        self._load_balancer_name = load_balancer_name
        self._load_balancer_arguments = load_balancer_arguments


    def set_log_filter_file(self, file_name):
        """! Set a log filter file name
        Pass in the empty string, if you want the code to use the hard-coded
        defaults.
        """
        self._log_filter_file = file_name


    def set_number_of_threads(self, value):
        """! Set number of threads to use
        Pass in number of threads (positive integer) or a string such as
        "tarch::multicore::Core::UseDefaultNumberOfThreads".
        """
        self._number_of_threads = value


    def set_multicore_orchestration(self, value):
        """! Set orchestration
        value has to be a string which creates an orchestration object,
        i.e., an object of the type tarch::multicore::orchestration::Orchestration.
        You can either create it directly (in this case you need to add a new),
        or you can use a factory method such as
        "tarch::multicore::orchestration::StrategyFactory::createDefaultStrategy()".
        """
        self._multicore_orchestration = value


    def set_timeout(self, value):
        """! Set timeout
        value has to be a number greater or equal to zero, this then sets the
        timeout in seconds after which a node waiting for an MPI message shall
        quit an shutdown the whole application with an error report.
        If zero is passed, this feature is switched off and nodes will wait
        indefinitely.
        """
        if value < 0:
            raise Exception("Timeout has to be greater or equal to 0")
        self._timeout = value


    def set_gpus(self, value):
        """! Set GPUs to be used
        value is a list of integers or the empty list if you want the code to
        use all GPUs that are visible to a rank.
        """
        self._gpus = value


    """
    The standard extensions that I use for both Peano and ExaHyPE.
    """
    LibraryDebug = "_debug"
    LibraryRelease = ""
    LibraryTrace = "_trace"
    LibraryAsserts = "_asserts"
    LibraryStats = "_stats"

    def set_Peano4_installation(self, src_path, mode=peano4.output.CompileMode.Release):
        """
        src_path: string
          Path (relative or absolute) to the src directory of Peano. This path
          should hold both the headers (in subdirectories) and all the static
          libraries.

        mode: peano4.output.CompileMode
        """
        self._Peano_src_directory = src_path
        self._build_mode = mode

    def add_solver(self, solver):
        self._solvers.append(solver)

    def add_plot_filter(self, offset, size, frequency=1):
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

        self._plot_filters.append(new_entry)

    def remove_all_solvers(self):
        self._solvers = []
        self._project.cleanup()

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
        self._project.constants.export("MinTerminalTime", str(self._min_terminal_time))
        if self._max_terminal_time >= self._min_terminal_time:
            self._project.constants.export(
                "MaxTerminalTime", str(self._max_terminal_time)
            )
        else:
            self._project.constants.export(
                "MaxTerminalTime", "std::numeric_limits<double>::max()"
            )
        self._project.constants.export(
            "FirstPlotTimeStamp", str(self._first_plot_time_stamp)
        )
        self._project.constants.export(
            "TimeInBetweenPlots", str(self._time_in_between_plots)
        )
        self._project.constants.export("PlotterPrecision", str(self._plotter_precision))
        self._project.constants.export_boolean_sequence("PeriodicBC", self._periodic_BC)

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

### ExaHyPE 2

This code uses ExaHyPE in its second generation. The first generation of the
code was developed through an EU FET HPC project called ExaHyPE. This first
generation was built on top of Peano in its third generation. The present
code uses ExaHyPE 2 which is a complete rewrite built on top of Peano 4. We
do not yet have a release paper for this second generation of ExaHyPE, and
thus appreciate any citation of the original release paper

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
                i.create_readme_descriptor(self._domain_offset[0], self._domain_size[0])
            )
        # self._min_volume_h         = min_volume_h
        # self._max_volume_h         = max_volume_h

    def __configure_makefile(self):
        self._project.output.makefile.set_dimension(self._dimensions)
        self._project.output.makefile.set_executable_name(self._executable_name)

    def set_global_simulation_parameters(
        self,
        dimensions,
        offset,
        size,
        min_end_time,
        first_plot_time_stamp,
        time_in_between_plots,
        periodic_BC=[False, False, False],
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
        self._domain_offset = offset
        self._domain_size = size
        self._dimensions = dimensions

        assert (
            len(self._domain_offset) == dimensions
        ), "domain offset vector and dimensions don't match"
        assert (
            len(self._domain_size) == dimensions
        ), "domain offset vector and dimensions don't match"

        self._max_terminal_time = max_end_time
        self._min_terminal_time = min_end_time
        self._first_plot_time_stamp = first_plot_time_stamp
        self._time_in_between_plots = time_in_between_plots
        self._periodic_BC = []
        self._plotter_precision = plotter_precision
        for d in range(0, dimensions):
            self._periodic_BC.append(periodic_BC[d])
        if plotter_precision <= 0:
            raise Exception("Plotter precision has to be bigger than 0")

    def set_output_path(self, path):
        self._output_path = path
        if not os.path.exists(self._output_path):
            try:
                os.makedirs(self._output_path)
            except:
                print("Couldn't create the defined output path, please ensure that this path exists")
        if not self._output_path.endswith("/"):
            self._output_path += "/"
            
    def add_mainfile_include(self,value):
        self._additional_includes.append( value )

    def __set_solver_repository_dict(self):
        self.solverRepositoryDictionary = {
            "SOLVERS":                  [],
            "LOAD_BALANCER":            self._load_balancer_name,
            "LOAD_BALANCER_ARGUMENTS":  self._load_balancer_arguments,
            "PLOT_FILTER":              self._plot_filters,
            "TRACERS":                  [x.name for x in self._tracer_particles],
            "LOG_FILTER_FILE":          self._log_filter_file,
            "NUMBER_OF_THREADS":        self._number_of_threads,
            "MULTICORE_ORCHESTRATION":  self._multicore_orchestration,
            "TIMEOUT":                  self._timeout,
            "GPUS":                     self._gpus,
            "ADDITIONAL_INCLUDES":      self._additional_includes,
        }

    def __generate_solver_repository(self):
        """
        I have to call finishedTraversal() for each tracer set. As tracers
        are a subclass of DoF, they have a name property.
        """

        self.__set_solver_repository_dict()

        for solver in self._solvers:
            self.solverRepositoryDictionary["SOLVERS"].append(
                (solver._name, solver.get_name_of_global_instance())
            )

        templatefile_prefix = (
            os.path.realpath(__file__).replace(".pyc", "").replace(".py", "")
        )
        generated_solver_files = (
            peano4.output.Jinja2TemplatedHeaderImplementationFilePair(
                templatefile_prefix + "SolverRepository.template.h",
                templatefile_prefix + "SolverRepository.template.cpp",
                "SolverRepository",
                self._project.namespace + ["repositories"],
                self._project.subdirectory + "repositories",
                self.solverRepositoryDictionary,
                True,
            )
        )

        self._project.output.add(generated_solver_files)
        self._project.output.makefile.add_cpp_file(
            self._project.subdirectory + "repositories/SolverRepository.cpp", generated=True
        )

    def add_tracer(
        self,
        name,
        attribute_count=0,
        plot=True,
        sort_descend_invocation_order_within_action_sets=-1,
        plot_descend_invocation_order_within_action_sets=65536,
    ):
        """!

        Add a tracer to the project
        
        Tracers have to be the very last thing you add to your project.
        At this point, all solvers have to be added to the project.
        However, you have to add the tracers prior to any user-defined
        routines. See docu of init_new_user_defined_algorithmic_step().

        We rely on the particle toolbox. Therefore, our particles have
        already a position x, a search radius (which we don't usually
        use here), and a (parallel) state. The search radius determines
        how Peano sorts particles within the tree, and we can set it to
        zero here. This ensures that the particle has no real volume and
        thus is always sieved through to the finest grid level.

        @see peano4.toolbox.particles.Particle
        
        
        ## Projection of data onto particles and database dumping
        
        The projection onto the particle has to be realised separatedly, as
        it depends on the solver variant chosen. The same holds for the 
        dump of particle data into a database if this shall happen 
        on-the-fly. 
        
        Most bigger applications add their solvers an add_tracer() operation,
        which takes all of this domain knowledge into account. This 
        add_tracer() then forwards to this routine, adding further 
        behaviour.
        
        See the class application.exahype2.ccz.CCZ4Solver and its derived
        variants for examples.


        ## Moving particles

        Tracers in ExaHyPE can move, though they do not have to move. Even if
        they move, they never have a native velocity, i.e. their velocity will
        always be a projection of an ExaHyPE solution. We are talking about
        tracers after all and not about a proper PIC approach. Due to this
        constraint, we do not need a velocity vector. However, we need an
        additional enumeration which clarifies if a
        particle has moved already or not. This enumeration is needed by the
        particle update routines. Obviously, nothing stops you from adding
        a separate velocity field to the tracer object and thus to redefine
        this behaviour.
        
        
        ## ExaHyPE-specific particle attributes

        We add each
        particle a number which is a tuple consisting of the tree number
        (on which tree has a particle been generationed plus what was the
        tree-local number of this particle at the time). Through this
        tuple, we can track tracers later on even if they move accross
        domains.

        We can plot the data - if there is data to be tracked - directly,
        as it is a large double array. For the index, we have to type cast
        the data, as this is a tuple of integers, and Peano's VTK plotter
        supports only double vectors.

        @see peano4.toolbox.particles.PlotParticlesInVTKFormat.add_attribute_to_plot()


        ## Action set order

        We add the particle ordering with descend order -1. This should do the job
        for most setups, but we might be wrong. The following action sets are
        automatically added:

        - UpdateParallelState is added to each and every time step and plotting
          sweep. As we also add
        - peano4.toolbox.particles.api.UpdateParticleGridAssociation_LiftDrop to each and every time step,
          particles can move at any point. We always resort them immediately.

        Both action sets are assigned the standard priority of sort_descend_invocation_order_within_action_sets.


        ## Arguments

        name: String
          Has to be a unique name for this tracer

        attribute_count: integer
          Number of attributes that we track per particle (scalar values added
          pre particle). Can be 0 if you don't want any attributes.

        h and noise:
          See tracer.InsertParticles

        plot: Boolean
          If this flag is set, ExaHyPE dumps the particles as vtu files whenever
          it writes patch files. You can switch this behaviour off. A lot of codes
          do so if they dump the tracer data independently into another database
          anyway.

        sort_descend_invocation_order_within_action_sets: Integer
          You have to clarify what the priority of the tracer is within the
          action sets. By default, all ExaHyPE solvers start with priority
          0 and then up to n. So you are fine with using the default of -1
          which means that the tracer precedes any solver. However, there
          might be cases where you wanna move the priorities manually.

        Returns the particle set that you can use to modify further
        """
        particle = peano4.toolbox.particles.Particle(name)

        if attribute_count > 0:
            particle_attr = peano4.dastgen2.Peano4DoubleArray(
                "data", str(attribute_count)
            )
            particle.data.add_attribute(particle_attr)

        particle.data.add_attribute(
            dastgen2.attributes.Enumeration("MoveState", ["NotMovedYet", "Moved"])
        )

        particle_number = peano4.dastgen2.Peano4IntegerArray("number", "2")
        particle.data.add_attribute(particle_number)

        particles = peano4.toolbox.particles.ParticleSet(particle)

        self._project.datamodel.add_global_object(particle)
        self._project.datamodel.add_vertex(particles)

        self.plot_solution.use_vertex(particles)
        self.init_grid.use_vertex(particles)
        self.perform_time_step.use_vertex(particles)
        self.create_grid.use_vertex(particles)
        self.create_grid_but_postpone_refinement.use_vertex(particles)
        self.create_grid_and_converge_lb.use_vertex(particles)

        #
        # Initialisation
        #
        self.init_grid.add_action_set(
            peano4.toolbox.particles.api.UpdateParticleGridAssociation_LiftDrop(particles)
        )

        update_particle_association_action_set = (
            peano4.toolbox.particles.api.UpdateParticleGridAssociation_LiftDrop(particles)
        )
        update_particle_association_action_set.descend_invocation_order = sort_descend_invocation_order_within_action_sets
        update_particle_association_action_set.parallel = True

        update_particle_parallel_state_action_set = (
            peano4.toolbox.particles.api.UpdateParallelState(particles)
        )
        update_particle_parallel_state_action_set.descend_invocation_order = (
            sort_descend_invocation_order_within_action_sets
        )
        update_particle_parallel_state_action_set.parallel = True

        #
        # Move particles
        #
        self.perform_time_step.add_action_set(update_particle_association_action_set)
        self.perform_time_step.add_action_set(update_particle_parallel_state_action_set)

        #
        # Plotter: The timestamp here is slightly wrong, as I use the min time stamp. It
        # really should be the time stamp of the underlying solver. However, tracers are
        # not tied to one solver, so I have to find a quantity that makes sense. If the
        # user requres the time stamp, they have to add this one as data field to the
        # particle
        #
        if plot:
            particle_plotter = peano4.toolbox.particles.PlotParticlesInVTKFormat(
                filename=name,
                particle_set=particles,
                time_stamp_evaluation="repositories::getMinTimeStamp()",
                additional_includes="""
#include "repositories/SolverRepository.h"
""",
            )
            if attribute_count > 0:
                particle_plotter.add_attribute_to_plot(particle_attr, attribute_count)
            particle_plotter.add_attribute_to_plot(
                particle_number, 2, "tarch::la::convertScalar<double>(p->getNumber())"
            )
            particle_plotter.descend_invocation_order = plot_descend_invocation_order_within_action_sets
            self.plot_solution.add_action_set(particle_plotter)

        self.plot_solution.add_action_set(update_particle_association_action_set)
        self.plot_solution.add_action_set(update_particle_parallel_state_action_set)

        self._tracer_particles.append(particle)
        self._tracer_particle_sets.append(particles)

        return particles

    def add_action_set_to_timestepping(self, action_set):
        """!

        Add a new action set to the time stepping

        You have to be careful with the priorities, i.e. you might want to
        use myproject.perform_time_step.max_priority() or myproject.perform_time_step.min_priority()
        to find out which
        priority you should assign to action_set.

        """
        self.perform_time_step.add_action_set(action_set)

    def add_action_set_to_plot_solution(self, action_set):
        """!

        Add a new action set to the plotting

        """
        self.plot_solution.add_action_set(action_set)

    def add_action_set_to_initialisation(self, action_set):
        self.init_grid.add_action_set(action_set)

    def add_action_set_to_create_grid(self, action_set):
        """!
        
        Add an action set to create grid
        
        This routine actually adds an action set to each and every grid
        creation routine. After all, we distinguish three flavours of it
        
        - with refinement;
        - without refinement as we've just refined or split up domain;
        - without refinement to let load balancing converge.
        
        """
        self.create_grid.add_action_set(action_set)
        self.create_grid_but_postpone_refinement.add_action_set(action_set)
        self.create_grid_and_converge_lb.add_action_set(action_set)

    def generate_Peano4_project(self, verbose=False):
        """!
        
        Construct a Peano 4 project and return it
        
        Build the Peano4 project, i.e., all the action sets et al that you require
        to run this ExaHyPE2 application. The project is built within self._project
        and eventually returned.

        This routine generates a Peano project, i.e. the domain-specific ExaHyPE
        view is translated into a Peano model. Once you have called this routine,
        any changes to the ExaHyPE 2 configuration do not propagate into the Peano
        setup anymore. If you alter the ExaHyPE setup, you have to call
        generate_Peano4_project() again to get a new snapshot/version of the
        Peano setup.
        """
        # self._project.cleanup()

        self._project.solversteps.add_step(self.create_grid)
        self._project.solversteps.add_step(self.init_grid)
        self._project.solversteps.add_step(self.create_grid_but_postpone_refinement)
        self._project.solversteps.add_step(self.create_grid_and_converge_lb)
        self._project.solversteps.add_step(self.plot_solution)
        self._project.solversteps.add_step(self.perform_time_step)

        for solver in self._solvers:
            print("---------------------------------------")
            print("Create data for solver " + solver._name)
            print("---------------------------------------")
            print(str(solver))

            solver.add_to_Peano4_datamodel(self._project.datamodel, verbose)

            solver.add_use_data_statements_to_Peano4_solver_step(self.create_grid)
            solver.add_use_data_statements_to_Peano4_solver_step(self.plot_solution)
            solver.add_use_data_statements_to_Peano4_solver_step(self.perform_time_step)
            solver.add_use_data_statements_to_Peano4_solver_step(self.init_grid)
            solver.add_use_data_statements_to_Peano4_solver_step(
                self.create_grid_but_postpone_refinement
            )
            solver.add_use_data_statements_to_Peano4_solver_step(
                self.create_grid_and_converge_lb
            )

            solver.add_actions_to_create_grid(
                self.create_grid, evaluate_refinement_criterion=True
            )
            solver.add_actions_to_init_grid(self.init_grid)
            solver.add_actions_to_create_grid(
                self.create_grid_but_postpone_refinement,
                evaluate_refinement_criterion=False,
            )
            solver.add_actions_to_create_grid(
                self.create_grid_and_converge_lb, evaluate_refinement_criterion=False
            )
            solver.add_actions_to_plot_solution(self.plot_solution, self._output_path)
            solver.add_actions_to_perform_time_step(self.perform_time_step)

            solver.add_implementation_files_to_project(
                self._project.namespace, self._project.output, self._dimensions, self._subdirectory
            )

        self.__generate_solver_repository()

        self._project.main = exahype2.ExaHyPEMain(self._project)

        # maybe use ..
        self.__configure_makefile()
        self._project.output.makefile.parse_configure_script_outcome(
            self._Peano_src_directory
        )
        self.__export_constants()

        # self._project.output.makefile.add_library( "ExaHyPE2Core$(DIMENSIONS)d$(LIBRARY_POSTFIX)",          self._Peano_src_directory + "/src/exahype2" )
        # self._project.output.makefile.add_library( "ToolboxLoadBalancing$(DIMENSIONS)d$(LIBRARY_POSTFIX)",  self._Peano_src_directory + "/src/toolbox/loadbalancing" )

        # if self._add_particles_library:
        #  self._project.output.makefile.add_library( "ToolboxParticles$(DIMENSIONS)d$(LIBRARY_POSTFIX)",  self._Peano_src_directory + "/src/toolbox/particles" )

        self._project.output.makefile.set_mode(self._build_mode)

        for solver in self._solvers:
            if isinstance(solver, ADERDG):
                pathToExaHyPERoot = Configuration.pathToExaHyPERoot
                if solver._use_BLIS:
                    archname = ""
                    for root, dirs, files in os.walk(pathToExaHyPERoot+ "/submodules/blis/include"):
                        archname = dirs[0]
                        break
                    self._project.output.makefile.add_header_search_path(pathToExaHyPERoot + "/submodules/blis/include/" + archname)
                    self._project.output.makefile.add_linker_flag(pathToExaHyPERoot + "/submodules/blis/lib/" + archname + "/libblis.a")
                    self._project.output.makefile.add_library("-lpthread")
                elif solver._use_Eigen:
                    self._project.output.makefile.add_header_search_path(pathToExaHyPERoot + "/submodules/eigen")
                elif solver._use_libxsmm_JIT:
                    self._project.output.makefile.add_header_search_path(pathToExaHyPERoot + "/submodules/libxsmmJIT/include")
                    self._project.output.makefile.add_linker_flag(pathToExaHyPERoot + "/submodules/libxsmmJIT/lib/libxsmm.a")
                    self._project.output.makefile.add_linker_flag(pathToExaHyPERoot + "/submodules/OpenBLAS/libopenblas.a")
                    self._project.output.makefile.add_library("-lpthread")
                break
        return self._project


    def init_new_user_defined_algorithmic_step(self,
                                               step,
                                               ):
        """! Inform ExaHyPE 2 that there is an additional solver step not resulting from vanilla solver versions
        
        Use this routine whenever you have created your own 
        @ref page_architecture "algorithmic step". Please note that the routine
        should be called after you have created all of your solvers. Otherwise, 
        we'll miss out a few of your solvers and the data access patterns 
        will/might be corrupted. Also all tracers have to be added at this 
        point already.

        The routine has no side-effects, i.e. does not alter the underlying 
        project. It only manipulates the step. Therefore, you can even call it 
        after you have distilled the Peano project from the ExaHyPE project, 
        i.e. after you have called exahype2.Project.generate_Peano4_project().
        In return, you however still have to add the new step manually to the
        Peano project. That is, this routine merely initialises the new step,
        but it does not yet add it to the sweeps known by Peano.

        @param step: peano4.solversteps.Step
  
        """
        for solver in self._solvers:
            solver.add_use_data_statements_to_Peano4_solver_step(step)

        for tracer_set in self._tracer_particle_sets:
            step.use_vertex(tracer_set)
