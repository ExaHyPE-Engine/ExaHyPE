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
    """The nonlinear-weighting policy types of the WENO scheme."""

    jiangshu = "::exahype2::fv::weno::JiangShu"


class SubcellScheme:
    """The C++ description of one finite-volume scheme on a patch or a subgrid."""

    def __init__(
        self,
        reconstruction,
        time_stepping,
        halo_width,
        fallback_reconstruction=None,
        includes=None,
    ):
        if not reconstruction:
            raise ValueError("A subcell scheme needs a reconstruction policy type.")
        if not time_stepping:
            raise ValueError("A subcell scheme needs a time-stepping policy type.")
        if (
            isinstance(halo_width, bool)
            or not isinstance(halo_width, int)
            or halo_width < 1
        ):
            raise ValueError("A subcell scheme needs a positive integer halo width.")
        self._reconstruction = reconstruction
        self._time_stepping = time_stepping
        self._halo_width = halo_width
        self._fallback_reconstruction = fallback_reconstruction
        self._includes = includes or ["exahype2/fv/FiniteVolume.h"]

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
            time_stepping="::exahype2::fv::timestepping::SingleStep",
            halo_width=self.halo_width,
            includes=[
                "exahype2/fv/FiniteVolume.h",
                "exahype2/fv/reconstruction/Constant.h",
                "exahype2/fv/timestepping/SingleStep.h",
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
                "The limiter is a policy type name of exahype2.fv.Limiter; a C++ body belongs "
                "to the limited_difference term."
            )
        self.limiter = limiter

    def subcell_scheme(self):
        return SubcellScheme(
            reconstruction="::exahype2::fv::reconstruction::LimitedLinear<"
            + self.limiter
            + ">",
            time_stepping="::exahype2::fv::timestepping::SingleStep",
            halo_width=self.halo_width,
            fallback_reconstruction=(
                "::exahype2::fv::reconstruction::LimitedLinear<::exahype2::fv::limiter::Zero>"
            ),
            includes=[
                "exahype2/fv/FiniteVolume.h",
                "exahype2/fv/limiter/SlopeLimiter.h",
                "exahype2/fv/reconstruction/LimitedLinear.h",
                "exahype2/fv/timestepping/SingleStep.h",
            ],
        )

    def __repr__(self):
        return "MusclHancock(limiter={})".format(self.limiter)


class Weno:
    """Fifth-order WENO reconstruction with a Hancock predictor and three halo volumes.

    ``weighting`` is a policy type of :class:`Weighting`.
    """

    halo_width = 3

    def __init__(self, weighting=Weighting.jiangshu):
        if not isinstance(weighting, str) or not weighting.startswith("::"):
            raise ValueError(
                "The weighting is a policy type name of exahype2.fv.Weighting."
            )
        self.weighting = weighting

    def subcell_scheme(self):
        return SubcellScheme(
            reconstruction="::exahype2::fv::reconstruction::WENO<"
            + self.weighting
            + ">",
            time_stepping="::exahype2::fv::timestepping::SingleStep",
            halo_width=self.halo_width,
            includes=[
                "exahype2/fv/FiniteVolume.h",
                "exahype2/fv/reconstruction/WENO.h",
                "exahype2/fv/timestepping/SingleStep.h",
                "exahype2/fv/weno/NonlinearWeights.h",
            ],
        )

    def __repr__(self):
        return "Weno(weighting={})".format(self.weighting)
