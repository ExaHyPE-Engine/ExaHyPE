#!/usr/bin/env python3
"""!
Tool related to the (computation of) the smoothing length.
"""

from .sph_kernels import sph_kernel_W, sph_kernel_dWdr, sph_kernel_get_H
from numpy import ndarray, sqrt

def neighbour_loop(r: ndarray, h:float, kernel: str, ndim:int):
    """!
    TODO: docu
    """
    wcount = 0.
    wcount_dh = 0.
    H = sph_kernel_get_H(h, kernel)

    # Collect neighbour values
    for n in range(r.shape[0]):
        q = r[n] / H
        w =  sph_kernel_W(r[n], h, kernel, ndim)
        dwdr =  sph_kernel_dWdr(r[n], h, kernel, ndim)
        wcount += w
        wcount_dh -= (ndim * w + q * dwdr)

    # Add self contribution
    w = sph_kernel_W(0., h, kernel, ndim)
    dwdr =  sph_kernel_dWdr(0., h, kernel, ndim)
    wcount += w
    wcount_dh -= dwdr

    return wcount, wcount_dh





def find_smoothing_length(xp: ndarray, xn: ndarray, kernel: str, eta: float = 1.2348, h_tolerance: float = 1e-4, ndim: int=3):
    """!
    Determine the smoothing length of a particle with coordinates xp

    TODO: finish documentation

    Parameters
    ----------


    Returns
    -------

    h: float
        the smoothing length
    """


    # get distances between particles
    dx = xn - xp
    r2 = dx[:,0]**2 + dx[:,1]**2 + dx[:,2]**2
    r = sqrt(r2)

    # Initial guess
    h = 0.5 * r.max()

    iteration = 0
    n_target = eta ** ndim

    while iteration < 100:
        iteration += 1

        h_dim = h**ndim
        h_dim_minus_one = h**(ndim-1.)
        wcount, wcount_dh = neighbour_loop(r, h, kernel, ndim)

        n_sum = wcount * h_dim
        f = n_sum - n_target
        f_prime = wcount_dh * h_dim + ndim * wcount * h_dim_minus_one

        h_new = h - f / f_prime

        diff = 1. - h_new / h

        if abs(diff < h_tolerance):
            return h_new
            break
        else:
            h = h_new

    print("Particle with coordinates", xp, "didn't converge smoothing length after", iteration, "iterations")

    return h




