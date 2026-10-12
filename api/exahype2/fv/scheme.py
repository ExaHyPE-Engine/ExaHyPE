# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org


class Limiter:
    """The slope-limiter policy types of the MUSCL-Hancock scheme."""

    superbee = "::exahype2::fv::limiter::Superbee"
    minmod = "::exahype2::fv::limiter::Minmod"
    zero = "::exahype2::fv::limiter::Zero"
    average = "::exahype2::fv::limiter::Average"
    vanalbada = "::exahype2::fv::limiter::VanAlbada"
    mclim = "::exahype2::fv::limiter::MinmodCentral"


class Weighting:
    """The nonlinear-weighting policy types of the WENO scheme: The Jiang-Shu weights with the
    exponent 2 of Jiang and Shu, or with the exponent 8 of the ADER-WENO schemes of Dumbser et al."""

    jiangshu = "::exahype2::fv::reconstruction::weno::JiangShu<2>"
    jiangshu_r8 = "::exahype2::fv::reconstruction::weno::JiangShu<8>"


class SubcellScheme:
    """The C++ description of one finite-volume scheme on a patch or a subgrid.

    ``time_stepping`` is the policy type that advances one volume, or ``None`` for a scheme that
    advances whole patches. ``subcell_family`` is the class template that advances the subgrids of
    the DG subcell limiter, declared in ``subcell_family_include``.
    """

    def __init__(
        self,
        reconstruction,
        halo_width,
        subcell_family,
        subcell_family_include,
        time_stepping=None,
        fallback_reconstruction=None,
        includes=None,
    ):
        if not reconstruction:
            raise ValueError("A subcell scheme needs a reconstruction policy type.")
        if not subcell_family or not subcell_family_include:
            raise ValueError("A subcell scheme needs a subcell family and its header.")
        if (
            isinstance(halo_width, bool)
            or not isinstance(halo_width, int)
            or halo_width < 1
        ):
            raise ValueError("A subcell scheme needs a positive integer halo width.")
        self._reconstruction = reconstruction
        self._time_stepping = time_stepping
        self._subcell_family = subcell_family
        self._subcell_family_include = subcell_family_include
        self._halo_width = halo_width
        self._fallback_reconstruction = fallback_reconstruction
        self._includes = includes or ["exahype2/fv/Patch.h"]

    @property
    def reconstruction(self):
        return self._reconstruction

    @property
    def time_stepping(self):
        return self._time_stepping

    @property
    def halo_width(self):
        return self._halo_width

    @property
    def subcell_family(self):
        return self._subcell_family

    @property
    def subcell_family_include(self):
        return self._subcell_family_include

    @property
    def fallback_reconstruction(self):
        return self._fallback_reconstruction

    @property
    def includes(self):
        return self._includes


class Godunov:
    """First-order Godunov: Constant reconstruction with one halo volume."""

    halo_width = 1

    def subcell_scheme(self):
        return SubcellScheme(
            reconstruction="::exahype2::fv::reconstruction::Constant",
            time_stepping="::exahype2::fv::singlestep::SingleStep",
            halo_width=self.halo_width,
            subcell_family="::exahype2::fv::singlestep::SubcellStep",
            subcell_family_include="exahype2/fv/singlestep/SubcellStep.h",
            includes=[
                "exahype2/fv/singlestep/PatchUpdate.h",
                "exahype2/fv/reconstruction/Constant.h",
                "exahype2/fv/singlestep/SingleStep.h",
            ],
        )

    def __repr__(self):
        return "Godunov()"


class MusclHancock:
    """Second-order MUSCL-Hancock: Limited linear reconstruction with two halo volumes.

    ``limiter`` is a policy type of :class:`Limiter`; a solver that sets the
    ``limited_difference`` term limits with its own function, and the policy stays unused.
    """

    halo_width = 2

    def __init__(self, limiter=Limiter.minmod):
        if not isinstance(limiter, str) or not limiter.startswith("::"):
            raise ValueError(
                "The MUSCL-Hancock scheme expects a limiter of exahype2.fv.Limiter. A C++ "
                "limiter body belongs into the limited_difference PDE term."
            )
        self.limiter = limiter

    def subcell_scheme(self):
        return SubcellScheme(
            reconstruction="::exahype2::fv::reconstruction::LimitedLinear<"
            + self.limiter
            + ">",
            time_stepping="::exahype2::fv::singlestep::SingleStep",
            halo_width=self.halo_width,
            subcell_family="::exahype2::fv::singlestep::SubcellStep",
            subcell_family_include="exahype2/fv/singlestep/SubcellStep.h",
            fallback_reconstruction=(
                "::exahype2::fv::reconstruction::LimitedLinear<::exahype2::fv::limiter::Zero>"
            ),
            includes=[
                "exahype2/fv/singlestep/PatchUpdate.h",
                "exahype2/fv/limiter/SlopeLimiter.h",
                "exahype2/fv/reconstruction/LimitedLinear.h",
                "exahype2/fv/singlestep/SingleStep.h",
            ],
        )

    def __repr__(self):
        return "MusclHancock(limiter={})".format(self.limiter)


