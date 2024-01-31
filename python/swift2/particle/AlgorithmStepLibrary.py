from swift2.particle.AlgorithmStep import AlgorithmStep
from swift2.particle.SPHParticle import SPHParticle


class ParticleIteratorTemplate(str):
    """!
    A mini-wrapper around a string for particle iterator.
    Exists for convenience: adds self.insert_function() method,
    which inserts the function which the iterators are supposed to call
    on a particle or particle pair.
    """

    def __init__(self, template):
        """!
        template: str
            the string containing the particle iterator template
        """
        self.template = template
        return

    def insert_function(self, function):
        """!
        insert a function into the template string.

        Returns the particle iterator template with the function inserted.
        """

        return self.template.replace("*FUNCTION*", function)




def _get_particle_iterator_templates(particle_name, kernel_realisation):
    """!
    Construct iterator call templates given the desired particle interaction
    kernel realisation. These signatures are strings containing c++ code
    snippets of the correct iterator call signature.

    @param particle_name: str
        the particle class name.
    @param particle_interaction_kernel_realisation: SPHParticle.ParticleKernelRealisation
        enum for kernel realisation.

    returns:

    strings containing particle iterator code snippets.

    iterate_over_particles_assigned_to_vertex_for_move:
        snippet to use for particle interactons which will move assigned to a vertex
    iterate_over_particles_assigned_to_cell:
        snippet to use for particle-particle interactions assigned to a cell
    iterate_over_particles_assigned_to_vertex:
        snippet to use for particle interactions assigned to a vertex
    iterate_over_particles_assigned_to_vertex_with_global_reduction
        snippet to use for particle interactions assigned to a vertex which also
        contain a global reduction
    """

    PARTICLE = particle_name

    iterate_over_particles_assigned_to_vertex_for_move = None
    iterate_over_particles_assigned_to_vertex = None
    iterate_over_particles_assigned_to_cell = None
    iterate_over_particles_assigned_to_vertex_with_global_reduction = None

    if (kernel_realisation == SPHParticle.ParticleKernelRealisation.NO_OPTIMISATION):
        iterate_over_particles_assigned_to_vertex = f"""::swift2::kernels::forAllParticles(marker, assignedParticles, *FUNCTION*<globaldata::{PARTICLE}>, ::swift2::kernels::alwaysUpdateInVertexKernel<globaldata::{PARTICLE}>);"""
        iterate_over_particles_assigned_to_cell = (                         "::swift2::kernels::forAllParticlePairs( marker, localParticles, activeParticles, *FUNCTION*<globaldata::" + particle_name + ">, ::swift2::kernels::alwaysUpdateInCellKernel<globaldata::" + particle_name + ">, ::swift2::kernels::alwaysUpdateParticlePairs<globaldata::" + particle_name + ">);")
        iterate_over_particles_assigned_to_vertex_with_global_reduction = ( "::swift2::kernels::forAllParticles( marker, assignedParticles, *FUNCTION*<globaldata::" + particle_name + ">, ::swift2::kernels::alwaysUpdateInVertexKernel<globaldata::" + particle_name + ">);")
    elif ( kernel_realisation == SPHParticle.ParticleKernelRealisation.USE_OUTER_GUARDS):
        iterate_over_particles_assigned_to_vertex = ( "::swift2::kernels::forAllParticles( marker, assignedParticles, *FUNCTION*<globaldata::" + particle_name + ">);")
        iterate_over_particles_assigned_to_cell = ( "::swift2::kernels::forAllParticlePairs( marker, localParticles, activeParticles, *FUNCTION*<globaldata::" + particle_name + ">);")
    elif ( kernel_realisation == SPHParticle.ParticleKernelRealisation.VECTORISE_ALL):
        iterate_over_particles_assigned_to_vertex = ( "::swift2::kernels::forAllParticlesVectorised( marker, assignedParticles, numberOfAssignedParticles, *FUNCTION*WithMasking<globaldata::" + particle_name + ">);")
        iterate_over_particles_assigned_to_cell = ( "::swift2::kernels::forAllParticlePairsVectorised( marker, localParticles, activeParticles, numberOfParticlesPerLocalVertex, _numberOfActiveParticlesPerVertex, *FUNCTION*WithMasking<globaldata::" + particle_name + ">);")
    elif ( kernel_realisation == SPHParticle.ParticleKernelRealisation.VECTORISE_WITH_SEPARATE_DISTANCE_CHECKS):
        iterate_over_particles_assigned_to_vertex = ( "::swift2::kernels::forAllParticlesVectoriseWithCheckPreamble( marker, assignedParticles, numberOfAssignedParticles, *FUNCTION*<globaldata::" + particle_name + ">);")
        iterate_over_particles_assigned_to_cell = ( "::swift2::kernels::forAllParticlePairsVectoriseWithCheckPreamble( marker, localParticles, activeParticles, numberOfParticlesPerLocalVertex, _numberOfActiveParticlesPerVertex, *FUNCTION*<globaldata::" + particle_name + ">);"
        )
    else:
        raise NotImplementedError(f"Requested kernel realisation {particle_interaction_kernel_realisatio} not implemented")

    if iterate_over_particles_assigned_to_vertex_for_move is None:
        iterate_over_particles_assigned_to_vertex_for_move = (
            """
            auto oldParticlePositions = ::toolbox::particles::assignmentchecks::recordParticlePositions(assignedParticles);
            ::swift2::kernels::forAllParticles( marker, assignedParticles, *FUNCTION*<globaldata::"""
            + particle_name
            + """>);
            ::toolbox::particles::assignmentchecks::traceParticleMovements(assignedParticles, oldParticlePositions, _spacetreeId);
            """
            )
        ## @todo This is not nice. We should still use the vectorised version if the vertex is gathered.


    #  Set defaults or raise error if there shouldn't be defaults and we missed something.

    if iterate_over_particles_assigned_to_vertex_for_move is None:
        raise ValueError("We shouldn't be here.")

    if iterate_over_particles_assigned_to_cell is None:
        raise ValueError("We shouldn't be here.")

    if iterate_over_particles_assigned_to_vertex is None:
        raise ValueError("We shouldn't be here.")

    if iterate_over_particles_assigned_to_vertex_with_global_reduction is None:
        iterate_over_particles_assigned_to_vertex_with_global_reduction = (
            "::swift2::kernels::forAllParticles( marker, assignedParticles, *FUNCTION*<globaldata::" + particle_name + ">);"
        )

    #  finally, wrap the templates into ParticleIteratorTemplate objects
    vertex_for_move = ParticleIteratorTemplate(iterate_over_particles_assigned_to_vertex_for_move)
    cell = ParticleIteratorTemplate(iterate_over_particles_assigned_to_cell)
    vertex = ParticleIteratorTemplate(iterate_over_particles_assigned_to_vertex_with_global_reduction)
    vertex_with_global_reduction = ParticleIteratorTemplate(iterate_over_particles_assigned_to_vertex_with_global_reduction)

    return vertex_for_move, cell, vertex, vertex_with_global_reduction




