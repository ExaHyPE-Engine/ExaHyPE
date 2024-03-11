#!/usr/bin/env python3
"""!
Tools related to the (computation of) the smoothing length.
"""

from .sph_kernels import (
    sph_kernel_W,
    sph_kernel_dWdr,
    sph_kernel_get_H,
    sph_kernel_root,
)
from numpy import ndarray, sqrt, cbrt, pi, sum


def _neighbour_loop_vectorized(r: ndarray, h: float, kernel: str, ndim: int):
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

    q = r / H
    w_all = sph_kernel_W(r, h, kernel, ndim)
    dwdr_all = sph_kernel_dWdr(r, h, kernel, ndim)
    wcount = w_all.sum()
    dwdh_term = sum(ndim * w_all / h + q * dwdr_all)
    wcount_dh = -dwdh_term

    # Add self contribution
    w = sph_kernel_W(0.0, h, kernel, ndim)
    dwdr = sph_kernel_dWdr(0.0, h, kernel, ndim)
    wcount += w
    wcount_dh -= ndim * w / h

    return wcount, wcount_dh


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
        dwdh_term = ndim * w / h + q * dwdr
        wcount_dh -= dwdh_term

    # Add self contribution
    w = sph_kernel_W(0.0, h, kernel, ndim)
    dwdr = sph_kernel_dWdr(0.0, h, kernel, ndim)
    wcount += w
    wcount_dh -= ndim * w / h

    return wcount, wcount_dh


def find_smoothing_length(
    xp: ndarray,
    xn: ndarray,
    kernel: str,
    eta: float,
    h_tolerance: float,
    ndim: int,
    verbose: bool = False,
):
    """!
    Determine the smoothing length of a particle with coordinates xp
    and neighbours with positions xn. This function assumes that the
    an appropriate amount of neighbours is passed on via the `xn` array
    of their positions, meaning that max{|xn - xp|} <= gamma * h.

    Parameters
    ----------

    xp: ndarray
        Coordiate of particle to determine smoothing length for

    xn: ndarray
        Array of coordinates of neighbour particles

    kernel: str
        Which kernel to use. Choices are given in sph_kernels.py:sph_kernel_list

    eta: float
        resolution eta to use, "smoothing scale". This determines the number of neighbours.

    h_tolerance: float
        tolerance to adapt to consider smoothing length converged

    ndim: int
        number of dimensions to work with

    verbose: bool
        whether to be verbose about what's going on


    Returns
    -------

    h: float
        the smoothing length
    """

    # get distances between particles
    dx = xn - xp
    r2 = sum(dx[:, :ndim] ** 2, axis=1)
    r = sqrt(r2)

    kernel_root = sph_kernel_root(kernel, ndim)

    # Set up boundaries. We assume that the correct
    # number of neighbour (candidates) has been passed.
    H_over_h = sph_kernel_get_H(1.0, kernel, ndim)
    h_max = r.max() / H_over_h
    h_min = h_max * 1e-9

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
        print("Startup: h_init", h)
        print("Startup: h_min", h_min)
        print("Startup: h_max", h_max)
        print("Startup: Neighbours: ", xn.shape[0])

    # This is a very high number of iterations. However, this function
    # is intended for confirmation checks that use untypically low
    # convergence criteria, so allow it to do a lot of iterations.
    while iteration < 10000:
        iteration += 1

        h_dim = h**ndim
        h_dim_minus_one = h ** (ndim - 1.0)
        wcount, wcount_dh = _neighbour_loop_vectorized(r, h, kernel, ndim)

        if abs(wcount - kernel_root) < 1e-5 * kernel_root:
            if verbose:
                print(f"Caught no neighbours case, h={h}")
            h_new = 2.0 * h

        else:
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
                # this might be a cause to crash, actually
                print("Error in bisection bounds. Resetting them.")
                left = left_old
                right = right_old

            if h >= h_max and f < 0:
                raise RuntimeError(
                    f"""
Particle with coordinates {xp} reached h_max without converging.
Increase the number of neighbours you pass to this script."""
                )
                return h_max

            if h <= h_min and f > 0:
                print(
                    f"WARNING: Particle with coordinates {xp} reached h_min. Returning that."
                )
                return h_min

            h_new = h - f / f_prime

            # don't overstep boundaries, and don't change too fast
            if verbose:
                print(
                    "boundaries check:",
                    h_new > 2.0 * h,
                    h_new < 0.5 * h,
                    h_new < left,
                    h_new == left,
                    h_new > right,
                    h_new == right,
                )
            h_new = min(2.0 * h, h_new)
            h_new = max(0.5 * h, h_new)
            h_new = max(h_new, left)
            h_new = min(h_new, right)

        diff = h - h_new

        if verbose:
            print("h =", h)
            print("h_new=", h_new)
            print("wcount", wcount)
            print("wcount_dh", wcount_dh)
            print("f", f)
            print("f_prime", f_prime)
            print("h - h_new", diff)

        diff = h - h_new

        # Are we done?
        if abs(diff) < h_tolerance * h:
            if verbose:
                print("done h=", h_new)
            return h_new

        # We're not done...
        # Case where we have been oscillating around the solution
        if (h_new == left and h == right) or (h == left and h_new == right):
            if verbose:
                if h_new == left:
                    print("h_new == left")
                if h == right:
                    print("h == right")
                if h_new == right:
                    print("h_new == right")
                if h == left:
                    print("h == left")
            h_new = (0.5 * (left**ndim + right**ndim)) ** (1.0 / ndim)

        h = h_new

        h = min(h_max, h)
        h = max(h_min, h)

        if verbose:
            print("------------------------------------")

    print(
        f"Particle with coordinates {xp} didn't converge smoothing length after {iteration} iterations"
    )

    quit()
    return h


def eta_from_number_of_neighbours(N: float, kernel: str, ndim: int):
    """!
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
    """!
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
