import peano4
import exahype2
import dastgen2

from abc import abstractmethod


class AbstractCCZ4Solver(object):
    """!

    Abstract base class for any CCZ4 solver

    Each CCZ4 solver inherits from this abstract base class which really only
    defines some generic stuff such as the unknowns and includes that every
    single solver will use.

    The solver should, more or less, work out of the box, but you have to do
    three things if you use a subclass:

    1. If you use a CCZ4 solver, you will still have to add all the libraries to
       your Peano project such that the Makefile picks them up. For this, the
       solver offers an add_makefile_parameters().

    2. You have to set the initial conditions via

    ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    my_solver.set_implementation(initial_conditions   = " " "
     for (int i=0; i<NumberOfUnknowns+NumberOfAuxiliaryVariables; i++) Q[i] = 0.0;
        ::applications::exahype2::ccz4::gaugeWave(Q, volumeCentre, 0);
        " " ")
    ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

      At this point, different CCZ4 solver variants might require different
      syntax. The term volumeCentre for example above is only defined in a
      finite volume ontext.

    3. Finally, you have to add domain-specific constants to the project.
       For this, call add_all_solver_constants(). See the comment below.

    Further to that, you might want to have to set boundary conditions. By
    default, we do not set any boundary conditions. This works fine if
    periodic boundary conditions are used. But once you switch off periodic
    boundary conditions, you have to tell the solver how to treat the boundary.
    This is typically done via set_implementation(), too.

    ## More complex scenarios

    Setting particular implementations via set_implementation() is not always
    convenient or possible. You might want to add new functions to your classes,
    do something in the solver constructor, and so forth. If so, feel free to
    modify the file MySolverName.cpp which the tool generates. In this context,
    you might want to pass in

    ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    my_solver.set_implementation(initial_conditions   = exahype2.solvers.PDETerms.User_Defined_Implementation,
                                 refinement_criterion = exahype2.solvers.PDETerms.User_Defined_Implementation,
                                 boundary_conditions=exahype2.solvers.PDETerms.User_Defined_Implementation
                                 )

    ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

    which ensures that you get the right hook-in methods generated when you
    invoke the Python script for the first time. These methods will contain
    todo comments for you. Subsequent runs of the Python API should not
    overwrite the solver implementation.

    ## Constants

    Each CCZ4 solver requires a minimal set of constants. These are represented
    by integer_constants and double_constants. Please augment these dictionaries.
    Eventually, you have to submit all the constants via add_all_solver_constants().

    """

    """!
  
  Dictionary which specifies the unknown names plus their cardinality

  Has to be class attribute, as we need it in the constructor, i.e. before the 
  abstract object is created.

  """
    _FO_formulation_unknowns = {
        "G": 6,
        "K": 6,
        "theta": 1,
        "Z": 3,
        "lapse": 1,
        "shift": 3,
        "b": 3,
        "dLapse": 3,
        "dxShift": 3,
        "dyShift": 3,
        "dzShift": 3,
        "dxG": 6,
        "dyG": 6,
        "dzG": 6,
        "traceK": 1,
        "phi": 1,
        "P": 3,
        "K0": 1
    }

    """!

  Primary unknowns of the CCZ4 formulation which are there in the initial
  formulation. All the other variables are auxiliary variables, i.e. ones
  introduced to return to a first-order formulation. Unfortunately, the 
  ordering in _FO_formulation_unknows is motivated by the original papers 
  and not by the fact which quantities are original ones and which one are
  helper or auxiliary variables.

  """
    _SO_formulation_unknowns = {
        "G",
        "K",
        "theta",
        "Z",
        "lapse",
        "shift",
        "b",
        "traceK",
        "phi",
    }

    Default_Time_Step_Size_Relaxation = 0.1

    def __init__(self):
        """!

        Constructor

        Initialise the two dictionaries with default values (which work).

        """
        self.integer_constants = {
            "CCZ4LapseType": 0,
            "CCZ4SO": 0,
        }
        self.double_constants = {
            "CCZ4ds": 1.0,
            "CCZ4c": 1.0,
            "CCZ4e": 1.0,
            "CCZ4f": 0.75,
            "CCZ4bs": 0.0,
            "CCZ4sk": 0.0,
            "CCZ4xi": 1.0,
            "CCZ4itau": 1.0,
            "CCZ4eta": 1.0,
            "CCZ4k1": 0.1,
            "CCZ4k2": 0.0,
            "CCZ4k3": 0.5,
            "CCZ4GLMc": 1.2,
            "CCZ4GLMd": 2.0,
            "CCZ4mu": 0.2
        }

    def enable_second_order(self):
        self.integer_constants.update({"CCZ4SO":1})
        self.Default_Time_Step_Size_Relaxation=self.Default_Time_Step_Size_Relaxation/2


    def _add_standard_includes(self):
        """!

        Add the headers for the compute kernels and initial condition implementations

        Usually called by the subclass constructor.

        """
        self.add_user_action_set_includes(
            """
#include "CCZ4Kernels.h"
#include "SecondOrderAuxiliaryVariablesReconstruction.h"
"""
        )
        self.add_user_solver_includes(
            """
#include "CCZ4Kernels.h"
#include "InitialValues.h"
#include "SecondOrderAuxiliaryVariablesReconstruction.h"
#include <cstring>
"""
        )

    def add_all_solver_constants(self):
        """!

        Add domain-specific constants

        I need a couple of constants. I could either replace them directly
        within the Python snippets below, but I prefer here to go a different
        way and to export them as proper C++ constants.

        There are two ways to inject solver constants into Peano: We can either
        add them to the Makefile as global const expressions, or we can add
        them to the ExaHyPE2 solver. The latter is the route we go down here,
        as these constants logically belong to the solver and not to the project.

        This operation uses the parent class' add_solver_constants(). You still
        can use this operation to add further parameters. Or you can, as a user,
        always add new entries to integer_constants or double_constants and then
        call this routine rather than adding individual constants one by one.

        """
        for key, value in self.integer_constants.items():
            self.add_solver_constants(
                "static constexpr int {} = {};".format(key, value)
            )
        for key, value in self.double_constants.items():
            self.add_solver_constants(
                "static constexpr double {} = {};".format(key, value)
            )

    def add_makefile_parameters(self, peano4_project, path_of_ccz4_application):
        """!

        Add include path and minimal required cpp files to makefile

        If you have multiple CCZ4 solvers, i.e. different solvers of CCZ4 or multiple
        instances of the CCZ4 type, please call this operation only once on one of
        your solvers. At the moment, I add hte following cpp files to the setup:

        - InitialValues.cpp
        - CCZ4Kernels.cpp
        - SecondOrderAuxiliaryVariablesReconstruction.cpp

        You can always add further files via
        ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
        peano4_project.output.makefile.add_cpp_file( "mypath/myfile.cpp" )
        ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

        """
        if path_of_ccz4_application[-1] != "/":
            path_of_ccz4_application += "/"

        peano4_project.output.makefile.add_cpp_file(
            path_of_ccz4_application + "InitialValues.cpp"
        )
        peano4_project.output.makefile.add_cpp_file(
            path_of_ccz4_application + "CCZ4Kernels.cpp"
        )
        peano4_project.output.makefile.add_cpp_file(
            path_of_ccz4_application + "SecondOrderAuxiliaryVariablesReconstruction.cpp"
        )
        peano4_project.output.makefile.add_header_search_path(path_of_ccz4_application)

    @abstractmethod
    def add_tracer(
        self,
        name,
        coordinates,
        project,
        number_of_entries_between_two_db_flushes,
        data_delta_between_two_snapsots,
        time_delta_between_two_snapsots,
        clear_database_after_flush,
        tracer_unknowns=None,
    ):
        """!

        Add tracer to project
        
        Consult exahype2.tracer.DumpTracerIntoDatabase for an explanation of
        some of the arguments. Most of them are simply piped through to this 
        class.

        The tracer is given a name and initial coordinates (list of three-tuples).
        We need to know the underlying project as well, as we have to add the
        tracing to the time stepping and the database update to the plotting.
        ~~~~~~~~~~~~~~~~~~~~~~~
        project.add_action_set_to_timestepping(my_interpolation)
        project.add_action_set_to_timestepping(exahype2.tracer.DumpTracerIntoDatabase(
          particle_set=tracer_particles,
          solver=self,
          filename=name + "-" + self._name,
          number_of_entries_between_two_db_flushes=number_of_entries_between_two_db_flushes,
          output_precision=10,
          data_delta_between_two_snapsots = data_delta_between_two_snapsots,
          time_delta_between_two_snapsots = time_delta_between_two_snapsots,
          clear_database_after_flush         = True,
          ))
        ~~~~~~~~~~~~~~~~~~~~~~~

        """
        assert "should not be called"
        pass


