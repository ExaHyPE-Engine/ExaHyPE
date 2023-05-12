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
                        \end{array} \right),
$$
with the $d$-dimensional velocity vector $\bold{u}=(u, v)^T$.

$F^h$ is the hyperbolic flux given by

$$
    F^h(Q) = \left(\begin{array}{lr} \rho \bold{u}\\
                        \bold{u} \otimes \rho \bold{u} + \bold{I}p \\
                        \bold{u} \cdot (\bold{I} (\rho E + p)) \\
                        \end{array}\right).
$$

$F^v$ is the viscous flux (modelled as non-conservative product):

$$
    F^v= \left(\begin{array}{lr} 0 \\
                        \sigma(Q,\nabla Q) \\
                        \bold{u} \cdot \sigma(Q,\nabla Q) - \kappa \nabla T \\
                        \end{array}\right),
$$

and $S(Q,x,t)$ is the source term.

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

We assume isothermal conditions for our setup. Thus, $\nabla T = 0$. We can write the non-conservative product as:

$$
    F^h(Q,\nabla Q) = \begin{pmatrix}
        0 & 0 \\
        -\mu \frac{4}{3}\frac{\partial u}{\partial x} & -\mu(\frac{\partial u}{\partial y} + \frac{\partial v}{\partial x}) \\
        -\mu(\frac{\partial u}{\partial y} + \frac{\partial v}{\partial x}) & -\mu \frac{4}{3}\frac{\partial v}{\partial y} \\
        u\sigma_{11}+v\sigma_{12} &u\sigma_{21}+v\sigma_{22}
    \end{pmatrix}
$$

## Simulation Setup: Lid Driven Cavity
### Material Parameters
Fluid Viscosity = $0.01$

Prandtl Number = $0.7$

Gamma ($\gamma$)= $1.4$

### Initial Conditions
$$
    \rho (\bold{x}) = 1 \\
    \bold{u(x)} = 0 \\
    p(x) = 100 / \gamma \\
$$

### Boundary Conditions
We use no-slip boundary conditions on walls, with the top layer of the cavity moving with **wall_velocity**.

### Source Term
In the lid-driven cavity scenario, we do not have any source terms and thus our right hand side is $\bold{0}$.
