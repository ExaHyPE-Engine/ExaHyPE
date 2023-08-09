import meshio
import numpy as np
import glob, os
import sys

import matplotlib.pyplot as plt
from matplotlib import gridspec
from matplotlib.colors import LogNorm
from matplotlib import mathtext
import matplotlib
import math
from scipy import stats

"""
This script is run as
python3 snapshot_number time_step_sim

where:
@param snapshot_number: snapshot number
@param time_step_sim: size of the time-step used in the simulation
"""

# Plot Setup
font = {"size": 14}
plt.rc("font", **font)
plt.rc("font", family="serif")
plt.rc("text", usetex=False)

# Do not display figures
matplotlib.use("Agg")

"""
Import particle snapshot file
"""
snapshot_dir = ""
snapshot_number = int(sys.argv[1])
files = sorted(glob.glob(snapshot_dir + "*.vtu"), key=len)

# Simulation Parameters
# time_step_sim = 1e-4
time_step_sim = float(sys.argv[2])
snapshot_rate = 10
time = float(snapshot_number) * snapshot_rate * time_step_sim

snapshot = files[snapshot_number]
print("Reading snapshot: " + snapshot)
print("time of simulation = ", time)
print("********************************************")

mesh = meshio.read(snapshot)
print(mesh.point_data_to_sets)
print("********************************************")

# Extract Fields
pos = mesh.point_data["x"]
x = pos[:, 0] - 0.5
y = pos[:, 1] - 0.5
vel = mesh.point_data["v"]
vel_x = vel[:, 0]
vel_y = vel[:, 1]
r = np.sqrt(x**2 + y**2)
v_r = (x * vel_x + y * vel_y) / r
density = mesh.point_data["density"][:, 0]
pressure = mesh.point_data["pressure"][:, 0]
mass = mesh.point_data["mass"][:, 0]
energy = mesh.point_data["u"][:, 0]

print("Measure energy conservation:")
E_ini = 1.0
E_kin = mass[0] * np.sum(0.5 * (vel_x**2 + vel_y**2))
E_int = mass[0] * np.sum(energy)
E_tot = E_kin + E_int
print(
    "Initial E=", E_ini, "Total E= ", E_tot, "Kinetic E=", E_kin, "Internal E=", E_int
)
print("Relative Energy difference = ", (E_tot - E_ini) / E_ini)
print("********************************************")

# Filter by position
x_mask = np.greater_equal(x, 0)
y_min = -1e-2
y_max = 1e-2
y_mask = np.less_equal(y, y_max)
y_mask = y_mask & np.greater_equal(y, y_min)
pos_mask = x_mask & y_mask

x_line = x[pos_mask]
y_line = y[pos_mask]
density_x = density[pos_mask]
pressure_x = pressure[pos_mask]
energy_x = energy[pos_mask]
vel_x_line = vel_x[pos_mask]

print("** " + str(len(x_line)) + " ** particles found with this filter.")

"""
Analytical solution (from Swift)
Computes the analytical solution of the 2D Sedov blast wave. Taken from `swiftsim/examples/HydroTests/SedovBlast_2D`
"""

from scipy.special import gamma as Gamma


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


"""
Parameters to evaluate the analytical solution
"""
rho_0 = 1.0  # Background Density
P_0 = 1.0e-6  # Background Pressure
u_0 = 1.0
E_0 = E_ini  # Total Energy of the explosion
gas_gamma = 5.0 / 3.0  # Gas polytropic index

# The main properties of the solution
r_s, P_s, rho_s, v_s, r_shock, _, _, _, _ = sedov(time, E_0, rho_0, gas_gamma, 2000, 1)

# Append points for after the shock
r_s = np.insert(r_s, np.size(r_s), [r_shock, r_shock * 3])
rho_s = np.insert(rho_s, np.size(rho_s), [rho_0, rho_0])
P_s = np.insert(P_s, np.size(P_s), [P_0, P_0])
v_s = np.insert(v_s, np.size(v_s), [0, 0])

# Additional arrays
u_s = P_s / (rho_s * (gas_gamma - 1.0))  # internal energy
s_s = P_s / rho_s**gas_gamma  # entropic function

"""
Plot x-axis profiles
"""

fig = plt.figure(figsize=(14, 12))
gs = gridspec.GridSpec(2, 2, height_ratios=[1, 1], wspace=0.3)

ax0 = plt.subplot(gs[0])
plt.plot(r_s, rho_s, c="r")
plt.scatter(x_line, density_x, s=10, c="k")
plt.xlabel(r"$x$")
plt.ylabel(r"$\rho$")
plt.margins(0.05, 0.1)
plt.tick_params(direction="in", top=True, right=True)
plt.axhline(rho_0, c="gray", ls=":", lw=1.2, zorder=-49)
# plt.axhline( (5./3.+1.)/(5./3.-1.)*rho_0, c='r', ls=':', lw=1.2,zorder=-49 )
ax0.text(0.45, 0.92, r"$t=$" + str(time), fontsize=12, transform=ax0.transAxes)

ax1 = plt.subplot(gs[1])
plt.plot(r_s, P_s, c="r")
plt.scatter(x_line, pressure_x, s=10, c="k")
plt.xlabel(r"$x$")
plt.ylabel(r"$P$")
plt.margins(0.05, 0.1)
plt.tick_params(direction="in", top=True, right=True)

ax2 = plt.subplot(gs[2])
plt.plot(r_s, u_s, c="r")
plt.scatter(x_line, energy_x, s=10, c="k")
plt.xlabel(r"$x$")
plt.ylabel(r"$u$")
plt.margins(0.05, 0.1)
plt.tick_params(direction="in", top=True, right=True)