class CCZ4Solver_FV_GlobalAdaptiveTimeStep(
    AbstractCCZ4Solver, exahype2.solvers.fv.rusanov.GlobalAdaptiveTimeStep
):
    """!

    CCZ4 solver using finite volumes and global adaptive time stepping incl enclave tasking

    Please consult CCZ4Solver_FV_GlobalAdaptiveTimeStepWithEnclaveTasking.

    """

    def __init__(
        self,
        name,
        patch_size,
        min_volume_h,
        max_volume_h,
        pde_terms_without_state
    ):
        AbstractCCZ4Solver.__init__(self)
        exahype2.solvers.fv.rusanov.GlobalAdaptiveTimeStep.__init__(
            self,
            name=name,
            patch_size=patch_size,
            unknowns=sum(self._FO_formulation_unknowns.values()),
            auxiliary_variables=0,
            min_volume_h=min_volume_h,
            max_volume_h=max_volume_h,
            time_step_relaxation=AbstractCCZ4Solver.Default_Time_Step_Size_Relaxation,
        )
        self._add_standard_includes()

        self.set_implementation(
            boundary_conditions=exahype2.solvers.PDETerms.Empty_Implementation,
            ncp=construct_FV_ncp(),
            flux=exahype2.solvers.PDETerms.None_Implementation,
            source_term=construct_FV_source_term(),
            refinement_criterion=exahype2.solvers.PDETerms.Empty_Implementation,
            eigenvalues=construct_FV_eigenvalues(),
        )

        self.postprocess_updated_patch += construct_FV_postprocessing_kernel()

    def add_tracer(
        self,
        name,
        coordinates,
        project,
        number_of_entries_between_two_db_flushes,
        data_delta_between_two_snapsots,
        time_delta_between_two_snapsots,
        clear_database_after_flush,
        tracer_unknowns,
    ):
        """!

        Add tracer to project
        
        Consult exahype2.tracer.DumpTracerIntoDatabase for an explanation of
        some of the arguments. Most of them are simply piped through to this 
        class.

        project: exahype2.Project

        """
        add_tracer_to_FV_solver(
            name,
            coordinates,
            project,
            self,
            number_of_entries_between_two_db_flushes,
            data_delta_between_two_snapsots,
            time_delta_between_two_snapsots,
            clear_database_after_flush,
            tracer_unknowns,
        )


class CCZ4Solver_FV_GlobalAdaptiveTimeStepWithEnclaveTasking(
    AbstractCCZ4Solver,
    exahype2.solvers.fv.rusanov.GlobalAdaptiveTimeStepWithEnclaveTasking,
):
    """!

    CCZ4 solver using finite volumes and global adaptive time stepping incl enclave tasking

    The constructor of this classs is straightforward and realises the standard
    steps of any numerical implementation of the CCZ4 scheme:

    1. Init the actual numerical scheme. This happens through the constructor
       of the base class.

    2. Add the header files that we need, i.e. those files which contain the
       actual CCZ4 implementation.

    3. Add some constants that any CCZ4 C++ code requires.

    4. Set the actual implementation, i.e. link the generic PDE terms to the
       CCZ4-specific function calls.

    5. Add the CCZ4-specific postprocessing.

    """

    def __init__(self, 
                 name, 
                 patch_size, 
                 min_volume_h, 
                 max_volume_h, 
                 pde_terms_without_state,
                 ):
        """!

        Construct solver with enclave tasking and adaptive time stepping

        """
        AbstractCCZ4Solver.__init__(self)
        exahype2.solvers.fv.rusanov.GlobalAdaptiveTimeStepWithEnclaveTasking.__init__(
            self,
            name=name,
            patch_size=patch_size,
            unknowns=sum(self._FO_formulation_unknowns.values()),
            auxiliary_variables=0,
            min_volume_h=min_volume_h,
            max_volume_h=max_volume_h,
            time_step_relaxation=AbstractCCZ4Solver.Default_Time_Step_Size_Relaxation,
            pde_terms_without_state=pde_terms_without_state
        )
        self._add_standard_includes()

        self.set_implementation(
            boundary_conditions=exahype2.solvers.PDETerms.Empty_Implementation,
            ncp=construct_FV_ncp(),
            flux=exahype2.solvers.PDETerms.None_Implementation,
            source_term=construct_FV_source_term(),
            refinement_criterion=exahype2.solvers.PDETerms.Empty_Implementation,
            eigenvalues=construct_FV_eigenvalues(),
        )

        self.postprocess_updated_patch += construct_FV_postprocessing_kernel()

    def add_tracer(
        self,
        name,
        coordinates,
        project,
        number_of_entries_between_two_db_flushes,
        data_delta_between_two_snapsots,
        time_delta_between_two_snapsots,
        clear_database_after_flush,
        tracer_unknowns,
    ):
        """!

        Add tracer to project
        
        Consult exahype2.tracer.DumpTracerIntoDatabase for an explanation of
        some of the arguments. Most of them are simply piped through to this 
        class.


        project: exahype2.Project

        """
        add_tracer_to_FV_solver(
            name,
            coordinates,
            project,
            self,
            number_of_entries_between_two_db_flushes,
            data_delta_between_two_snapsots,
            time_delta_between_two_snapsots,
            clear_database_after_flush,
            tracer_unknowns,
        )


