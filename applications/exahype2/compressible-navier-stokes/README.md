# The Compressible Navier-Stokes Equations

The compressible Navier-Stokes equations are defined as follows:
$$
    \frac{\partial}{\partial t} Q
    +
    \nabla \cdot (F^h(Q)
    +
    F^v(Q,\nabla Q))
    =
    S(Q,x,t),
$$
where $Q$ is our vector of state:

$$
    Q = \left( \begin{array}{lr} \rho \\
                        \rho \bold{u} \\
                        \rho E \\
                        \rho Z \\
                        \end{array} \right),
$$
with the $d$-dimensional velocity vector $\bold{u}=(u, v)^T$.

$F^h$ is the hyperbolic flux given by

$$
    F^h(Q) = \left(\begin{array}{lr} \rho \bold{u}\\
                        \bold{u} \otimes \rho \bold{u} + \bold{I}p \\
                        \bold{u} \cdot (\bold{I} (\rho E + p)) \\
                        \rho \bold{u} Z \\
                        \end{array}\right).
$$

$F^v$ is the viscous flux (modelled as non-conservative product):

$$
    F^v= \left(\begin{array}{lr} 0 \\
                        \sigma(Q,\nabla Q) \\
                        \bold{u} \cdot \sigma(Q,\nabla Q) - \kappa \nabla T \\
                        0\\
                        \end{array}\right),
$$

and $S(Q,x,t)$ is the source term. The pressure is coupled to conserved variables as
$$
p = (\gamma -1)\left(\rho E -\frac{1}{2}(\bold{u}\cdot\rho\bold{u}-q_0\rho Z)\right).
$$

The maximum eigenvalues for both convective and viscous parts in direction of normal vector $n$ is given as
$$
|\lambda_c^{max}| = |\bold{u}_n| + c, \\
|\lambda_v^{max}| = max\left(\frac{4}{3}\frac{\mu}{\rho},\frac{\gamma\mu}{Pr\rho}\right); \\
$$

where $c=\sqrt{\gamma R T}$ is the speed of sound and $Pr$ is Prandtl number.

## Hyperbolic Flux Implementation
$$
    (u,v) \otimes \rho (u, v) + \bold{I}p = \begin{pmatrix}
                                                \rho u^2 & \rho uv \\
                                                \rho uv & \rho v^2
                                            \end{pmatrix} +
                                            \begin{pmatrix}
                                                p & 0 \\
                                                0 & p
                                            \end{pmatrix} =
                                            \begin{pmatrix}
                                                \rho u^2 + p & \rho uv \\
                                                \rho uv & \rho v^2 +p
                                            \end{pmatrix}
$$

$$
    \bold{u} \cdot (\bold{I} (\rho E + p)) = (u,v)^T \cdot \begin{pmatrix}
                                                            \rho E + p & 0 \\
                                                            0 & \rho E + p \\
                                                            \end {pmatrix} = (u(\rho E +p), v(\rho E + p))
$$

Therefore,
$$
F^h(Q)= \begin{pmatrix}
            \rho u & \rho v \\
            \rho u^2 + p & \rho uv \\
            \rho uv & \rho v^2 +p \\
            u(\rho E +p) & v(\rho E + p)
        \end{pmatrix}
$$

## Viscous Flux Implementation
The viscous stress tensor is defined as

$$
    \sigma(Q,\nabla Q) = \mu ((\frac{2}{3}\nabla\cdot\bold{u})\bold{I}-(\nabla\bold{u} + (\nabla\bold{u})^T))
$$

$$
    \nabla \bold{u} =   \begin{pmatrix}
                            \frac{\partial u}{\partial x} & \frac{\partial v}{\partial x} \\
                            \frac{\partial u}{\partial y} & \frac{\partial v}{\partial y} \\
                        \end{pmatrix}
    \implies
    \nabla \bold{u} + (\nabla \bold{u})^T = \begin{pmatrix}
                            2\frac{\partial u}{\partial x} &\frac{\partial u}{\partial y} + \frac{\partial v}{\partial x} \\
                            \frac{\partial u}{\partial y} + \frac{\partial v}{\partial x} & 2\frac{\partial v}{\partial y} \\
                        \end{pmatrix}
$$

