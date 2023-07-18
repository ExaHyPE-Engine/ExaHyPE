#!/usr/bin/env python3

"""!
SPH Kernel related constants, coefficients, and functions.

All constants and kernel coefficients are taken from table 1 of
Dehnen & Aly, MNRAS, 425, pp. 1062-1082 (2012).
"""

import numpy as np
from typing import Union

# List of available kernel names. Used to check whether user choice is valid.
sph_kernel_list = [
            "cubic_spline",
            "quartic_spline",
            "quintic_spline",
            "wendland_C2",
            "wendland_C4",
            "wendland_C6",
        ]

# Translation from kernel name to kernel macro
sph_kernel_macro_name = {
        "cubic_spline": "CUBIC_SPLINE_KERNEL",
        "quartic_spline": "QUARTIC_SPLINE_KERNEL",
        "quintic_spline": "QUINTIC_SPLINE_KERNEL",
        "wendland_C2": "WENDLAND_C2_KERNEL",
        "wendland_C4": "WENDLAND_C4_KERNEL",
        "wendland_C6": "WENDLAND_C6_KERNEL",
        }


# list for 1D, 2D, 3D values of H/h of kernel dicts
sph_kernel_H_over_h = [{}, {}, {}]

# list for 1D, 2D, 3D values of kernel normalization coefficients of kernel dicts
sph_kernel_normalization = [{}, {}, {}]

# dict containing functions that evaluate the kernel
sph_kernel_eval_function = {}

# dict containing functions that evaluate the kernel derivatives
sph_kernel_deval_function = {}

# Kernel Gammas (ratio of compact support radius H to smoothing length h)
# 3D values
sph_kernel_H_over_h[2]["cubic_spline"] =1.825742
sph_kernel_H_over_h[2]["quartic_spline"] = 2.018932
sph_kernel_H_over_h[2]["quintic_spline"] =2.195775
sph_kernel_H_over_h[2]["wendland_C2"] = 1.936492
sph_kernel_H_over_h[2]["wendland_C4"] =2.207940
sph_kernel_H_over_h[2]["wendland_C6"] =2.449490

# 2D values
sph_kernel_H_over_h[1]["cubic_spline"] = 1.778002
sph_kernel_H_over_h[1]["quartic_spline"] = 1.977173
sph_kernel_H_over_h[1]["quintic_spline"] = 2.158131
sph_kernel_H_over_h[1]["wendland_C2"] = 1.897367
sph_kernel_H_over_h[1]["wendland_C4"] = 2.171239
sph_kernel_H_over_h[1]["wendland_C6"] = 2.415230

# 1D values
sph_kernel_H_over_h[0]["cubic_spline"] = 1.732051
sph_kernel_H_over_h[0]["quartic_spline"] =1.936492
sph_kernel_H_over_h[0]["quintic_spline"] =2.121321
sph_kernel_H_over_h[0]["wendland_C2"] =1.620185
sph_kernel_H_over_h[0]["wendland_C4"] = None # not defined in 1D
sph_kernel_H_over_h[0]["wendland_C6"] = None # not defined in 1D


# Kernel Normalizations
# 3D values
sph_kernel_normalization[2]["cubic_spline"] = np.pi * 16.
sph_kernel_normalization[2]["quartic_spline"] =15625. * np.pi / 512.
sph_kernel_normalization[2]["quintic_spline"] =2187. * np.pi / 40.
sph_kernel_normalization[2]["wendland_C2"] =21. * np.pi / 2.
sph_kernel_normalization[2]["wendland_C4"] =495. * np.pi / 32.
sph_kernel_normalization[2]["wendland_C6"] =1365. * np.pi / 64.

# 2D values
sph_kernel_normalization[1]["cubic_spline"] = 80. * np.pi / 7.
sph_kernel_normalization[1]["quartic_spline"] = 46875. * np.pi / 2398.
sph_kernel_normalization[1]["quintic_spline"] =15309. * np.pi / 478.
sph_kernel_normalization[1]["wendland_C2"] = 7. * np.pi
sph_kernel_normalization[1]["wendland_C4"] =9. * np.pi
sph_kernel_normalization[1]["wendland_C6"] =78. * np.pi / 7.

# 1D values
sph_kernel_normalization[0]["cubic_spline"] = 8. / 3.
sph_kernel_normalization[0]["quartic_spline"] =3125. / 768.
sph_kernel_normalization[0]["quintic_spline"] = 243. / 40.
sph_kernel_normalization[0]["wendland_C2"] = 5. / 4.
sph_kernel_normalization[0]["wendland_C4"] = None # not defined in 1D
sph_kernel_normalization[0]["wendland_C6"] = None # not defined in 1D

