# Euler examples

All the Euler examples compute the solution to the Euler equation with
a Gaussian energy distribution as initial stimulus. We use homogeneous
Neumann (outflow) conditions, and we can run this example in parallel.
While all kernels are written for 2d and 3d, only 2d is used by the 
examples. No other features such as periodic boundary conditions or AMR
are used.
 

## Clean up temporary files

As Euler is so simplistic, we can clean up a lot of temporary files in 
one rush:

     rm *patch-file 
     rm Abstract* *.h
     rm -rf celldata facedata observers repositories
     rm *.vtu *.pvd


## The solver variants


### Finite Volumes

The Finite Volume solver as pre-prepared uses a Rusanov Riemann solver 
and an explicit Euler as time integration scheme.

     python3 finitevolumes.py
     ./finitevolumes

     
### Finite Differences

The example Finite Differences solver uses the FD4 (fourth order) 
formulation which we have developed for the astrophysics setup. A
Runge-Kutta time stepping (default, i.e. 4th order) is used as time
integrator.


    python3 finitedifferences.py
    ./finitedifferences
    
    
### Discontinuous Galerkin    
    
The DG example uses a fourth order DG scheme in combination with fourth
order Runge-Kutta.
         