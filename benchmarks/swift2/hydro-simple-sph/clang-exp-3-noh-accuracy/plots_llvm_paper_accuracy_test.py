#!/usr/bin/env python

import numpy as np
from tkinter import Tcl
import glob
from matplotlib import gridspec
import matplotlib.pyplot as plt
from scipy import stats 
import meshio


"""
Plot radial profiles for the accuracy tests.

@param snapshot_dir should point to the directory with peano output files.

Nothing else needs modification.
"""


verbose=True

time = 0.1   # output time
mantissa_size = ["52", "23", "10", "X"]

for mantissa in mantissa_size:

    snapshot_dir = '/home/cristianbarrahinojosa/Overleaf/paper-llvm2022/experiments/18_sph_simulation/hamilton8/Noh_2D/accuracy_test/output_np_400_et_0.1_M'+mantissa+'/snapshots/et_0.1/'
    
    # We just care about the snapshots corresponding to t=0.1
    files = glob.glob(snapshot_dir + "particles-16*.vtu")

    # Sort and Select the correct set of snapshots
    files = Tcl().call('lsort', '-dict', files)
    files = files[11:]

    for i, file in enumerate(files):
        snapshot = file
        if(verbose):
            print('Reading snapshot: ' + snapshot )
            print("********************************************")

        mesh = meshio.read( snapshot )
        if(verbose):
            print('File contents info:')
            print(mesh.point_data_to_sets)
            print("********************************************")

        # Extract Fields
        pos_data = mesh.point_data["x"]
        x_data = pos_data[:,0] - 0.5
        y_data = pos_data[:,1] - 0.5
        r_data = np.sqrt(x_data**2 + y_data**2)
        v_data= mesh.point_data["v"]
        v_x_data= v_data[:,0] 
        v_y_data= v_data[:,1] 
        density_data = mesh.point_data["density"][:,0]
        pressure_data= mesh.point_data["pressure"][:,0]
        energy_data  = mesh.point_data["u"][:,0]
        if(i==0):
            pos = pos_data
            r = r_data
            density = density_data
            pressure= pressure_data
            energy  = energy_data
            v_x = v_x_data
            v_y = v_y_data
        else:
            pos = np.vstack((pos, pos_data))
            r =  np.append(r, r_data)
            density = np.append(density , density_data)
            pressure= np.append(pressure, pressure_data)
            energy  = np.append(energy  , energy_data)
            v_x = np.append(v_x, v_x_data)
            v_y = np.append(v_y, v_y_data)
            
    
    # Bin the data -----------------------
    rho = density       
    P = pressure
    v = - np.sqrt(v_x ** 2 + v_y ** 2)
    u = energy

    r_bin_edge = np.arange(0.0, 1.0, 0.02)
    r_bin = 0.5 * (r_bin_edge[1:] + r_bin_edge[:-1])
    rho_bin, _, _ = stats.binned_statistic(r, rho, statistic="mean", bins=r_bin_edge)
    v_bin, _, _ = stats.binned_statistic(r, v, statistic="mean", bins=r_bin_edge)
    P_bin, _, _ = stats.binned_statistic(r, P, statistic="mean", bins=r_bin_edge)
    u_bin, _, _ = stats.binned_statistic(r, u, statistic="mean", bins=r_bin_edge)
    rho2_bin, _, _ = stats.binned_statistic(r, rho ** 2, statistic="mean", bins=r_bin_edge)
    v2_bin, _, _ = stats.binned_statistic(r, v ** 2, statistic="mean", bins=r_bin_edge)
    P2_bin, _, _ = stats.binned_statistic(r, P ** 2, statistic="mean", bins=r_bin_edge)
    u2_bin, _, _ = stats.binned_statistic(r, u ** 2, statistic="mean", bins=r_bin_edge)
    rho_sigma_bin = np.sqrt(rho2_bin - rho_bin ** 2)
    v_sigma_bin = np.sqrt(v2_bin - v_bin ** 2)
    P_sigma_bin = np.sqrt(P2_bin - P_bin ** 2)
    u_sigma_bin = np.sqrt(u2_bin - u_bin ** 2)

    # Parameters
    rho0 = 1.0                     # Background Density
    v0 = 1                         # Magnitude of the initial velocity
    P0 = 6.6e-7                    # Background Pressure
    u0 = 1e-6                      # Background internal energy
    gas_gamma = 5.0/3.0            # Gas polytropic index

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
    rho_s[np.abs(x_s) < rs] = rho0 * ((gas_gamma + 1) / (gas_gamma - 1)) ** 2
    P_s[np.abs(x_s) < rs] = 0.5 * rho0 * v0 ** 2 * (gas_gamma + 1) ** 2 / (gas_gamma - 1)
    v_s[np.abs(x_s) < rs] = 0.0

    # Pre-shock values
    rho_s[np.abs(x_s) >= rs] = rho0 * (1 + v0 * time / np.abs(x_s[np.abs(x_s) >= rs]))
    P_s[np.abs(x_s) >= rs] = 0
    v_s[x_s >= rs] = -v0
    v_s[x_s <= -rs] = v0

    # Additional arrays
    u_s = P_s / (rho_s * (gas_gamma - 1.0))  # internal energy
    s_s = P_s / rho_s ** gas_gamma  # entropic function  
    
    
    # Save data for plots ------------------------------
    if mantissa == "52":
        ## Density
        r_52 = r
        rho_52 = rho
        r_bin_52 = r_bin
        rho_bin_52 = rho_bin
        rho_sigma_bin_52 = rho_sigma_bin

        ## Velocity
        v_52 = v
        v_bin_52 = v_bin
        v_sigma_bin_52 = v_sigma_bin
    
    elif mantissa == "23":
        r_23 = r
        rho_23 = rho
        r_bin_23 = r_bin
        rho_bin_23 = rho_bin
        rho_sigma_bin_23 = rho_sigma_bin

        v_23 = v
        v_bin_23 = v_bin
        v_sigma_bin_23 = v_sigma_bin
    
    elif mantissa == "10":
        r_10 = r
        rho_10 = rho
        r_bin_10 = r_bin
        rho_bin_10 = rho_bin
        rho_sigma_bin_10 = rho_sigma_bin

        v_10 = v
        v_bin_10 = v_bin
        v_sigma_bin_10 = v_sigma_bin

    elif mantissa == "X":
        r_MX = r
        rho_MX = rho
        r_bin_MX = r_bin
        rho_bin_MX = rho_bin
        rho_sigma_bin_MX = rho_sigma_bin

        v_MX = v
        v_bin_MX = v_bin
        v_sigma_bin_MX = v_sigma_bin



