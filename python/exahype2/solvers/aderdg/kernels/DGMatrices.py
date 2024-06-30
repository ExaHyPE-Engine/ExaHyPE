# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
from copy import deepcopy

import peano4.output

from .MathUtils import MathUtils


class DGMatrices(object):
    def __init__(self, dictionary):
        self.d = deepcopy(dictionary)

    def generate_kernels(
        self, namespace, output, subdirectory, template_prefix, output_path
    ):
        if self.d["POLYNOMIAL_TYPE"] == "legendre":
            # Get GL nodes
            wGPN, xGPN = MathUtils.getGaussLegendre(self.d["NUMBER_OF_DOF"])
        elif self.d["POLYNOMIAL_TYPE"] == "lobatto":
            # Get GLL nodes
            wGPN, xGPN = MathUtils.getGaussLobatto(self.d["NUMBER_OF_DOF"])
            xGPN = xGPN[::-1]
        else:
            raise ValueError(
                "Quadrature type " + self.d["POLYNOMIAL_TYPE"] + " not supported."
            )

        padSize = self.d["NUMBER_OF_DOF_PADDED"] - self.d["NUMBER_OF_DOF"]
        self.d["nDofPad_seq"] = range(self.d["NUMBER_OF_DOF_PADDED"])
        self.d["nDofPadTimesnDof_seq"] = range(
            self.d["NUMBER_OF_DOF_PADDED"] * self.d["NUMBER_OF_DOF"]
        )

        # [FLCoeff 0...0]; [FRCoeff 0...0];
        # right now FLCoeff, FRCoeff no pad (gives no benefit w.r.t libxsmm)
        FLCoeff, _ = MathUtils.baseFunc1d(
            0.0, xGPN, self.d["NUMBER_OF_DOF"]
        )  # is also F0
        FRCoeff, _ = MathUtils.baseFunc1d(1.0, xGPN, self.d["NUMBER_OF_DOF"])
        self.d["FLCoeff"] = MathUtils.vectorPad(FLCoeff, padSize)
        self.d["FRCoeff"] = MathUtils.vectorPad(FRCoeff, padSize)

        # Matrices are stored in column major order (so the padding should be on the bottom rows)
        # [ Mat ]
        # [0...0]
        # [0...0]
        # Kxi
        Kxi = MathUtils.assembleStiffnessMatrix(xGPN, wGPN, self.d["NUMBER_OF_DOF"])
        self.d["Kxi"] = MathUtils.matrixPadAndFlatten_ColMajor(Kxi, padSize)
        self.d["Kxi_T"] = MathUtils.matrixPadAndFlatten_RowMajor(
            Kxi, padSize
        )  # transpose

        # iK1_T
        iK1 = MathUtils.matrixInverse(
            MathUtils.assembleK1(Kxi, xGPN, self.d["NUMBER_OF_DOF"])
        )
        self.d["iK1_T"] = MathUtils.matrixPadAndFlatten_RowMajor(
            iK1, padSize
        )  # transpose

        # dudx
        MM = MathUtils.assembleMassMatrix(xGPN, wGPN, self.d["NUMBER_OF_DOF"])
        dudx = MathUtils.assembleDiscreteDerivativeOperator(MM, Kxi)
        self.d["dudx"] = MathUtils.matrixPadAndFlatten_ColMajor(dudx, padSize)
        self.d["dudx_T"] = MathUtils.matrixPadAndFlatten_RowMajor(
            dudx, padSize
        )  # transpose

        # fineGridProjector1d
        fineGridProjector1d_0 = MathUtils.assembleFineGridProjector1d(
            xGPN, 0, self.d["NUMBER_OF_DOF"]
        )
        fineGridProjector1d_1 = MathUtils.assembleFineGridProjector1d(
            xGPN, 1, self.d["NUMBER_OF_DOF"]
        )
        fineGridProjector1d_2 = MathUtils.assembleFineGridProjector1d(
            xGPN, 2, self.d["NUMBER_OF_DOF"]
        )
        self.d["fineGridProjector1d_0"] = MathUtils.matrixPadAndFlatten_ColMajor(
            fineGridProjector1d_0, padSize
        )
        self.d["fineGridProjector1d_1"] = MathUtils.matrixPadAndFlatten_ColMajor(
            fineGridProjector1d_1, padSize
        )
        self.d["fineGridProjector1d_2"] = MathUtils.matrixPadAndFlatten_ColMajor(
            fineGridProjector1d_2, padSize
        )

        # fineGridProjector1d_T_weighted
        for i in range(self.d["NUMBER_OF_DOF"]):
            for j in range(self.d["NUMBER_OF_DOF"]):
                fineGridProjector1d_0[i][j] *= wGPN[j] / wGPN[i] / 3.0
                fineGridProjector1d_1[i][j] *= wGPN[j] / wGPN[i] / 3.0
                fineGridProjector1d_2[i][j] *= wGPN[j] / wGPN[i] / 3.0
        self.d[
            "fineGridProjector1d_T_weighted_0"
        ] = MathUtils.matrixPadAndFlatten_RowMajor(fineGridProjector1d_0, padSize)
        self.d[
            "fineGridProjector1d_T_weighted_1"
        ] = MathUtils.matrixPadAndFlatten_RowMajor(fineGridProjector1d_1, padSize)
        self.d[
            "fineGridProjector1d_T_weighted_2"
        ] = MathUtils.matrixPadAndFlatten_RowMajor(fineGridProjector1d_2, padSize)

        generated_kernels = peano4.output.Jinja2TemplatedHeaderImplementationFilePair(
            headfile_template=template_prefix + "DGMatrices.template.h",
            cppfile_template=template_prefix + "DGMatrices.template.cpp",
            classname=output_path + "/DGMatrices",
            namespace=namespace,
            subdirectory=subdirectory + ".",
            dictionary=self.d,
            default_overwrite=True,
            apply_iteratively=True,
        )
        output.add(generated_kernels)
        output.makefile.add_h_file(output_path + "/DGMatrices.h", generated=True)
        output.makefile.add_cpp_file(output_path + "/DGMatrices.cpp", generated=True)
