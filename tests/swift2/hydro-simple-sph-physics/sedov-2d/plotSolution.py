#!/usr/bin/env python3

###############################################################################
# This file is part of SWIFT.
# Copyright (c) 2015 Bert Vandenbroucke (bert.vandenbroucke@ugent.be)
#                    Matthieu Schaller (schaller@strw.leidenuniv.nl)
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU Lesser General Public License as published
# by the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.
#
##############################################################################


import matplotlib
matplotlib.use("Agg")

import matplotlib.pyplot as plt
from scipy import stats
from scipy.special import gamma as Gamma
import numpy as np
import sys

from peano4.toolbox.particles.postprocessing.ParticleVTUReader import ParticleVTUReader



"""
!
Computes the analytical solution of the 3D Sedov blast wave.
The script works for a given initial box and dumped energy and computes the
solution at a later time t.


python3 plotSolution.py time

where:
@param time_step_sim : time to plot solution of
"""


if len(sys.argv) < 2:
    time = 0.05
else:
    time = float(sys.argv[1])

# -------------------------
# Extract data
# -------------------------

pvdfile = "particles.pvd"
reader = ParticleVTUReader(pvdfile=pvdfile, snapshot_time=time)
partData = reader.load()
time = reader.get_snapshot_time()
boxsize = 1.

x = partData.x[:, 0] - 0.5 * boxsize
y = partData.x[:, 1] - 0.5 * boxsize
vel_x = partData.v[:, 0]
vel_y = partData.v[:, 1]
pressure = partData.pressure
density = partData.density
mass = partData.mass
energy = partData.u

r = np.sqrt(x**2 + y**2)
v_r = (x * vel_x + y * vel_y)/ r





# -------------------------
# Get analytical solution
# -------------------------

def calc_a(g, nu=3):
    """
    exponents of the polynomials of the sedov solution
    g - the polytropic gamma
    nu - the dimension
    """
    a = [0] * 8

    a[0] = 2.0 / (nu + 2)
    a[2] = (1 - g) / (2 * (g - 1) + nu)
    a[3] = nu / (2 * (g - 1) + nu)
    a[5] = 2 / (g - 2)
    a[6] = g / (2 * (g - 1) + nu)

    a[1] = (((nu + 2) * g) / (2.0 + nu * (g - 1.0))) * (
        (2.0 * nu * (2.0 - g)) / (g * (nu + 2.0) ** 2) - a[2]
    )
    a[4] = a[1] * (nu + 2) / (2 - g)
    a[7] = (2 + nu * (g - 1)) * a[1] / (nu * (2 - g))
    return a


def calc_beta(v, g, nu=3):
    """
    beta values for the sedov solution (coefficients of the polynomials of the similarity variables)
    v - the similarity variable
    g - the polytropic gamma
    nu- the dimension
    """

    beta = (
        (nu + 2)
        * (g + 1)
        * np.array(
            (
                0.25,
                (g / (g - 1)) * 0.5,
                -(2 + nu * (g - 1))
                / 2.0
                / ((nu + 2) * (g + 1) - 2 * (2 + nu * (g - 1))),
                -0.5 / (g - 1),
            ),
        )
    )

    beta = np.outer(beta, v)

    beta += (g + 1) * np.array(
        (
            0.0,
            -1.0 / (g - 1),
            (nu + 2) / ((nu + 2) * (g + 1) - 2.0 * (2 + nu * (g - 1))),
            1.0 / (g - 1),
        ),
    ).reshape((4, 1))

    return beta


