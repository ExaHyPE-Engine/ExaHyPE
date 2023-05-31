
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

With $b(x)$ being the bathymetry measured from a reference $D$ (normal sea level, negative below, positive above), $h(x,y,t)$ being the total positive water depth and $g$ the earth's gravitational acceleration set to $9.81ms^{-2}$.

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
