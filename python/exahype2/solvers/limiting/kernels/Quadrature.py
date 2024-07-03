# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
from copy import deepcopy

import peano4.output

from exahype2.solvers.MathUtils import MathUtils


class Quadrature(object):
    def __init__(self, dictionary):
        self.d = deepcopy(dictionary)

    def generate_kernels(
        self, namespace, output, subdirectory, template_prefix, output_path
    ):
        if self.d["POLYNOMIAL_TYPE"] == "legendre":
            QuadratureWeights, QuadratureNodes = MathUtils.getGaussLegendre(
                self.d["NUMBER_OF_DOF"]
            )
            OtherQuadratureWeights, OtherQuadratureNodes = MathUtils.getGaussLobatto(
                self.d["NUMBER_OF_DOF"]
            )
        elif self.d["POLYNOMIAL_TYPE"] == "lobatto":
            QuadratureWeights, QuadratureNodes = MathUtils.getGaussLobatto(
                self.d["NUMBER_OF_DOF"]
            )
            OtherQuadratureWeights, OtherQuadratureNodes = MathUtils.getGaussLobatto(
                self.d["NUMBER_OF_DOF"]
            )
            QuadratureNodes = QuadratureNodes[::-1]
        else:
            raise ValueError(
                "Quadrature type " + self.d["POLYNOMIAL_TYPE"] + " not supported."
            )

        self.d["QuadratureWeights"], self.d["QuadratureNodes"] = (
            QuadratureWeights,
            QuadratureNodes,
        )
        self.d["quadrature_seq"] = range(self.d["NUMBER_OF_DOF"])

        l_padSize = self.d["NUMBER_OF_DOF_PADDED"] - self.d["NUMBER_OF_DOF"]
        uh2lob = MathUtils.assembleQuadratureConversion(QuadratureNodes, OtherQuadratureNodes, self.d["NUMBER_OF_DOF"])
        self.d["uh2lob"] = MathUtils.matrixPadAndFlatten_ColMajor(uh2lob,l_padSize)
        self.d["uh2lobSize"] = len(self.d["uh2lob"])
        self.d["uh2lob_seq"] = range(self.d["uh2lobSize"])

        l_padSize = self.d["NUMBER_OF_DOF_PADDED"] - self.d["NUMBER_OF_DOF"]
        dg2fv = MathUtils.assembleDGToFV(QuadratureNodes, QuadratureWeights, self.d["NUMBER_OF_DOF"], self.d["NUMBER_OF_DOF_LIMITER"])
        self.d["dg2fv"] = MathUtils.matrixPadAndFlatten_ColMajor(dg2fv,l_padSize)
        self.d["dg2fvSize"] = len(self.d["dg2fv"])
        self.d["dg2fv_seq"] = range(self.d["dg2fvSize"])

        l_padSize = self.d["NUMBER_OF_DOF_LIMITER_PADDED"] - self.d["NUMBER_OF_DOF_LIMITER"]
        fv2dg = MathUtils.assembleFVToDG(dg2fv, QuadratureWeights, self.d["NUMBER_OF_DOF"], self.d["NUMBER_OF_DOF_LIMITER"])
        self.d["fv2dg"] = MathUtils.matrixPadAndFlatten_ColMajor(fv2dg,l_padSize)
        self.d["fv2dgSize"] = len(self.d["fv2dg"])
        self.d["fv2dg_seq"] = range(self.d["fv2dgSize"])

        generated_kernels = peano4.output.Jinja2TemplatedHeaderImplementationFilePair(
            headfile_template=template_prefix + "Quadrature.template.h",
            cppfile_template=template_prefix + "Quadrature.template.cpp",
            classname=output_path + "/Quadrature",
            namespace=namespace,
            subdirectory=subdirectory + ".",
            dictionary=self.d,
            default_overwrite=True,
            apply_iteratively=True,
        )
        output.add(generated_kernels)
        output.makefile.add_h_file(output_path + "/Quadrature.h", generated=True)
        output.makefile.add_cpp_file(output_path + "/Quadrature.cpp", generated=True)