class PolynomialLimiter:
    """The policy types that limit the polynomial of every volume after the WENO reconstruction,
    before the space-time predictor evolves it. ``admissibility_scaling`` scales the polynomial
    towards the volume average until it passes the ``physical_admissibility`` PDE term at the nodes
    and on the faces of the volume (Zhang and Shu)."""

    none = "::exahype2::fv::limiter::Unlimited"
    admissibility_scaling = "::exahype2::fv::limiter::PolynomialScaling"


class Fallback:
    """The a-posteriori fallback of a high-order finite-volume scheme after the MOOD paradigm: A
    patch whose candidate solution fails the ``physical_admissibility`` PDE term or the discrete
    maximum principle repeats its time step with ``scheme``. Only the Godunov scheme can be the
    fallback scheme."""

    def __init__(self, scheme=None):
        self.scheme = Godunov() if scheme is None else scheme
        if not isinstance(self.scheme, Godunov):
            raise TypeError("The fallback scheme is exahype2.fv.Godunov().")

    @property
    def reconstruction(self):
        return self.scheme.subcell_scheme().reconstruction

    @property
    def includes(self):
        return self.scheme.subcell_scheme().includes

    def __repr__(self):
        return "Fallback(scheme={!r})".format(self.scheme)


class Weno:
    """ADER-WENO after Dumbser et al.: A polynomial WENO reconstruction of order ``order`` evolved
    by the space-time predictor of the ADER-DG solvers, patch by patch. The scheme of
    :class:`exahype2.fv.ader.Solver` and of the DG subcell limiter.

    ``weighting`` is a policy type of :class:`Weighting` and ``reconstruction_limiter`` one of
    :class:`PolynomialLimiter`. ``fallback`` is a :class:`Fallback`, or ``None`` for a scheme
    without fallback. The scheme reads ``order + 1`` halo volumes and needs at least
    ``2 * order`` volumes per patch axis.
    """

    SUPPORTED_ORDERS = (2, 3, 4, 5)

    def __init__(
        self,
        order=2,
        weighting=Weighting.jiangshu_r8,
        reconstruction_limiter=PolynomialLimiter.none,
        fallback=Fallback(),
    ):
        if isinstance(order, bool) or order not in self.SUPPORTED_ORDERS:
            raise ValueError(
                "The WENO order is one of {}.".format(self.SUPPORTED_ORDERS)
            )
        if not isinstance(weighting, str) or not weighting.startswith("::"):
            raise ValueError(
                "The weighting is a policy type name of exahype2.fv.Weighting."
            )
        if reconstruction_limiter not in (
            PolynomialLimiter.none,
            PolynomialLimiter.admissibility_scaling,
        ):
            raise ValueError(
                "The reconstruction limiter is a policy type name of exahype2.fv.PolynomialLimiter."
            )
        if fallback is not None and not isinstance(fallback, Fallback):
            raise TypeError("The fallback is exahype2.fv.Fallback(...) or None.")
        self.order = order
        self.weighting = weighting
        self.reconstruction_limiter = reconstruction_limiter
        self.fallback = fallback

    @property
    def halo_width(self):
        return self.order + 1

    @property
    def minimum_volumes_per_axis(self):
        return 2 * self.order

    @property
    def scales_polynomials(self):
        return self.reconstruction_limiter == PolynomialLimiter.admissibility_scaling

    def subcell_scheme(self):
        includes = [
            "exahype2/fv/Patch.h",
            "exahype2/fv/limiter/PolynomialScaling.h",
            "exahype2/fv/reconstruction/WENO.h",
            "exahype2/fv/reconstruction/weno/NonlinearWeights.h",
        ]
        if self.fallback is not None:
            includes += self.fallback.includes
        return SubcellScheme(
            reconstruction="::exahype2::fv::reconstruction::WENO<{}, {}, {}>".format(
                self.order, self.weighting, self.reconstruction_limiter
            ),
            halo_width=self.halo_width,
            subcell_family="::exahype2::fv::ader::SubcellStep",
            subcell_family_include="exahype2/fv/ader/SubcellStep.h",
            fallback_reconstruction=(
                None if self.fallback is None else self.fallback.reconstruction
            ),
            includes=includes,
        )

    def __repr__(self):
        return "Weno(order={}, weighting={}, reconstruction_limiter={}, fallback={!r})".format(
            self.order, self.weighting, self.reconstruction_limiter, self.fallback
        )
