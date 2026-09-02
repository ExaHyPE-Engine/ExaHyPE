# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
from copy import deepcopy

import peano4.output

from exahype2.solvers.MathUtils import MathUtils


def create_triadic_transfer_matrices(polynomial_type, number_of_dof):
    """Returns (prolongation, restriction) triples for the triadic subdivision.

    Each entry is one matrix per subinterval, indexed [coarse][fine], with the
    prolongation holding the coarse shape functions at the child's nodes and the
    restriction its mass-lumped transpose. Both triples are symmetrised so that
    mirror entries are identical to the last bit.
    """
    if polynomial_type == "legendre":
        weights, nodes = MathUtils.getGaussLegendre(number_of_dof)
    elif polynomial_type == "lobatto":
        weights, nodes = MathUtils.getGaussLobatto(number_of_dof)
        nodes = nodes[::-1]
    else:
        raise ValueError("Quadrature type " + polynomial_type + " not supported.")

    prolongation = [
        MathUtils.assembleFineGridProjector1D(nodes, interval, number_of_dof)
        for interval in range(3)
    ]
    _symmetriseTriadicProjectors(
        prolongation[0], prolongation[1], prolongation[2], number_of_dof
    )

    restriction = deepcopy(prolongation)
    for interval in range(3):
        for coarse in range(number_of_dof):
            for fine in range(number_of_dof):
                restriction[interval][coarse][fine] *= (
                    weights[fine] / weights[coarse] / 3.0
                )
    # The quadrature weights are palindromic, but the division is not symmetric
    # to the last bit, so the scaled operator is symmetrised again.
    _symmetriseTriadicProjectors(
        restriction[0], restriction[1], restriction[2], number_of_dof
    )
    return prolongation, restriction


def _symmetriseTriadicProjectors(p0, p1, p2, n):
    """
    Overwrite the three triadic sub-cell projectors to be mirror symmetric.

    The symmetry holds to the last bit: subinterval s is the reflection of
    subinterval 2-s with both node indices reversed (p_s[i][j] ==
    p_{2-s}[n-1-i][n-1-j]), and the central one (s=1) is self-mirror. Each mirror
    pair is replaced by its common average, so the stored doubles are identical
    under reflection at a perturbation of ~1e-16. A raw Lagrange evaluation
    satisfies the relation only to ~1e-15, because the product accumulates in a
    fixed node order and the mirror entry rounds differently.
    """
    for i in range(n):
        for j in range(n):
            outer0 = p0[i][j]
            outer2 = p2[n - 1 - i][n - 1 - j]
            avg02 = 0.5 * (outer0 + outer2)
            p0[i][j] = avg02
            p2[n - 1 - i][n - 1 - j] = avg02

            centre = p1[i][j]
            centreMirror = p1[n - 1 - i][n - 1 - j]
            avg1 = 0.5 * (centre + centreMirror)
            p1[i][j] = avg1
            p1[n - 1 - i][n - 1 - j] = avg1


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
        self.d["nDofPad_seq"] = range(self.d["NUMBER_OF_DOF_PADDED"])
        self.d["nDofPadTimesnDof_seq"] = range(
            self.d["NUMBER_OF_DOF_PADDED"] * self.d["NUMBER_OF_DOF"]
        )
        self.d["nDofTimesnDof_seq"] = range(
            self.d["NUMBER_OF_DOF"] * self.d["NUMBER_OF_DOF"]
        )

        # [FLCoeff 0...0]; [FRCoeff 0...0];
        FLCoeff, _ = MathUtils.baseFunc1D(
            0.0, xGPN, self.d["NUMBER_OF_DOF"]
        )  # Also known as F0
        FRCoeff, _ = MathUtils.baseFunc1D(1.0, xGPN, self.d["NUMBER_OF_DOF"])
        self.d["FLCoeff"] = MathUtils.vectorPad(FLCoeff, padSize)
        self.d["FRCoeff"] = MathUtils.vectorPad(FRCoeff, padSize)

        # Matrices are stored in column major order (so the padding should be on the bottom rows)
        # [ Mat ]
        # [0...0]
        # [0...0]
        Kxi = MathUtils.assembleStiffnessMatrix(xGPN, wGPN, self.d["NUMBER_OF_DOF"])
        self.d["Kxi"] = MathUtils.matrixPadAndFlatten_ColMajor(Kxi, padSize)
        self.d["Kxi_T"] = MathUtils.matrixPadAndFlatten_RowMajor(Kxi, padSize)

        iK1 = MathUtils.matrixInverse(
            MathUtils.assembleK1(Kxi, xGPN, self.d["NUMBER_OF_DOF"])
        )
        self.d["iK1_T"] = MathUtils.matrixPadAndFlatten_RowMajor(iK1, padSize)

        MM = MathUtils.assembleMassMatrix(xGPN, wGPN, self.d["NUMBER_OF_DOF"])
        dudx = MathUtils.assembleDiscreteDerivativeOperator(MM, Kxi)
        self.d["dudx"] = MathUtils.matrixPadAndFlatten_ColMajor(dudx, 0)
        self.d["dudx_T"] = MathUtils.matrixPadAndFlatten_RowMajor(dudx, padSize)

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