def sedov(t, E0, rho0, g, n=1000, nu=3):
    """
    solve the sedov problem
    t - the time
    E0 - the initial energy
    rho0 - the initial density
    n - number of points (10000)
    nu - the dimension
    g - the polytropic gas gamma
    """
    # the similarity variable
    v_min = 2.0 / ((nu + 2) * g)
    v_max = 4.0 / ((nu + 2) * (g + 1))

    v = v_min + np.arange(n) * (v_max - v_min) / (n - 1.0)

    a = calc_a(g, nu)
    beta = calc_beta(v, g=g, nu=nu)
    lbeta = np.log(beta)

    r = np.exp(-a[0] * lbeta[0] - a[2] * lbeta[1] - a[1] * lbeta[2])
    rho = ((g + 1.0) / (g - 1.0)) * np.exp(
        a[3] * lbeta[1] + a[5] * lbeta[3] + a[4] * lbeta[2]
    )
    p = np.exp(
        nu * a[0] * lbeta[0] + (a[5] + 1) * lbeta[3] + (a[4] - 2 * a[1]) * lbeta[2]
    )
    u = beta[0] * r * 4.0 / ((g + 1) * (nu + 2))
    p *= 8.0 / ((g + 1) * (nu + 2) * (nu + 2))

    # we have to take extra care at v=v_min, since this can be a special point.
    # It is not a singularity, however, the gradients of our variables (wrt v) are.
    # r -> 0, u -> 0, rho -> 0, p-> constant

    u[0] = 0.0
    rho[0] = 0.0
    r[0] = 0.0
    p[0] = p[1]

    # volume of an n-sphere
    vol = (np.pi ** (nu / 2.0) / Gamma(nu / 2.0 + 1)) * np.power(r, nu)

    # note we choose to evaluate the integral in this way because the
    # volumes of the first few elements (i.e near v=vmin) are shrinking
    # very slowly, so we dramatically improve the error convergence by
    # finding the volumes exactly. This is most important for the
    # pressure integral, as this is on the order of the volume.

    # (dimensionless) energy of the model solution
    de = rho * u * u * 0.5 + p / (g - 1)
    # integrate (trapezium rule)
    q = np.inner(de[1:] + de[:-1], np.diff(vol)) * 0.5

    # the factor to convert to this particular problem
    fac = (q * (t**nu) * rho0 / E0) ** (-1.0 / (nu + 2))

    # shock speed
    shock_speed = fac * (2.0 / (nu + 2))
    rho_s = ((g + 1) / (g - 1)) * rho0
    r_s = shock_speed * t * (nu + 2) / 2.0
    p_s = (2.0 * rho0 * shock_speed * shock_speed) / (g + 1)
    u_s = (2.0 * shock_speed) / (g + 1)

    r *= fac * t
    u *= fac
    p *= fac * fac * rho0
    rho *= rho0
    return r, p, rho, u, r_s, p_s, rho_s, u_s, shock_speed

# Parameters
rho_0 = 1.0  # Background Density
P_0 = 1.0e-6  # Background Pressure
E_0 = 1.0  # Energy of the explosion
gas_gamma = 5.0 / 3.0  # Gas polytropic index

# The main properties of the solution
r_s, P_s, rho_s, v_s, r_shock, _, _, _, _ = sedov(time, E_0, rho_0, gas_gamma, 1000, 2)

# Append points for after the shock
r_s = np.insert(r_s, np.size(r_s), [r_shock, r_shock * 3])
rho_s = np.insert(rho_s, np.size(rho_s), [rho_0, rho_0])
P_s = np.insert(P_s, np.size(P_s), [P_0, P_0])
v_s = np.insert(v_s, np.size(v_s), [0, 0])

# Additional arrays
u_s = P_s / (rho_s * (gas_gamma - 1.0))  # internal energy
#  s_s = P_s / rho_s**gas_gamma  # entropic function


# -------------------------
# Plot, plot, plot!
# -------------------------

sim_props = {
    "marker":".",
    "ms":4,
    "markeredgecolor":"none",
    "alpha":1.0,
    "zorder":-1,
    "rasterized":True,
    "linestyle":"none",
    }
solution_props = {
        "linestyle": "--",
        "color" : "C4",
        "alpha": 0.4,
        "linewidth": 1.2,
        }



fig = plt.figure(figsize=(7., 7.))
ax1 = fig.add_subplot(2, 2, 1)
ax2 = fig.add_subplot(2, 2, 2)
ax3 = fig.add_subplot(2, 2, 3)
ax4 = fig.add_subplot(2, 2, 4)

ax1.plot(r, v_r, **sim_props)
ax1.plot(r_s, v_s, **solution_props)
ax1.set_ylabel(r"Radial velocity $v_r$")
ax1.set_ylim(-0.2, 3.8)

ax2.plot(r, density, **sim_props)
ax2.plot(r_s, rho_s, **solution_props)
ax2.set_ylabel(r"Density $\rho$")
ax2.set_ylim(-0.2, 5.2)

ax3.plot(r, pressure, **sim_props)
ax3.plot(r_s, P_s, **solution_props)
ax3.set_ylabel(r"Pressure $p$")
ax3.set_ylim(-1., 12.5)

ax4.plot(r, energy, **sim_props)
ax4.plot(r_s, u_s, **solution_props)
ax4.set_ylabel(r"Internal Energy $u$")
ax4.set_ylim(-2., 22)

for ax in fig.axes:
    ax.tick_params(direction="in", top=True, right=True)
    ax.set_xlim(0, 1.3 * r_shock)
    ax.set_xlabel(r"Radius $r$")

plt.tight_layout()
fname = f"sedov_profile-{time:.4f}.png"
fig.savefig(fname)
print("Saved", fname)