def _sph_kernel_eval_cubic_spline(q: float):
    """!
    Evaluates the cubic spline kernel
    """
    if q < 0.5:
        res = 3 * q ** 2 * (q - 1) + 0.5
    elif q < 1.:
        #  res =  q * (q * (3 - q) - 3) + 1
        res = -(q ** 3) + 3 * q ** 2 - 3 * q + 1
    else:
        res = 0.
    return res

def _sph_kernel_eval_quartic_spline(q: float):
    """!
    Evaluates the quartic spline kernel
    """
    if q < 0.2:
        res = 6 * q ** 4 - 2.4 * q ** 2 + 46 / 125
    elif q < 0.6:
        res = -4 * q ** 4 + 8 * q ** 3 - 4.8 * q ** 2 + 8 / 25 * q + 44.0 / 125
    elif q < 1:
        res = q ** 4 - 4 * q ** 3 + 6 * q ** 2 - 4 * q + 1
    else:
        res = 0.
    return res

def _sph_kernel_eval_quintic_spline(q: float):
    """!
    Evaluates the quintic spline kernel
    """
    if q < 0.333333333333:
        res = 10 * (q ** 4 * (1 - q) - 0.2222222222 * q ** 2) + 0.2682926829268
    elif q < 0.666666666666:
        qsq = q ** 2
        q4 = qsq ** 2
        res = (
            5
            * (
                q4 * (q - 3)
                + qsq * (3.333333333333333 * q - 1.5555555555555)
                + 0.18518518518518517 * q
            )
            + 0.20987654320987653
        )
    elif q < 1.:
        qsq = q ** 2
        res = qsq * (qsq * (5 - q) + 10 * (1 - q)) - 5 * q + 1
    else:
        res = 0.
    return res

def _sph_kernel_eval_wendland_C2(q: float):
    """!
    Evaluates the Wendland C2 kernel
    """
    if q < 1.:
        qsq = q ** 2
        res = qsq * (qsq * (4. * q - 15.) + 10. * (2. * q - 1.)) + 1.
    else:
        res = 0.
    return res

def _sph_kernel_eval_wendland_C4(q: float):
    """!
    Evaluates the Wendland C4 kernel
    """
    if q < 1.:
        qsq = q ** 2
        q4 = qsq ** 2

        res = 11.666666666666666 * q4 * q4 - 64. * q4 * qsq * q + 140. * qsq * q4 - 149.3333333333333 * q4 * q + 70. * q4 - 9.33333333333333 * qsq + 1.
    else:
        res = 0.
    return res

def _sph_kernel_eval_wendland_C6(q: float):
    """!
    Evaluates the Wendland C6 kernel
    """
    if q < 1.:
        res = 32. * q ** 11 - 231. * q ** 10 + 704. * q ** 9 - 1155. * q ** 8 + 1056. * q ** 7 - 462. * q ** 6 + 66. * q ** 4 - 11. * q ** 2 + 1.
    else:
        res = 0.
    return res

def _sph_kernel_deval_cubic_spline(q:float):
    """!
    Evaluates the cubic spline kernel derivative
    """
    if q < 0.5:
        res = 9 * q ** 2 - 6 * q
    elif q < 1:
        res = 6 * q - 3 * q ** 2 - 3
    else:
        res = 0.

    return res

def _sph_kernel_deval_quartic_spline(q:float):
    """!
    Evaluates the quartic spline kernel derivative
    """
    if q < 0.2:
        res = 24 * q ** 3 - 4.8 * q
    elif q < 0.6:
        res = -16 * q ** 3 + 24 * q ** 2 - 9.6 * q + 8 / 25
    elif q < 1:
        res = 4 * q ** 3 - 12 * q ** 2 + 12 * q - 4
    else:
        res = 0.
    return res

def _sph_kernel_deval_quintic_spline(q:float):
    """!
    Evaluates the quintic spline kernel derivative
    """
    if q < 0.333333333333:
        res = 40 * q ** 3 - 50 * q ** 4 - 4.44444444444 * q
    elif q < 0.666666666666:
        res = 25 * q ** 4 - 60 * q ** 3 + 50 * q ** 2 - 15.555555555555 * q + 0.9259259259259259
    elif q < 1:
        res = 20 * q ** 3 - 5 * q ** 4 + 20 * q - 30 * q ** 2 - 5
    else:
        res = 0.
    return res

def _sph_kernel_deval_wendland_C2(q:float):
    """!
    Evaluates the Wendland C2 kernel derivative
    """
    if q < 1.:
        res = 20. * q ** 4 - 60. * q ** 3 + 60. * q ** 2 - 20. * q
    else:
        res = 0.
    return res