def construct_FD4_ncp():
    return """
  double deltaQSerialised[NumberOfUnknowns*3];
  for (int i=0; i<NumberOfUnknowns; i++) {
    deltaQSerialised[i+0*NumberOfUnknowns] = 0.0;
    deltaQSerialised[i+1*NumberOfUnknowns] = 0.0;
    deltaQSerialised[i+2*NumberOfUnknowns] = 0.0;

    deltaQSerialised[i+normal*NumberOfUnknowns] = deltaQ[i];
  }
  ::applications::exahype2::ccz4::ncp(BTimesDeltaQ, Q, deltaQSerialised, normal%Dimensions, CCZ4LapseType, CCZ4ds, CCZ4c, CCZ4e, CCZ4f, CCZ4bs, CCZ4sk, CCZ4xi, CCZ4mu, CCZ4SO);
"""


def construct_FV_ncp():
    return """
  double deltaQSerialised[NumberOfUnknowns*3];
  for (int i=0; i<NumberOfUnknowns; i++) {
    deltaQSerialised[i+0*NumberOfUnknowns] = 0.0;
    deltaQSerialised[i+1*NumberOfUnknowns] = 0.0;
    deltaQSerialised[i+2*NumberOfUnknowns] = 0.0;

    deltaQSerialised[i+normal*NumberOfUnknowns] = deltaQ[i];
  }
  ::applications::exahype2::ccz4::ncp(BTimesDeltaQ, Q, deltaQSerialised, normal%Dimensions, CCZ4LapseType, CCZ4ds, CCZ4c, CCZ4e, CCZ4f, CCZ4bs, CCZ4sk, CCZ4xi, CCZ4mu, CCZ4SO);
"""


def construct_FD4_source_term():
    return """
  tarch::memset(S, 0.0, NumberOfUnknowns*sizeof(double)); 
  ::applications::exahype2::ccz4::source(S,Q, CCZ4LapseType, CCZ4ds, CCZ4c, CCZ4e, CCZ4f, CCZ4bs, CCZ4sk, CCZ4xi, CCZ4itau, CCZ4eta, CCZ4k1, CCZ4k2, CCZ4k3, CCZ4SO);
"""


def construct_FV_source_term():
    return """
  tarch::memset(S, 0.0, NumberOfUnknowns*sizeof(double)); 
  ::applications::exahype2::ccz4::source(S,Q, CCZ4LapseType, CCZ4ds, CCZ4c, CCZ4e, CCZ4f, CCZ4bs, CCZ4sk, CCZ4xi, CCZ4itau, CCZ4eta, CCZ4k1, CCZ4k2, CCZ4k3, CCZ4SO);
"""


def construct_FD4_eigenvalues():
    return """
  return ::applications::exahype2::ccz4::maxEigenvalue(Q, normal%Dimensions, CCZ4e, CCZ4ds, CCZ4GLMc, CCZ4GLMd );
"""


def construct_FV_eigenvalues():
    return """
  return ::applications::exahype2::ccz4::maxEigenvalue(Q, normal%Dimensions, CCZ4e, CCZ4ds, CCZ4GLMc, CCZ4GLMd );
"""


def construct_FD4_postprocessing_kernel():
    return """
{
    constexpr int itmax = {{NUMBER_OF_GRID_CELLS_PER_PATCH_PER_AXIS}} * {{NUMBER_OF_GRID_CELLS_PER_PATCH_PER_AXIS}} * {{NUMBER_OF_GRID_CELLS_PER_PATCH_PER_AXIS}};
    int index = 0;
    for (int i=0;i<itmax;i++)
    {
      applications::exahype2::ccz4::enforceCCZ4constraints( newQ+index );
      index += {{NUMBER_OF_UNKNOWNS}} + {{NUMBER_OF_AUXILIARY_VARIABLES}};
    }
  }
"""


def construct_FV_postprocessing_kernel():
    return """
{
    constexpr int itmax = {{NUMBER_OF_VOLUMES_PER_AXIS}} * {{NUMBER_OF_VOLUMES_PER_AXIS}} * {{NUMBER_OF_VOLUMES_PER_AXIS}};
    int index = 0;
    for (int i=0;i<itmax;i++)
    {
      applications::exahype2::ccz4::enforceCCZ4constraints( newQ+index );
      index += {{NUMBER_OF_UNKNOWNS}} + {{NUMBER_OF_AUXILIARY_VARIABLES}};
    }
  }
"""


