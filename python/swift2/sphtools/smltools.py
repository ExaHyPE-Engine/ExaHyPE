#!/usr/bin/env python3
"""!
Tools related to the (computation of) the smoothing length.
"""

from .sph_kernels import (
    sph_kernel_W,
    sph_kernel_dWdr,
    sph_kernel_get_H,
    sph_kernel_get_H,
)
from numpy import ndarray, sqrt, cbrt, pi


def _neighbour_loop(r: ndarray, h: float, kernel: str, ndim: int):
    """!
    A neighbour interaction loop used to determine the smoothing lengths.

    Parameters
    ----------

    r: ndarray
        distances from the particle to the neighbours

    h: float
        current guess for the particle's smoothing length

    kernel: str
        which SPH kernel to use.

    ndim: int
        number of dimensions to work for.


    Returns
    -------

    wcount: float
        sum of particle's kernel weight at neighbour positions

    wcount_dh: float
        sum of particle's kernel derivative weight at neighbour positions
    """
    wcount = 0.0
    wcount_dh = 0.0
    H = sph_kernel_get_H(h, kernel, ndim)

    # Collect neighbour values
    for n in range(r.shape[0]):
        q = r[n] / H
        w = sph_kernel_W(r[n], h, kernel, ndim)
        dwdr = sph_kernel_dWdr(r[n], h, kernel, ndim)
        wcount += w
        wcount_dh -= ndim * w + q * dwdr

    # Add self contribution
    w = sph_kernel_W(0.0, h, kernel, ndim)
    dwdr = sph_kernel_dWdr(0.0, h, kernel, ndim)
    wcount += w
    wcount_dh -= dwdr

    return wcount, wcount_dh


def find_smoothing_length(
    xp: ndarray, xn: ndarray, kernel: str, eta: float, h_tolerance: float, ndim: int
):
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


    # This is for debugging purposes
    verbose = False

    # get distances between particles
    dx = xn - xp
    r2 = dx[:, 0] ** 2 + dx[:, 1] ** 2 + dx[:, 2] ** 2
    r = sqrt(r2)

    # Set up boundaries. We assume that the correct
    # number of neighbour (candidates) has been passed.
    h_max = r.max()
    h_min = h_max * 1e-12

    # Get initial guess for smoothing length
    dx_average = r.max() / xn.shape[0]
    nneigh_expect = number_of_neighbours_from_eta(eta, kernel, ndim)
    h = nneigh_expect * dx_average
    h = max(h_min, h)
    h = min(h_max, h)

    # safety check
    if int(nneigh_expect) > xn.shape[0]:
        print("WARNING: trying to find more neighbours than candidates provided")

    # bisection bounds
    right = h_max
    right_old = right
    left = h_min
    left_old = left

    iteration = 0
    n_target = eta**ndim
    if verbose:
        print("h_init", h)

    # This is a very high number of iterations. However, this function
    # is intended for confirmation checks that use untypically low
    # convergence criteria, so allow it to do a lot of iterations.
    while iteration < 500:
        iteration += 1

        h_dim = h**ndim
        h_dim_minus_one = h ** (ndim - 1.0)
        wcount, wcount_dh = _neighbour_loop(r, h, kernel, ndim)

        n_sum = wcount * h_dim
        f = n_sum - n_target
        f_prime = wcount_dh * h_dim + ndim * wcount * h_dim_minus_one

        # improve bisection bounds
        if n_sum < n_target:
            left_old = left
            left = max(left, h)
        elif n_sum > n_target:
            right_old = right
            right = min(right, h)

        if left > right:
            print("Error in bisection bounds. Resetting them.")
            left = left_old
            right = right_old

        if h >= h_max and f < 0:
            print("Particle with coordinates", xp, "reached h_max. Returning that.")
            return h_max

        if h <= h_min and f > 0:
            print("Particle with coordinates", xp, "reached h_min. Returning that.")
            return h_min

        h_new = h - f / f_prime

        # don't overstep boundaries, and don't change too fast
        h_new = min(2.0 * h, h_new)
        h_new = max(0.5 * h, h_new)
        h_new = max(h_new, left)
        h_new = min(h_new, right)

        if verbose:
            print("h_new=", h_new)
            print("wcount", wcount)
            print("wcount_dh", wcount_dh)
            print("f", f)
            print("f_prime", f_prime)

        diff = h - h_new

        # Are we done?
        if abs(diff < h_tolerance * h):
            if verbose:
                print("done h=", h_new)
            return h_new
            break

        # We're not done...
        # Case where we have been oscillating around the solution
        if (h_new == left and h == right) or (h == left and h_new == right):
            h_new = (0.5 * (left**ndim + right**ndim)) ** (-ndim)
        else:
            h = h_new

        h = min(h_max, h)
        h = max(h_min, h)

        if verbose:
            print("------------------------------------")

    print(
        "Particle with coordinates",
        xp,
        "didn't converge smoothing length after",
        iteration,
        "iterations",
    )

    return h


def eta_from_number_of_neighbours(N: float, kernel: str, ndim: int):
    """
    Compute the resolution eta from a given (approximate) number of neighbours.
    Note that the neighbour number does not need be an integer.
    See Price 2012, https://ui.adsabs.harvard.edu/abs/2012JCoPh.231..759P, eq. 12

    Parameters
    ----------

    N: float
        Number of neighbours

    kernel: str
        which kernel we're computing for.

    ndim: int
        how many dimensions we are working with.


    Returns
    -------

    eta: float
        the corresponding resolution eta
    """

    H_over_h = sph_kernel_get_H(1.0, kernel, ndim)

    if ndim == 1:
        return N / (2.0 * H_over_h)
    elif ndim == 2:
        return sqrt(N / pi) / H_over_h
    elif ndim == 3:
        return cbrt(N * 3.0 / 4.0 / pi) / H_over_h
    else:
        raise ValueError("Invalid number of dimensions", ndim)


def number_of_neighbours_from_eta(eta: float, kernel: str, ndim: int):
    """
    Compute the (approximate) number of neighbours from a given resolution eta.
    Note that the neighbour number does not need be an integer.
    See Price 2012, https://ui.adsabs.harvard.edu/abs/2012JCoPh.231..759P, eq. 12

    Parameters
    ----------

    eta: float
        resolution eta

    kernel: str
        which kernel we're computing for.

    ndim: int
        how many dimensions we are working with.


    Returns
    -------

    N: float
        the (approximate) number of neighbours
    """

    H_over_h = sph_kernel_get_H(1.0, kernel, ndim)

    if ndim == 1:
        return 2.0 * H_over_h * eta
    elif ndim == 2:
        return pi * (H_over_h * eta) ** 2
    elif ndim == 3:
        return 4.0 / 3.0 * pi * (H_over_h * eta) ** 3
    else:
        raise ValueError("Invalid number of dimensions", ndim)
