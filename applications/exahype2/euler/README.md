# The Euler Equations

The Euler application uses the rich feature set of ExaHyPE 2,
a set of more advanced solvers, (dynamically adaptive) mesh refinement,
enclave tasking, GPU offloading, and hints to achieve better performance and scaling behaviour.

## Limitations
With plain DG, Rusanov, and without an additional limiter, we cannot solve
discontinuous initial conditions. At the moment, most of the scenarios
are physically meaningless. The only setup that should work
out-of-the-box is the Gaussian, as it is infinitely smooth. Having said
this, the Gaussian still can run into problems if it is too steep: If the
polynomial order and mesh resolution are too low, the Gaussian continues
to look like a discontinuous initial condition and thus introduces
problems. Therefore, we scale its diameter with the maximum cell width. In
a real setup, we should also take the polynomial order into account.