def add_tracer_to_FV_solver(
    name,
    coordinates,
    project,
    solver,
    number_of_entries_between_two_db_flushes,
    data_delta_between_two_snapsots,
    time_delta_between_two_snapsots,
    clear_database_after_flush,
    tracer_unknowns,
):
    """!

    Add tracer to project
    
    Consult exahype2.tracer.DumpTracerIntoDatabase for an explanation of
    some of the arguments. Most of them are simply piped through to this 
    class.

    I realise this as a separate routine, as we need it for all FV flavours

    """
    number_of_attributes = (
        (solver.unknowns + solver.auxiliary_variables)
        if tracer_unknowns == None
        else len(tracer_unknowns)
    )
    tracer_particles = project.add_tracer(
        name=name, attribute_count=number_of_attributes
    )
    init_action_set = exahype2.tracer.InsertParticlesByCoordinates(
        particle_set=tracer_particles, coordinates=coordinates
    )
    init_action_set.descend_invocation_order = 0
    project.add_action_set_to_initialisation(init_action_set)

    project_on_tracer_properties_kernel = ""
    if tracer_unknowns == None:
        project_on_tracer_properties_kernel = (
            "::exahype2::fv::projectAllValuesOntoParticle_piecewiseLinear"
        )
    #      project_on_tracer_properties_kernel = "::exahype2::fv::projectAllValuesOntoParticle_piecewiseLinear_explicit_Euler"
    elif len(tracer_unknowns) == 1:
        project_on_tracer_properties_kernel = (
            "::exahype2::fv::projectValueOntoParticle_piecewiseLinear<{},{}>".format(
                i, tracer_unknowns.index(i)
            )
        )
    else:
        project_on_tracer_properties_kernel = (
            "::exahype2::fv::projectValuesOntoParticle_piecewiseLinear<{}>".format(
                tracer_unknowns
            )
            .replace("[", "")
            .replace("]", "")
        )

    tracing_action_set = exahype2.tracer.FiniteVolumesTracing(
        tracer_particles,
        solver,
        project_on_tracer_properties_kernel=project_on_tracer_properties_kernel,
    )
    tracing_action_set.descend_invocation_order = solver._action_set_update_cell.descend_invocation_order + 1
    project.add_action_set_to_timestepping(tracing_action_set)
    project.add_action_set_to_initialisation(tracing_action_set)

    dump_into_database_action_set = exahype2.tracer.DumpTracerIntoDatabase(
        particle_set=tracer_particles,
        solver=solver,
        filename=name + "-" + solver._name,
        number_of_entries_between_two_db_flushes=number_of_entries_between_two_db_flushes,
        output_precision=10,
        data_delta_between_two_snapsots = data_delta_between_two_snapsots,
        time_delta_between_two_snapsots = time_delta_between_two_snapsots,
        clear_database_after_flush         = clear_database_after_flush,
    )
    dump_into_database_action_set.descend_invocation_order = solver._action_set_update_cell.descend_invocation_order + 2
    project.add_action_set_to_timestepping(dump_into_database_action_set)


def add_tracer_to_FD4_solver(
    name,
    coordinates,
    project,
    solver,
    number_of_entries_between_two_db_flushes,
    data_delta_between_two_snapsots,
    time_delta_between_two_snapsots,
    clear_database_after_flush,
    tracer_unknowns,
):
    """!

    I realise this as a separate routine, as we need it for all FD4 flavours
    
    This is a wrapper around all the tracer handling. It adds the tracer to the
    exahype2.Project, but it also instantiates the solution to tracer mapping
    as well as the database bookkeeping.
    
    @param tracer_unknowns: Integer
      You can set this variable to None. In this case, all variables are 
      dumped.

    """
    number_of_attributes = (
        (solver.unknowns + solver.auxiliary_variables)
        if tracer_unknowns == None
        else len(tracer_unknowns)
    )
    tracer_particles = project.add_tracer(
        name=name, attribute_count=number_of_attributes
    )
    project.add_action_set_to_initialisation(
        exahype2.tracer.InsertParticlesByCoordinates(
            particle_set=tracer_particles, coordinates=coordinates
        )
    )
    project_on_tracer_properties_kernel = ""
    if tracer_unknowns == None:
        project_on_tracer_properties_kernel = (
            "::exahype2::fv::projectAllValuesOntoParticle_piecewiseLinear"
        )
    elif len(tracer_unknowns) == 1:
        project_on_tracer_properties_kernel = (
            "::exahype2::fv::projectValueOntoParticle_piecewiseLinear<{},{}>".format(
                i, tracer_unknowns.index(i)
            )
        )
    else:
        project_on_tracer_properties_kernel = (
            "::exahype2::fv::projectValuesOntoParticle_piecewiseLinear<{}>".format(
                tracer_unknowns
            )
            .replace("[", "")
            .replace("]", "")
        )

    tracing_action_set = exahype2.tracer.FiniteVolumesTracing(
        tracer_particles,
        solver,
        project_on_tracer_properties_kernel=project_on_tracer_properties_kernel,
    )
    tracing_action_set.descend_invocation_order = (
        solver._action_set_compute_final_linear_combination.descend_invocation_order + 1
    )
    project.add_action_set_to_timestepping(tracing_action_set)
    project.add_action_set_to_initialisation(tracing_action_set)

    dump_into_database_action_set = exahype2.tracer.DumpTracerIntoDatabase(
        particle_set=tracer_particles,
        solver=solver,
        filename=name + "-" + solver._name,
        number_of_entries_between_two_db_flushes=number_of_entries_between_two_db_flushes,
        output_precision=10,
        data_delta_between_two_snapsots = data_delta_between_two_snapsots,
        time_delta_between_two_snapsots = time_delta_between_two_snapsots,
        clear_database_after_flush      = clear_database_after_flush,
    )
    dump_into_database_action_set.descend_invocation_order = (
        solver._action_set_compute_final_linear_combination.descend_invocation_order + 2
    )
    project.add_action_set_to_timestepping(dump_into_database_action_set)


