# The Euler Equations

This application reuses the symbolically implemented Euler equations
from the tutorial and implements more scenarios with more complicated solvers.

For a detailed explanation, please refer to the [README](../../../tutorials/exahype2/euler/README.md)
in the Euler tutorial.

## Solvers
To write...

## Scenarios
To write...

## Limitations
With plain DG, Rusanov and without an additional limiter, we cannot solve
discontinuous initial condition. At the moment, most of the scenarios
are physically meaningless. The only setup that should work
out-of-the-box is the Gaussian, as it is infinitely smooth. Having said
this, the Gaussian still can run into problems if it is too steep: If the
polynomial order and mesh resolution are too low, the Gaussian continues
to look like a discontinuous initial condition and thus introduces
problems. Therefore, we scale its diameter with the maximum cell width. In
a real setup, we should also take the polynomial order into account.