$$
    (\nabla\cdot\bold{u})\bold{I} = \begin{pmatrix}
    \frac{\partial u}{\partial x} & 0 \\
    0 & \frac{\partial v}{\partial y} \\
    \end{pmatrix}
$$

Therefore,
$$
    \sigma(Q,\nabla Q)=\mu ((\frac{2}{3}\nabla\cdot\bold{u})\bold{I}-(\nabla\bold{u} + (\nabla\bold{u})^T)) = -\mu \begin{pmatrix}
                                \frac{4}{3}\frac{\partial u}{\partial x} &\frac{\partial u}{\partial y} + \frac{\partial v}{\partial x} \\
                                \frac{\partial u}{\partial y} + \frac{\partial v}{\partial x} & \frac{4}{3}\frac{\partial v}{\partial y} \\
    \end{pmatrix} = \begin{pmatrix} \sigma_{11} & \sigma_{12}\\ \sigma_{21}& \sigma_{22}\end{pmatrix}
$$

$$
    \bold{u}\cdot\sigma(Q,\nabla Q) = (u\sigma_{11}+v\sigma_{12}, u\sigma_{21}+v\sigma_{22})
$$

$\nabla T = (\frac{\partial T}{\partial x}, \frac{\partial T}{\partial y})$. Thus, we can write the non-conservative product as:

$$
    F^v(Q,\nabla Q) = \begin{pmatrix}
        0 & 0 \\
        -\mu \frac{4}{3}\frac{\partial u}{\partial x} & -\mu(\frac{\partial u}{\partial y} + \frac{\partial v}{\partial x}) \\
        -\mu(\frac{\partial u}{\partial y} + \frac{\partial v}{\partial x}) & -\mu \frac{4}{3}\frac{\partial v}{\partial y} \\
        u\sigma_{11}+v\sigma_{12} - \kappa\frac{\partial T}{\partial x}&u\sigma_{21}+v\sigma_{22}-\kappa\frac{\partial T}{\partial y} \\
    \end{pmatrix}
$$

## Chemical Reaction Source Term
The chemical reaction source term is given as

$$
S_{\rho Z} = -K(T)\rho Z,
$$

$K(T)$ is dependent on the reaction model. In this case we use 

$$
K(T)=\begin{cases}
-\frac{1}{\tau} \quad T > T_{ign} \\
0 \quad otherwise
\end{cases}
$$

$\tau$ is the reaction timescale and $T_{ign}$ is the activation temperature.

## Reconstruction of Temperature Gradient From Conservative Variables

$$
\frac{\partial T}{\partial x} = \frac {1}{c_v} \left(a\frac{\partial rho}{\partial x} + b \frac{\partial j_x}{\partial x}+c \frac{\partial j_y}{\partial x} -q_0\frac{\rho \frac{\partial Z}{\partial x} - Z\frac{\partial \rho}{\partial x}}{\rho^2}\right)
$$

$$
\frac{\partial T}{\partial y} = \frac {1}{c_v} \left(a\frac{\partial rho}{\partial y} + b \frac{\partial j_x}{\partial y}+c \frac{\partial j_y}{\partial y} -q_0\frac{\rho \frac{\partial Z}{\partial y} - Z\frac{\partial \rho}{\partial y}}{\rho^2}\right),
$$
where,
$$
a = -\frac{E}{\rho^2} + \frac{j_x^2+j_y^2}{\rho^3}, \\
b= -\frac{j_x}{\rho^2}, \\
c=-\frac{j_y}{\rho^2}, \\
j_x = \rho u,\\
j_y = \rho v.
$$

## Simulation Setup
Except for the CJ Detonation Wave case, all the scenarios use a $1\times 1$ domain centered at $(0.5,0.5)$.
###  Lid Driven Cavity
#### Material Parameters
Fluid Viscosity = $0.01$

Prandtl Number = $0.7$

Gamma ($\gamma$)= $1.4$

#### Initial Conditions
$$
    \rho (\bold{x}) = 1 \\
    \bold{u(x)} = 0 \\
    p(x) = 100 / \gamma \\
$$

#### Boundary Conditions
We use no-slip boundary conditions on walls, with the top layer of the cavity moving with **wall_velocity**.