class CCZ4Solver_FD4_GlobalAdaptiveTimeStepWithEnclaveTasking(
    AbstractCCZ4Solver,
    exahype2.solvers.rkfd.fd4.GlobalAdaptiveTimeStepWithEnclaveTasking,
):
    """!

    CCZ4 solver using fourth-order finite differences and global adaptive time stepping incl enclave tasking

    The constructor of this classs is straightforward and realises the standard
    steps of any numerical implementation of the CCZ4 scheme:

    1. Init the actual numerical scheme. This happens through the constructor
       of the base class.

    2. Add the header files that we need, i.e. those files which contain the
       actual CCZ4 implementation.

    3. Add some constants that any CCZ4 C++ code requires.

    4. Set the actual implementation, i.e. link the generic PDE terms to the
       CCZ4-specific function calls.

    5. Add the CCZ4-specific postprocessing.

    6. Switch to higher-order interpolation and restriction.

    """

    def __init__(
        self,
        name,
        patch_size,
        rk_order,
        min_meshcell_h,
        max_meshcell_h,
        pde_terms_without_state,
        second_order=False
    ):
        """!

        Constructor

        Calibrate the default time step size calibration with 1/16 to take into
        account that we have a higher-order numerical scheme.

        """
        AbstractCCZ4Solver.__init__(self)
        if (second_order):
            AbstractCCZ4Solver.enable_second_order(self)
        exahype2.solvers.rkfd.fd4.GlobalAdaptiveTimeStepWithEnclaveTasking.__init__(
            self,
            name=name,
            patch_size=patch_size,
            rk_order=rk_order,
            unknowns=sum(self._FO_formulation_unknowns.values()),
            auxiliary_variables=0,
            min_meshcell_h=min_meshcell_h,
            max_meshcell_h=max_meshcell_h,
            time_step_relaxation=self.Default_Time_Step_Size_Relaxation,
            pde_terms_without_state = pde_terms_without_state,
        )

        self._add_standard_includes()

        self.set_implementation(
            boundary_conditions=exahype2.solvers.PDETerms.Empty_Implementation,
            ncp=construct_FD4_ncp(),
            flux=exahype2.solvers.PDETerms.None_Implementation,
            source_term=construct_FD4_source_term(),
            refinement_criterion=exahype2.solvers.PDETerms.Empty_Implementation,
            eigenvalues=construct_FD4_eigenvalues(),
        )

        self.postprocess_updated_patch += construct_FD4_postprocessing_kernel()

        exahype2.solvers.rkfd.fd4.switch_to_FD4_tensor_product_interpolation(
            self, "TP_linear_with_linear_extrap_normal_interp"
        )
        exahype2.solvers.rkfd.fd4.switch_to_FD4_tensor_product_restriction(
            self, "TP_average_normal_extrap"
        )

    def add_tracer(
        self,
        name,
        coordinates,
        project,
        number_of_entries_between_two_db_flushes,
        data_delta_between_two_snapsots,
        time_delta_between_two_snapsots,
        clear_database_after_flush,
        tracer_unknowns,
    ):
        """!

        Add tracer to project
        
        This is a delegate to add_tracer_to_FD4_solver() which passes the 
        object in as first argument.
        
        Consult exahype2.tracer.DumpTracerIntoDatabase for an explanation of
        some of the arguments. Most of them are simply piped through to this 
        class.

        @param project: exahype2.Project
        
        @param tracer_unknowns: Integer
          You can set this variable to None. In this case, all variables are 
          dumped.

        """
        add_tracer_to_FD4_solver(
            name,
            coordinates,
            project,
            self,
            number_of_entries_between_two_db_flushes,
            data_delta_between_two_snapsots,
            time_delta_between_two_snapsots,
            clear_database_after_flush,
            tracer_unknowns,
        )


class CCZ4Solver_FD4_GlobalAdaptiveTimeStep(
    AbstractCCZ4Solver, exahype2.solvers.rkfd.fd4.GlobalAdaptiveTimeStep
):
    """!

    CCZ4 solver using fourth-order finite differences and global adaptive time stepping without enclave tasking

    Consult CCZ4Solver_FD4_GlobalAdaptiveTimeStepWithEnclaveTasking please.

    """

    def __init__(
        self,
        name,
        patch_size,
        rk_order,
        min_meshcell_h,
        max_meshcell_h,
        second_order=False
    ):
        """!

        Constructor

        Calibrate the default time step size calibration with 1/16 to take into
        account that we have a higher-order numerical scheme.

        """
        AbstractCCZ4Solver.__init__(self)
        if (second_order):
            AbstractCCZ4Solver.enable_second_order(self)
        exahype2.solvers.rkfd.fd4.GlobalAdaptiveTimeStep.__init__(
            self,
            name=name,
            patch_size=patch_size,
            rk_order=rk_order,
            unknowns=sum(self._FO_formulation_unknowns.values()),
            auxiliary_variables=0,
            min_meshcell_h=min_meshcell_h,
            max_meshcell_h=max_meshcell_h,
            time_step_relaxation=self.Default_Time_Step_Size_Relaxation
            ,
        )

        self._add_standard_includes()

        self.set_implementation(
            boundary_conditions=exahype2.solvers.PDETerms.Empty_Implementation,
            ncp=construct_FD4_ncp(),
            flux=exahype2.solvers.PDETerms.None_Implementation,
            source_term=construct_FD4_source_term(),
            refinement_criterion=exahype2.solvers.PDETerms.Empty_Implementation,
            eigenvalues=construct_FD4_eigenvalues(),
        )

        self.postprocess_updated_patch += construct_FD4_postprocessing_kernel()

        exahype2.solvers.rkfd.fd4.switch_to_FD4_tensor_product_interpolation(
            self, "TP_linear_with_linear_extrap_normal_interp"
        )
        exahype2.solvers.rkfd.fd4.switch_to_FD4_tensor_product_restriction(
            self, "TP_average_normal_extrap"
        )

    def add_tracer(
        self,
        name,
        coordinates,
        project,
        number_of_entries_between_two_db_flushes,
        data_delta_between_two_snapsots,
        time_delta_between_two_snapsots,
        clear_database_after_flush,
        tracer_unknowns,
    ):
        """!

        Add tracer to project
        
        Consult exahype2.tracer.DumpTracerIntoDatabase for an explanation of
        some of the arguments. Most of them are simply piped through to this 
        class.

        project: exahype2.Project

        """
        add_tracer_to_FD4_solver(
            name,
            coordinates,
            project,
            self,
            number_of_entries_between_two_db_flushes,
            data_delta_between_two_snapsots,
            time_delta_between_two_snapsots,
            clear_database_after_flush = clear_database_after_flush,
            tracer_unknowns = tracer_unknowns,
        )


