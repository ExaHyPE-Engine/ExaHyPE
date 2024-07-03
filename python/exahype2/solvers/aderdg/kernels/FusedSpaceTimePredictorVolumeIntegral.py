# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
from copy import deepcopy

import peano4.output

from .Gemms import Gemms
from exahype2.solvers.MathUtils import MathUtils


class FusedSpaceTimePredictorVolumeIntegral(object):
    def __init__(self, dictionary):
        self.d = deepcopy(dictionary)

    def generate_kernels(
        self, namespace, output, subdirectory, template_prefix, output_path
    ):
        self._build_gemms()

        if self.d["IS_LINEAR"]:
            # Size of the tmp array
            self.d["TMP_ARRAY_SIZE"] = max(
                (
                    self.d["NUMBER_OF_DOF"] * self.d["NUMBER_OF_UNKNOWNS_PADDED"]
                    if self.d["USE_FLUX"]
                    else 0
                ),
                (
                    self.d["DIMENSIONS"] * self.d["NUMBER_OF_UNKNOWNS_PADDED"]
                    if self.d["USE_NCP"]
                    else 0
                ),
            )
            self.d["NAME_SUFFIX"] = ""

            if self.d["USE_POINT_SOURCE"]:
                # Assembling the basis function for the point source
                QuadratureWeights, QuadratureNodes = MathUtils.getGaussLegendre(
                    self.d["NUMBER_OF_DOF"]
                )
                self.d["BASIS_FUNCTIONS"] = MathUtils.assembleBasisFunction(
                    QuadratureNodes
                )

                d = deepcopy(self.d)
                d["USE_POINT_SOURCE"] = False
                d["NAME_SUFFIX"] = "WithoutPointSources"

                generated_kernels = (
                    peano4.output.Jinja2TemplatedHeaderImplementationFilePair(
                        headfile_template=template_prefix
                        + "FusedSpaceTimePredictorVolumeIntegral.template.h",
                        cppfile_template=template_prefix
                        + "FusedSpaceTimePredictorVolumeIntegralLinear.template.cpp",
                        classname=output_path
                        + "/FusedSpaceTimePredictorVolumeIntegral"
                        + d["NAME_SUFFIX"],
                        namespace=namespace,
                        subdirectory=subdirectory + ".",
                        dictionary=d,
                        default_overwrite=True,
                        apply_iteratively=True,
                    )
                )
                output.add(generated_kernels)
                output.makefile.add_h_file(
                    output_path
                    + "/FusedSpaceTimePredictorVolumeIntegral"
                    + d["NAME_SUFFIX"]
                    + ".h",
                    generated=True,
                )
                output.makefile.add_cpp_file(
                    output_path
                    + "/FusedSpaceTimePredictorVolumeIntegral"
                    + d["NAME_SUFFIX"]
                    + ".cpp",
                    generated=True,
                )
        else:  # nonlinear
            generated_kernels = (
                peano4.output.Jinja2TemplatedHeaderImplementationFilePair(
                    headfile_template=template_prefix + "PicardIteration.template.h",
                    cppfile_template=template_prefix + "PicardIteration.template.cpp",
                    classname=output_path + "/PicardIteration",
                    namespace=namespace,
                    subdirectory=subdirectory + ".",
                    dictionary=self.d,
                    default_overwrite=True,
                    apply_iteratively=True,
                )
            )
            output.add(generated_kernels)
            output.makefile.add_h_file(
                output_path + "/PicardIteration" + ".h", generated=True
            )
            output.makefile.add_cpp_file(
                output_path + "/PicardIteration" + ".cpp", generated=True
            )

        generated_kernels = peano4.output.Jinja2TemplatedHeaderImplementationFilePair(
            headfile_template=template_prefix
            + "FusedSpaceTimePredictorVolumeIntegral.template.h",
            cppfile_template=template_prefix
            + "FusedSpaceTimePredictorVolumeIntegralLinear.template.cpp"
            if self.d["IS_LINEAR"]
            else template_prefix
            + "FusedSpaceTimePredictorVolumeIntegralNonlinear.template.cpp",
            classname=output_path + "/FusedSpaceTimePredictorVolumeIntegral",
            namespace=namespace,
            subdirectory=subdirectory + ".",
            dictionary=self.d,
            default_overwrite=True,
            apply_iteratively=True,
        )
        output.add(generated_kernels)
        output.makefile.add_h_file(
            output_path + "/FusedSpaceTimePredictorVolumeIntegral.h", generated=True
        )
        output.makefile.add_cpp_file(
            output_path + "/FusedSpaceTimePredictorVolumeIntegral.cpp", generated=True
        )

    def _build_gemms(self):
        # GEMMS: M, N, K, LDA, LDB, LDC, alpha, beta, Align A, Align C, name, prefetching, type
        self.d["GEMMS"] = {}
        nDim = self.d["DIMENSIONS"]
        nDof = self.d["NUMBER_OF_DOF"]
        nDof2 = nDof * nDof
        nDofPad = self.d["NUMBER_OF_DOF_PADDED"]
        nUnknowns = self.d["NUMBER_OF_UNKNOWNS"]
        nUnknownsPad = self.d["NUMBER_OF_UNKNOWNS_PADDED"]
        nDataPad = self.d["NUMBER_OF_DATA_PADDED"]

        if self.d["IS_LINEAR"]:
            if self.d["USE_FLUX"]:
                self.d["GEMMS"]["flux_x"] = Gemms(
                    nUnknownsPad,
                    nDof,
                    nDof,
                    nUnknownsPad,
                    nDofPad,
                    nUnknownsPad,
                    1,
                    0,
                    1,
                    1,
                    "flux_x",
                    "nopf",
                    "gemm",
                    self.d["PREDICTOR_COMPUTATION_PRECISIONS"],
                )  # beta, 0 => overwrite C
                self.d["GEMMS"]["flux_y"] = Gemms(
                    nUnknownsPad,
                    nDof,
                    nDof,
                    nUnknownsPad * nDof,
                    nDofPad,
                    nUnknownsPad,
                    1,
                    0,
                    1,
                    1,
                    "flux_y",
                    "nopf",
                    "gemm",
                    self.d["PREDICTOR_COMPUTATION_PRECISIONS"],
                )  # beta, 0 => overwrite C
                if self.d["DIMENSIONS"] >= 3:
                    self.d["GEMMS"]["flux_z"] = Gemms(
                        nUnknownsPad,
                        nDof,
                        nDof,
                        nUnknownsPad * nDof2,
                        nDofPad,
                        nUnknownsPad,
                        1,
                        0,
                        1,
                        1,
                        "flux_z",
                        "nopf",
                        "gemm",
                        self.d["PREDICTOR_COMPUTATION_PRECISIONS"],
                    )  # beta, 0 => overwrite C
            if self.d["USE_NCP"]:
                self.d["GEMMS"]["gradQ_x"] = Gemms(
                    nUnknowns,
                    nDof,
                    nDof,
                    nDataPad,
                    nDofPad,
                    nUnknownsPad,
                    1,
                    1,
                    1,
                    1,
                    "gradQ_x",
                    "nopf",
                    "gemm",
                    self.d["PREDICTOR_COMPUTATION_PRECISIONS"],
                )
                self.d["GEMMS"]["gradQ_y"] = Gemms(
                    nUnknowns,
                    nDof,
                    nDof,
                    nDataPad * nDof,
                    nDofPad,
                    nUnknownsPad * nDof,
                    1,
                    1,
                    1,
                    1,
                    "gradQ_y",
                    "nopf",
                    "gemm",
                    self.d["PREDICTOR_COMPUTATION_PRECISIONS"],
                )
                if self.d["DIMENSIONS"] >= 3:
                    self.d["GEMMS"]["gradQ_z"] = Gemms(
                        nUnknowns,
                        nDof,
                        nDof,
                        nDataPad * nDof2,
                        nDofPad,
                        nUnknownsPad * nDof2,
                        1,
                        1,
                        1,
                        1,
                        "gradQ_z",
                        "nopf",
                        "gemm",
                        self.d["PREDICTOR_COMPUTATION_PRECISIONS"],
                    )
        else:  # nonlinear
            if self.d["USE_FLUX"]:
                self.d["GEMMS"]["rhs_x"] = Gemms(
                    nUnknownsPad,
                    nDof,
                    nDof,
                    nUnknownsPad,
                    nDofPad,
                    nUnknownsPad,
                    1,
                    1,
                    1,
                    1,
                    "rhs_x",
                    "nopf",
                    "gemm",
                    self.d["PREDICTOR_COMPUTATION_PRECISIONS"],
                )
                self.d["GEMMS"]["rhs_y"] = Gemms(
                    nUnknownsPad,
                    nDof,
                    nDof,
                    nUnknownsPad * nDof,
                    nDofPad,
                    nUnknownsPad * nDof,
                    1,
                    1,
                    1,
                    1,
                    "rhs_y",
                    "nopf",
                    "gemm",
                    self.d["PREDICTOR_COMPUTATION_PRECISIONS"],
                )
                if self.d["DIMENSIONS"] >= 3:
                    self.d["GEMMS"]["rhs_z"] = Gemms(
                        nUnknownsPad,
                        nDof,
                        nDof,
                        nUnknownsPad * nDof2,
                        nDofPad,
                        nUnknownsPad * nDof2,
                        1,
                        1,
                        1,
                        1,
                        "rhs_z",
                        "nopf",
                        "gemm",
                        self.d["PREDICTOR_COMPUTATION_PRECISIONS"],
                    )
                self.d["GEMMS"]["lduh_x"] = Gemms(
                    nUnknownsPad,
                    nDof,
                    nDof,
                    nUnknownsPad,
                    nDofPad,
                    nUnknownsPad,
                    1,
                    1,
                    1,
                    1,
                    "lduh_x",
                    "nopf",
                    "gemm",
                    self.d["PREDICTOR_COMPUTATION_PRECISIONS"],
                )
                self.d["GEMMS"]["lduh_y"] = Gemms(
                    nUnknownsPad,
                    nDof,
                    nDof,
                    nUnknownsPad,
                    nDofPad,
                    nUnknownsPad * nDof,
                    1,
                    1,
                    1,
                    1,
                    "lduh_y",
                    "nopf",
                    "gemm",
                    self.d["PREDICTOR_COMPUTATION_PRECISIONS"],
                )
                if self.d["DIMENSIONS"] >= 3:
                    self.d["GEMMS"]["lduh_z"] = Gemms(
                        nUnknownsPad,
                        nDof,
                        nDof,
                        nUnknownsPad,
                        nDofPad,
                        nUnknownsPad * nDof2,
                        1,
                        1,
                        1,
                        1,
                        "lduh_z",
                        "nopf",
                        "gemm",
                        self.d["PREDICTOR_COMPUTATION_PRECISIONS"],
                    )
            if self.d["USE_NCP"]:
                self.d["GEMMS"]["gradQ_x"] = Gemms(
                    nUnknowns,
                    nDof,
                    nDof,
                    nDataPad,
                    nDofPad,
                    nUnknownsPad,
                    1,
                    1,
                    1,
                    1,
                    "gradQ_x",
                    "nopf",
                    "gemm",
                    self.d["PREDICTOR_COMPUTATION_PRECISIONS"],
                )
                self.d["GEMMS"]["gradQ_y"] = Gemms(
                    nUnknowns,
                    nDof,
                    nDof,
                    nDataPad * nDof,
                    nDofPad,
                    nUnknownsPad * nDof,
                    1,
                    1,
                    1,
                    1,
                    "gradQ_y",
                    "nopf",
                    "gemm",
                    self.d["PREDICTOR_COMPUTATION_PRECISIONS"],
                )
                if self.d["DIMENSIONS"] >= 3:
                    self.d["GEMMS"]["gradQ_z"] = Gemms(
                        nUnknowns,
                        nDof,
                        nDof,
                        nDataPad * nDof2,
                        nDofPad,
                        nUnknownsPad * nDof2,
                        1,
                        1,
                        1,
                        1,
                        "gradQ_z",
                        "nopf",
                        "gemm",
                        self.d["PREDICTOR_COMPUTATION_PRECISIONS"],
                    )

            self.d["GEMMS"]["lqi"] = Gemms(
                nUnknownsPad,
                nDof,
                nDof,
                nUnknownsPad * (nDof**nDim),
                nDofPad,
                nUnknownsPad,
                1,
                0,
                1,
                1,
                "lqi",
                "nopf",
                "gemm",
                self.d["PREDICTOR_COMPUTATION_PRECISIONS"],
            )  # beta, 0 => overwrite C

        if self.d["USE_LIBXSMM"]:
            self.controller.generateGemms("asm_fstpvi.c", self.d["GEMMS"].values())
