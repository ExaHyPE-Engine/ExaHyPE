# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
from copy import deepcopy

import peano4.output

from .MathUtils import MathUtils
from ..Architectures import Architectures


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
        elif ["POLYNOMIAL_TYPE"] == "lobatto":
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

        weightsVector = MathUtils.vectorPad(
            QuadratureWeights, self.d["NUMBER_OF_DOF_PADDED"] - self.d["NUMBER_OF_DOF"]
        )
        self.d["weights1"] = weightsVector
        self.d["w1Size"] = len(self.d["weights1"])
        self.d["w1_seq"] = range(self.d["w1Size"])

        # inverse of weights1
        self.d["iweights1"] = [
            1.0 / self.d["weights1"][i] if self.d["weights1"][i] != 0.0 else 0.0
            for i in self.d["w1_seq"]
        ]

        if self.d["DIMENSIONS"] == 2:
            # weightsVector is QuadratureWeights itself
            weightsVector = MathUtils.vectorPad(
                QuadratureWeights,
                Architectures.get_pad_size(
                    self.d["ARCHITECTURE"], len(QuadratureWeights)
                ),
            )
            self.d["weights2"] = weightsVector
            self.d["w2Size"] = len(self.d["weights2"])
            self.d["w2_seq"] = range(self.d["w2Size"])

            # all combinations of two weights, written as an 1D array
            weightsVector = [
                QuadratureWeights[i] * QuadratureWeights[j]
                for i in range(self.d["NUMBER_OF_DOF"])
                for j in range(self.d["NUMBER_OF_DOF"])
            ]
            weightsVector = MathUtils.vectorPad(
                weightsVector,
                Architectures.get_pad_size(self.d["ARCHITECTURE"], len(weightsVector)),
            )
            self.d["weights3"] = weightsVector
            self.d["w3Size"] = len(self.d["weights3"])
            self.d["w3_seq"] = range(self.d["w3Size"])

            # all combinations of three weights, written as an 1D array
            weightsVector = [
                QuadratureWeights[i] * QuadratureWeights[j] * QuadratureWeights[k]
                for i in range(self.d["NUMBER_OF_DOF"])
                for j in range(self.d["NUMBER_OF_DOF"])
                for k in range(self.d["NUMBER_OF_DOF"])
            ]
            weightsVector = MathUtils.vectorPad(
                weightsVector,
                Architectures.get_pad_size(self.d["ARCHITECTURE"], len(weightsVector)),
            )
            self.d["weights4"] = weightsVector
            self.d["w4Size"] = len(self.d["weights4"])
            self.d["w4_seq"] = range(self.d["w4Size"])
        else:
            # all combinations of two weights, written as an 1D array
            weightsVector = [
                QuadratureWeights[i] * QuadratureWeights[j]
                for i in range(self.d["NUMBER_OF_DOF"])
                for j in range(self.d["NUMBER_OF_DOF"])
            ]
            weightsVector = MathUtils.vectorPad(
                weightsVector,
                Architectures.get_pad_size(self.d["ARCHITECTURE"], len(weightsVector)),
            )
            self.d["weights2"] = weightsVector
            self.d["w2Size"] = len(self.d["weights2"])
            self.d["w2_seq"] = range(self.d["w2Size"])

            # all combination of three weights, written as an 1D array
            weightsVector = [
                QuadratureWeights[i] * QuadratureWeights[j] * QuadratureWeights[k]
                for i in range(self.d["NUMBER_OF_DOF"])
                for j in range(self.d["NUMBER_OF_DOF"])
                for k in range(self.d["NUMBER_OF_DOF"])
            ]
            weightsVector = MathUtils.vectorPad(
                weightsVector,
                Architectures.get_pad_size(self.d["ARCHITECTURE"], len(weightsVector)),
            )
            self.d["weights3"] = weightsVector
            self.d["w3Size"] = len(self.d["weights3"])
            self.d["w3_seq"] = range(self.d["w3Size"])

            # all combination of four weights, written as an 1D array
            weightsVector = [
                QuadratureWeights[i]
                * QuadratureWeights[j]
                * QuadratureWeights[k]
                * QuadratureWeights[l]
                for i in range(self.d["NUMBER_OF_DOF"])
                for j in range(self.d["NUMBER_OF_DOF"])
                for k in range(self.d["NUMBER_OF_DOF"])
                for l in range(self.d["NUMBER_OF_DOF"])
            ]
            weightsVector = MathUtils.vectorPad(
                weightsVector,
                Architectures.get_pad_size(self.d["ARCHITECTURE"], len(weightsVector)),
            )
            self.d["weights4"] = weightsVector
            self.d["w4Size"] = len(self.d["weights4"])
            self.d["w4_seq"] = range(self.d["w4Size"])

        # inverse of weight3
        self.d["iweights3"] = [
            1.0 / self.d["weights3"][i] if self.d["weights3"][i] != 0.0 else 0.0
            for i in self.d["w3_seq"]
        ]

        self.d["QuadratureWeights"], self.d["QuadratureNodes"] = (
            QuadratureWeights,
            QuadratureNodes,
        )
        self.d["quadrature_seq"] = range(self.d["NUMBER_OF_DOF"])

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