class CCZ4Solver_FD4_SecondOrderFormulation_GlobalAdaptiveTimeStepWithEnclaveTasking(
    AbstractCCZ4Solver,
    exahype2.solvers.rkfd.fd4.GlobalAdaptiveTimeStepWithEnclaveTasking,
):
    """!

    Variation of classic FD4 which relies on second order PDE formulation

    The traditional ExaHyPE CCZ4 formulation is the first order formulation
    introduced by Dumbser et al. In this formulation, the second order terms
    in CCZ4 are substituted with helper variables which represent first order
    derivatives. While formally straightforward, keeping the whole system
    consistent and stricly hyperbolic is a different challenge.

    In this revised version, we have to evolve the primary quantities of CCZ4
    and also the helper variables, which blows the overall system up to 59
    equations in its simplest form. The work by Dumbser and others suggest that
    this is a consistent and stable approach, but limited work is actually
    published on proper physical simulations. We therefore also implemented a
    second order PDE version within ExaHyPE.

    This second order variant is not really second order from the start.
    Instead, we use the first order formulation, and we reconstruct the helper
    term via finite differences prior to the compute kernel application. That is,
    the compute kernels see variables representing first order derivatives, and
    they also evolve these guys. Afterwards, we throw away the evolved quantities
    and reconstruct them from the primary unknowns prior to the next time step.

    This might not be super efficient (it would be faster to stick to the
    second order formulation right from the start), but it allows us to reuse
    the compute kernels written for the first order PDE formulation.

    ## Data layout

    We have now a smaller number of real unknowns, i.e. only those guys who
    belong to the "original" second-order formulation. The remaining quantities
    compared to a first-order formulation are technically material or auxiliary
    quantities. We model them as such, which allows ExaHyPE`s data management
    to deal more efficiently with them.


    reconstruction_type: "4thOrder", "centralDifferences", "leftDifference", "rightDifference"

    """

    def __init__(
        self,
        name,
        patch_size,
        rk_order,
        min_meshcell_h,
        max_meshcell_h,
        reconstruction_type,
    ):
        """!

        Constructor

        Calibrate the default time step size calibration with 1/16 to take into
        account that we have a higher-order numerical scheme.

        """
        AbstractCCZ4Solver.__init__(self)
        exahype2.solvers.rkfd.fd4.GlobalAdaptiveTimeStepWithEnclaveTasking.__init__(
            self,
            name=name,
            patch_size=patch_size,
            rk_order=rk_order,
            unknowns=len(self._SO_formulation_unknowns),
            auxiliary_variables=sum(self._FO_formulation_unknowns.values())
            - len(self._SO_formulation_unknowns),
            min_meshcell_h=min_meshcell_h,
            max_meshcell_h=max_meshcell_h,
            time_step_relaxation=AbstractCCZ4Solver.Default_Time_Step_Size_Relaxation
            / 16.0,
        )

        self._add_standard_includes()

        self.set_implementation(
            boundary_conditions=exahype2.solvers.PDETerms.Empty_Implementation,
            ncp="""
  double deltaQSerialised[NumberOfUnknowns*3];
  for (int i=0; i<NumberOfUnknowns; i++) {
    deltaQSerialised[i+0*NumberOfUnknowns] = 0.0;
    deltaQSerialised[i+1*NumberOfUnknowns] = 0.0;
    deltaQSerialised[i+2*NumberOfUnknowns] = 0.0;

    deltaQSerialised[i+normal*NumberOfUnknowns] = deltaQ[i];
  }
  ::applications::exahype2::ccz4::ncpSecondOrderFormulation(BTimesDeltaQ, Q, deltaQSerialised, normal%Dimensions, CCZ4LapseType, CCZ4ds, CCZ4c, CCZ4e, CCZ4f, CCZ4bs, CCZ4sk, CCZ4xi, CCZ4mu, CCZ4SO);
""",
            flux=exahype2.solvers.PDETerms.None_Implementation,
            source_term="""
  tarch::memset(S, 0.0, NumberOfUnknowns*sizeof(double)); 
  ::applications::exahype2::ccz4::sourceSecondOrderFormulation(S,Q, CCZ4LapseType, CCZ4ds, CCZ4c, CCZ4e, CCZ4f, CCZ4bs, CCZ4sk, CCZ4xi, CCZ4itau, CCZ4eta, CCZ4k1, CCZ4k2, CCZ4k3);
""",
            refinement_criterion=exahype2.solvers.PDETerms.Empty_Implementation,
            eigenvalues="""
          // do we only set Q
  return ::applications::exahype2::ccz4::maxEigenvalueSecondOrderFormulation(Q, normal%Dimensions, CCZ4e, CCZ4ds, CCZ4GLMc, CCZ4GLMd );
""",
        )

        self.postprocess_updated_patch += """
{
    constexpr int itmax = {{NUMBER_OF_GRID_CELLS_PER_PATCH_PER_AXIS}} * {{NUMBER_OF_GRID_CELLS_PER_PATCH_PER_AXIS}} * {{NUMBER_OF_GRID_CELLS_PER_PATCH_PER_AXIS}};
    int index = 0;
    for (int i=0;i<itmax;i++)
    {
      applications::exahype2::ccz4::enforceCCZ4constraintsSecondOrderFormulation( newQ+index );
      index += {{NUMBER_OF_UNKNOWNS}} + {{NUMBER_OF_AUXILIARY_VARIABLES}};
    }
  }
"""

        exahype2.solvers.rkfd.fd4.switch_to_FD4_tensor_product_interpolation(
            self, "TP_linear_with_linear_extrap_normal_interp"
        )
        exahype2.solvers.rkfd.fd4.switch_to_FD4_tensor_product_restriction(
            self, "TP_average_normal_extrap"
        )

        self.preprocess_reconstructed_patch += (
            """
::exahype2::CellData  reconstructedPatchData( 
  oldQWithHalo, 
  marker.x(), 
  marker.h(), 
  timeStamp, 
  timeStepSize,
  nullptr // targetPatch 
);
::applications::exahype2::ccz4::recomputeAuxiliaryVariablesFD4_"""
            + reconstruction_type
            + """(
  reconstructedPatchData,
  {{NUMBER_OF_GRID_CELLS_PER_PATCH_PER_AXIS}},
  3,                        // haloSize,
  {{NUMBER_OF_UNKNOWNS}},
  {{NUMBER_OF_AUXILIARY_VARIABLES}}
);
"""
        )

    def add_tracer(
        self,
        name,
        coordinates,
        project,
        number_of_entries_between_two_db_flushes,
        data_delta_between_two_snapsots,
        time_delta_between_two_snapsots,
        clear_database_after_flush,
        tracer_unknowns,
    ):
        """!

        Add tracer to project
        
        Consult exahype2.tracer.DumpTracerIntoDatabase for an explanation of
        some of the arguments. Most of them are simply piped through to this 
        class.

        project: exahype2.Project

        """
        number_of_attributes = (
            (self.unknowns + self.auxiliary_variables)
            if tracer_unknowns == None
            else len(tracer_unknowns)
        )
        tracer_particles = project.add_tracer(
            name=name, attribute_count=number_of_attributes
        )
        init_action_set = exahype2.tracer.InsertParticlesByCoordinates(
            particle_set=tracer_particles, coordinates=coordinates
        )
        init_action_set.descend_invocation_order = 0
        project.add_action_set_to_initialisation(init_action_set)

        project_on_tracer_properties_kernel = ""
        if tracer_unknowns == None:
            project_on_tracer_properties_kernel = (
                "::exahype2::fv::projectAllValuesOntoParticle_piecewiseLinear"
            )
        elif len(tracer_unknowns) == 1:
            project_on_tracer_properties_kernel = "::exahype2::fv::projectValueOntoParticle_piecewiseLinear<{},{}>".format(
                i, tracer_unknowns.index(i)
            )
        else:
            project_on_tracer_properties_kernel = (
                "::exahype2::fv::projectValuesOntoParticle_piecewiseLinear<{}>".format(
                    tracer_unknowns
                )
                .replace("[", "")
                .replace("]", "")
            )

        tracing_action_set = exahype2.tracer.FiniteVolumesTracing(
            tracer_particles,
            self,
            project_on_tracer_properties_kernel=project_on_tracer_properties_kernel,
        )
        tracing_action_set.descend_invocation_order = (
            self._action_set_compute_final_linear_combination.descend_invocation_order + 1
        )
        project.add_action_set_to_timestepping(tracing_action_set)
        project.add_action_set_to_initialisation(tracing_action_set)

        dump_into_database_action_set = exahype2.tracer.DumpTracerIntoDatabase(
            particle_set=tracer_particles,
            solver=self,
            filename=name + "-" + self._name,
            number_of_entries_between_two_db_flushes=number_of_entries_between_two_db_flushes,
            output_precision=10,
            data_delta_between_two_snapsots = data_delta_between_two_snapsots,
            time_delta_between_two_snapsots = time_delta_between_two_snapsots,
            clear_database_after_flush         = True,
        )
        dump_into_database_action_set.descend_invocation_order = (
            self._action_set_compute_final_linear_combination.descend_invocation_order + 2
        )
        project.add_action_set_to_timestepping(dump_into_database_action_set)