def _sph_kernel_deval_wendland_C4(q:float):
    """!
    Evaluates the Wendland C4 kernel derivative
    """
    if q < 1:
        res = 93.3333333333 * q ** 7 - 448 * q ** 6 + 840 * q ** 5 - 746.6666666666 * q ** 4 + 280 * q ** 3 - 18.66666666666666 * q
    else:
        res = 0.
    return res

def _sph_kernel_deval_wendland_C6(q:float):
    """!
    Evaluates the Wendland C6 kernel derivative
    """
    if q < 1.:
        res = 352 * q ** 10 - 2310 * q ** 9 + 6336 * q ** 8 - 9240 * q ** 7 + 7392 * q ** 6 - 2772 * q ** 5 + 264 * q ** 3 - 22 * q
    else:
        res =  0.
    return res


sph_kernel_eval_function["cubic_spline"] = _sph_kernel_eval_cubic_spline
sph_kernel_eval_function["quartic_spline"] = _sph_kernel_eval_quartic_spline
sph_kernel_eval_function["quintic_spline"] = _sph_kernel_eval_quintic_spline
sph_kernel_eval_function["wendland_C2"] = _sph_kernel_eval_wendland_C2
sph_kernel_eval_function["wendland_C4"] = _sph_kernel_eval_wendland_C4
sph_kernel_eval_function["wendland_C6"] = _sph_kernel_eval_wendland_C6

sph_kernel_deval_function["cubic_spline"] = _sph_kernel_deval_cubic_spline
sph_kernel_deval_function["quartic_spline"] = _sph_kernel_deval_quartic_spline
sph_kernel_deval_function["quintic_spline"] = _sph_kernel_deval_quintic_spline
sph_kernel_deval_function["wendland_C2"] = _sph_kernel_deval_wendland_C2
sph_kernel_deval_function["wendland_C4"] = _sph_kernel_deval_wendland_C4
sph_kernel_deval_function["wendland_C6"] = _sph_kernel_deval_wendland_C6


def sph_kernel_W(r: float, h: float, kernel: str = "cubic_spline", ndim: int = 3):
    """
    Evaluates various kernels.
    The kernels are scaled such that W(q > 1) = 0.
    Currently implemented:
        cubic_spline,
        quintic_spline,
        wendland_C2,
        wendland_C4,
        wendland_C6


    Parameters
    ----------

    r: float
        particle distance

    h: float
        smoothing length

    kernel: str
        which kernel to use

    ndim: int
        what dimension to use. [1-3]


    Returns
    -------

    W: float
        evaluated kernel

    """

    sigma = sph_kernel_normalization[ndim-1][kernel]
    kernel_W = sph_kernel_eval_function[kernel]

    H = sph_kernel_get_H(h, kernel)
    q = r / H
    W = kernel_W(q)

    return sigma * W / H**ndim


def sph_kernel_dWdr(r: float, h: float, kernel: str = "cubic_spline", ndim: int = 3):
    """
    Evaluates kernel derivatives for various kernels:
    returns dW/dr = dW/dq dq/dr = 1/H * dW/dq

    The kernels are scaled such that W(q > 1) = 0.
    Currently implemented:
        cubic_spline,
        quintic_spline,
        wendland_C2,
        wendland_C4,
        wendland_C6,
        gaussian (no compact support)


    Parameters
    ----------

    r: float
        particle distance

    h: float
        smoothing length

    kernel: str
        which kernel to use

    ndim: int
        what dimension to use. [1-3]

    Returns
    -------

    dWdr: float
        evaluated kernel derivative

    """

    sigma = sph_kernel_normalization[ndim-1][kernel]
    kernel_dWdr = sph_kernel_deval_function[kernel]
    H = sph_kernel_get_H(h, kernel)
    q = r / H
    dWdr = kernel_dWdr(q)

    return sigma * dWdr / H**(ndim + 1.)


def sph_kernel_get_H(h: Union[float, np.ndarray], kernel: str = "cubic_spline", ndim: int = 3):
    """!
    Compute the smoothing length in terms of the compact support length
    of a given kernel.
    The kernels defined above are defined and scaled to support a region <= H.


    Parameters
    ----------

    h: float or np.ndarray
        smoothing length

    kernel: str
        which kernel to use.

    ndim: int
        how many dimensions we're working with


    Returns
    -------

    H: float or np.dnarray
        compact support radii.

    """

    return h * sph_kernel_H_over_h[ndim - 1][kernel]
