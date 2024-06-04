#!/usr/bin/env python3

# Computes the analytical solution of the Sod shock and plots the SPH answer

import sys
import matplotlib

matplotlib.use("Agg")
from matplotlib import pyplot as plt
import numpy as np
import h5py
from peano4.toolbox.particles.postprocessing.ParticleVTUReader import ParticleVTUReader
import swift2
import swift2.sphtools as sphtools


# Generates the analytical  solution for the Sod shock test case
# The script works for a given left (x<0) and right (x>0) state and computes the solution at a later time t.
# This follows the solution given in (Toro, 2009)

# Parameters
gas_gamma = 5.0 / 3.0  # Polytropic index
rho_L = 1.0  # Density left state
rho_R = 0.125  # Density right state
v_L = 0.0  # Velocity left state
v_R = 0.0  # Velocity right state
P_L = 1.0  # Pressure left state
P_R = 0.1  # Pressure right state

if len(sys.argv) < 2:
    time = 0.1
else:
    time = float(sys.argv[1])


pvdfile = "particles.pvd"
reader = ParticleVTUReader(pvdfile=pvdfile, snapshot_time=time, verbose=False)
partData = reader.load()
# update the actual time of the snapshot
time = reader.get_snapshot_time()

# show me what particle fields are stored
#  partData.show_attribute_list()

x = partData.x[:, 0]
#  y = partData.x[:,1]
v = partData.v[:, 0]
u = partData.u
#  S = partData.S # doesn't exist yet?
P = partData.pressure
rho = partData.density


N = 1000  # Number of points
x_min = 0.0
x_max = 2.0

x += x_min

# Prepare reference solution
solver = sphtools.RiemannSolver(gas_gamma)

delta_x = (x_max - x_min) / N
center = (x_max + x_min) / 2
x_s = np.arange(x_min - center, x_max - center, delta_x)
rho_s, v_s, P_s, _ = solver.solve(rho_L, v_L, P_L, rho_R, v_R, P_R, x_s / time)
x_s += center

# Additional arrays
u_s = P_s / (rho_s * (gas_gamma - 1.0))  # internal energy
s_s = P_s / rho_s**gas_gamma  # entropic function

# Shock position (since we want to overplot it in the viscosity/diffusion plot
c_R = np.sqrt(gas_gamma * P_R / rho_R)
x_shock = (c_R + v_R) * time

# Plot the interesting quantities
plt.figure(figsize=(7, 7 / 1.6))

rows = 2
cols = 2

# Velocity profile --------------------------------
plt.subplot(rows, cols, 1)
plt.plot(x, v, ".", color="r", ms=4.0)
plt.plot(x_s, v_s, "--", color="k", alpha=0.8, lw=1.2)
plt.xlabel("${\\rm{Position}}~x$", labelpad=0)
plt.ylabel("${\\rm{Velocity}}~v_x$", labelpad=0)
plt.xlim(0.5, 1.5)
plt.ylim(-0.1, 0.95)

# Density profile --------------------------------
plt.subplot(rows, cols, 2)
plt.plot(x, rho, ".", color="r", ms=4.0)
plt.plot(x_s, rho_s, "--", color="k", alpha=0.8, lw=1.2)
plt.ylabel("${\\rm{Density}}~\\rho$", labelpad=0)
plt.ylim(0.05, 1.1)

plt.xlabel("${\\rm{Position}}~x$", labelpad=0)
plt.xlim(0.5, 1.5)

# Pressure profile --------------------------------
plt.subplot(rows, cols, 3)
plt.plot(x, P, ".", color="r", ms=4.0)
plt.plot(x_s, P_s, "--", color="k", alpha=0.8, lw=1.2)
plt.xlabel("${\\rm{Position}}~x$", labelpad=0)
plt.ylabel("${\\rm{Pressure}}~P$", labelpad=0)
plt.xlim(0.5, 1.5)
plt.ylim(0.01, 1.1)

# Internal energy profile -------------------------
plt.subplot(rows, cols, 4)
plt.plot(x, u, ".", color="r", ms=4.0)
plt.plot(x_s, u_s, "--", color="k", alpha=0.8, lw=1.2)
plt.xlabel("${\\rm{Position}}~x$", labelpad=0)
plt.ylabel("${\\rm{Internal~Energy}}~u$", labelpad=0)
plt.xlim(0.5, 1.5)
plt.ylim(0.8, 2.2)

plt.tight_layout()

plt.savefig("SodShock{0:.5f}.png".format(time), dpi=200)
