# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
from copy import deepcopy

import peano4.output

from exahype2.solvers.aderdg.kernels.Gemms import Gemms


class Limiter:
    def __init__(self, dictionary):
        self.d = deepcopy(dictionary)

    def generate_kernels(
        self, namespace, output, subdirectory, template_prefix, output_path
    ):
        self._build_gemms()

        generated_kernels = peano4.output.Jinja2TemplatedHeaderImplementationFilePair(
            headfile_template=template_prefix + "Limiter.template.h",
            cppfile_template=template_prefix + "Limiter.template.cpp",
            classname=output_path + "/Limiter",
            namespace=namespace,
            subdirectory=subdirectory + ".",
            dictionary=self.d,
            default_overwrite=True,
            apply_iteratively=True,
        )
        output.add(generated_kernels)
        output.makefile.add_h_file(output_path + "/Limiter.h", generated=True)
        output.makefile.add_cpp_file(output_path + "/Limiter.cpp", generated=True)

    def _build_gemms(self):
        self.d["GEMMS"] = {}
        nDim = self.d["DIMENSIONS"]
        nDof = self.d["NUMBER_OF_DOF"]
        nDof2 = nDof * nDof
        nDofPad = self.d["NUMBER_OF_DOF_PADDED"]
        nDofLim = self.d["NUMBER_OF_DOF_LIMITER"]
        nDofLim2 = nDofLim * nDofLim
        nDofLimPad = self.d["NUMBER_OF_DOF_LIMITER_PADDED"]
        nUnknowns = self.d["NUMBER_OF_UNKNOWNS"]
        nUnknownsPad = self.d["NUMBER_OF_UNKNOWNS_PADDED"]
        nData = self.d["NUMBER_OF_DATA"]
        nDataPad = self.d["NUMBER_OF_DATA_PADDED"]

        self.d["GEMMS"]["dg2fv_x"] = Gemms(
            nData,
            nDofLim,
            nDof,
            nData,
            nDofPad,
            nDataPad,
            1,
            0,
            0,
            1,
            "dg2fv_x",
            "nopf",
            "gemm",
        )  # input slice not aligned
        if nDim == 3:
            self.d["GEMMS"]["dg2fv_y"] = Gemms(
                nDataPad,
                nDofLim,
                nDof,
                nDofLim * nDataPad,
                nDofPad,
                nDofLim * nDataPad,
                1,
                0,
                1,
                1,
                "dg2fv_y",
                "nopf",
                "gemm",
            )  # M is padded in both input and output
            self.d["GEMMS"]["dg2fv_z"] = Gemms(
                nData,
                nDofLim,
                nDof,
                nDofLim2 * nDataPad,
                nDofPad,
                nDofLim2 * nData,
                1,
                0,
                1,
                0,
                "dg2fv_z",
                "nopf",
                "gemm",
            )  # output slice not aligned
        else:
            self.d["GEMMS"]["dg2fv_y"] = Gemms(
                nData,
                nDofLim,
                nDof,
                nDofLim * nDataPad,
                nDofPad,
                nDofLim * nData,
                1,
                0,
                1,
                0,
                "dg2fv_y",
                "nopf",
                "gemm",
            )  # output slice not aligned

        # Project to DG
        self.d["GEMMS"]["fv2dg_x"] = Gemms(
            nData,
            nDof,
            nDofLim,
            nData,
            nDofLimPad,
            nDataPad,
            1,
            0,
            0,
            1,
            "fv2dg_x",
            "nopf",
            "gemm",
        )  # input slice not aligned
        if nDim == 3:
            self.d["GEMMS"]["fv2dg_y"] = Gemms(
                nDataPad,
                nDof,
                nDofLim,
                nDof * nDataPad,
                nDofLimPad,
                nDof * nDataPad,
                1,
                0,
                1,
                1,
                "fv2dg_y",
                "nopf",
                "gemm",
            )  # M is padded in both input and output
            self.d["GEMMS"]["fv2dg_z"] = Gemms(
                nData,
                nDof,
                nDofLim,
                nDof2 * nDataPad,
                nDofLimPad,
                nDof2 * nData,
                1,
                0,
                1,
                0,
                "fv2dg_z",
                "nopf",
                "gemm",
            )  # output slice not aligned
        else:
            self.d["GEMMS"]["fv2dg_y"] = Gemms(
                nData,
                nDof,
                nDofLim,
                nDof * nDataPad,
                nDofLimPad,
                nDof * nData,
                1,
                0,
                1,
                0,
                "fv2dg_y",
                "nopf",
                "gemm",
            )  # output slice not aligned

        # Project to Lobatto for Min/Max
        self.d["GEMMS"]["uh2lob_x"] = Gemms(
            nData,
            nDof,
            nDof,
            nData,
            nDofPad,
            nDataPad,
            1,
            0,
            0,
            1,
            "uh2lob_x",
            "nopf",
            "gemm",
        )  # input slice not aligned
        if nDim == 3:
            self.d["GEMMS"]["uh2lob_y"] = Gemms(
                nDataPad,
                nDof,
                nDof,
                nDof * nDataPad,
                nDofPad,
                nDof * nDataPad,
                1,
                0,
                1,
                1,
                "uh2lob_y",
                "nopf",
                "gemm",
            )  # M is padded in both input and output
            self.d["GEMMS"]["uh2lob_z_slice"] = Gemms(
                nDataPad,
                nDof,
                nDof,
                nDof2 * nDataPad,
                nDofPad,
                nDataPad,
                1,
                0,
                1,
                1,
                "uh2lob_z_slice",
                "nopf",
                "gemm",
            )  # will only write a slice, overwrite
        else:
            self.d["GEMMS"]["uh2lob_y_slice"] = Gemms(
                nDataPad,
                nDof,
                nDof,
                nDof * nDataPad,
                nDofPad,
                nDataPad,
                1,
                0,
                1,
                1,
                "uh2lob_y_slice",
                "nopf",
                "gemm",
            )  # will only write a slice, overwrite

        # Project to FV for Min/Max, reuse previous GEMM except last one for slice.
        if nDim == 3:
            self.d["GEMMS"]["dg2fv_z_slice"] = Gemms(
                nDataPad,
                nDofLim,
                nDof,
                nDofLim2 * nDataPad,
                nDofPad,
                nDataPad,
                1,
                0,
                1,
                1,
                "dg2fv_z_slice",
                "nopf",
                "gemm",
            )  # will only write a slice, overwrite
        else:
            self.d["GEMMS"]["dg2fv_y_slice"] = Gemms(
                nDataPad,
                nDofLim,
                nDof,
                nDofLim * nDataPad,
                nDofPad,
                nDataPad,
                1,
                0,
                1,
                1,
                "dg2fv_y_slice",
                "nopf",
                "gemm",
            )  # will only write a slice, overwrite

        if self.d["USE_LIBXSMM"]:
            self.controller.generateGemms("asm_limiter.c", self.d["GEMMS"].values())
