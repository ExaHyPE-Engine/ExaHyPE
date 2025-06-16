#!/usr/bin/env python3
###############################################################################
# This file is part of SWIFT.
# Copyright (c) 2016 Matthieu Schaller (schaller@strw.leidenuniv.nl)
# Modified to fit the purposes of the Peano4 Noh 3D benchmark.
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

# ----------------------------------------------------------------------------
# Computes the analytical solution of the Noh problem and plots the SPH answer
#
# Usage: python3 plotSolution.py <snapshot-time>
#
# The script will look for the snapshot output with simulation time closest to
# the provided <snapshot-time> in `particles.pvd` and create a plot of that
# time.
# ----------------------------------------------------------------------------


# Parameters
gas_gamma = 5.0 / 3.0  # Polytropic index
rho0 = 1.0  # Background density
P0 = 1.0e-6  # Background pressure
v0 = 1

plot_errorbars = False

import matplotlib

matplotlib.use("Agg")
import matplotlib.pyplot as plt
from scipy import stats
import numpy as np
import sys

from peano4.toolbox.particles.postprocessing.ParticleVTUReader import ParticleVTUReader

if len(sys.argv) < 2:
    time = 0.6
else:
    time = float(sys.argv[1])

pvdfile = "particles.pvd"
reader = ParticleVTUReader(pvdfile=pvdfile, snapshot_time=time)
partData = reader.load()
time = reader.get_snapshot_time()

# show me what particle fields are stored
#  partData.show_attribute_list()

x = partData.x[:, 0]
y = partData.x[:, 1]
z = partData.x[:, 2]
vx = partData.v[:, 0]
vy = partData.v[:, 1]
vz = partData.v[:, 2]
u = partData.u
#  S = partData.S # doesn't exist yet?
P = partData.pressure
rho = partData.density


r = np.sqrt((x - 1.0) ** 2 + (y - 1.0) ** 2 + (z - 1.0) ** 2)
v = -np.sqrt(vx**2 + vy**2 + vz**2)

# Bin the data
r_bin_edge = np.arange(0.0, 1.0, 0.02)
r_bin = 0.5 * (r_bin_edge[1:] + r_bin_edge[:-1])
rho_bin, _, _ = stats.binned_statistic(r, rho, statistic="mean", bins=r_bin_edge)
v_bin, _, _ = stats.binned_statistic(r, v, statistic="mean", bins=r_bin_edge)
P_bin, _, _ = stats.binned_statistic(r, P, statistic="mean", bins=r_bin_edge)
#  S_bin, _, _ = stats.binned_statistic(r, S, statistic="mean", bins=r_bin_edge)
u_bin, _, _ = stats.binned_statistic(r, u, statistic="mean", bins=r_bin_edge)
rho2_bin, _, _ = stats.binned_statistic(r, rho**2, statistic="mean", bins=r_bin_edge)
v2_bin, _, _ = stats.binned_statistic(r, v**2, statistic="mean", bins=r_bin_edge)
P2_bin, _, _ = stats.binned_statistic(r, P**2, statistic="mean", bins=r_bin_edge)
#  S2_bin, _, _ = stats.binned_statistic(r, S ** 2, statistic="mean", bins=r_bin_edge)
u2_bin, _, _ = stats.binned_statistic(r, u**2, statistic="mean", bins=r_bin_edge)
rho_sigma_bin = np.sqrt(rho2_bin - rho_bin**2)
v_sigma_bin = np.sqrt(v2_bin - v_bin**2)
P_sigma_bin = np.sqrt(P2_bin - P_bin**2)
#  S_sigma_bin = np.sqrt(S2_bin - S_bin ** 2)
u_sigma_bin = np.sqrt(u2_bin - u_bin**2)

# Analytic solution
N = 1000  # Number of points

x_s = np.arange(0, 2.0, 2.0 / N) - 1.0
rho_s = np.ones(N) * rho0
P_s = np.ones(N) * rho0
v_s = np.ones(N) * v0

# Shock position
u0 = rho0 * P0 * (gas_gamma - 1)
us = 0.5 * (gas_gamma - 1) * v0
rs = us * time