def construct_DG_ncp():
    return """
  double dQdxSerialised[NumberOfUnknowns*3];
  for (int i=0; i<NumberOfUnknowns; i++) {
    dQdxSerialised[i+0*NumberOfUnknowns] = 0.0;
    dQdxSerialised[i+1*NumberOfUnknowns] = 0.0;
    dQdxSerialised[i+2*NumberOfUnknowns] = 0.0;

    dQdxSerialised[i+normal*NumberOfUnknowns] = deltaQ[i];
  }
  ::applications::exahype2::ccz4::ncp(BTimesDeltaQ, Q, dQdxSerialised, normal%Dimensions, CCZ4LapseType, CCZ4ds, CCZ4c, CCZ4e, CCZ4f, CCZ4bs, CCZ4sk, CCZ4xi, CCZ4mu, CCZ4SO);
"""


def construct_DG_source_term():
    return """
  tarch::memset(S, 0.0, NumberOfUnknowns*sizeof(double)); 
  ::applications::exahype2::ccz4::source(S,Q, CCZ4LapseType, CCZ4ds, CCZ4c, CCZ4e, CCZ4f, CCZ4bs, CCZ4sk, CCZ4xi, CCZ4itau, CCZ4eta, CCZ4k1, CCZ4k2, CCZ4k3, CCZ4SO);
"""


def construct_DG_eigenvalues():
    return """
  return ::applications::exahype2::ccz4::maxEigenvalue(Q, normal%Dimensions, CCZ4e, CCZ4ds, CCZ4GLMc, CCZ4GLMd );
"""

def construct_DG_postprocessing_kernel():
    return """
{
    constexpr int itmax = ({{DG_ORDER}}+1) * ({{DG_ORDER}}+1) * ({{DG_ORDER}}+1);
    int index = 0;
    for (int i=0;i<itmax;i++)
    {
      applications::exahype2::ccz4::enforceCCZ4constraints( newQ+index );
      index += {{NUMBER_OF_UNKNOWNS}} + {{NUMBER_OF_AUXILIARY_VARIABLES}};
    }
  }
"""


def add_tracer_to_DG_solver(name,
            coordinates,
            project,
            self,
            number_of_entries_between_two_db_flushes,
            data_delta_between_two_snapsots,
            time_delta_between_two_snapsots,
            clear_database_after_flush,
            tracer_unknowns,
):
    number_of_attributes = (
        (self.unknowns + self.auxiliary_variables)
        if tracer_unknowns == None
        else len(tracer_unknowns)
    )
    tracer_particles = project.add_tracer(
        name=name, attribute_count=number_of_attributes
    )
    init_action_set = exahype2.tracer.InsertParticlesByCoordinates(
        particle_set=tracer_particles, coordinates=coordinates
    )
    init_action_set.descend_invocation_order = 0
    project.add_action_set_to_initialisation(init_action_set)

    assert tracer_unknowns == None

    tracing_action_set = exahype2.tracer.DiscontinuousGalerkinTracing(
        tracer_particles,
        solver=self,
        project_on_tracer_properties_kernel="::exahype2::dg::projectAllValuesOntoParticle",
    )
    tracing_action_set.descend_invocation_order = (
        self._action_set_compute_final_linear_combination_and_project_solution_onto_faces.descend_invocation_order
        + 1
    )
    project.add_action_set_to_timestepping(tracing_action_set)
    project.add_action_set_to_initialisation(tracing_action_set)

    dump_into_database_action_set = exahype2.tracer.DumpTracerIntoDatabase(
        particle_set=tracer_particles,
        solver=self,
        filename=name + "-" + self._name,
        number_of_entries_between_two_db_flushes = number_of_entries_between_two_db_flushes,
        output_precision=10,
        data_delta_between_two_snapsots = data_delta_between_two_snapsots,
        time_delta_between_two_snapsots = time_delta_between_two_snapsots,
        clear_database_after_flush      = clear_database_after_flush,
    )
    dump_into_database_action_set.descend_invocation_order = (
        self._action_set_compute_final_linear_combination_and_project_solution_onto_faces.descend_invocation_order
        + 1
    )
    project.add_action_set_to_timestepping(dump_into_database_action_set)


