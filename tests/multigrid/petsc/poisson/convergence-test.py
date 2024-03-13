import subprocess
import os
import numpy as np
from datetime import datetime
from matplotlib import pyplot as plt
from matrixReader import populateRHS

os.system('export PYTHONPATH=../../../../python/')

meshes = (3, 9, 81, 243)

# Get the current date and time
current_date_time = datetime.now()
# Format the date and time as a string
formatted_date_time = current_date_time.strftime("%Y-%m-%d_%H-%M-%S")

folder_name = f"convergence-test-{formatted_date_time}"

input(f"This will run a convergence test for meshes {meshes}. The output will be saved in {folder_name}/. Press Enter to continue...")
#os.system(f"git clean -xdf")
os.system(f"mkdir {folder_name}")

for mesh in meshes:
    meshsize = 1.1 * 1.0/mesh
    os.system("python3 dg.py -m release --meshsize " + str(meshsize))
    os.system("./peano4petsc")

    mesh_folder_name = f"{folder_name}/{mesh}"
    os.system(f"mkdir {mesh_folder_name}")
    os.system(f"mv mat.xml {mesh_folder_name}/")
    os.system(f"mv rhs.xml {mesh_folder_name}/")
    os.system(f"mv sol.xml {mesh_folder_name}/")
    os.system(f"mv exactSol.txt {mesh_folder_name}/")

    os.system(f"git clean -xdf -e {folder_name}")

    #input("Press Enter to continue...")

# Compute error
Err = []
H = []
for mesh in meshes:
    cell_ndof = mesh ** 2 * 4
    mesh_folder_name = f"{folder_name}/{mesh}"
    sol = populateRHS(f"{mesh_folder_name}/sol.xml")[:cell_ndof]
    exact_sol = np.loadtxt(f"{mesh_folder_name}/exactSol.txt")
    assert(len(sol) == len(exact_sol))

    error = np.max(np.abs(sol - exact_sol))
    Err.append(error)
    H.append(1.0/mesh)
    print(f"for mesh {mesh}, error = {error}")

# Plot convergence data
X = np.asarray(H)
Y = np.asarray(Err)

plt.plot(
    X,
    Y,
    linewidth=2,
    color="blue",
    markersize=6,
    marker="o",
    label=f"$degree=1$",
)

ax = plt.gca()
plt.title(r"Interiror penatly method for 2D Poisson equation: " + '\n' + r"$-\Delta u = f(x,y), \quad f(x,y) = 8 \pi^2 \, \sin(2\pi x) \, \sin(2\pi y), \quad u |_{\partial \Omega} = 0$")
ax.set_xscale("log")
ax.set_yscale("log")
ax.set_xlabel("Grid spacing $h$")
ax.set_ylabel(r"Max error: $||u-u_{\mathrm{exact}}||_\inf$")
plt.plot(
    [1e-2, 1e-1],
    [2.0e-3, 2.0e-3 * 10**2.0],
    linewidth=2,
    linestyle="--",
    color="black",
    label=r"$\propto h^{-2.0}$",
)

plt.legend(loc="lower right")
plt.savefig(f"{folder_name}/error.png", bbox_inches="tight")
print(f"Error plot saved in {folder_name}/error.png")