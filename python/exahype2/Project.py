# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import os
import sys
import logging
import re
import json

import peano4
import dastgen2
import exahype2

from peano4.Logger import setup_logging, log

class Project(object):
    """!
    ExaHyPE 2 project

    Represents an ExaHyPE 2 project. An ExaHyPE 2 project is a Peano 4
    project with a particular set of actions (algorithmic phases)
    that you can choose from and with particular solver types. It
    realises a builder mechanism, i.e. you build up your ExaHyPE 2
    project and then you finally tell the project "give me the Peano 4
    project". From hereon, you can use this Peano 4 project to actually
    set up the Peano 4 application.

    The project will have a marker per cell that encodes stuff alike
    a boundary marker. But it is also used to coordinate different
    solver types.

    @see generate_Peano4_project()
    """

    def __init__(self, namespace, project_name, executable, directory=".", subdirectory="", abstract_overwrite=True):
        self._project = peano4.Project(namespace, project_name, directory, subdirectory)
        self._subdirectory = subdirectory
        self._abstract_overwrite = abstract_overwrite

        self._solvers = []
        self._domain_offset = [0.0, 0.0, 0.0]
        self._domain_size = [1.0, 1.0, 1.0]
        self._dimensions = 3
        self._min_terminal_time = 1.0
        self._max_terminal_time = 0.0
        self._first_plot_time_stamp = 0.0
        self._time_in_between_plots = 0.1
        self._load_balancer_configuration = ""
        self._log_filter_file         = "exahype.log-filter"
        self._log_time_step_interval_in_seconds = 10
        self._additional_includes     = [ "tarch/logging/LogFilterFileReader.h",
                                          "tarch/accelerator/Device.h"
                                        ]
        self._build_mode = peano4.output.CompileMode.Asserts
        self._executable_name = executable
        self._periodic_BC = [False, False, False]
        self._output_path = "./"
        self._tracer_particles = []
        self._tracer_particle_sets = []

        self._particle_unknowns = 0
        self._particle_variable_names = []
        self._particle_variable_pos = [0]
        self._use_particle_var_shortcut = False
        self._runtime_parameters = []

        self.create_grid_with_refinement = peano4.solversteps.Step("CreateGridWithRefinement", False)
        self.create_grid_without_refinement = peano4.solversteps.Step("CreateGridWithoutRefinement", False)
        self.init_grid = peano4.solversteps.Step("InitGrid", False)
        self.plot_solution = peano4.solversteps.Step("PlotSolution", False)
        self.perform_time_step = peano4.solversteps.Step("TimeStep", False)

    @staticmethod
    def _parse_exahype_load_balancer_configuration(load_balancer_configuration):
        match = re.match(
            r"^(?P<prefix>\s*new\s+[\w:]*LoadBalancingConfiguration)\s*(?:\((?P<args>.*)\))?\s*$",
            load_balancer_configuration,
        )
        if not match:
            return None

        args = match.group("args")
        if args is None or args.strip() == "":
            return {
                "prefix": match.group("prefix").strip(),
                "args": [],
            }

        parsed_args = [arg.strip() for arg in args.split(",")]
        if any(arg == "" for arg in parsed_args):
            return None

        return {
            "prefix": match.group("prefix").strip(),
            "args": parsed_args,
        }

    def _render_load_balancer_configuration(self):
        parsed_configuration = self._parse_exahype_load_balancer_configuration(
            self._load_balancer_configuration
        )
        if parsed_configuration is None:
            return self._load_balancer_configuration

        args = parsed_configuration["args"]
        if len(args) >= 5:
            return self._load_balancer_configuration

        defaults = [
            "0.9",
            "1",
            "-1",
            "peano4::grid::SplitInstruction::Mode::BottomUp",
            "PeriodicBC.count()>0",
        ]
        rendered_args = args + defaults[len(args) :]
        return f"{parsed_configuration['prefix']}({', '.join(rendered_args)})"

    def set_load_balancer(self, load_balancer_configuration=""):
        """
        load_balancer_configuration: string
          User-defined load balancer configuration that shall be used.
        """
        self._load_balancer_configuration = load_balancer_configuration

    def set_log_filter_file(self, file_name):
        """! Set a log filter file name
        Pass in the empty string, if you want the code to use the hard-coded
        defaults.
        """
        self._log_filter_file = file_name

    def set_log_time_step_interval_in_seconds(self, value):
        self._log_time_step_interval_in_seconds = value

    def set_build_mode(self, mode=peano4.output.CompileMode.Release):
        self._build_mode = mode

    def add_solver(self, solver):
        self._solvers.append(solver)

    def add_runtime_parameter(self, name, parameter_type, default_value):
        if not re.fullmatch(r"[A-Za-z_][A-Za-z0-9_]*", name):
            raise ValueError(f"Invalid runtime parameter name '{name}'. Use a valid C++ identifier.")

        is_array = isinstance(parameter_type, (tuple, list))
        if is_array:
            if len(parameter_type) != 2:
                raise ValueError(
                    f"Array runtime parameter '{name}' must use a type specification of the form ('double', N)."
                )
            base_type = parameter_type[0]
            array_size = parameter_type[1]
            if base_type not in ("double", "int", "bool"):
                raise ValueError(
                    f"Unsupported array runtime parameter base type '{base_type}'. Supported types are: double, int, bool."
                )
            if not isinstance(array_size, int) or array_size <= 0:
                raise ValueError(f"Array runtime parameter '{name}' expects a positive integer size.")
            if not isinstance(default_value, (list, tuple)) or len(default_value) != array_size:
                raise ValueError(
                    f"Array runtime parameter '{name}' expects exactly {array_size} default values."
                )
        else:
            base_type = parameter_type
            array_size = None
            if base_type not in ("double", "int", "bool", "string"):
                raise ValueError(
                    f"Unsupported runtime parameter type '{base_type}'. Supported types are: double, int, bool, string."
                )

        accessor_name = "get" + name[0].upper() + name[1:]
        scalar_cpp_type = {
            "double": "double",
            "int": "int",
            "bool": "bool",
            "string": "std::string",
        }[base_type]
        if is_array:
            cpp_type = f"tarch::la::Vector<{array_size}, {scalar_cpp_type}>"
            literal = "{" + ", ".join(
                (
                    repr(float(value))
                    if base_type == "double"
                    else str(value)
                    if base_type == "int"
                    else "true" if value else "false"
                )
                for value in default_value
            ) + "}"
            getter = {
                "double": "getDoubleArray",
                "int": "getIntegerArray",
                "bool": "getBooleanArray",
            }[base_type]
            return_type = f"tarch::la::Vector<DIMENSIONS, {scalar_cpp_type}>"
        else:
            if base_type == "double" and not isinstance(default_value, (int, float)):
                raise ValueError(f"Runtime parameter '{name}' expects a numeric default value.")
            if base_type == "int" and not isinstance(default_value, int):
                raise ValueError(f"Runtime parameter '{name}' expects an integer default value.")
            if base_type == "bool" and not isinstance(default_value, bool):
                raise ValueError(f"Runtime parameter '{name}' expects a boolean default value.")
            if base_type == "string" and not isinstance(default_value, str):
                raise ValueError(f"Runtime parameter '{name}' expects a string default value.")

            cpp_type = scalar_cpp_type
            if base_type == "double":
                literal = repr(float(default_value))
                getter = "getDouble"
            elif base_type == "int":
                literal = str(default_value)
                getter = "getInteger"
            elif base_type == "bool":
                literal = "true" if default_value else "false"
                getter = "getBoolean"
            else:
                literal = json.dumps(default_value)
                getter = "getString"
            return_type = cpp_type

        self._runtime_parameters.append(
            {
                "name": name,
                "accessor_name": accessor_name,
                "cpp_type": cpp_type,
                "return_type": return_type,
                "is_array": is_array,
                "scalar_cpp_type": scalar_cpp_type,
                "default_value": literal,
                "getter": getter,
            }
        )

    def remove_all_solvers(self):
        self._solvers = []
        self._project.cleanup()

    def __export_constants(self):
        """
        We export ExaHyPE's constants. Besides the constants from ExaHyPE,
        we also export some parameters from Peano onto the ExaHyPE constants
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
        self._project.constants.add_include("""#include "peano4/Globals.h" """)
        self._project.constants.export_const_with_type(
            "DomainOffset", offset_string, "tarch::la::Vector<DIMENSIONS,double>"
        )
        self._project.constants.export_const_with_type(
            "DomainSize", size_string, "tarch::la::Vector<DIMENSIONS,double>"
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

        self._project.constants.export_boolean_sequence("PeriodicBC", self._periodic_BC)
        self._project.constants.export_string("OutputPath", self._output_path)

        build_string = "python3 "
        for i in sys.argv:
            build_string += " "
            build_string += i
        self._project.constants.export_string("BuildInformation", build_string)

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
        max_end_time=0.0,
    ):
        """
        ! Set the global simulation parameters

        @param dimensions: 2 or 3

        @param offset: [Float]
          Has to have dimension number of entries.

        @param size: [Float}
          Has to have dimension number of entries and all of them have to be
          positive. At this point, we strongly suggest that all size entries
          are exactly the same. While Peano and ExaHyPE do support anisotropic
          meshes, most compute kernels do not, so working with something else
          than a square or cube will likely break all numerics.

        @param first_plot_time_stamp: Float
          Is irrelevant if time_in_between_plots equals zero

        @param time_in_between_plots: Float
          Set to zero if you don't want to have any plots

        @param max_end_time: Float
          If you set it zero (or actually any value smaller than min_end_time), then
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
        self._project.constants.export(
            "TimeInBetweenPlots", str(self._time_in_between_plots)
        )

        self._periodic_BC = []
        for d in range(0, dimensions):
            self._periodic_BC.append(periodic_BC[d])

    def set_output_path(self, path):
        self._output_path = path
        if not self._output_path.endswith("/"):
            self._output_path += "/"

    def add_mainfile_include(self,value):
        self._additional_includes.append( value )

    def __set_solver_repository_dict(self):
        self.solverRepositoryDictionary = {
            "SOLVERS":                  [],
            "LOAD_BALANCER_CONFIGURATION": self._render_load_balancer_configuration(),
            "TRACERS":                  [x.name for x in self._tracer_particles],
            "LOG_FILTER_FILE":          self._log_filter_file,
            "LOG_TIME_STEP_INTERVAL_IN_SECONDS": self._log_time_step_interval_in_seconds,
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

    def __generate_runtime_parameters(self):
        template_directory = os.path.dirname(os.path.realpath(__file__).replace(".pyc", "").replace(".py", ""))
        dictionary = {
            "RUNTIME_PARAMETERS": self._runtime_parameters,
        }
        generated_files = peano4.output.Jinja2TemplatedHeaderImplementationFilePair(
            os.path.join(template_directory, "RuntimeParameters.template.h"),
            os.path.join(template_directory, "RuntimeParameters.template.cpp"),
            "RuntimeParameters",
            self._project.namespace,
            self._project.subdirectory,
            dictionary,
            True,
        )
        self._project.output.add(generated_files)
        self._project.output.makefile.add_h_file(
            self._project.subdirectory + "RuntimeParameters.h", generated=True
        )
        self._project.output.makefile.add_cpp_file(
            self._project.subdirectory + "RuntimeParameters.cpp", generated=True
        )

    # TODO: Move to solvers
    def add_tracer(
        self,
        name,
        particle_attributes,
        sort_descend_invocation_order_within_action_sets=-1,
        plot_descend_invocation_order_within_action_sets=65536,
    ):
        particle = peano4.toolbox.particles.Particle(name)
        if isinstance(particle_attributes, dict):
            self._particle_unknowns = sum(particle_attributes["unknowns"].values())
            self._particle_variable_names += list(particle_attributes["unknowns"].keys())

            for var in particle_attributes["unknowns"].values():
                self._particle_variable_pos.append(var + self._particle_variable_pos[-1])

            self._use_particle_var_shortcut = True
            self._generate_particle_shortcuts(name)

        elif isinstance(particle_attributes, int):
            self._particle_unknowns = particle_attributes

        else:
            raise Exception(
                "Invalid format for 'unknowns'. Must be an integer or a dictionary."
            )

        particle_attr = peano4.dastgen2.Peano4DoubleArray(
            "data", str(self._particle_unknowns)
        )
        particle.data.add_attribute(particle_attr)

        particle.data.add_attribute(
            dastgen2.attributes.Enumeration("MoveState", ["NotMovedYet", "Moved"])
        )

        particle_number = peano4.dastgen2.Peano4IntegerArray("number", "2")
        particle.data.add_attribute(particle_number)

        self._project.datamodel.add_global_object(particle)
        particles = peano4.toolbox.particles.ParticleSet(particle)
        self._project.datamodel.add_vertex(particles)

        self.plot_solution.use_vertex(particles)
        self.init_grid.use_vertex(particles)
        self.perform_time_step.use_vertex(particles)
        self.create_grid_with_refinement.use_vertex(particles)
        self.create_grid_without_refinement.use_vertex(particles)

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

        self._tracer_particles.append(particle)
        self._tracer_particle_sets.append(particles)

        return particles

    def _generate_particle_shortcuts(self, name):
        generated_shortcut_file = peano4.output.Jinja2TemplatedHeaderFile(
            os.path.join(os.path.dirname(os.path.realpath(__file__)), "VariableShortcuts.template.h"),
            "VariableShortcuts" + name,
            self._project.namespace,
            self._subdirectory + ".",
            {
                "NUMBER_OF_VARIABLES": len(self._particle_variable_names),  # Standardised naming
                "VARIABLE_NAMES": self._particle_variable_names,
                "VARIABLE_POSITIONS": self._particle_variable_pos,
                "SOLVER_NAME": name,
            },
            True,
            True,
        )
        self._project.output.add(generated_shortcut_file)
        self._project.output.makefile.add_h_file(self._subdirectory + "VariableShortcuts"+ name + ".h", generated=True)

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
        - without refinement as we've just refined or split up domain and to let load balancing converge.

        """
        self.create_grid_with_refinement.add_action_set(action_set)
        self.create_grid_without_refinement.add_action_set(action_set)

    def generate_Peano4_project(self, verbose=False):
        """!
        Construct a Peano 4 project and return it

        Build the Peano 4 project, i.e., all the action sets et al that you require
        to run this ExaHyPE 2 application. The project is built within self._project
        and eventually returned.

        This routine generates a Peano project, i.e. the domain-specific ExaHyPE
        view is translated into a Peano model. Once you have called this routine,
        any changes to the ExaHyPE 2 configuration do not propagate into the Peano
        setup anymore. If you alter the ExaHyPE setup, you have to call
        generate_Peano4_project() again to get a new snapshot/version of the
        Peano setup.
        """
        if verbose:
            setup_logging(level=logging.DEBUG)
        else:
            setup_logging(level=logging.INFO)

        # self._project.cleanup()

        self.__configure_makefile()
        self._project.output.makefile.parse_configuration_outcome()
        self._project.output.makefile.set_mode(self._build_mode)
        self.__export_constants()

        self._project.solversteps.add_step(self.create_grid_with_refinement)
        self._project.solversteps.add_step(self.create_grid_without_refinement)
        self._project.solversteps.add_step(self.init_grid)
        self._project.solversteps.add_step(self.plot_solution)
        self._project.solversteps.add_step(self.perform_time_step)

        for solver in self._solvers:
            log.debug("---------------------------------------")
            log.debug("Create data for solver " + solver._name)
            log.debug("---------------------------------------")
            log.debug(str(solver))

            solver.add_to_Peano4_datamodel(self._project.datamodel)

            solver.add_use_data_statements_to_Peano4_solver_step(self.create_grid_with_refinement)
            solver.add_use_data_statements_to_Peano4_solver_step(self.create_grid_without_refinement)
            solver.add_use_data_statements_to_Peano4_solver_step(self.plot_solution)
            solver.add_use_data_statements_to_Peano4_solver_step(self.perform_time_step)
            solver.add_use_data_statements_to_Peano4_solver_step(self.init_grid)

            solver.add_actions_to_create_grid(self.create_grid_with_refinement, evaluate_refinement_criterion=True)
            solver.add_actions_to_create_grid(self.create_grid_without_refinement, evaluate_refinement_criterion=False)

            solver.add_actions_to_init_grid(self.init_grid)
            solver.add_actions_to_plot_solution(self.plot_solution, self._output_path)

            solver.add_actions_to_perform_time_step(self.perform_time_step)

            if self._abstract_overwrite==False:
                solver.add_implementation_files_to_project(
                    self._project.namespace, self._project.output, self._dimensions, self._subdirectory, self._abstract_overwrite
                )
            else:
                solver.add_implementation_files_to_project(self._project.namespace, self._project.output, self._dimensions, self._subdirectory)

        self.__generate_solver_repository()
        self.__generate_runtime_parameters()

        self._project.main = exahype2.ExaHyPEMain(self._project)
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

    @property
    def subdirectory(self):
        return self._subdirectory

    @property
    def additional_includes(self):
        return self._additional_includes
