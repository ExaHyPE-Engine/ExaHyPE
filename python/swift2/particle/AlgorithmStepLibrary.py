from swift2.particle.AlgorithmStep import AlgorithmStep
from swift2.particle.SPHParticle import SPHParticle


class ParticleIteratorTemplate(str):
    """!
    A mini-wrapper around a string for particle iterator.
    Exists for convenience: adds self.insert() method,
    which inserts the function which the iterators are supposed to call
    on a particle or particle pair.

    The function insertion works via string replacement. It will
    look for the string '*FUNCTION*' to be replaced.
    """

    def __init__(self, template):
        """!
        template: str
            the string containing the particle interacion iterator template
        """
        self.template = template
        return

    def insert(self, function):
        """!
        insert a function into the template string.

        Returns the particle iterator template with the function inserted.
        """

        return self.template.replace("*FUNCTION*", function)


def _get_particle_iterator_templates(particle_name, kernel_realisation):
    """!
    Construct particle interaction iterator call templates given the
    desired particle interaction kernel realisation. These signature
    templates are strings containing c++ code snippets of the correct
    iterator call signature.

    In total, there are four types of iterators. The essential two are:

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

    The iterators require a function to be passed as an argument to
    act on the particles. In the template snippets being generated here,
    the functions shall be replaced by a placeholder string '*FUNCTION*'.

    Finally, the code snippets being returned are stored as a
    ParticleIteratorTemplate object.

    @param particle_name: str
        the particle class name.
    @param particle_interaction_kernel_realisation: SPHParticle.ParticleKernelRealisation
        enum for kernel realisation.


    4 ParticleIteratorTemplate objects containing particle iterator code snippets.

    @return vertex_for_move: snippet (in a ParticleIteratorTemplate object) to
        use for particle interactons which will move assigned to a vertex
    @return cell: snippet (in a ParticleIteratorTemplate object) to use for
        particle-particle interactions assigned to a cell
    @return vertex: snippet (in a ParticleIteratorTemplate object) to use for
        particle interactions assigned to a vertex
    @return vertex_with_global_reduction: snippet (in a ParticleIteratorTemplate
        object) to use for particle interactions assigned to a vertex which also
        contain a global reduction
    """

    PARTICLE = particle_name

    iterate_over_particles_assigned_to_vertex_for_move = None
    iterate_over_particles_assigned_to_vertex = None
    iterate_over_particles_assigned_to_cell = None
    iterate_over_particles_assigned_to_vertex_with_global_reduction = None

    if kernel_realisation == SPHParticle.ParticleKernelRealisation.NO_OPTIMISATION:
        iterate_over_particles_assigned_to_vertex_for_move = None
        iterate_over_particles_assigned_to_vertex = f"""::swift2::kernels::forAllParticles(marker, assignedParticles, *FUNCTION*<globaldata::{PARTICLE}>, ::swift2::kernels::alwaysUpdateInVertexKernel<globaldata::{PARTICLE}>);"""
        iterate_over_particles_assigned_to_cell = f"::swift2::kernels::forAllParticlePairs( marker, localParticles, activeParticles, *FUNCTION*<globaldata::{PARTICLE}>, ::swift2::kernels::alwaysUpdateInCellKernel<globaldata::{PARTICLE}>, ::swift2::kernels::alwaysUpdateParticlePairs<globaldata::{PARTICLE}>);"
        iterate_over_particles_assigned_to_vertex_with_global_reduction = f"::swift2::kernels::forAllParticles( marker, assignedParticles, *FUNCTION*<globaldata::{PARTICLE}>, ::swift2::kernels::alwaysUpdateInVertexKernel<globaldata::{PARTICLE}>);"
    elif kernel_realisation == SPHParticle.ParticleKernelRealisation.USE_OUTER_GUARDS:
        iterate_over_particles_assigned_to_vertex_for_move = None
        iterate_over_particles_assigned_to_vertex = f"::swift2::kernels::forAllParticles( marker, assignedParticles, *FUNCTION*<globaldata::{PARTICLE}>);"
        iterate_over_particles_assigned_to_cell = f"::swift2::kernels::forAllParticlePairs( marker, localParticles, activeParticles, *FUNCTION*<globaldata::{PARTICLE}>);"
        iterate_over_particles_assigned_to_vertex_with_global_reduction = None
    elif kernel_realisation == SPHParticle.ParticleKernelRealisation.VECTORISE_ALL:
        iterate_over_particles_assigned_to_vertex_for_move = None
        iterate_over_particles_assigned_to_vertex = f"::swift2::kernels::forAllParticlesVectorised( marker, assignedParticles, numberOfAssignedParticles, *FUNCTION*WithMasking<globaldata::{PARTICLE}>);"
        iterate_over_particles_assigned_to_cell = f"::swift2::kernels::forAllParticlePairsVectorised( marker, localParticles, activeParticles, numberOfParticlesPerLocalVertex, _numberOfActiveParticlesPerVertex, *FUNCTION*WithMasking<globaldata::{PARTICLE}>);"
        iterate_over_particles_assigned_to_vertex_with_global_reduction = None
    elif (
        kernel_realisation
        == SPHParticle.ParticleKernelRealisation.VECTORISE_WITH_SEPARATE_DISTANCE_CHECKS
    ):
        iterate_over_particles_assigned_to_vertex_for_move = None
        iterate_over_particles_assigned_to_vertex = f"::swift2::kernels::forAllParticlesVectoriseWithCheckPreamble( marker, assignedParticles, numberOfAssignedParticles, *FUNCTION*<globaldata::{PARTICLE}>);"
        iterate_over_particles_assigned_to_cell = f"::swift2::kernels::forAllParticlePairsVectoriseWithCheckPreamble( marker, localParticles, activeParticles, numberOfParticlesPerLocalVertex, _numberOfActiveParticlesPerVertex, *FUNCTION*<globaldata::{PARTICLE}>);"
        iterate_over_particles_assigned_to_vertex_with_global_reduction = None
    else:
        raise NotImplementedError(
            f"Requested kernel realisation {particle_interaction_kernel_realisatio} not implemented"
        )

    #  Set defaults or raise error if there shouldn't be defaults and we missed something.

    if iterate_over_particles_assigned_to_vertex_for_move is None:
        iterate_over_particles_assigned_to_vertex_for_move = f"""
            auto oldParticlePositions = ::toolbox::particles::assignmentchecks::recordParticlePositions(assignedParticles);
            ::swift2::kernels::forAllParticles( marker, assignedParticles, *FUNCTION*<globaldata::{PARTICLE}>);
            ::toolbox::particles::assignmentchecks::traceParticleMovements(assignedParticles, oldParticlePositions, _spacetreeId);
            """
        ## @todo This is not nice. We should still use the vectorised version if the vertex is gathered.

    if iterate_over_particles_assigned_to_cell is None:
        raise ValueError("We shouldn't be here.")

    if iterate_over_particles_assigned_to_vertex is None:
        raise ValueError("We shouldn't be here.")

    if iterate_over_particles_assigned_to_vertex_with_global_reduction is None:
        iterate_over_particles_assigned_to_vertex_with_global_reduction = f"::swift2::kernels::forAllParticles( marker, assignedParticles, *FUNCTION*<globaldata::{PARTICLE}>);"

    #  finally, wrap the templates into ParticleIteratorTemplate objects
    vertex_for_move = ParticleIteratorTemplate(
        iterate_over_particles_assigned_to_vertex_for_move
    )
    cell = ParticleIteratorTemplate(iterate_over_particles_assigned_to_cell)
    vertex = ParticleIteratorTemplate(
        iterate_over_particles_assigned_to_vertex_with_global_reduction
    )
    vertex_with_global_reduction = ParticleIteratorTemplate(
        iterate_over_particles_assigned_to_vertex_with_global_reduction
    )

    return vertex_for_move, cell, vertex, vertex_with_global_reduction