# Plots -------------------------------

# Note about density normalisation:
# 
# Notice that all but the mixed precision densities need to be normalised by
# 1/np.pi**2, which is the factor that was missing in the 2D W kernel. See
# commit be3185231c17d6a09b7f67d7c69c47a7e97e9b88
# 
# Notice this does not affect the velocity, which gives the correct solution
# before and after that bug fix.

# Plot Setup
font = {'size':11}
plt.rc('font', **font)
plt.rc('font', family='serif')
plt.rc('text', usetex=False)

fig = plt.figure(figsize=(10, 4)) 
gs = gridspec.GridSpec(1,2, wspace=0.3)

# Density profile --------------------------------
ax0 = plt.subplot(gs[0])
plt.plot(x_s, rho_s, "--", color="k", alpha=0.8, lw=1.2, label=r'analytical', zorder=-5)
# 52 bits
plt.plot(r_52, rho_52/np.pi**2, "s", color="b", ms=3.5, alpha=1.0, label=r'52 bits', zorder=-2)
#plt.errorbar(r_bin_52, rho_bin_52, yerr=rho_sigma_bin_52, fmt=".", ms=8.0, color="b", lw=1.2)
# 23 bits
plt.plot(r_23, rho_23/np.pi**2, "^", color="y", ms=2.0, alpha=0.5, label=r'23 bits', zorder=-1)
#plt.errorbar(r_bin_23, rho_bin_23, yerr=rho_sigma_bin_23, fmt=".", ms=8.0, color="b", lw=1.2)
# 10 bits
plt.plot(r_10, rho_10/np.pi**2, "v", color="g", ms=2.0, alpha=0.5, label=r'10 bits', zorder=-3)
#plt.errorbar(r_bin_10, rho_bin_10, yerr=rho_sigma_bin_10, fmt=".", ms=8.0, color="b", lw=1.2)
# Mixed precission
plt.plot(r_MX, rho_MX,  ".", color="r", ms=0.5, alpha=1.0, label=r'mixed', zorder=1)
#plt.errorbar(r_bin_10, rho_bin_10, yerr=rho_sigma_bin_10, fmt=".", ms=8.0, color="b", lw=1.2)

# Axis setup
plt.xlim(0, 0.055)
plt.ylim(1, 18)
plt.xlabel("${\\rm{Radius}}~r$", labelpad=0)
plt.ylabel("${\\rm{Density}}~\\rho$", labelpad=0)
plt.margins(0.05, 0.1)
plt.tick_params(direction='in', top=True, right=True)

#leg = plt.legend(loc ="upper right", fontsize=10)

# Velocity profile -------------------------------
ax1 = plt.subplot(gs[1])
plt.plot(x_s, v_s, "--", color="k", alpha=0.8, lw=1.2, label=r'analytical', zorder=-5)
# 52 bits
plt.plot(r_52, v_52, "s", color="b", ms=3.5, alpha=1.0, label=r'52 bits', zorder=-2)
#plt.errorbar(r_bin_52, v_bin_52, yerr=v_sigma_bin_52, fmt=".", ms=8.0, color="b", lw=1.2)
# 23 bits
plt.plot(r_23, v_23, "^", color="y", ms=2.5, alpha=0.5, label=r'23 bits', zorder=-1)
#plt.errorbar(r_bin_23, v_bin_23, yerr=v_sigma_bin_23, fmt=".", ms=8.0, color="b", lw=1.2)
# 10 bits
plt.plot(r_10, v_10, "v", color="g", ms=2.5, alpha=0.5, label=r'10 bits', zorder=-3)
#plt.errorbar(r_bin_10, v_bin_10, yerr=v_sigma_bin_10, fmt=".", ms=8.0, color="b", lw=1.2)
# Mixed precission
plt.plot(r_MX, v_MX, ".", color="r", ms=0.5, alpha=1.0, label=r'mixed', zorder=1,)
#plt.errorbar(r_bin_10, v_bin_10, yerr=v_sigma_bin_10, fmt=".", ms=8.0, color="b", lw=1.2)

# Axis setup
plt.xlim(0, 0.055)
plt.ylim(-1.1, 0.1)
plt.xlabel("${\\rm{Radius}}~r$", labelpad=0)
plt.ylabel("${\\rm{Radial velocity}}~v_r$", labelpad=0)
plt.margins(0.05, 0.1)
plt.tick_params(direction='in', top=True, right=True)

leg = plt.legend(loc ="lower left", fontsize=10)

plt.show()
fig.savefig( "profiles_accuracy_Noh_Problem_2D_Np_400_" + "t_" + str(time) + ".pdf", bbox_inches='tight')
plt.close()

