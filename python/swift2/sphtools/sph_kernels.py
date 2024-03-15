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

sph_vectorized_kernel_list = [
    "quartic_spline_vectorized",
]


class _forbidInvalidKernelsDict(dict):
    """!
    Modify the default dict to raise an error if a user tries to
    access kernels which aren't defined. Undefined kernels are
    specified via blocked_keys kwarg during initialization.

    Also limit the available kernels to those specified in the
    sph_kernel_list list.
    """

    def __init__(self, *args, blocked_keys=(), ndim=0, **kwargs):
        super().__init__(*args, **kwargs)
        for key in blocked_keys:
            if key not in sph_kernel_list and key not in sph_vectorized_kernel_list:
                raise KeyError(
                    f"Invalid kernel key {key}. Only {sph_kernel_list} are permitted"
                )
        self.blocked_keys = blocked_keys
        self.ndim = ndim

    def __setitem__(self, key, value):
        if key not in sph_kernel_list and key not in sph_vectorized_kernel_list:
            raise KeyError(
                f"Invalid kernel key {key}. Only {sph_kernel_list} are permitted"
            )
        super().__setitem__(key, value)

    def __getitem__(self, key):
        if key in self.blocked_keys:
            raise ValueError(f"SPH kernel {key} is not defined for ndim={self.ndim}")
        return super().__getitem__(key)


# Translation from kernel name to kernel macro
sph_kernel_macro_name = _forbidInvalidKernelsDict()

# list for 1D, 2D, 3D values of H/h of kernel dicts
sph_kernel_H_over_h = [
    _forbidInvalidKernelsDict(blocked_keys=("wendland_C4", "wendland_C6"), ndim=1),
    _forbidInvalidKernelsDict(ndim=2),
    _forbidInvalidKernelsDict(ndim=3),
]


# list for 1D, 2D, 3D values of kernel normalization coefficients of kernel dicts
sph_kernel_normalization = [
    _forbidInvalidKernelsDict(blocked_keys=("wendland_C4", "wendland_C6"), ndim=1),
    _forbidInvalidKernelsDict(ndim=2),
    _forbidInvalidKernelsDict(ndim=3),
]

# dict containing functions that evaluate the kernel
sph_kernel_eval_function = _forbidInvalidKernelsDict()

# dict containing functions that evaluate the kernel derivatives
sph_kernel_deval_function = _forbidInvalidKernelsDict()


# Now fill up the dicts with content
# Macro names
sph_kernel_macro_name["cubic_spline"] = "CUBIC_SPLINE_KERNEL"
sph_kernel_macro_name["quartic_spline"] = "QUARTIC_SPLINE_KERNEL"
sph_kernel_macro_name["quintic_spline"] = "QUINTIC_SPLINE_KERNEL"
sph_kernel_macro_name["wendland_C2"] = "WENDLAND_C2_KERNEL"
sph_kernel_macro_name["wendland_C4"] = "WENDLAND_C4_KERNEL"
sph_kernel_macro_name["wendland_C6"] = "WENDLAND_C6_KERNEL"
sph_kernel_macro_name["quartic_spline_vectorized"] = sph_kernel_macro_name[
    "quartic_spline"
]

# Kernel Gammas (ratio of compact support radius H to smoothing length h)
# 3D values
sph_kernel_H_over_h[2]["cubic_spline"] = 1.825742
sph_kernel_H_over_h[2]["quartic_spline"] = 2.018932
sph_kernel_H_over_h[2]["quintic_spline"] = 2.195775
sph_kernel_H_over_h[2]["wendland_C2"] = 1.936492
sph_kernel_H_over_h[2]["wendland_C4"] = 2.207940
sph_kernel_H_over_h[2]["wendland_C6"] = 2.449490
sph_kernel_H_over_h[2]["quartic_spline_vectorized"] = sph_kernel_H_over_h[2][
    "quartic_spline"
]

# 2D values
sph_kernel_H_over_h[1]["cubic_spline"] = 1.778002
sph_kernel_H_over_h[1]["quartic_spline"] = 1.977173
sph_kernel_H_over_h[1]["quintic_spline"] = 2.158131
sph_kernel_H_over_h[1]["wendland_C2"] = 1.897367
sph_kernel_H_over_h[1]["wendland_C4"] = 2.171239
sph_kernel_H_over_h[1]["wendland_C6"] = 2.415230
sph_kernel_H_over_h[1]["quartic_spline_vectorized"] = sph_kernel_H_over_h[1][
    "quartic_spline"
]

