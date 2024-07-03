# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org


class Gemms:
    """Specification of a dense matrix-matrix multiplication

        C       = alpha  *   A   *    B   + beta  *  C
     (M x N)             (M x K)  (K x N)

    for (int it_1 = 0; it_1 < K; it_1++) {
      for (int it_2 = 0; it_2 < N; it_2++) {
        for (int it_3 = 0; it_3 < M; it_3++) {
          C[it_1*LDC+it_3] = alpha * A[it_2*LDA+it_3] * B[it_1*LDB+it_2] + beta * C[it_1*LDC+it_3];
        }
      }
    }
    """

    PrecisionTranslate = {
        "DP": "DP",
        "double": "DP",
        "SP": "SP",
        "float": "SP",
        "single": "SP",
        "_Float16": "HP",
        "std::float16_t": "HP",
        "__bf16": "HP",
        "std::bfloat16_t": "HP",
        "half_float::half": "HP",
    }

    # dgemm, dgemv, ....
    operationType = ""

    baseroutinename = ""

    name = ""

    # dimension of the matrices
    M = -1
    N = -1
    K = -1

    # leading dimension of A, B, and C
    LDA = -1
    LDB = -1
    LDC = -1

    # scalars
    alpha = 1
    # -1, 1
    beta = 1
    #  0, 1

    # alignment flags
    alignment_A = 0  # 1 aligned, 0 unaligned
    alignment_C = 0  # 1 aligned, 0 unaligned

    # prefetching
    prefetchStrategy = ""

    # precision
    precision = "DP"  # "DP" = double, "SP" = float

    # Constructor
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
        prefetchStrategy,
        operationType="gemm",
        precision=["DP"],
    ):
        if type(precision) is not list:
            precision = [precision]
        for key in precision:
            if key not in self.PrecisionTranslate.keys():
                raise ("Unknown precision")
        if (M > LDC) or (K > LDB) or (M > LDA):
            raise ("Incompatible matrix sizes and leading dimensions")
        if alignment_A not in [0, 1]:
            raise ("Something is wrong with the alignment choice of matrix A")
        if alignment_C not in [0, 1]:
            raise ("Something is wrong with the alignment choice of matrix C")

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
        self.prefetchStrategy = prefetchStrategy
        self.baseroutinename = (
            operationType + "_" + str(M) + "_" + str(N) + "_" + str(K) + "_" + name
        )
        self.precision = [self.PrecisionTranslate[key] for key in precision]

    def __repr__(self):
        return (
            "<%s: %s LDA=%s, LDB=%s, LDC=%s, alpha=%s, beta=%s, alignment_A=%s, alignment_C=%s>"
            % (
                self.name,
                self.baseroutinename,
                self.LDA,
                self.LDB,
                self.LDC,
                self.alpha,
                self.beta,
                self.alignment_A,
                self.alignment_C,
            )
        )