class CCZ4Solver_RKDG_GlobalAdaptiveTimeStepWithEnclaveTasking(
    AbstractCCZ4Solver,
    exahype2.solvers.rkdg.rusanov.GlobalAdaptiveTimeStepWithEnclaveTasking,
):
    """!

    CCZ4 solver using Runge-Kutta Discontinuous Galerkin and global adaptive time stepping incl enclave tasking

    The constructor of this classs is straightforward and realises the standard
    steps of any numerical implementation of the CCZ4 scheme:

    1. Init the actual numerical scheme. This happens through the constructor
       of the base class.

    2. Add the header files that we need, i.e. those files which contain the
       actual CCZ4 implementation.

    3. Add some constants that any CCZ4 C++ code requires.

    4. Set the actual implementation, i.e. link the generic PDE terms to the
       CCZ4-specific function calls.

    5. Add the CCZ4-specific postprocessing.

    6. Switch to higher-order interpolation and restriction.

    """

    def __init__(
        self,
        name,
        rk_order,
        polynomials,
        min_cell_h,
        max_cell_h,
        pde_terms_without_state,
    ):
        """!

        Construct solver with enclave tasking

        """
        AbstractCCZ4Solver.__init__(self)
        exahype2.solvers.rkdg.rusanov.GlobalAdaptiveTimeStepWithEnclaveTasking.__init__(
            self,
            name=name,
            rk_order=rk_order,
            polynomials=polynomials,
            unknowns=sum(self._FO_formulation_unknowns.values()),
            auxiliary_variables=0,
            min_cell_h=min_cell_h,
            max_cell_h=max_cell_h,
            time_step_relaxation=AbstractCCZ4Solver.Default_Time_Step_Size_Relaxation,
            pde_terms_without_state=pde_terms_without_state
        )

        self._add_standard_includes()

        self.set_implementation(
            boundary_conditions=exahype2.solvers.PDETerms.Empty_Implementation,
            ncp=construct_DG_ncp(),
            flux=exahype2.solvers.PDETerms.None_Implementation,
            source_term=construct_DG_source_term(),
            refinement_criterion=exahype2.solvers.PDETerms.Empty_Implementation,
            eigenvalues=construct_DG_eigenvalues()
        )

        self.postprocess_updated_cell_after_final_linear_combination += construct_DG_postprocessing_kernel()

    def add_tracer(
        self,
        name,
        coordinates,
        project,
        number_of_entries_between_two_db_flushes,
        data_delta_between_two_snapsots,
        time_delta_between_two_snapsots,
        clear_database_after_flush,
        tracer_unknowns,
    ):
        """!

        Add tracer to project
        
        Consult exahype2.tracer.DumpTracerIntoDatabase for an explanation of
        some of the arguments. Most of them are simply piped through to this 
        class.

        At this point, we have not yet created the Peano 4 project. Therefore, we
        have not yet befilled the time stepping action set.

        project: exahype2.Project

        """
        add_tracer_to_DG_solver(
            name,
            coordinates,
            project,
            self,
            number_of_entries_between_two_db_flushes,
            data_delta_between_two_snapsots,
            time_delta_between_two_snapsots,
            clear_database_after_flush,
            tracer_unknowns,
        )


class CCZ4Solver_RKDG_GlobalAdaptiveTimeStep(
    AbstractCCZ4Solver,
    exahype2.solvers.rkdg.rusanov.GlobalAdaptiveTimeStep,
):
    """!

    CCZ4 solver using Runge-Kutta Discontinuous Galerkin and global adaptive time stepping incl enclave tasking

    The constructor of this classs is straightforward and realises the standard
    steps of any numerical implementation of the CCZ4 scheme:

    1. Init the actual numerical scheme. This happens through the constructor
       of the base class.

    2. Add the header files that we need, i.e. those files which contain the
       actual CCZ4 implementation.

    3. Add some constants that any CCZ4 C++ code requires.

    4. Set the actual implementation, i.e. link the generic PDE terms to the
       CCZ4-specific function calls.

    5. Add the CCZ4-specific postprocessing.

    6. Switch to higher-order interpolation and restriction.

    """

    def __init__(
        self,
        name,
        rk_order,
        polynomials,
        min_cell_h,
        max_cell_h,
        pde_terms_without_state,
    ):
        """!

        Construct solver with enclave tasking

        """
        AbstractCCZ4Solver.__init__(self)
        exahype2.solvers.rkdg.rusanov.GlobalAdaptiveTimeStep.__init__(
            self,
            name=name,
            rk_order=rk_order,
            polynomials=polynomials,
            unknowns=sum(self._FO_formulation_unknowns.values()),
            auxiliary_variables=0,
            min_cell_h=min_cell_h,
            max_cell_h=max_cell_h,
            time_step_relaxation=AbstractCCZ4Solver.Default_Time_Step_Size_Relaxation,
            pde_terms_without_state=pde_terms_without_state
        )

        self._add_standard_includes()

        self.set_implementation(
            boundary_conditions=exahype2.solvers.PDETerms.Empty_Implementation,
            ncp=construct_DG_ncp(),
            flux=exahype2.solvers.PDETerms.None_Implementation,
            source_term=construct_DG_source_term(),
            refinement_criterion=exahype2.solvers.PDETerms.Empty_Implementation,
            eigenvalues=construct_DG_eigenvalues()
        )

        self.postprocess_updated_cell_after_final_linear_combination += construct_DG_postprocessing_kernel()

    def add_tracer(
        self,
        name,
        coordinates,
        project,
        number_of_entries_between_two_db_flushes,
        data_delta_between_two_snapsots,
        time_delta_between_two_snapsots,
        clear_database_after_flush,
        tracer_unknowns,
    ):
        """!

        Add tracer to project
        
        Consult exahype2.tracer.DumpTracerIntoDatabase for an explanation of
        some of the arguments. Most of them are simply piped through to this 
        class.

        At this point, we have not yet created the Peano 4 project. Therefore, we
        have not yet befilled the time stepping action set.

        project: exahype2.Project

        """
        add_tracer_to_DG_solver(
            name,
            coordinates,
            project,
            self,
            number_of_entries_between_two_db_flushes,
            data_delta_between_two_snapsots,
            time_delta_between_two_snapsots,
            clear_database_after_flush,
            tracer_unknowns,
        )
