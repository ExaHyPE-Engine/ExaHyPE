# The acoustic wave equation

The acoustic wave equation describes the propagation of a wave through a material. This
equation assumes that all movements are only small perturbations over the background flow
field. They can be obtained by linearizing the more general Euler equations over this background field. This clearly cannot work for complex flows, e.g., for supersonic “booms”.
We describe our material by the parameters $`K_0`$ , which is the so-called bulk-modulus and by the density $`\rho_0`$. Waves propagate through the material with a velocity of $`c=\sqrt{\rho K_0}`$.

We solve here for velocity $`v`$ and pressure $`p`$ in one dimension

```math
    \partial_t \begin{pmatrix}
                p\\v_x\\v_y
               \end{pmatrix}
               + \nabla \cdot F
               \begin{pmatrix}
                p\\v_x\\v_y
               \end{pmatrix} = 0,
```
where $`v_x, v_y`$ denote the velocity in $`x`$- and $`y`$-direction respectively.

Further, the flux is given by:
```math
  F = \left[
       \begin{pmatrix}
                0 & K_0 & 0\\
                \frac{1}{\rho_0} &  0 &0\\
                0 & 0 & 0 
               \end{pmatrix}  ,
              \begin{pmatrix}
                0 & 0 & K_0\\
                0 & 0 & 0\\
                \frac{1}{\rho_0} & 0 & 0
               \end{pmatrix} \right],
```
The eigenvalues of the flux are $`-c`$, $`0`$ and $`c`$.
These equations are telling us that the change in time of the velocity is proportional to the gradient of pressure.
