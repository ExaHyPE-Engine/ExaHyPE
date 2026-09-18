# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
from copy import deepcopy

import peano4.output

from .Gemms import Gemms

from exahype2.solvers.MathUtils import MathUtils


class FusedSpaceTimePredictorVolumeIntegral(object):
    def __init__(self, dictionary):
        self.d = deepcopy(dictionary)

    def generate_kernels(self, solver, namespace, output, template_prefix, output_path):
        self._build_gemms()

        self.d["NAME_SUFFIX"] = ""

        if self.d["IS_LINEAR"]:
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

            if self.d["USE_POINT_SOURCE"]:
                if self.d["POLYNOMIAL_TYPE"] == "legendre":
                    _, QuadratureNodes = MathUtils.getGaussLegendre(
                        self.d["NUMBER_OF_DOF"]
                    )
                elif self.d["POLYNOMIAL_TYPE"] == "lobatto":
                    _, QuadratureNodes = MathUtils.getGaussLobatto(
                        self.d["NUMBER_OF_DOF"]
                    )
                else:
                    raise ValueError(
                        "Quadrature type "
                        + self.d["POLYNOMIAL_TYPE"]
                        + " not supported."
                    )

                self.d["BASIS_FUNCTIONS"] = MathUtils.assembleBasisFunction(
                    QuadratureNodes
                )

                point_source_dictionary = deepcopy(self.d)
                self.d["USE_POINT_SOURCE"] = False
                point_source_dictionary["NAME_SUFFIX"] = "WithPointSources"

                generated_kernels = (
                    peano4.output.Jinja2TemplatedHeaderImplementationFilePair(
                        headfile_template=template_prefix
                        + "FusedSpaceTimePredictorVolumeIntegral.template.h",
                        cppfile_template=template_prefix
                        + "FusedSpaceTimePredictorVolumeIntegralLinear.template.cpp",
                        classname=output_path
                        + "/FusedSpaceTimePredictorVolumeIntegral"
                        + point_source_dictionary["NAME_SUFFIX"],
                        namespace=namespace,
                        subdirectory=".",
                        dictionary=point_source_dictionary,
                        apply_iteratively=True,
                    )
                )
                output.add(generated_kernels)
                output.makefile.add_h_file(
                    output_path
                    + "/FusedSpaceTimePredictorVolumeIntegral"
                    + point_source_dictionary["NAME_SUFFIX"]
                    + ".h",
                    generated=True,
                )
                output.makefile.add_cpp_file(
                    output_path
                    + "/FusedSpaceTimePredictorVolumeIntegral"
                    + point_source_dictionary["NAME_SUFFIX"]
                    + ".cpp",
                    generated=True,
                )
                solver._user_action_set_includes += f'#include "{output_path}/FusedSpaceTimePredictorVolumeIntegral{point_source_dictionary["NAME_SUFFIX"]}.h"\n'

        generated_kernels = peano4.output.Jinja2TemplatedHeaderImplementationFilePair(
            headfile_template=template_prefix
            + "FusedSpaceTimePredictorVolumeIntegral.template.h",
            cppfile_template=(
                template_prefix
                + "FusedSpaceTimePredictorVolumeIntegralLinear.template.cpp"
                if self.d["IS_LINEAR"]
                else template_prefix
                + "FusedSpaceTimePredictorVolumeIntegralNonlinear.template.cpp"
            ),
            classname=output_path + "/FusedSpaceTimePredictorVolumeIntegral",
            namespace=namespace,
            subdirectory=".",
            dictionary=self.d,
            apply_iteratively=True,
        )
        output.add(generated_kernels)
        output.makefile.add_h_file(
            output_path + "/FusedSpaceTimePredictorVolumeIntegral.h", generated=True
        )
        output.makefile.add_cpp_file(
            output_path + "/FusedSpaceTimePredictorVolumeIntegral.cpp", generated=True
        )
        solver._user_action_set_includes += (
            f'#include "{output_path}/FusedSpaceTimePredictorVolumeIntegral.h"\n'
        )

    def _build_gemms(self):
        # Gemms takes M, N, K, LDA, LDB, LDC, alpha, beta, alignment_A, alignment_C and name.
        self.d["GEMMS"] = {}
        nDim = self.d["DIMENSIONS"]
        nDof = self.d["NUMBER_OF_DOF"]
        nDof2 = nDof * nDof
        nDofPad = self.d["NUMBER_OF_DOF_PADDED"]
        nUnknowns = self.d["NUMBER_OF_UNKNOWNS"]
        nUnknownsPad = self.d["NUMBER_OF_UNKNOWNS_PADDED"]
        nData = self.d["NUMBER_OF_DATA"]
        nDataPad = self.d["NUMBER_OF_DATA_PADDED"]

        # Linear kernels expand their tensor-product operators inline, so only the
        # nonlinear predictor needs matrix multiplication specifications.
        if not self.d["IS_LINEAR"]:
            if self.d["USE_FLUX"]:
                self.d["GEMMS"]["rhsX"] = Gemms(
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
                    "rhsX",
                )
                self.d["GEMMS"]["rhsY"] = Gemms(
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
                    "rhsY",
                )
                if self.d["DIMENSIONS"] >= 3:
                    self.d["GEMMS"]["rhsZ"] = Gemms(
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
                        "rhsZ",
                    )
                self.d["GEMMS"]["lduhX"] = Gemms(
                    nUnknowns,
                    nDof,
                    nDof,
                    nUnknownsPad,
                    nDofPad,
                    nData,
                    1,
                    1,
                    1,
                    1,
                    "lduhX",
                )
                self.d["GEMMS"]["lduhY"] = Gemms(
                    nUnknowns,
                    nDof,
                    nDof,
                    nUnknownsPad,
                    nDofPad,
                    nData * nDof,
                    1,
                    1,
                    1,
                    1,
                    "lduhY",
                )
                if self.d["DIMENSIONS"] >= 3:
                    self.d["GEMMS"]["lduhZ"] = Gemms(
                        nUnknowns,
                        nDof,
                        nDof,
                        nUnknownsPad,
                        nDofPad,
                        nData * nDof2,
                        1,
                        1,
                        1,
                        1,
                        "lduhZ",
                    )
            if (
                self.d["USE_NCP"]
                or self.d["USE_DIFFUSIVE_FLUX"]
                or self.d["USE_DIFFUSIVE_SOURCE"]
            ):
                self.d["GEMMS"]["gradQX"] = Gemms(
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
                    "gradQX",
                )
                self.d["GEMMS"]["gradQY"] = Gemms(
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
                    "gradQY",
                )
                if self.d["DIMENSIONS"] >= 3:
                    self.d["GEMMS"]["gradQZ"] = Gemms(
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
                        "gradQZ",
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
            )  # beta is zero, so the product overwrites C