# 1D values
sph_kernel_H_over_h[0]["cubic_spline"] = 1.732051
sph_kernel_H_over_h[0]["quartic_spline"] = 1.936492
sph_kernel_H_over_h[0]["quintic_spline"] = 2.121321
sph_kernel_H_over_h[0]["wendland_C2"] = 1.620185
sph_kernel_H_over_h[0]["wendland_C4"] = None  # not defined in 1D
sph_kernel_H_over_h[0]["wendland_C6"] = None  # not defined in 1D
sph_kernel_H_over_h[0]["quartic_spline_vectorized"] = sph_kernel_H_over_h[0][
    "quartic_spline"
]


# Kernel Normalizations
# 3D values
sph_kernel_normalization[2]["cubic_spline"] = 16.0 / np.pi
sph_kernel_normalization[2]["quartic_spline"] = 15625.0 / (np.pi * 512.0)
sph_kernel_normalization[2]["quintic_spline"] = 2187.0 / (np.pi * 40.0)
sph_kernel_normalization[2]["wendland_C2"] = 21.0 / (np.pi * 2.0)
sph_kernel_normalization[2]["wendland_C4"] = 495.0 / (np.pi * 32.0)
sph_kernel_normalization[2]["wendland_C6"] = 1365.0 / (np.pi * 64.0)
sph_kernel_normalization[2]["quartic_spline_vectorized"] = sph_kernel_normalization[2][
    "quartic_spline"
]

# 2D values
sph_kernel_normalization[1]["cubic_spline"] = 80.0 / (np.pi * 7.0)
sph_kernel_normalization[1]["quartic_spline"] = 46875.0 / (np.pi * 2398.0)
sph_kernel_normalization[1]["quintic_spline"] = 15309.0 / (np.pi * 478.0)
sph_kernel_normalization[1]["wendland_C2"] = 7.0 / np.pi
sph_kernel_normalization[1]["wendland_C4"] = 9.0 / np.pi
sph_kernel_normalization[1]["wendland_C6"] = 78.0 / (np.pi * 7.0)
sph_kernel_normalization[1]["quartic_spline_vectorized"] = sph_kernel_normalization[1][
    "quartic_spline"
]

# 1D values
sph_kernel_normalization[0]["cubic_spline"] = 8.0 / 3.0
sph_kernel_normalization[0]["quartic_spline"] = 3125.0 / 768.0
sph_kernel_normalization[0]["quintic_spline"] = 243.0 / 40.0
sph_kernel_normalization[0]["wendland_C2"] = 5.0 / 4.0
sph_kernel_normalization[0]["wendland_C4"] = None  # not defined in 1D
sph_kernel_normalization[0]["wendland_C6"] = None  # not defined in 1D
sph_kernel_normalization[0]["quartic_spline_vectorized"] = sph_kernel_normalization[0][
    "quartic_spline"
]


def _sph_kernel_eval_cubic_spline(q: float):
    """!
    Evaluates the cubic spline kernel
    """
    if q < 0.5:
        res = 3.0 * q**2.0 * (q - 1.0) + 0.5
    elif q < 1.0:
        #  res =  q * (q * (3 - q) - 3) + 1
        res = -(q**3.0) + 3.0 * q**2.0 - 3.0 * q + 1.0
    else:
        res = 0.0
    return res


def _sph_kernel_eval_quartic_spline(q: float):
    """!
    Evaluates the quartic spline kernel
    """
    if q < 0.2:
        res = 6 * q**4 - 2.4 * q**2 + 46 / 125
    elif q < 0.6:
        res = -4 * q**4 + 8 * q**3 - 4.8 * q**2 + 8 / 25 * q + 44.0 / 125
    elif q < 1:
        res = q**4 - 4 * q**3 + 6 * q**2 - 4 * q + 1
    else:
        res = 0.0
    return res