# Post-shock values
rho_s[np.abs(x_s) < rs] = rho0 * ((gas_gamma + 1) / (gas_gamma - 1)) ** 3
P_s[np.abs(x_s) < rs] = (
    0.5 * rho0 * v0**2 * (gas_gamma + 1) ** 3 / (gas_gamma - 1) ** 2
)
v_s[np.abs(x_s) < rs] = 0.0

# Pre-shock values
rho_s[np.abs(x_s) >= rs] = rho0 * (1 + v0 * time / np.abs(x_s[np.abs(x_s) >= rs])) ** 2
P_s[np.abs(x_s) >= rs] = 0
v_s[x_s >= rs] = -v0
v_s[x_s <= -rs] = v0

# Additional arrays
u_s = P_s / (rho_s * (gas_gamma - 1.0))  # internal energy
s_s = P_s / rho_s**gas_gamma  # entropic function

# Plot the interesting quantities
fig = plt.figure(figsize=(7, 7 / 1.6))
fig.suptitle(f"Noh 3D, t={time:.3f}")

line_color = "C1"
binned_color = "C2"
binned_marker_size = 4

scatter_props = dict(
    marker=".",
    ms=1,
    markeredgecolor="none",
    alpha=0.4,
    zorder=-1,
    rasterized=True,
    linestyle="none",
)

errorbar_props = dict(color=binned_color, ms=binned_marker_size, fmt=".", lw=1.2)

rows = 2
cols = 2

# Velocity profile --------------------------------
plt.subplot(rows, cols, 1)
plt.plot(r, v, **scatter_props)
plt.plot(x_s, v_s, "--", color=line_color, alpha=0.8, lw=1.2)
if plot_errorbars:
    plt.errorbar(r_bin, v_bin, yerr=v_sigma_bin, **errorbar_props)
plt.xlabel("${\\rm{Radius}}~r$", labelpad=0)
plt.ylabel("${\\rm{Velocity}}~v_r$", labelpad=-4)
plt.xlim(0, 0.5)
#  plt.ylim(-1.2, 0.4)

# Density profile --------------------------------
plt.subplot(rows, cols, 2)
plt.plot(r, rho, **scatter_props)
plt.plot(x_s, rho_s, "--", color=line_color, alpha=0.8, lw=1.2)
if plot_errorbars:
    plt.errorbar(r_bin, rho_bin, yerr=rho_sigma_bin, **errorbar_props)
plt.xlabel("${\\rm{Radius}}~r$", labelpad=0)
plt.ylabel("${\\rm{Density}}~\\rho$", labelpad=0)
plt.xlim(0, 0.5)
#  plt.ylim(0.95, 19)

# Pressure profile --------------------------------
plt.subplot(rows, cols, 3)
plt.plot(r, P, **scatter_props)
plt.plot(x_s, P_s, "--", color=line_color, alpha=0.8, lw=1.2)
if plot_errorbars:
    plt.errorbar(r_bin, P_bin, yerr=P_sigma_bin, **errorbar_props)
plt.xlabel("${\\rm{Radius}}~r$", labelpad=0)
plt.ylabel("${\\rm{Pressure}}~P$", labelpad=0)
plt.xlim(0, 0.5)
#  plt.ylim(-0.5, 11)

# Internal energy profile -------------------------
plt.subplot(rows, cols, 4)
plt.plot(r, u, **scatter_props)
plt.plot(x_s, u_s, "--", color=line_color, alpha=0.8, lw=1.2)
if plot_errorbars:
    plt.errorbar(r_bin, u_bin, yerr=u_sigma_bin, **errorbar_props)
plt.xlabel("${\\rm{Radius}}~r$", labelpad=0)
plt.ylabel("${\\rm{Internal~Energy}}~u$", labelpad=0)
plt.xlim(0, 0.5)
#  plt.ylim(-0.05, 0.8)


plt.tight_layout()

fname = f"NohProfile{time:.4f}.png"
plt.savefig(fname, dpi=200)
print("Saved", fname)
