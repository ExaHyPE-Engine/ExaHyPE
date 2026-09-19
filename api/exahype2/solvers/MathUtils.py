# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import mpmath as mp
import math
import sympy
import re


class MathUtils:
    @staticmethod
    def matrixTranspose(M):
        """Returns the transpose of matrix M."""
        return [[M[j][i] for j in range(len(M))] for i in range(len(M[0]))]

    @staticmethod
    def matrixDot(A, B):
        """Returns the product of matrices A and B."""
        return [
            [sum([A[n][k] * B[k][m] for k in range(len(B))]) for m in range(len(B[0]))]
            for n in range(len(A))
        ]

    @staticmethod
    def matrixInverse(M):
        """Returns the inverse of matrix M."""
        return MathUtils.matrixInverse_Pivot(M)

    @staticmethod
    def matrixInverse_Pivot(M):
        """Returns the inverse of matrix M through Gauss elimination with row pivoting."""
        n = len(M)
        # c holds the transpose of M beside the identity matrix.
        c = [[0.0 for _ in range(2 * n)] for _ in range(n)]
        for i in range(n):
            for j in range(n):
                c[i][j] = M[j][i]
                c[i][j + n] = 0.0
            c[i][i + n] = 1.0

        # Forward elimination; the row with the largest pivot moves up.
        for i in range(n):
            ml = i
            mlV = math.fabs(c[i][i])
            for j in range(i + 1, n):
                if math.fabs(c[j][i]) > mlV:
                    ml = j
                    mlV = math.fabs(c[j][i])
            for k in range(0, 2 * n):
                tmp = c[ml][k]
                c[ml][k] = c[i][k]
                c[i][k] = tmp

            if c[i][i] == 0.0:
                raise ValueError("The matrix is singular.")

            piv = 1.0 / c[i][i]
            for k in range(0, 2 * n):
                c[i][k] *= piv
            for j in range(i + 1, n):
                tmp = c[j][i]
                for k in range(0, 2 * n):
                    c[j][k] -= tmp * c[i][k]

        # Back substitution
        for i in range(n - 1, -1, -1):
            for j in range(i - 1, -1, -1):
                tmp = c[j][i]
                for k in range(0, 2 * n):
                    c[j][k] -= tmp * c[i][k]

        return [[c[i][j + n] for i in range(n)] for j in range(n)]

    @staticmethod
    def vectorPad(v, padSize):
        """Returns vector v extended by padSize zeros."""
        if padSize <= 0:
            return v
        return v + [0.0 for _ in range(padSize)]

    @staticmethod
    def matrixPadAndFlatten_RowMajor(M, padSize):
        """Returns the rows of M, each extended by padSize zeros, as one flat list."""
        result = []
        for i in range(len(M)):
            result += MathUtils.vectorPad(M[i], padSize)
        return result

    @staticmethod
    def matrixPadAndFlatten_ColMajor(M, padSize):
        """Returns the columns of M, each extended by padSize zeros, as one flat list."""
        return MathUtils.matrixPadAndFlatten_RowMajor(
            MathUtils.matrixTranspose(M), padSize
        )

    @staticmethod
    def baseFunc1D(xi, xin, N):
        """
        Computes the ADER-DG basis functions and their first derivative.

        Args:
           xi:
              The reference element point the basis functions are evaluated at.
              Here, xi refers to the greek letter that is often used as a reference element coordinate.
           xin:
              The reference element nodes corresponding to the nodal basis functions.
           N:
              Number of nodal basis functions (=order+1).
        Returns:
           phi:
              Basis function values.
           phi_xi:
              First derivatives of the basis functions.
        """
        phi = [1.0] * N
        phi_xi = [0.0] * N
        for m in range(0, N):
            for j in range(0, N):
                if j == m:
                    continue
                phi[m] = phi[m] * (xi - xin[j]) / (xin[m] - xin[j])
            for i in range(0, N):
                if i == m:
                    continue
                tmp = 1.0
                for j in range(0, N):
                    if j == i:
                        continue
                    if j == m:
                        continue
                    tmp = tmp * (xi - xin[j]) / (xin[m] - xin[j])
                phi_xi[m] += tmp / (xin[m] - xin[i])
        return phi, phi_xi

    @staticmethod
    def assembleStiffnessMatrix(xGPN, wGPN, N):
        """
        Computes the (reference) element stiffness matrix for an approximation of
        order N.

        Args:
           xGPN:
              Gauss-Legendre nodes (N nodes).
           wGPN:
              Gauss-Legendre weights (N weights).
           N:
              Number of nodal basis functions (=order+1).
        Returns:
           Kxi:
              The (reference) element stiffness matrix.
        """
        Kxi = [[0 for _ in range(N)] for _ in range(N)]

        for i in range(0, N):
            phi, phi_xi = MathUtils.baseFunc1D(xGPN[i], xGPN, N)
            for k in range(0, N):
                for l in range(0, N):
                    Kxi[k][l] += wGPN[i] * phi_xi[k] * phi[l]

        return Kxi

    @staticmethod
    def assembleK1(Kxi, xGPN, N):
        """
        Computes the difference between the reference element mass operator
        evaluated at point xi=1.0 and the element stiffness matrix.

        Args:
           Kxi:
              The (reference) element stiffness matrix.
           xGPN:
              Gauss-Legendre nodes (N nodes).
           N:
              Number of nodal basis functions (=order+1).
        Returns:
           K1:
              The mass operator at xi=1.0 minus the element stiffness matrix.
        """
        phi1, _ = MathUtils.baseFunc1D(1.0, xGPN, N)
        FRm = [[0 for _ in range(N)] for _ in range(N)]

        for k in range(0, N):
            for l in range(0, N):
                FRm[k][l] = phi1[k] * phi1[l]

        return [[FRm[i][j] - Kxi[i][j] for j in range(N)] for i in range(N)]

    @staticmethod
    def assembleMassMatrix(xGPN, wGPN, N):
        """
        Computes the (reference) element mass matrix for an approximation of
        order N.

        Args:
           xGPN:
              Gauss-Legendre nodes (N nodes).
           wGPN:
              Gauss-Legendre weights (N weights).
           N:
              Number of nodal basis functions (=order+1).
        Returns:
           MM:
              The (reference) element mass matrix.
        """
        MM = [[0 for _ in range(N)] for _ in range(N)]

        for i in range(0, N):
            phi, _ = MathUtils.baseFunc1D(xGPN[i], xGPN, N)
            for k in range(0, N):
                for l in range(0, N):
                    MM[k][l] += wGPN[i] * phi[k] * phi[l]

        return MM

    @staticmethod
    def assembleDiscreteDerivativeOperator(MM, Kxi):
        """
        Computes the discrete derivative operator MM^-1 * Kxi^T.

        Args:
           MM:
              The (reference) element mass matrix.
           Kxi:
              The (reference) element stiffness matrix.

        Returns:
           dudx:
              The discrete derivative operator.
        """
        dudx = MathUtils.matrixDot(
            MathUtils.matrixInverse(MM), MathUtils.matrixTranspose(Kxi)
        )
        return dudx

    @staticmethod
    def LagrangBasisPoly(x, order, i, xi=None):
        """
        Returns the i-th Lagrange basis polynomial as a symbolic expression.
        https://wanglongqi.github.io/python/2014/03/24/implement-of-lagrange-polynomial/
        describes the algorithm.

        Args:
           x:
              The symbolic variable of the polynomial.
           order:
              Order of the Lagrange basis polynomial.
           i:
              Index of the node at which the polynomial takes the value one.
           xi:
              The order+1 quadrature nodes.
        Returns:
           Li:
              The i-th Lagrange basis polynomial as a symbolic expression.
        """
        if xi is None:
            raise ValueError("LagrangBasisPoly needs the quadrature nodes xi.")
        index = list(range(order + 1))
        index.pop(i)
        symbolic_function = sympy.prod([(x - xi[j]) / (xi[i] - xi[j]) for j in index])
        return symbolic_function

    @staticmethod
    def assembleBasisFunction(quadNodes):
        """Returns the C++ definitions of the Lagrange basis functions of quadNodes."""
        order = len(quadNodes) - 1
        mp2 = mp.mp.clone()
        mp2.dps = 64
        x2 = quadNodes.copy()

        for i, xi in enumerate(x2):
            x2[i] = mp2.mpf(xi)

        res = ""

        for m in range(0, order + 1):
            s = sympy.symbols("s")
            res += "\ndouble basisFunction" + str(m) + "(const double& s) {\n"
            refphi = MathUtils.LagrangBasisPoly(s, order, m, x2)

            ret = "  return %s;" % sympy.simplify(refphi)
            ret = re.sub(r"s\*\*([0-9]+)", r"std::pow(s, \1)", ret)
            res += ret + "\n}\n"

        res += "\ndouble (*basisFunction[" + str(order + 1) + "])(const double& s) {\n"
        for m in range(0, order):
            res += "  basisFunction" + str(m) + ","
        res += "  basisFunction" + str(order) + "\n};\n"

        return res

    @staticmethod
    def assembleQuadratureConversion(fromQ, toQ, N):
        """Returns the matrix that converts values from the fromQ nodes to the toQ nodes."""
        conversionMat = [[0.0 for _ in range(N)] for _ in range(N)]
        for i in range(0, N):
            phi, _ = MathUtils.baseFunc1D(toQ[i], fromQ, N)
            for j in range(0, N):
                conversionMat[j][i] = phi[j]
        return conversionMat

    @staticmethod
    def assembleDGToFV(nodes, weights, N, Nlim):
        """Returns the matrix that converts the DG nodes into the Nlim finite volumes."""
        dg2fv = [[0.0 for _ in range(Nlim)] for _ in range(N)]
        dxi = 1.0 / float(Nlim)
        xLeft = 0.0
        xi = 0.0
        for i in range(0, Nlim):
            xLeft = i * dxi
            for j in range(0, N):
                xi = xLeft + dxi * nodes[j]
                phi, _ = MathUtils.baseFunc1D(xi, nodes, N)
                for k in range(0, N):
                    dg2fv[k][i] += weights[j] * phi[k]
        return dg2fv

    @staticmethod
    def assembleFVToDG(dg2fv, weights, N, Nlim):
        """Returns the matrix that converts the Nlim finite volumes into the DG nodes."""
        fv2dg = [[0.0 for _ in range(N)] for _ in range(Nlim)]
        lsqm = [[0.0 for _ in range(N + 1)] for _ in range(N + 1)]
        lsqrhs = [[0.0 for _ in range(Nlim)] for _ in range(N + 1)]

        dxi = 1.0 / float(Nlim)

        for i in range(0, N):
            for j in range(0, N):
                for k in range(0, Nlim):
                    lsqm[j][i] += 2 * dg2fv[i][k] * dg2fv[j][k]
            lsqm[N][i] = weights[i]
        for i in range(0, N):
            lsqm[i][N] = -weights[i]
        lsqm[N][N] = 0.0

        ilsqm = MathUtils.matrixInverse(lsqm)

        for i in range(0, Nlim):
            for j in range(0, N):
                lsqrhs[j][i] = 2 * dg2fv[j][i]
            lsqrhs[N][i] = dxi

        for i in range(0, Nlim):
            for j in range(0, N):
                for k in range(0, N + 1):
                    fv2dg[i][j] += ilsqm[j][k] * lsqrhs[k][i]

        return fv2dg