def _sph_kernel_eval_quartic_spline_vectorized(q: np.ndarray):
    """!
    Evaluates the quartic spline kernel
    """

    res = np.zeros(q.shape)
    mask1 = q <= 0.2
    mask2 = np.logical_and(q > 0.2, q <= 0.6)
    mask3 = np.logical_and(q > 0.6, q <= 1.0)

    qa = q[mask1]
    qa2 = qa**2
    res[mask1] = 6.0 * qa2**2 - 2.4 * qa2 + 46.0 / 125.0

    qb = q[mask2]
    qb2 = qb**2
    res[mask2] = (
        -4.0 * qb2**2 + 8.0 * qb2 * qb - 4.8 * qb2 + 8.0 / 25.0 * qb + 44.0 / 125.0
    )

    qc = q[mask3]
    qc2 = qc**2
    res[mask3] = qc2**2 - 4.0 * qc2 * qc + 6.0 * qc2 - 4.0 * qc + 1.0

    return res


def _sph_kernel_eval_quintic_spline(q: float):
    """!
    Evaluates the quintic spline kernel
    """
    if q < 1.0 / 3.0:
        res = 10.0 * (q**4 * (1.0 - q) - 0.2222222222 * q**2) + 0.2716049382716049
    elif q < 0.666666666666:
        qsq = q**2
        q4 = qsq**2
        res = (
            5
            * (
                q4 * (q - 3)
                + qsq * (3.333333333333333 * q - 1.5555555555555)
                + 0.18518518518518517 * q
            )
            + 0.20987654320987653
        )
    elif q < 1.0:
        qsq = q**2
        res = qsq * (qsq * (5 - q) + 10 * (1 - q)) - 5 * q + 1
    else:
        res = 0.0
    return res


def _sph_kernel_eval_quintic_spline_vectorized(q: np.ndarray):
    """!
    Evaluates the quintic spline kernel
    """

    res = np.zeros(q.shape)
    mask1 = q <= 1.0 / 3.0
    mask2 = np.logical_and(q > 1.0 / 3.0, q <= 2.0 / 3)
    mask3 = np.logical_and(q > 2.0 / 3, q <= 1.0)

    qa = q[mask1]
    res[mask1] = (
        10.0 * (qa**4 * (1.0 - qa) - 0.2222222222 * qa**2) + 0.2716049382716049
    )

    qb = q[mask2]
    qb2 = qb**2
    qb4 = qb2**2

    res[mask2] = (
        5.0
        * (
            qb4 * (qb - 3.0)
            + qb2 * (3.333333333333333 * qb - 1.5555555555555)
            + 0.18518518518518517 * qb
        )
        + 0.20987654320987653
    )

    qc = q[mask3]
    qc2 = qc**2
    res[mask3] = qc2 * (qc2 * (5.0 - qc) + 10.0 * (1.0 - qc)) - 5.0 * qc + 1.0

    return res


def _sph_kernel_eval_wendland_C2(q: float):
    """!
    Evaluates the Wendland C2 kernel
    """
    if q < 1.0:
        qsq = q**2
        res = qsq * (qsq * (4.0 * q - 15.0) + 10.0 * (2.0 * q - 1.0)) + 1.0
    else:
        res = 0.0
    return res


def _sph_kernel_eval_wendland_C4(q: float):
    """!
    Evaluates the Wendland C4 kernel
    """
    if q < 1.0:
        qsq = q**2
        q4 = qsq**2

        res = (
            11.666666666666666 * q4 * q4
            - 64.0 * q4 * qsq * q
            + 140.0 * qsq * q4
            - 149.3333333333333 * q4 * q
            + 70.0 * q4
            - 9.33333333333333 * qsq
            + 1.0
        )
    else:
        res = 0.0
    return res


def _sph_kernel_eval_wendland_C6(q: float):
    """!
    Evaluates the Wendland C6 kernel
    """
    if q < 1.0:
        res = (
            32.0 * q**11
            - 231.0 * q**10
            + 704.0 * q**9
            - 1155.0 * q**8
            + 1056.0 * q**7
            - 462.0 * q**6
            + 66.0 * q**4
            - 11.0 * q**2
            + 1.0
        )
    else:
        res = 0.0
    return res