ax3 = plt.subplot(gs[3])
plt.plot(r_s, v_s, c="r")
plt.scatter(x_line, vel_x_line, s=10, c="k")
plt.xlabel(r"$x$")
plt.ylabel(r"$v_x$")
plt.margins(0.05, 0.1)
plt.tick_params(direction="in", top=True, right=True)

plt.show()
fig.savefig(
    "profile_Sedov_x_axis_" + "snap_" + str(snapshot_number) + ".pdf",
    bbox_inches="tight",
)
plt.close()

"""
Bin the data for radial profiles
"""

r_bin_edge = np.arange(0.0, 0.5, 0.005)
r_bin = 0.5 * (r_bin_edge[1:] + r_bin_edge[:-1])
rho_bin, _, _ = stats.binned_statistic(r, density, statistic="mean", bins=r_bin_edge)
v_bin, _, _ = stats.binned_statistic(r, v_r, statistic="mean", bins=r_bin_edge)
P_bin, _, _ = stats.binned_statistic(r, pressure, statistic="mean", bins=r_bin_edge)
u_bin, _, _ = stats.binned_statistic(r, energy, statistic="mean", bins=r_bin_edge)
rho2_bin, _, _ = stats.binned_statistic(
    r, density**2, statistic="mean", bins=r_bin_edge
)
v2_bin, _, _ = stats.binned_statistic(r, v_r**2, statistic="mean", bins=r_bin_edge)
P2_bin, _, _ = stats.binned_statistic(
    r, pressure**2, statistic="mean", bins=r_bin_edge
)
u2_bin, _, _ = stats.binned_statistic(r, energy**2, statistic="mean", bins=r_bin_edge)
rho_sigma_bin = np.sqrt(rho2_bin - rho_bin**2)
v_sigma_bin = np.sqrt(v2_bin - v_bin**2)
P_sigma_bin = np.sqrt(P2_bin - P_bin**2)
u_sigma_bin = np.sqrt(u2_bin - u_bin**2)

# Plot radial profiles
fig = plt.figure(figsize=(14, 12))
gs = gridspec.GridSpec(2, 2, height_ratios=[1, 1], wspace=0.3)

line_color = "k"
binned_color = "blue"
binned_marker_size = 8
scatter_props = dict(
    color="red",
    marker=".",
    ms=3,
    markeredgecolor="none",
    alpha=0.5,
    zorder=-1,
    rasterized=True,
    linestyle="none",
)
errorbar_props = dict(color=binned_color, ms=binned_marker_size, fmt=".", lw=1.5)

ax0 = plt.subplot(gs[0])
plt.plot(r_s, rho_s, c=line_color, lw=0.9)
plt.plot(r, density, **scatter_props)
plt.errorbar(r_bin, rho_bin, yerr=rho_sigma_bin, **errorbar_props)
plt.xlabel(r"Radial distance $r$")
plt.ylabel(r"Density $\rho$")
plt.margins(0.05, 0.1)
plt.tick_params(direction="in", top=True, right=True)
plt.axhline(rho_0, c="gray", ls=":", lw=1.2, zorder=-49)
plt.axhline(
    (5.0 / 3.0 + 1.0) / (5.0 / 3.0 - 1.0) * rho_0, c="r", ls=":", lw=1.2, zorder=-49
)
plt.xlim(0.1 * r_shock, 1.3 * r_shock)
plt.ylim(0, 1.1 * rho_s.max())

ax1 = plt.subplot(gs[1])
plt.plot(r_s, P_s, c=line_color, lw=0.9)
plt.plot(r, pressure, **scatter_props)
plt.errorbar(r_bin, P_bin, yerr=P_sigma_bin, **errorbar_props)
plt.xlabel(r"Radial distance $r$")
plt.ylabel(r"Pressure $P$")
plt.margins(0.05, 0.1)
plt.tick_params(direction="in", top=True, right=True)
plt.xlim(0.1 * r_shock, 1.3 * r_shock)
plt.ylim(0, 1.1 * P_s.max())

ax2 = plt.subplot(gs[2])
plt.plot(r_s, u_s, c=line_color, lw=0.9)
plt.plot(r, energy, **scatter_props)
plt.errorbar(r_bin, u_bin, yerr=u_sigma_bin, **errorbar_props)
plt.xlabel(r"Radial distance $r$")
plt.ylabel(r"Internal energy $u$")
plt.margins(0.05, 0.1)
plt.tick_params(direction="in", top=True, right=True)
plt.xlim(0.1 * r_shock, 1.3 * r_shock)
plt.ylim(0, 1.1 * u_s[5:].max())

ax3 = plt.subplot(gs[3])
plt.plot(r_s, v_s, c=line_color, lw=0.9)
plt.plot(r, v_r, **scatter_props)
plt.errorbar(r_bin, v_bin, yerr=v_sigma_bin, **errorbar_props)
plt.xlabel(r"Radial distance $r$")
plt.ylabel(r"Radial velocity $v_r$")
plt.margins(0.05, 0.1)
plt.tick_params(direction="in", top=True, right=True)
plt.xlim(0.1 * r_shock, 1.3 * r_shock)
plt.ylim(0, 1.1 * v_s.max())

plt.show()
fig.savefig(
    "profile_Sedov_spherical_average_" + "snap_" + str(snapshot_number) + ".pdf",
    bbox_inches="tight",
)
plt.close()
