#!/usr/bin/env python3

"""!
Plot the available kernels for visual inspection.
"""

import numpy as np
from matplotlib import pyplot as plt


from sph_kernels import *

fig = plt.figure()
ax1 = fig.add_subplot(2, 3, 1)
ax2 = fig.add_subplot(2, 3, 2)
ax3 = fig.add_subplot(2, 3, 3)
ax4 = fig.add_subplot(2, 3, 4)
ax5 = fig.add_subplot(2, 3, 5)
ax6 = fig.add_subplot(2, 3, 6)


ax1.set_title("1D")
ax2.set_title("2D")
ax3.set_title("3D")

r = np.linspace(0.0, 4.0, 400)

#  h = 1.0
h = 1.8522633292987498  # what swiftsim/theory/SPH/Kernels/kernels.py uses

for kernel in sph_kernel_list:
    if kernel not in ["wendland_C4", "wendland_C6"]:
        vals1d = [sph_kernel_W(ri, h, kernel, 1) for ri in r]
        ax1.plot(r, vals1d, label=kernel)

        deriv1d = [sph_kernel_dWdr(ri, h, kernel, 1) for ri in r]
        ax4.plot(r, deriv1d, label=kernel)

    vals2d = [sph_kernel_W(ri, h, kernel, 2) for ri in r]
    ax2.plot(r, vals2d, label=kernel)

    deriv2d = [sph_kernel_dWdr(ri, h, kernel, 2) for ri in r]
    ax5.plot(r, deriv2d, label=kernel)

    vals3d = [sph_kernel_W(ri, h, kernel, 3) for ri in r]
    ax3.plot(r, vals3d, label=kernel)

    deriv3d = [sph_kernel_dWdr(ri, h, kernel, 3) for ri in r]
    ax6.plot(r, deriv3d, label=kernel)


for ax in fig.axes:
    ax.legend()


plt.show()