###  Point Explosion
#### Material Parameters
Fluid Viscosity = $0.01$

Prandtl Number = $0.7$

Gamma ($\gamma$)= $1.4$

#### Initial Conditions
$$
    \rho (\bold{x}) = 1 \\
    \bold{u(x)} = 0 \\
    E = \begin{cases}
    1.00 \quad (x-0.5)^2+(y-0.5)^2<0.2\\
    1.01 \quad otherwise
    \end{cases}
$$

#### Boundary Conditions
We use zero gradient bouandary condition on all boundaries and for all conservatives.

###  Taylor Green Vortex (Not Working)
#### Material Parameters
Fluid Viscosity = $0.1$

Prandtl Number = $0.7$

Gamma ($\gamma$)= $1.4$

#### Initial Conditions
$$
    p = \frac{1}{4} (cos(4\pi x) + sin(4\pi y))
                            + \frac{100}{\gamma}, \\
    \rho = 1, \\
    u   = sin(2 \pi x) cos(2 \pi y)\\
    v   = -cos(2 \pi x) sin(2 \pi y)\\
$$

#### Boundary Conditions (Not Fully Implemented)
We use cauchy boundary conditions.

###  Shocktube
#### Material Parameters
Fluid Viscosity = $0.01$

Prandtl Number = $0.7$

Gamma ($\gamma$)= $1.4$

#### Initial Conditions
$$
    \rho = \begin{cases}
        0.125 \quad x<0.45 \\
        1.0 \quad else \\
    \end{cases}\\

    p = \begin{cases}
        0.1 \quad x<0.45 \\
        1.0 \quad else \\
    \end{cases}\\

    u = 0\\
    v=0\\
$$

#### Boundary Conditions
We use zero gradient bouandary condition on all boundaries and for all conservatives.

###  Double Shocktube
#### Material Parameters
Fluid Viscosity = $0.01$

Prandtl Number = $0.7$

Gamma ($\gamma$)= $1.4$

#### Initial Conditions
$$
    \rho = \begin{cases}
        0.125 \quad x<0.33 \ or \ x>0.66 \\
        1.0 \quad else \\
    \end{cases} \\

    p = \begin{cases}
        0.1 \quad x<0.33 \ or \ x>0.66\\
        1.0 \quad else \\
    \end{cases} \\

    u = 0\\
    v=0\\
$$

#### Boundary Conditions
We use zero gradient bouandary condition on all boundaries and for all conservatives.

###  Smooth Wave (Not Working)
#### Material Parameters
Fluid Viscosity = $0.01$

Prandtl Number = $0.7$

Gamma ($\gamma$)= $1.4$

#### Initial Conditions
$$
    \rho = 1 - (x-0.5)^2 -(y-0.5)^2 \\
    p = 1 - (x-0.5)^2 -(y-0.5)^2 \\
    u = 0\\
    v=0\\
$$

#### Boundary Conditions
We use zero gradient bouandary condition on vertical boundaries (x-normal) and reflective boundary condition on horizontal boundaries (y-normal).


###  CJ Detonation Wave

We use a $2\times2$ domain centered at origin for this scenario.

#### Material Parameters
Fluid Viscosity = $0.01$

Prandtl Number = $0.75$

$c_v = 2.5$

$R=1$

Activation Temperature ($T_{ign}$) = $0.26$

Chemical Reaction Timescale ($\tau$) = $0.1$
$q_0 = 1$

Gamma ($\gamma$)= $1.4$

#### Initial Conditions
$$
    \rho_u = 0.887565, \\
    \bold{u}_u = -0.57735 \begin{pmatrix}
    cos(\alpha) \\
    sin(\alpha)
    \end{pmatrix},
\\

p_u = 0.191709, \\
Z_u = 1. 
$$

$$
\rho_b = 1.4, \\
\bold{u}_b = -0.57735 \begin{pmatrix}
    0 \\
    0
\end{pmatrix}, \\
p_b = 1, \\
Z_b = 0,\\
$$

where $\alpha = atan2(y,x)$ and subscripts $b$ and $u$ represent burnt and unburnt values respectively. The domain is burnt for $(x^2+y^2\leq0.3)$.

#### Boundary Conditions
We use zero gradient bouandary condition on all boundaries and for all conservatives.
