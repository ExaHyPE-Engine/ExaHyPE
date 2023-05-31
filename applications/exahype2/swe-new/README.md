
# Shallow-Water Equations #

## Full Equations ##

$\partial_t \begin{pmatrix}
                h\\hu\\hv
               \end{pmatrix}
               + \partial_x
               \begin{pmatrix}
                hu\\hu^2 + \frac{1}{2} \cdot gh^2\\huv
               \end{pmatrix}
               +  \partial_y
               \begin{pmatrix}
                hv\\huv\\hv^2 + \frac{1}{2} \cdot gh^2
               \end{pmatrix} =
               \begin{pmatrix}
                0\\-ghb_x\\-ghb_y
               \end{pmatrix}$

With $b(x)$ being the bathymetry measured from a reference $D$ (normal sea level, negative below, positive above), $h(x,y,t)$ being the total positive water depth and $g$ the earth's gravitational acceleration set to $9.81ms^{-2}$. The two velocities $u(x,y,t)$ and $v(x,y,t)$ are the velocities in x- and y-dimension respectively.

The eigenvalues (wave speeds) in x-dimension are given by:

$\lambda_{x_1} = u$, $\lambda_{x_2,_3} = u \pm \sqrt{gh}$

And in y-dimension by:

$\lambda_{y_1} = v$, $\lambda_{y_2,_3} = v \pm \sqrt{gh}$

The respective derivations of the various aspects of the SWE can be found in "Shock-Capturing Methods for Free-Surface Shallow Flows" by E.F. Toro (2001).

## Implementation ##

All important aspects of the shallow-water equations are modelled in [template_scenario.py](scenarios/template_scenario.py) with the eigenvalues being modelled in [pde.py](pde.py) of which [template_scenario.py](scenarios/template_scenario.py) is a child class.

### Flux Implementation ###

The two fluxes in x- and y-dimension respectively are given by:

$
F(U) = \begin{pmatrix}
        hu\\hu^2 + \frac{1}{2} \cdot gh^2\\huv
    \end{pmatrix}
$

and

$
G(U) = \begin{pmatrix}
        hv\\huv\\hv^2 + \frac{1}{2} \cdot gh^2
    \end{pmatrix}
$

They are implemented in the following way:

```cpp
switch(normal) {
    case 0:
        F[0] = hu;
        F[1] = (hu * hu)/ h  + (g * h * h) / 2;
        F[2] = (hu * hv) / h;
        break;
    case 1:
        F[0] = hv;
        F[1] = (hu * hv) / h;
        F[2] = (hv * hv) / h + (g * h * h) / 2;
        break;
        }
```

### Sources Implementation ###

The source term

$
S(U) =  \begin{pmatrix}
            0\\-ghb_x\\-ghb_y
        \end{pmatrix}
$

is implemented as follows:

```cpp
S[0] = 0;
S[1] = -g * h * b;
S[2] = -g * h * b;
```

## Scenarios ##

There is a plethora of scenarios implemented in [scenarios](scenarios/), which are going to be quickly detailed here.

Two files are of note here as they are not typical scenarios:

- [scenario.py](scenarios/scenario.py), which is an interface detailing the different methods each scenario has to implement.
- [template_scenario.py](scenarios/template_scenario.py), which is a template implementation of the fluxes and sources of which every other scenario implemented derives from.

### Standard Scenarios ###

Standard scenarios are taken from the Practical Course Tsunami Simulation (IN4153) held at Technical University Munich in WS21/22.

They are used to test basic functionality of the SWE implementation such as flux and bathymetry.

- [Radial Dam Break](scenarios/radial_dam_break.py):  
  Scenario simulating a radial dam break.  
  In the middle of the simulation plane, there is a circular discontinuity in water height leading to a dam break problem at $t=0$.
- [Radial Bathymetry Dam Break](scenarios/radial_bathymetry_dam_break.py):  
  Scenario simulating a radial dam break caused by discontinuity in the bathymetry.  
  In the middle of the simulation plane, there is a circular discontinuity in the bathymetry leading to a discontinuity in water height at $t=0$.
- [Artificial Tsunami](scenarios/artificial_tsunami.py):  
  Scenario simulating an artificial tsunami.
  - The displacment for the tsunami is given by:  
  $d(x,y) = 5m \cdot d_x(x)d_y(y)$ with $d_x(x) = sin((\frac{x}{500m}+1) \cdot \pi)$, $d_y(y) = -(\frac{y}{500m})^2+1$, on a 1km $\times$ 1km domain.
  - Bathymetry and water height are given by:  
  $b(x,y) = -100m$ and $h(x,y,0)=100m$, on a 10km $\times$ 10km domain.
- [Sea At Rest](scenarios/sea_at_rest.py):  
  Scenario simulating a sea at rest. It uses the same bathymetry and displacement data as the artificial tsunami scenario but bathymetry + displacement balance out with the water height leading to no expected perturbations.

### Wetting and Drying Scenarios ###

These scenarios are taken from "A continuous/discontinuous Galerkin solution of the shallow water equations with dynamic viscosity, high-order wetting and drying, and implicit time integration" by Marras et al. (2016).

They implement various wetting and drying scenarios to test the inundation capabilities of ExaHyPE.

- [Oscillating Lake](scenarios/oscillating_lake.py):
- [Three Mounds Channel](scenarios/three_mounds_channel.py):
- [Circular Island](scenarios/circular_island.py):

### Tsunami Scenario ###

This [scenario](scenarios/tsunami.py) uses a NetCDF reader to read in bathymetry and displacement files and initializes the variables accordingly. With it, tsunamis such as the 2011 Tohoku event can be simulated.