def get_algorithm_step_dict(particle):
    """!

    Sets up a dict of the algorithm steps for this scheme so they can be accessed
    at various points and by various models.

    """

    algorithm_steps_dict = {}

    realisation = particle._particle_interaction_kernel_realisation
    project_namespace = particle._swift_project_namespace

    PARTICLE = particle.name
    NAMESPACE = "::".join(project_namespace)

    (
        v_iter_for_move,
        c_iter,
        v_iter,
        v_iter_with_global_reduction,
    ) = _get_particle_iterator_templates(PARTICLE, realisation)

    # --------------------------------------------------------------------------
    # --------------------------------------------------------------------------
    sph_kick1 = AlgorithmStep(
        name="SPH_Kick1",
        dependencies=AlgorithmStep.Dependencies.SELF,
        effect=AlgorithmStep.Effect.ALTER_LOCAL_STATE,
        touch_vertex_first_time_kernel=v_iter.insert(
            "::swift2::timestepping::resetMovedParticleMarker"
        ),
        touch_vertex_last_time_kernel=v_iter.insert(
            "::swift2::kernels::legacy::leapfrogKickWithGlobalTimeStepSize"
        ),
        prepare_traversal_kernel=f"::swift2::timestepping::computeCFLTimeStepSizeSPH<globaldata::{PARTICLE}>();",
        includes="""
            #include "swift2/kernels/legacy/Leapfrog.h"
            #include "swift2/timestepping/GlobalTimeStepping.h"
            #include "swift2/timestepping/TimeStepping.h"
        """,
    )
    algorithm_steps_dict["SPH_Kick1"] = sph_kick1

    # --------------------------------------------------------------------------
    #
    #
    # Correctness: Whenever we drift a particle, we might subsequently have to
    # resort it. That is, we assign it to another vertex. Therefore, it can
    # happen that another cell checks a particle again and that we notably call
    # touchVertexLastTime() with one particle twice. Therefore, we only know
    # that we touch a particle at least once in touchVertexLastTime, that cell
    # updates can be done multiple times (subsequent cell and vertex updates
    # will mask out any changes to the particle as the moved flag is set, but
    # they check), and that a cell update can follow a touchVertexLastTime()
    # (on a finer level if we move particles from fine to coarse).
    # --------------------------------------------------------------------------
    sph_drift = AlgorithmStep(
        name="SPH_Drift",
        dependencies=AlgorithmStep.Dependencies.SELF,
        effect=AlgorithmStep.Effect.CHANGE_POSITION_OR_INTERACTION_RADIUS,
        touch_vertex_first_time_kernel=v_iter.insert(
            "::swift2::timestepping::resetMovedParticleMarker"
        ),
        touch_vertex_last_time_kernel=v_iter_for_move.insert(
            "::swift2::kernels::legacy::leapfrogDriftWithGlobalTimeStepSize"
        ),
        includes="""
            #include "Constants.h"
            #include "swift2/kernels/legacy/Leapfrog.h"
            #include "swift2/timestepping/GlobalTimeStepping.h"
            #include "swift2/timestepping/TimeStepping.h"
        """,
        cell_kernel_dependency_policy="TouchAtLeastOnce_AllPreviousStepsUpdateAtLeastOnce_MayOverwritePreviousCellUpdatesFromSameSweep",
        touch_vertex_first_time_dependency_policy="TouchAtLeastOnce_AllPreviousStepsUpdateAtLeastOnce",
        touch_vertex_last_time_dependency_policy="TouchAtLeastOnce_AllPreviousStepsUpdateAtLeastOnce",
    )
    algorithm_steps_dict["SPH_Drift"] = sph_drift

    # --------------------------------------------------------------------------
    # --------------------------------------------------------------------------
    # From the SPH side of things, this doesn't need to be an AlgorithmStep
    # on its own. We could merge it together with the drifts. However, in order
    # to ensure that the particles which have just been moved in the preceeding
    # drift sweep are sorted correctly into the cells and vertices, we need an
    # extra sweep.
    predictHydro = AlgorithmStep(
        name="PredictHydro",
        dependencies=AlgorithmStep.Dependencies.SELF,
        effect=AlgorithmStep.Effect.ALTER_LOCAL_STATE,
        # touch_vertex_first_time_kernel = iterate_over_particles_assigned_to_vertex.format("""::swift2::timestepping::resetMovedParticleMarker(assignedParticles);""",
        touch_vertex_first_time_kernel=v_iter.insert(
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

    # --------------------------------------------------------------------------
    # --------------------------------------------------------------------------
    sph_densityWithConstantSearchRadius = AlgorithmStep(
        name="SPH_DensityLoopWithConstantSearchRadius",
        dependencies=AlgorithmStep.Dependencies.NEIGHBOURS,
        effect=AlgorithmStep.Effect.ALTER_LOCAL_STATE_AND_MIGHT_RERUN,
        prepare_traversal_kernel=f"""globaldata::{PARTICLE}::getSpecies().clearRerunPreviousGridSweepFlag();""",
        touch_vertex_first_time_kernel=v_iter.insert(
            "::swift2::kernels::legacy::prepareDensity"
        ),
        cell_kernel=c_iter.insert("::swift2::kernels::legacy::densityKernel"),
        touch_vertex_last_time_kernel=(
            v_iter.insert("::swift2::kernels::legacy::endDensityCalculation")
            + v_iter_with_global_reduction.insert(
                "::swift2::kernels::legacy::updateSmoothingLengthAndRerunIfRequired"
            )
        ),
        includes="""
            #include "swift2/kernels/legacy/SmoothingLength.h"
            #include "swift2/kernels/legacy/Density.h"
        """,
    )
    algorithm_steps_dict["SPH_Density"] = sph_densityWithConstantSearchRadius

    # --------------------------------------------------------------------------
    # --------------------------------------------------------------------------
    sph_force = AlgorithmStep(
        name="SPH_ForceLoop",
        dependencies=AlgorithmStep.Dependencies.NEIGHBOURS,
        effect=AlgorithmStep.Effect.ALTER_LOCAL_STATE,
        touch_vertex_first_time_kernel=(
            v_iter.insert(
                "::swift2::kernels::legacy::resetSmoothingLengthIterationCounter"
            )
            + v_iter.insert("::swift2::kernels::legacy::prepareHydroForce")
            + v_iter.insert("::swift2::kernels::legacy::resetAcceleration")
        ),
        cell_kernel=c_iter.insert("::swift2::kernels::legacy::forceKernel"),
        touch_vertex_last_time_kernel=v_iter.insert(
            "::swift2::kernels::legacy::endHydroForceCalculation"
        ),
        includes="""
          #include "swift2/kernels/legacy/SmoothingLength.h"
          #include "swift2/kernels/legacy/HydroForce.h"
        """,
    )
    algorithm_steps_dict["SPH_Force"] = sph_force

    # --------------------------------------------------------------------------
    # --------------------------------------------------------------------------
    sph_kick2 = AlgorithmStep(
        name="SPH_Kick2",
        dependencies=AlgorithmStep.Dependencies.SELF,
        effect=AlgorithmStep.Effect.ALTER_LOCAL_STATE,
        touch_vertex_last_time_kernel=(
            v_iter.insert(
                "::swift2::kernels::legacy::leapfrogKickWithGlobalTimeStepSize"
            )
            + v_iter.insert("::swift2::kernels::legacy::resetPredictedValues")
        ),
        prepare_traversal_kernel=f"::swift2::timestepping::computeCFLTimeStepSizeSPH<globaldata::{PARTICLE}>();",
        # TODO: temporary solution for initialization step
        includes="""
            #include "Constants.h"
            #include "swift2/kernels/legacy/Leapfrog.h"
            #include "swift2/timestepping/GlobalTimeStepping.h"
            #include "swift2/timestepping/TimeStepping.h"
        """,
    )
    algorithm_steps_dict["SPH_Kick2"] = sph_kick2

    # --------------------------------------------------------------------------
    # --------------------------------------------------------------------------
    sph_reduceGlobals = AlgorithmStep(
        name="SPH_ReduceGlobalQuantities",
        dependencies=AlgorithmStep.Dependencies.SELF,
        effect=AlgorithmStep.Effect.ALTER_GLOBAL_STATE,
        touch_vertex_last_time_kernel=v_iter_with_global_reduction.insert(
            "::swift2::statistics::reduceVelocityAndSearchRadius"
        ),
        prepare_traversal_kernel=f"""
                                   globaldata::{PARTICLE}::getSpecies().clearSearchRadius();
                                   globaldata::{PARTICLE}::getSpecies().clearVelocity();
        """,
        unprepare_traversal_kernel=f"""
                                   globaldata::{PARTICLE}::getSpecies().setTimeStamp( globaldata::{PARTICLE}::getSpecies().getMinTimeStamp() + globaldata::{PARTICLE}::getSpecies().getMinTimeStepSize(), false );
                                   ::swift2::statistics::reportSearchRadiusVTDt<globaldata::{PARTICLE}>( "{PARTICLE}" );
        """,
        includes="""
                                   #include "Constants.h"
                                   #include "swift2/statistics/Reports.h"
                                   #include "swift2/statistics/Statistics.h"
        """,
    )
    algorithm_steps_dict["SPH_ReduceGlobalQuantities"] = sph_reduceGlobals

    return algorithm_steps_dict
