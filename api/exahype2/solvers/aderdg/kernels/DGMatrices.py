# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
from copy import deepcopy

import peano4.output

from exahype2.solvers.MathUtils import MathUtils


class DGMatrices(object):
    def __init__(self, dictionary):
        self.d = deepcopy(dictionary)

    def generate_kernels(self, solver, namespace, output, template_prefix, output_path):
        if self.d["POLYNOMIAL_TYPE"] == "legendre":
            wGPN, xGPN = MathUtils.getGaussLegendre(self.d["NUMBER_OF_DOF"])
        elif self.d["POLYNOMIAL_TYPE"] == "lobatto":
            wGPN, xGPN = MathUtils.getGaussLobatto(self.d["NUMBER_OF_DOF"])
            xGPN = xGPN[::-1]
        else:
            raise ValueError(
                "Quadrature type " + self.d["POLYNOMIAL_TYPE"] + " not supported."
            )

        padSize = self.d["NUMBER_OF_DOF_PADDED"] - self.d["NUMBER_OF_DOF"]
        self.d["nDofPadRange"] = range(self.d["NUMBER_OF_DOF_PADDED"])
        self.d["nDofPadTimesnDofRange"] = range(
            self.d["NUMBER_OF_DOF_PADDED"] * self.d["NUMBER_OF_DOF"]
        )
        self.d["nDofTimesnDofRange"] = range(
            self.d["NUMBER_OF_DOF"] * self.d["NUMBER_OF_DOF"]
        )

        # The ADER-DG literature names FLCoeff F0.
        FLCoeff, _ = MathUtils.baseFunc1D(0.0, xGPN, self.d["NUMBER_OF_DOF"])
        FRCoeff, _ = MathUtils.baseFunc1D(1.0, xGPN, self.d["NUMBER_OF_DOF"])
        self.d["FLCoeff"] = MathUtils.vectorPad(FLCoeff, padSize)
        self.d["FRCoeff"] = MathUtils.vectorPad(FRCoeff, padSize)

        # The matrices are stored in column major order, so the padding fills the bottom rows.
        Kxi = MathUtils.assembleStiffnessMatrix(xGPN, wGPN, self.d["NUMBER_OF_DOF"])
        self.d["Kxi"] = MathUtils.matrixPadAndFlatten_ColMajor(Kxi, padSize)
        self.d["KxiTransposed"] = MathUtils.matrixPadAndFlatten_RowMajor(Kxi, padSize)

        iK1 = MathUtils.matrixInverse(
            MathUtils.assembleK1(Kxi, xGPN, self.d["NUMBER_OF_DOF"])
        )
        self.d["iK1Transposed"] = MathUtils.matrixPadAndFlatten_RowMajor(iK1, padSize)

        MM = MathUtils.assembleMassMatrix(xGPN, wGPN, self.d["NUMBER_OF_DOF"])
        dudx = MathUtils.assembleDiscreteDerivativeOperator(MM, Kxi)
        self.d["dudx"] = MathUtils.matrixPadAndFlatten_ColMajor(dudx, 0)
        self.d["dudxTransposed"] = MathUtils.matrixPadAndFlatten_RowMajor(dudx, padSize)

        generated_kernels = peano4.output.Jinja2TemplatedHeaderImplementationFilePair(
            headfile_template=template_prefix + "DGMatrices.template.h",
            cppfile_template=template_prefix + "DGMatrices.template.cpp",
            classname=output_path + "/DGMatrices",
            namespace=namespace,
            subdirectory=".",
            dictionary=self.d,
            apply_iteratively=True,
        )
        output.add(generated_kernels)
        output.makefile.add_h_file(output_path + "/DGMatrices.h", generated=True)
        output.makefile.add_cpp_file(output_path + "/DGMatrices.cpp", generated=True)
        solver._user_action_set_includes += f'#include "{output_path}/DGMatrices.h"\n'