def _sph_kernel_deval_cubic_spline(q: float):
    """!
    Evaluates the cubic spline kernel derivative
    """
    if q < 0.5:
        res = 9 * q**2 - 6 * q
    elif q < 1:
        res = 6 * q - 3 * q**2 - 3
    else:
        res = 0.0

    return res


def _sph_kernel_deval_quartic_spline(q: float):
    """!
    Evaluates the quartic spline kernel derivative
    """
    if q < 0.2:
        res = 24 * q**3 - 4.8 * q
    elif q < 0.6:
        res = -16 * q**3 + 24 * q**2 - 9.6 * q + 8 / 25
    elif q < 1:
        res = 4 * q**3 - 12 * q**2 + 12 * q - 4
    else:
        res = 0.0
    return res


def _sph_kernel_deval_quartic_spline_vectorized(q: np.ndarray):
    """!
    Evaluates the quartic spline kernel derivative
    """

    res = np.zeros(q.shape)
    mask1 = q <= 0.2
    mask2 = np.logical_and(q > 0.2, q <= 0.6)
    mask3 = np.logical_and(q > 0.6, q <= 1.0)

    qa = q[mask1]
    qa2 = qa**2
    res[mask1] = 24.0 * qa**3 - 4.8 * qa

    qb = q[mask2]
    qb2 = qb**2
    res[mask2] = -16.0 * qb2 * qb + 24.0 * qb2 - 9.6 * qb + 8.0 / 25.0

    qc = q[mask3]
    qc2 = qc**2
    res[mask3] = 4.0 * qc2 * qc - 12.0 * qc2 + 12.0 * qc - 4.0

    return res


def _sph_kernel_deval_quintic_spline(q: float):
    """!
    Evaluates the quintic spline kernel derivative
    """
    if q < 0.333333333333:
        res = 40 * q**3 - 50 * q**4 - 4.44444444444 * q
    elif q < 0.666666666666:
        res = (
            25 * q**4
            - 60 * q**3
            + 50 * q**2
            - 15.555555555555 * q
            + 0.9259259259259259
        )
    elif q < 1:
        res = 20 * q**3 - 5 * q**4 + 20 * q - 30 * q**2 - 5
    else:
        res = 0.0
    return res


def _sph_kernel_deval_wendland_C2(q: float):
    """!
    Evaluates the Wendland C2 kernel derivative
    """
    if q < 1.0:
        res = 20.0 * q**4 - 60.0 * q**3 + 60.0 * q**2 - 20.0 * q
    else:
        res = 0.0
    return res


def _sph_kernel_deval_wendland_C4(q: float):
    """!
    Evaluates the Wendland C4 kernel derivative
    """
    if q < 1:
        res = (
            93.3333333333 * q**7
            - 448 * q**6
            + 840 * q**5
            - 746.6666666666 * q**4
            + 280 * q**3
            - 18.66666666666666 * q
        )
    else:
        res = 0.0
    return res


def _sph_kernel_deval_wendland_C6(q: float):
    """!
    Evaluates the Wendland C6 kernel derivative
    """
    if q < 1.0:
        res = (
            352 * q**10
            - 2310 * q**9
            + 6336 * q**8
            - 9240 * q**7
            + 7392 * q**6
            - 2772 * q**5
            + 264 * q**3
            - 22 * q
        )
    else:
        res = 0.0
    return res


sph_kernel_eval_function["cubic_spline"] = _sph_kernel_eval_cubic_spline
sph_kernel_eval_function["quartic_spline"] = _sph_kernel_eval_quartic_spline
sph_kernel_eval_function["quintic_spline"] = _sph_kernel_eval_quintic_spline
sph_kernel_eval_function["wendland_C2"] = _sph_kernel_eval_wendland_C2
sph_kernel_eval_function["wendland_C4"] = _sph_kernel_eval_wendland_C4
sph_kernel_eval_function["wendland_C6"] = _sph_kernel_eval_wendland_C6
sph_kernel_eval_function[
    "quartic_spline_vectorized"
] = _sph_kernel_eval_quartic_spline_vectorized

