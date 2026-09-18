# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org


class Gemms:
    """Specification of a dense matrix-matrix multiplication.

        C       = alpha  *   A   *    B   + beta  *  C
     (M x N)             (M x K)  (K x N)

    LDA, LDB and LDC are the leading dimensions of the column major arrays A, B and C.
    """

    name = ""

    # Number of rows and columns of the matrices.
    M = -1
    N = -1
    K = -1

    # Leading dimensions of A, B and C.
    LDA = -1
    LDB = -1
    LDC = -1

    # Scalar factors; alpha is -1 or 1, beta is 0 or 1.
    alpha = 1
    beta = 1

    # Alignment flags; 1 marks an aligned array, 0 an unaligned one.
    alignment_A = 0
    alignment_C = 0

    def __init__(
        self,
        M,
        N,
        K,
        LDA,
        LDB,
        LDC,
        alpha,
        beta,
        alignment_A,
        alignment_C,
        name,
    ):
        if (M > LDC) or (K > LDB) or (M > LDA):
            raise ValueError("Matrix sizes M and K must not exceed LDA, LDB and LDC.")
        if alignment_A not in [0, 1]:
            raise ValueError("The alignment flag of matrix A must be 0 or 1.")
        if alignment_C not in [0, 1]:
            raise ValueError("The alignment flag of matrix C must be 0 or 1.")

        self.M = M
        self.N = N
        self.K = K
        self.LDA = LDA
        self.LDB = LDB
        self.LDC = LDC
        self.alpha = alpha
        self.beta = beta
        self.alignment_A = alignment_A
        self.alignment_C = alignment_C
        self.name = name

    def __repr__(self):
        return (
            "<%s: M=%s, N=%s, K=%s, LDA=%s, LDB=%s, LDC=%s, alpha=%s, beta=%s, alignment_A=%s, alignment_C=%s>"
            % (
                self.name,
                self.M,
                self.N,
                self.K,
                self.LDA,
                self.LDB,
                self.LDC,
                self.alpha,
                self.beta,
                self.alignment_A,
                self.alignment_C,
            )
        )
