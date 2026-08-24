# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
"""Describes a finite-volume scheme for a patch or subgrid.

A :class:`SubcellScheme` names the C++ reconstruction and time-stepping policies,
the required headers, the reconstruction halo width, and an optional fallback
reconstruction. Each finite-volume package exposes a ``subcell_scheme`` factory
so its standalone solver and the a-posteriori limiter share one configuration.
"""


class SubcellScheme:
    def __init__(
        self,
        reconstruction,
        time_stepping,
        halo_width,
        fallback_reconstruction=None,
        includes=None,
        riemann_solver=None,
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
        self._riemann_solver = riemann_solver

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
        """Returns the headers that define the scheme's policy types."""
        return self._includes

    @property
    def riemann_solver(self):
        return self._riemann_solver