sph_kernel_deval_function["cubic_spline"] = _sph_kernel_deval_cubic_spline
sph_kernel_deval_function["quartic_spline"] = _sph_kernel_deval_quartic_spline
sph_kernel_deval_function["quintic_spline"] = _sph_kernel_deval_quintic_spline
sph_kernel_deval_function["wendland_C2"] = _sph_kernel_deval_wendland_C2
sph_kernel_deval_function["wendland_C4"] = _sph_kernel_deval_wendland_C4
sph_kernel_deval_function["wendland_C6"] = _sph_kernel_deval_wendland_C6
sph_kernel_deval_function[
    "quartic_spline_vectorized"
] = _sph_kernel_deval_quartic_spline_vectorized


def sph_kernel_W(
    r: Union[float, np.ndarray], h: Union[float, np.ndarray], kernel: str, ndim: int
):
    """!
    Evaluates various kernels.
    The kernels are scaled such that W(q > 1) = 0.
    Currently implemented:
        cubic_spline,
        quartic_spline,
        quintic_spline,
        wendland_C2,
        wendland_C4, [not with ndim=1]
        wendland_C6, [not with ndim=1]


    Parameters
    ----------

    r: float or ndarray
        particle distance(s)

    h: float or ndarray
        smoothing length(s)

    kernel: str
        which kernel to use

    ndim: int
        what dimension to use. [1-3]


    Returns
    -------

    W: float
        evaluated kernel

    """

    sigma = sph_kernel_normalization[ndim - 1][kernel]
    H = sph_kernel_get_H(h, kernel, ndim)
    kernel_W = sph_kernel_eval_function[kernel]
    q = r / H
    W = sigma * kernel_W(q) / H**ndim

    return W


def sph_kernel_dWdr(
    r: Union[float, np.ndarray], h: Union[float, np.ndarray], kernel: str, ndim: int
):
    """
    Evaluates kernel derivatives for various kernels:
    returns dW/dr = dW/dq dq/dr = 1/H * dW/dq

    The kernels are scaled such that W(q > 1) = 0.
    Currently implemented:
        cubic_spline,
        quartic_spline,
        quintic_spline,
        wendland_C2,
        wendland_C4, [not with ndim=1]
        wendland_C6, [not with ndim=1]


    Parameters
    ----------

    r: float or ndarray
        particle distance(s)

    h: float or ndarray
        smoothing length(s)

    kernel: str
        which kernel to use

    ndim: int
        what dimension to use. [1-3]

    Returns
    -------

    dWdr: float or ndarray
        evaluated kernel derivative

    """

    sigma = sph_kernel_normalization[ndim - 1][kernel]
    kernel_dWdr = sph_kernel_deval_function[kernel]
    H = sph_kernel_get_H(h, kernel, ndim)
    q = r / H
    dWdr = sigma * kernel_dWdr(q) / H ** (ndim + 1)

    return dWdr


def sph_kernel_get_search_radius(h: Union[float, np.ndarray], kernel: str, ndim: int):
    return sph_kernel_get_H(h, kernel, ndim)


def sph_kernel_get_H(h: Union[float, np.ndarray], kernel: str, ndim: int):
    """!
    Compute the compact support radius of a given kernel and given smoothing length.
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


def sph_kernel_get_smoothing_length_from_search_radius(
    H: Union[float, np.ndarray], kernel: str, ndim: int
):
    """!
    Compute the smoothing length given the compact support length of a given kernel.
    The kernels defined above are defined and scaled to support a region <= H.


    Parameters
    ----------

    H: float or np.ndarray
        compact support radii.

    kernel: str
        which kernel to use.

    ndim: int
        how many dimensions we're working with


    Returns
    -------

    h: float or np.dnarray
        smoothing lengt(s)

    """

    return H / sph_kernel_H_over_h[ndim - 1][kernel]


def sph_kernel_root(kernel: str, ndim: int):
    """!
    Evaluate kernel root, i.e. W(0, h).
    We don't actually need `h` here, because the quantity used in the kernels,
    `q = r/h` is also zero by construction.

    Parameters
    ----------

    kernel: str
        which kernel to use

    ndim: int
        what dimension to use. [1-3]


    Returns
    -------

    W: float
        evaluated kernel

    """
    kernel_novector = kernel
    if kernel.endswith("vectorized"):
        kernel_novector = kernel[:-11]

    return sph_kernel_W(0.0, 1.0, kernel_novector, ndim)
