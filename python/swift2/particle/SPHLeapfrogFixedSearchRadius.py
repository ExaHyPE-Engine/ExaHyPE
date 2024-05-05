# This file is part of the SWIFT2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
from swift2.particle.SPHParticle import SPHParticle
from swift2.particle.AlgorithmStep import AlgorithmStep
from swift2.particle.AlgorithmStepLibrary import get_algorithm_step_dict

import peano4
import dastgen2
import peano4.dastgen2

from abc import ABC
import copy


from enum import Enum


class SPHLeapfrogFixedSearchRadius(SPHParticle):
    """!

        SPH particle with fixed search radius

        Same as LeapfrogFixedSearchRadius but augmented for full SPH.
        Implements the Minimal SPH scheme.

        Simple particle with a fixed search radius h which moves according
        to leapfrog KDK scheme.
        By default, it uses global time stepping,
        i.e. the combination of maximum velocity and minimal mesh size determines
        the time step size of the subsequent time step. Besides the default
        variables x and h, the particle has the following properties:

        - a vector a which is the acceleration;
        - a vector v which is the velocity.

        You can add further properties via

             myparticle.data.add_attribute( peano4.dastgen2.Peano4DoubleArray("myFancyArray","Dimensions") )

        in your code. Or you can create a subclass which adds additional fields
        after it has called the baseline constructor.

        You will need to add further properties for any SPH project.

        ## Force calculation

        particle_particle_interaction_over_particle_sets_kernel is a C++ string which defines a force between two
        particles. It has access to three important objects:

        - localParticles is a container (list) over pointers to local particles
        - activeParticles is a container (list) over pointers to active particles
        - marker is a cell marker which identifies the current cell.

        Please consult the guidebook for a definition of local and active particles
        but take into account that the local particles always are a subset of the
        active particles.

        Besides the actual particle-to-particle calculation, i.e. a force
        calculation, users can also provide kernels that kick in when you touch
        particles for the first time before you actually compute any
        particle-particle interaction, and there is a plug-in point what you do
        just once the particle-particle interaction has terminated. The latter
        point is reached before we do the actual time stepping. In both plug-in
        points, you have a vertex marker which gives you the position of the
        vertex to which a particular is associated, and you have the localParticles.
        This is a vector of pointers in this particular case.

        The force calculation does exist in two variants: There's the default one
        and then we have one that is aggressively optimised. The latter one requires
        us to have properly sorted particle-mesh associations.

        ## Precision

        By default, all particle attributes are modelled as doubles or arrays of
        doubles. This might be overly precise. You can alter the precision through
        our LLVM compiler modifications if you invoke the following commands on a
        particle species:

        ~~~~~~~~~~~~~~~~~~~~~~~~~~
        particle.mass.valid_mantissa_bits         = args.mantissa_size
        particle.velocity.valid_mantissa_bits     = args.mantissa_size
        particle.acceleration.valid_mantissa_bits = args.mantissa_size
        particle.density.valid_mantissa_bits      = args.mantissa_size
        particle.pressure.valid_mantissa_bits     = args.mantissa_size
        particle.smoothL.valid_mantissa_bits      = args.mantissa_size
        particle.u.valid_mantissa_bits            = args.mantissa_size
        particle.uDot.valid_mantissa_bits         = args.mantissa_size
        particle.f.valid_mantissa_bits            = args.mantissa_size
        particle.wcount_dh.valid_mantissa_bits    = args.mantissa_size
        particle.rho_dh.valid_mantissa_bits       = args.mantissa_size
        particle.wcount.valid_mantissa_bits       = args.mantissa_size
        particle.hDot.valid_mantissa_bits         = args.mantissa_size
        particle.balsara.valid_mantissa_bits      = args.mantissa_size
        particle.rot_v.valid_mantissa_bits        = args.mantissa_size
        particle.div_v.valid_mantissa_bits        = args.mantissa_size
        particle.v_sig_AV.valid_mantissa_bits     = args.mantissa_size
        particle.soundSpeed.valid_mantissa_bits   = args.mantissa_size
        particle.v_full.valid_mantissa_bits       = args.mantissa_size
        particle.u_full.valid_mantissa_bits       = args.mantissa_size
        ~~~~~~~~~~~~~~~~~~~~~~~~~~

        This is an example. You can also add ony a few of these fields, and picking
        a value for args.mantissa_size is obviously up to you. We refrain from
        illustrating that you can also alter the precision of the position and
        maximum search radius of a particle this way. This is on purpose: We have
        made bad experience with such modifications.

        ## Boundary conditions

        Boundary conditions for Swift are @ref swift_boundary_conditions "discussed on a separate page".
        If you decide that you want to plug into the drift mechanism of
        this SPH flavour, you typically add something similar to the
        code snippet below to your code:

        ~~~~~~~~~~~~~~~~~~~~~~
    particle.algorithm_steps_dict["Drift"].touch_vertex_last_time_kernel += " " "
    if (::swift2::boundaryconditions::isVertexOnGlobalBoundary(marker,DomainOffset,DomainSize)) {
      ::swift2::kernels::forAllParticles(
        marker,
        assignedParticles,
        [&](const peano4::datamanagement::VertexMarker& marker, globaldata::" " " + name + " " "&  assignedParticle)->void {
          ::swift2::boundaryconditions::applyFixedBoundaryCondition(
            assignedParticle,
            marker,
            DomainOffset,
            DomainSize,
            0.1,
            _spacetreeId
          );
        }
      );
    }
    " " "
    particle.algorithm_steps_dict["Drift"].includes += " " "
    #include "swift2/boundaryconditions/FixedBoundary.h"
    #include "swift2/boundaryconditions/Utils.h"
    " " "
    particle._setup_algorithm_steps()
    particle._setup_initialisation_steps()
        ~~~~~~~~~~~~~~~~~~~~~~

        The spaced-out syntax is just there to ensure that the Python documentation
        syntax is not broken.


        ## Attributes

        name: String
        To be in line with other conventions, I recommend you start with an
        uppercase letter. This has to be a valid C++ identifier, i.e. don't
        use any special symbols besides an underscore.

        ## Vectorisation

        This type can be used with aggressively optimised kernels if you follow
        @ref page_swift_performance_optimisation Performance optimisation and use
        the map_particle_steps_onto_separate_mesh_traversals_insert_dummy_sweeps_for_coalesced_memory_access
        graph compiler from the Sequential suite; or literally any other
        flavour which yields coalesced memory.

        @param particle_interaction_kernel_realisation: ParticleKernelRealisation
          Switch through various interaction variants.


        @see peano4::datamanagement::CellMarker
        @see peano4::datamanagement::VertexMarker

    """

    def __init__(
        self,
        name,
        dimensions_hydro=2,
        cfl_factor=0.1,
        initial_time_step_size=1e-4,
        constant_time_step_size=True,
        swift_project_namespace="SPH",
        particles_per_cell=0,  # makes no sense, and should likely be forbidden
        min_h=0.3,
        max_h=0.3,
        particle_interaction_kernel_realisation: SPHParticle.ParticleKernelRealisation = SPHParticle.ParticleKernelRealisation.USE_OUTER_GUARDS,
    ):
        super(SPHLeapfrogFixedSearchRadius, self).__init__(
            name=name,
            dimensions_hydro=dimensions_hydro,
            cfl_factor=cfl_factor,
            initial_time_step_size=initial_time_step_size,
            constant_time_step_size=constant_time_step_size,
            swift_project_namespace=swift_project_namespace,
            particles_per_cell=particles_per_cell,
            min_h=min_h,
            max_h=max_h,
        )

        # Initialize default parameters.
        # ---------------------------------------

        # This is an SPH scheme.
        # Hardcoded this for now until we can switch flavours.
        self._sph_flavour = "Minimal"

        # Minimal SPH parameters
        self._alpha_av = 0.8
        self._beta_av = 3.0

        # now set these parameters as dastgen attributes.
        # This needs to be done in a careful way so that
        # changes to parameters outside of the initializer
        # propagate into the generated c++ files.
        self.set_parameters()

        # Construct particle properties
        # --------------------------------

        # Artificial viscosity scheme --------------------------------------------------
        self.balsara = dastgen2.attributes.Double("balsara")
        if self._hydro_dimensions == 3:
            self.rot_v = peano4.dastgen2.Peano4DoubleArray("rot_v", "Dimensions")
        else:  # Curl is a scalar in lower dimensions
            self.rot_v = dastgen2.attributes.Double(
                "rot_v"
            )  # In 2D sims it only has 1 component (z)
        self.div_v = dastgen2.attributes.Double("div_v")
        self.v_sig_AV = dastgen2.attributes.Double("v_sig_AV")
        self.soundSpeed = dastgen2.attributes.Double("soundSpeed")
        self.data.add_attribute(self.balsara)
        self.data.add_attribute(self.rot_v)
        self.data.add_attribute(self.div_v)
        self.data.add_attribute(self.v_sig_AV)
        self.data.add_attribute(self.soundSpeed)

        # Algorithm Steps ---------------------------------------------------------------
        # actual list containing the algorithm steps
        self._algorithm_steps = []
        # actual list containing the algorithm steps for initialization
        self._initialisation_steps = []
        # the dict containing all possible algorithm steps
        self.algorithm_steps_dict = get_algorithm_step_dict(self)

        # Now store an independent instance of the algorithm steps in the object
        # We need an independent instance for situations where we modify the
        # algorithm steps later. For example when we are adding debugging or
        # dependence checks for each algorithm step.
        self._setup_algorithm_steps()
        # same for initialization steps
        self._setup_initialisation_steps()

        self._add_dependency_checks()

        return

    def set_parameters(self):
        """
        This function translates "global" particle parameters which are
        constant throughout the simulation (like CFL factor, minimal time step
        size, viscosity parameters...) into dastgen attributes of the C++
        particle class.
        If you modify any of the attributes manually outside of the particle
        initialisation, e.g. by invoking

        ```
        particle = SPHLeapfrogFixedSearchRadius(initial_time_step_size=ABC, ...)
        particle.initial_time_step_size = XYZ
        ```

        you need to call this function manually so your changes propagate
        into the generated C++ files.
        """

        super(SPHLeapfrogFixedSearchRadius, self).set_parameters()

        const_static = dastgen2.attributes.Attribute.Qualifier.CONST_STATIC

        alpha_av_attr = dastgen2.attributes.Double(
            "alphaAV", qualifier=const_static, initval=self._alpha_av
        )
        self.data.add_or_replace_attribute(alpha_av_attr)

        beta_av_attr = dastgen2.attributes.Double(
            "betaAV", qualifier=const_static, initval=self._beta_av
        )
        self.data.add_or_replace_attribute(beta_av_attr)

        return

    def _setup_algorithm_steps(self):
        """!
        Set up the internal list of algorithm steps for this particle.
        We need to maintain an individual instance of this list for cases
        where we modify the algorithm steps later on. This can happen for
        example when adding debugging/dependency checks later on.


        The algorithm steps shall be a list of AlgorithmStep objects to be
        executed in that order.

        Definitions of the algorithm steps stored in the algorithm_steps_dict are
        placed in get_algorithm_steps_dict(self). The dict is set up during
        instantiation of this class, and reads in all available algorithm steps
        defined in AlgorithmStepLibrary.py.

        Leapfrog consists basically of four steps per particle. We first
        determine the force. Then we update the velocity by half a timestep and move
        the particle by a full timestep. Then the force is re-computed and the
        second half of the velocity update is done.
        Some variations of this KDK form re-arrange the steps executed per timestep
        to avoid a second force loop. We can't do this here because we're not planning
        on running with fixed time step sizes throughout the run.
        """

        steps = [
            self.algorithm_steps_dict["SPH_Drift"],
            self.algorithm_steps_dict["PredictHydro"],
            self.algorithm_steps_dict["SPH_Density"],
            self.algorithm_steps_dict["SPH_Force"],
            self.algorithm_steps_dict["SPH_Kick2"],
            self.algorithm_steps_dict["SPH_ReduceGlobalQuantities"],
            self.algorithm_steps_dict["SPH_Kick1"],
        ]

        # make a copy of the steps, so if you modify them later (e.g. by
        # adding dependency checks), it won't modify the steps in the dict.
        self._algorithm_steps = [copy.deepcopy(i) for i in steps]
        return

    def _setup_initialisation_steps(self):
        """!
        Define the algorithm steps to be taken during initialization here.
        This follows the same logic as _setup_algorithm_steps. See documentation
        there for more info.

        Make sure get_algorithm_steps_dict(self) has been called before.
        """
        initsteps = [
            self.algorithm_steps_dict["SPH_Density"],
            self.algorithm_steps_dict["SPH_Force"],
            # for the first kick in the simulation, we need the reset that
            # is done in the kick2 algorithm step, but not in the kick1.
            self.algorithm_steps_dict["SPH_Kick2"],
        ]

        # return deep copies of algorithms steps. Otherwise,
        # modifications down the line (like the dependency checks)
        # will cause trouble.
        self._initialisation_steps = [copy.deepcopy(i) for i in initsteps]

        return

    @property
    def alpha_av(self):
        """

        Viscosity parameters of the Minimal SPH model. Default SWIFT values are
        alpha_av=0.8 and beta_av=3.

        """
        return self._alpha_av

    @alpha_av.setter
    def alpha_av(self, alpha_av):
        if alpha_av <= 0:
            raise ValueError("Value for the alpha_av viscosiy parameter not valid.")
        self._alpha_av = alpha_av

    @property
    def beta_av(self):
        return self._alpha_av

    @beta_av.setter
    def beta_av(self, beta_av):
        if beta_av <= 0:
            raise ValueError("Value for the beta_av viscosiy parameter not valid.")
        self._beta_av = beta_av

    def get_cpp_namespace_from_project_namespace(self):
        """

        Transform namespace into cpp format. Could be used to append namespace to
        constants in kernels (not used currently).

        """
        namespace = "::".join(self._swift_project_namespace) + "::"

        print(namespace)

    @property
    def readme_descriptor(self):
        return (
            super(SPHLeapfrogFixedSearchRadius, self).readme_descriptor
            + """

  Time integrator: SPH Leapfrog

  - search radius:                  fixed
  - CFL factor:                     """
            + str(self._cfl_factor)
            + """
  - dt_initial:                     """
            + str(self._initial_time_step_size)
            + """

    """
        )