def get_algorithm_step_dict(particle):
    """!

    Sets up a dict of the algorithm steps for this scheme so they can be accessed at various points.

    There are three types of iterators:

    - The one that's used to run over all particle assigned to a vertex. It
      is called iterate_over_particles_assigned_to_vertex.
    - There is one to iterate over all particle pairs associated with a
      cell. This one is called iterate_over_particles_assigned_to_cell.

    From hereon, we can define some "derived" iterators:

    - One special iterator runs over all particles associated with a vertex
      and allows them to move. This one is titled
      iterate_over_particles_assigned_to_vertex_for_move and equals
      iterate_over_particles_assigned_to_vertex. However, it embeds the
      latter into some bookkeeping for the validation of the
      vertex-particle assocation.
    - Another special iterator runs over all particles but allows them to
      reduce into some global space. If you use vectorisation, this one
      cannot be vectorised by definition.

    Both special iterators are constructed to default values unless you
    explicitly set them beforehand.

    """

    algorithm_steps_dict = {}

    realisation = particle._particle_interaction_kernel_realisation
    project_namespace = particle._swift_project_namespace

    NAME = particle.name
    NAMESPACE ="::".join(project_namespace)


    v_iter_for_move, c_iter, v_iter, v_iter_with_global_reduction = _get_particle_iterator_templates(NAME, realisation)

    # @todo Mladen, can we remove this one please?
    flagBoundaryParticlesStep = AlgorithmStep(
        name="FlagBoundaryParticles",
        dependencies=AlgorithmStep.Dependencies.SELF,
        effect=AlgorithmStep.Effect.ALTER_LOCAL_STATE,
        touch_vertex_first_time_kernel="",
        touch_vertex_last_time_kernel="""::swift2::kernels::flagBoundaryParticles(assignedParticles, {0}, {1}, {2});""".format(
            NAMESPACE + "::HYDRO_PART_NUMBER", NAMESPACE + "::DomainSize", NAMESPACE + "::DomainOffset",
        ),
        includes="""
                   #include "Constants.h"
                   #include "swift2/kernels/ParticleSelfInteraction.h"
                   """,
    )
    algorithm_steps_dict["FlagBoundaryParticles"] = flagBoundaryParticlesStep


    kick1 = AlgorithmStep(
        name="Kick1",
        dependencies=AlgorithmStep.Dependencies.SELF,
        effect=AlgorithmStep.Effect.ALTER_LOCAL_STATE,
        touch_vertex_first_time_kernel=v_iter.insert_function( "::swift2::timestepping::resetMovedParticleMarker"),
        touch_vertex_last_time_kernel=v_iter.insert_function( "::swift2::kernels::legacy::leapfrogKickWithGlobalTimeStepSize"),
        prepare_traversal_kernel="""::swift2::timestepping::computeCFLTimeStepSizeSPH<globaldata::{}>();""".format( NAME,),
        includes="""
            #include "swift2/kernels/legacy/Leapfrog.h"
            #include "swift2/timestepping/GlobalTimeStepping.h"
            #include "swift2/timestepping/TimeStepping.h"
        """,
    )
    algorithm_steps_dict["Kick1"] = kick1

    drift = AlgorithmStep(
        name="Drift",
        dependencies=AlgorithmStep.Dependencies.SELF,
        effect=AlgorithmStep.Effect.CHANGE_POSITION_OR_INTERACTION_RADIUS,
        touch_vertex_first_time_kernel=v_iter.insert_function(
            "::swift2::timestepping::resetMovedParticleMarker"
        ),
        touch_vertex_last_time_kernel=v_iter_for_move.insert_function(
            "::swift2::kernels::legacy::leapfrogDriftWithGlobalTimeStepSize"
        ),
        includes="""
            #include "Constants.h"
            #include "swift2/kernels/legacy/Leapfrog.h"
            #include "swift2/timestepping/GlobalTimeStepping.h"
            #include "swift2/timestepping/TimeStepping.h"
                                       """,
        touch_vertex_first_time_dependency_policy="TouchAtLeastOnce_AllPreviousStepsUpdateAtLeastOnce",
        touch_vertex_last_time_dependency_policy="TouchAtLeastOnce_AllPreviousStepsUpdateAtLeastOnce",
    )
    algorithm_steps_dict["Drift"] = drift

    # From the SPH side of things, this doesn't need to be an AlgorithmStep
    # on its own. We could merge it together with the drifts. However, in order
    # to ensure that the particles which have just been moved in the preceeding
    # drift sweep are sorted correctly into the cells and vertices, we need an
    # extra sweep.
    predictHydro = AlgorithmStep(
        name="PredictHydro",
        dependencies=AlgorithmStep.Dependencies.SELF,
        effect=AlgorithmStep.Effect.ALTER_LOCAL_STATE,
        #            touch_vertex_first_time_kernel = iterate_over_particles_assigned_to_vertex.format("""::swift2::timestepping::resetMovedParticleMarker(assignedParticles);""",
        touch_vertex_first_time_kernel=v_iter.insert_function(
            "::swift2::kernels::legacy::hydroPredictExtra"
        ),
        includes="""
            #include "Constants.h"
            #include "swift2/kernels/legacy/Leapfrog.h"
                                   """,
        touch_vertex_first_time_dependency_policy="TouchAtLeastOnce_AllPreviousStepsUpdateAtLeastOnce_MayOverwritePreviousCellUpdatesFromSameSweep",
        cell_kernel_dependency_policy="TouchAtLeastOnce_AllPreviousStepsUpdateAtLeastOnce_MayOverwritePreviousCellUpdatesFromSameSweep",
    )
    algorithm_steps_dict["PredictHydro"] = predictHydro

    densityWithConstantSearchRadius = AlgorithmStep(
        name="DensityLoopWithConstantSearchRadius",
        dependencies=AlgorithmStep.Dependencies.NEIGHBOURS,
        effect=AlgorithmStep.Effect.ALTER_LOCAL_STATE_AND_MIGHT_RERUN,
        prepare_traversal_kernel="""
                                   globaldata::{PARTICLE}::getSpecies().clearRerunPreviousGridSweepFlag();
                                   """.replace(
            "{PARTICLE}", NAME
        ),
        touch_vertex_first_time_kernel=v_iter.insert_function(
            "::swift2::kernels::legacy::prepareDensity"
        ),
        cell_kernel=c_iter.insert_function( "::swift2::kernels::legacy::densityKernel"),
        touch_vertex_last_time_kernel=(
            v_iter.insert_function( "::swift2::kernels::legacy::endDensityCalculation")
            + v_iter_with_global_reduction.insert_function( "::swift2::kernels::legacy::updateSmoothingLengthAndRerunIfRequired")
        ),
        includes="""
            #include "swift2/kernels/legacy/SmoothingLength.h"
            #include "swift2/kernels/legacy/Density.h"
                 """,
    )
    algorithm_steps_dict["Density"] = densityWithConstantSearchRadius

    force = AlgorithmStep(
        name="ForceLoop",
        dependencies=AlgorithmStep.Dependencies.NEIGHBOURS,
        effect=AlgorithmStep.Effect.ALTER_LOCAL_STATE,
        touch_vertex_first_time_kernel=(
            v_iter.insert_function(
                "::swift2::kernels::legacy::resetSmoothingLengthIterationCounter"
            )
            + v_iter.insert_function(
                "::swift2::kernels::legacy::prepareHydroForce"
            )
            + v_iter.insert_function(
                "::swift2::kernels::legacy::resetAcceleration"
            )
        ),
        cell_kernel=c_iter.insert_function(
            "::swift2::kernels::legacy::forceKernel"
        ),
        touch_vertex_last_time_kernel=v_iter.insert_function(
            "::swift2::kernels::legacy::endHydroForceCalculation"
        ),
        includes="""
          #include "swift2/kernels/legacy/Swift.h"
          #include "swift2/kernels/legacy/HydroForce.h"
        """,
    )
    algorithm_steps_dict["Force"] = force

    kick2 = AlgorithmStep(
        name="Kick2",
        dependencies=AlgorithmStep.Dependencies.SELF,
        effect=AlgorithmStep.Effect.ALTER_LOCAL_STATE,
        touch_vertex_last_time_kernel=(
            v_iter.insert_function(
                "::swift2::kernels::legacy::leapfrogKickWithGlobalTimeStepSize"
            )
            + v_iter.insert_function(
                "::swift2::kernels::legacy::leapfrogKickExtraWithGlobalTimeStepSize"
            )
            + v_iter.insert_function(
                "::swift2::kernels::legacy::resetPredictedValues"
            )
        ),
        prepare_traversal_kernel="""::swift2::timestepping::computeCFLTimeStepSizeSPH<globaldata::{}>();""".format(
            NAME,
        ),  # TODO: temporary solution for initialization step
        includes="""
            #include "Constants.h"
            #include "swift2/kernels/legacy/Leapfrog.h"
            #include "swift2/timestepping/GlobalTimeStepping.h"
            #include "swift2/timestepping/TimeStepping.h"
                                   """,
    )
    algorithm_steps_dict["Kick2"] = kick2

    reduceGlobals = AlgorithmStep(
        name="ReduceGlobalQuantities",
        dependencies=AlgorithmStep.Dependencies.SELF,
        effect=AlgorithmStep.Effect.ALTER_GLOBAL_STATE,
        touch_vertex_last_time_kernel=v_iter_with_global_reduction.insert_function(
            "::swift2::statistics::reduceVelocityAndSearchRadius"
        ),
        prepare_traversal_kernel="""
                                   globaldata::{PARTICLE}::getSpecies().clearSearchRadius();
                                   globaldata::{PARTICLE}::getSpecies().clearVelocity();
                                   """.replace(
            "{PARTICLE}", NAME
        ),
        unprepare_traversal_kernel="""
                                   globaldata::{PARTICLE}::getSpecies().setTimeStamp( globaldata::{PARTICLE}::getSpecies().getMinTimeStamp() + globaldata::{PARTICLE}::getSpecies().getMinTimeStepSize(), false );
                                   ::swift2::statistics::reportSearchRadiusVTDt<globaldata::{PARTICLE}>( "{PARTICLE}" );
                                   """.replace(
            "{PARTICLE}", NAME
        ),
        includes="""
                                   #include "Constants.h"
                                   #include "swift2/statistics/Reports.h"
                                   #include "swift2/statistics/Statistics.h"
                                   """,
    )
    algorithm_steps_dict["ReduceGlobalQuantities"] = reduceGlobals

    return algorithm_steps_dict


