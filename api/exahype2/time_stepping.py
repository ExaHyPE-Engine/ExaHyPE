# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import math


def _positive_finite(value, name):
    if (
        isinstance(value, bool)
        or not isinstance(value, (int, float))
        or not math.isfinite(value)
        or value <= 0
    ):
        raise ValueError(
            "{} must be a finite positive number, got {!r}.".format(name, value)
        )
    return value


class Fixed:
    """The time step size follows the smallest mesh width, scaled by the given factor."""

    rule = "Fixed"
    adaptive = False

    def __init__(self, normalised_time_step_size):
        self.factor = _positive_finite(
            normalised_time_step_size, "The normalised time step size"
        )

    def __repr__(self):
        return "TimeStepping.Fixed({})".format(self.factor)


class Adaptive:
    """The time step size follows the CFL condition, relaxed by the given factor."""

    rule = "Adaptive"
    adaptive = True

    def __init__(self, relaxation):
        self.factor = _positive_finite(relaxation, "The time step relaxation")

    def __repr__(self):
        return "TimeStepping.Adaptive({})".format(self.factor)


class TimeStepping:
    Fixed = Fixed
    Adaptive = Adaptive


def of(value):
    """Returns ``value`` as a time-stepping mode and rejects anything else."""
    if isinstance(value, (Fixed, Adaptive)):
        return value
    raise TypeError(
        "time_stepping must be TimeStepping.Fixed(...) or TimeStepping.Adaptive(...), "
        "got {!r}.".format(value)
    )
