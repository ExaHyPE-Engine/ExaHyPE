# This file is part of the SWIFT2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
from swift2.particle.Particle import Particle
from swift2.particle.AlgorithmStep import AlgorithmStep

import peano4
import dastgen2
import peano4.dastgen2

import abc
import copy


from enum import Enum


class SPHParticle(Particle):
    class ParticleKernelRealisation(Enum):
        """!

        Enum to toggle between the various vectorised/not vectorised
        kernels.

        """

        NO_OPTIMISATION = 0
        USE_OUTER_GUARDS = 1
        VECTORISE_ALL = 2
        VECTORISE_WITH_SEPARATE_DISTANCE_CHECKS = 3

    """!

    The SPH particle base class. Contains the bare minimum of data
    required for the base algorithm, in particular the neighboursearch/
    smoothing length computation, to work. Actual SPH implementations
    should inherit these properties from this class.
    Note that this base class contains no algorithm steps. The derived
    classes will have to implement that individually.

    The data contained in this super class are:

    General simulation data:
        - CFL factor
        - initial time step size
        - hydro dimension

    Smoothing length related global simulation parameters:
        - eta factor (resolution eta, which determines number of neighbours)
        - max h iterations
        - h_min
        - h_max
        - h_tolerance: Convergence criterion for smoothing length iteration

    Implementation specifics:
        - particle interaction kernel realisation
        - swift project namespace

    Particle Data:
        - mass
        - velocity
        - acceleration
        - density
        - pressure
        - smoothing length
        - specific internal energy u
        - time derivative of specific internal energy uDot

    Smoothing lenght related particle fields:
        - wcount : Neighbour weight number
        - wcount_dh : derivative of eighbour weight number w.r.t smoothing length
        - hDot: derivative of smoothing length w.r.t. time
        - f: a collected factor required for sml computation
        - sml_iteration_count

    Additional Particle fields
        - hasNoNeighbours: Flag whether particle has no neighbours
        - partID: unique particle ID


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
        particle_interaction_kernel_realisation: ParticleKernelRealisation = ParticleKernelRealisation.USE_OUTER_GUARDS,
    ):
        super(SPHParticle, self).__init__(
            name,
            particles_per_cell,
            min_h,
            max_h,
        )

        # Initialize default parameters.
        # ---------------------------------------

        self._sph_flavour = "none"

        # Simulation properties
        self._cfl_factor = cfl_factor
        self._initial_time_step_size = initial_time_step_size
        self._constant_time_step_size = constant_time_step_size
        self._particle_interaction_kernel_realisation = (
            particle_interaction_kernel_realisation
        )

        # SPH parameters (default values)
        self._hydro_dimensions = dimensions_hydro

        # resolution eta. Determines the number of neighbours considered
        self._eta_factor = 1.2348

        # Maximal number of smoothing length iterations
        self._h_max_iterations = 30

        # TODO: these values should probably not be hardcoded like this.
        # But it's good enough for the elementary tests and benchmarks
        # we have for now.
        self._h_hydro_min = 1e-6
        self._h_hydro_max = (
            1.0 / 3.0 / 2.0
        )  # Approximated value for coarsest grid level
        self._h_tolerance = 1e-4

        # Project namespace to have consistent use of exported Constants in kernels
        # Not used right now.
        self._swift_project_namespace = swift_project_namespace

        # now set these parameters as dastgen attributes
        #  self.set_parameters()
        # NOTE: derived classes should do this at this point. But not this
        # super class - the reason being that when instantiated as an object
        # of the subclass type, this line will call the subclass' set_parameters()
        # method, which will try to set parameters that aren't set yet.
        # So don't call this here, but make sure to include a super class' invocation
        # of set_parameters() in your subclass' set_parameters() definition.

        # Construct particle properties
        # --------------------------------

        # Baseline particle properties -------------------------------------------------
        self.mass = dastgen2.attributes.Double("mass")
        self.velocity = peano4.dastgen2.Peano4DoubleArray("v", "Dimensions")
        self.acceleration = peano4.dastgen2.Peano4DoubleArray("a", "Dimensions")
        self.data.add_attribute(self.mass)
        self.data.add_attribute(self.velocity)
        self.data.add_attribute(self.acceleration)

        # Basic SPH properties
        self.density = dastgen2.attributes.Double("density")
        self.pressure = dastgen2.attributes.Double("pressure")
        self.smoothL = dastgen2.attributes.Double("smoothingLength")
        self.u = dastgen2.attributes.Double("u")
        self.uDot = dastgen2.attributes.Double("uDot")
        self.data.add_attribute(self.density)
        self.data.add_attribute(self.pressure)
        self.data.add_attribute(self.smoothL)
        self.data.add_attribute(self.u)
        self.data.add_attribute(self.uDot)

        # Extended arrays for time integration consistency
        self.v_full = peano4.dastgen2.Peano4DoubleArray("v_full", "Dimensions")
        self.u_full = dastgen2.attributes.Double("u_full")
        self.data.add_attribute(self.v_full)
        self.data.add_attribute(self.u_full)

        # Variable smoothing length scheme ---------------------------------------------
        self.wcount = dastgen2.attributes.Double("wcount")
        self.wcount_dh = dastgen2.attributes.Double("wcount_dh")
        self.f = dastgen2.attributes.Double("f")
        self.hDot = dastgen2.attributes.Double("hDot")
        self.rho_dh = dastgen2.attributes.Double("rho_dh")
        self.sml_iteration_count = dastgen2.attributes.Integer(
            "smoothingLengthIterCount"
        )
        self.data.add_attribute(self.wcount)
        self.data.add_attribute(self.wcount_dh)
        self.data.add_attribute(self.f)
        self.data.add_attribute(self.hDot)
        self.data.add_attribute(self.rho_dh)
        self.data.add_attribute(self.sml_iteration_count)

        # Extra ------------------------------------------------------------------------
        self.has_no_neighbours = dastgen2.attributes.Boolean("hasNoNeighbours")
        self.sml_converged = dastgen2.attributes.Boolean(
            "smoothingLengthConverged", initval="false"
        )
        self.density_neighbourcount = dastgen2.attributes.Integer(
            "densityNeighbourCount", ifdefs=["PeanoDebug > 0"]
        )
        self.force_neighbourcount = dastgen2.attributes.Integer(
            "forceNeighbourCount", ifdefs=["PeanoDebug > 0"]
        )
        self.part_id = dastgen2.attributes.Integer("partid")
        self.data.add_attribute(self.has_no_neighbours)
        self.data.add_attribute(self.sml_converged)
        self.data.add_attribute(self.density_neighbourcount)
        self.data.add_attribute(self.force_neighbourcount)
        self.data.add_attribute(self.part_id)

        return

    def set_parameters(self):
        """!
        This function translates "global" particle parameters which are
        constant throughout the simulation (like CFL factor, minimal time step
        size, viscosity parameters...) into dastgen attributes of the C++
        particle class.
        If you modify any of the attributes manually outside of the particle
        initialisation, e.g. by invoking

        ```
        particle = SPHParticle(initial_time_step_size=ABC, ...)
        particle.initial_time_step_size = XYZ
        ```

        you need to call this function manually so your changes propagate
        into the generated C++ files.
        """

        const_static = dastgen2.attributes.Attribute.Qualifier.CONST_STATIC

        cfl_attr = dastgen2.attributes.Double(
            "cfl", qualifier=const_static, initval=self._cfl_factor
        )
        self.data.add_or_replace_attribute(cfl_attr)

        initial_time_step_size_attr = dastgen2.attributes.Double(
            "initialTimeStepSize",
            qualifier=const_static,
            initval=self._initial_time_step_size,
        )
        self.data.add_or_replace_attribute(initial_time_step_size_attr)

        if self._constant_time_step_size:
            adjustTimeStepSize = "false"
        else:
            adjustTimeStepSize = "true"
        adjust_time_step_size_attr = dastgen2.attributes.Boolean(
            "adjustTimeStepSize", qualifier=const_static, initval=adjustTimeStepSize
        )
        self.data.add_or_replace_attribute(adjust_time_step_size_attr)

        hydro_dimensions_attr = dastgen2.attributes.Double(
            "hydroDimensions", qualifier=const_static, initval=self._hydro_dimensions
        )
        self.data.add_or_replace_attribute(hydro_dimensions_attr)

        eta_factor_attr = dastgen2.attributes.Double(
            "etaFactor", qualifier=const_static, initval=self._eta_factor
        )
        self.data.add_or_replace_attribute(eta_factor_attr)

        h_hydro_min_attr = dastgen2.attributes.Double(
            "smlMin", qualifier=const_static, initval=self._h_hydro_min
        )
        self.data.add_or_replace_attribute(h_hydro_min_attr)

        h_hydro_max_attr = dastgen2.attributes.Double(
            "smlMax", qualifier=const_static, initval=self._h_hydro_max
        )
        self.data.add_or_replace_attribute(h_hydro_max_attr)

        h_tolerance_attr = dastgen2.attributes.Double(
            "smlTolerance", qualifier=const_static, initval=self._h_tolerance
        )
        self.data.add_or_replace_attribute(h_tolerance_attr)

        h_max_iterations_attr = dastgen2.attributes.Integer(
            "smlMaxIterations", qualifier=const_static, initval=self._h_max_iterations
        )
        self.data.add_or_replace_attribute(h_max_iterations_attr)

        return

    @abc.abstractmethod
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
        """
        return

    def algorithm_steps(self):
        """!

        Return algorithm steps: A list of AlgorithmStep objects to be executed in
        that order for this particle.

        Make sure that self._setup_algorithm_steps_dict() and
        self._setup_algorithm_steps() have been called before using this function.

        """

        return self._algorithm_steps

    @abc.abstractmethod
    def _setup_initialisation_steps(self):
        """
        Define the algorithm steps to be taken during initialization here.
        This follows the same logic as _setup_algorithm_steps. See documentation
        there for more info.

        Make sure self._setup_algorithm_steps_dict() has been called before.

        Every derived class must implement this function depending on its own
        requirements.
        """
        return

    def initialisation_steps(self):
        """
        Return the list of algorithm steps to be executed during initialisation.
        Make sure self._setup_algorithm_steps_dict() and self._setup_initialisation_steps() have been called before.

        Every derived class must implement this function depending on its own
        requirements.
        """
        return self._initialisation_steps

    @property
    def hydro_dimensions(self):
        """!
        Forbid users to modify hydro dimensions on-the-fly.
        """
        return self._hydro_dimensions

    @hydro_dimensions.setter
    def hydro_dimensions(self, hydro_dimensions):
        raise ValueError(
            "Can't change hydro_dimensions now. Modify it while instantiating the class."
        )

    @property
    def eta_factor(self):
        """!

        Set the eta factor used by the SPH kernel to target a certain number of
        neighbour particles.
        The default value is eta = 1.2348.

        """
        return self._eta_factor

    @eta_factor.setter
    def eta_factor(self, eta_factor):
        if eta_factor <= 0:
            raise ValueError("SPH eta factor value not valid.")
        self._eta_factor = eta_factor

    @property
    def h_hydro_min(self):
        """

        Set the limits allowed for the SPH smoothing length. Notice that the max value
        is bounded by Peano's search radius as h_max = R_search_max = max_grid_size /
        gamma_k, where gamma_k is the SPH kernel factor that relates h and H.
        Typically gamma_k = 2, so h_max = max_grid_size / 2.

        """
        return self._h_hydro_min

    @h_hydro_min.setter
    def h_hydro_min(self, h_hydro_min):
        if h_hydro_min > self._h_hydro_max:
            raise ValueError("Min value of smoothing length larger than max allowed.")
        self._h_hydro_min = h_hydro_min

    @property
    def h_hydro_max(self):
        return self._h_hydro_max

    @h_hydro_max.setter
    def h_hydro_max(self, h_hydro_max):
        if h_hydro_max < self._h_hydro_min:
            raise ValueError("max value of smoothing length smaller than min allowed.")
        self._h_hydro_max = h_hydro_max

    @property
    def h_tolerance(self):
        """

        Tolerance for Newton-Raphson convergence criterion.

        """
        return self._h_tolerance

    @h_tolerance.setter
    def h_tolerance(self, h_tolerance):
        if h_tolerance > 1:
            raise ValueError("Value of h_tolerance cannot be larger than 1.")
        self._h_tolerance = h_tolerance

    @property
    def h_max_iterations(self):
        """
        Max number of iterations to adapt the SPH smoothing length
        """
        return self._h_max_iterations

    @h_max_iterations.setter
    def h_max_iterations(self, h_max_iterations):
        if h_max_iterations > 50:
            raise ValueError("Value of h_max_iterations is too large.")
        self._h_max_iterations = h_max_iterations

    @property
    def mantissa_size(self):
        """

        Set the mantissa size of doubles and Peano double arrays if
        we want to use reduced precission via Clang annotations. As a reference,
        floats have mantissa size = 23.

        """
        return self._mantissa_size

    @mantissa_size.setter
    def mantissa_size(self, mantissa_size):
        if mantissa_size < 0:
            raise ValueError("Mantissa size has to be larger than 0.")
        self._mantissa_size = mantissa_size

    def get_cpp_namespace_from_project_namespace(self):
        """

        Transform namespace into cpp format. Could be used to append namespace to
        constants in kernels (not used currently).

        """
        namespace = "::".join(self._swift_project_namespace) + "::"

        print(namespace)

    @property
    @abc.abstractmethod
    def readme_descriptor(self):
        return super(SPHParticle, self).readme_descriptor
