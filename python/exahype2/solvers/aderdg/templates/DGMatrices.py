# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org

from .AbstractKernelTemplate import AbstractKernelTemplate

from .MathUtils import MathUtils

class DGMatrices(AbstractKernelTemplate):
    def generateCode(self):
        if self.context["quadratureType"] == "Gauss-Legendre":
            #get GL nodes
            wGPN, xGPN = MathUtils.getGaussLegendre(self.context["nDof"])
        elif self.context["quadratureType"] == "Gauss-Lobatto":
            #get GLL nodes
            wGPN, xGPN = MathUtils.getGaussLobatto(self.context["nDof"])
            xGPN = xGPN[::-1]
        else:
            raise ValueError("Quadrature type " + self.context["quadratureType"] + " not supported." )

        padSize = self.context["nDofPad"] - self.context["nDof"]
        self.context["nDofPad_seq"] = range(self.context["nDofPad"])
        self.context["nDofPadTimesnDof_seq"] = range(self.context["nDofPad"]*self.context["nDof"])

        # [FLCoeff 0...0]; [FRCoeff 0...0];
        # right now FLCoeff, FRCoeff no pad (gives no benefit w.r.t libxsmm)
        FLCoeff, _ = MathUtils.baseFunc1d(0.0, xGPN, self.context["nDof"]) #is also F0
        FRCoeff, _ = MathUtils.baseFunc1d(1.0, xGPN, self.context["nDof"])
        self.context["FLCoeff"] = MathUtils.vectorPad(FLCoeff, padSize)
        self.context["FRCoeff"] = MathUtils.vectorPad(FRCoeff, padSize)

        # Matrices are stored in column major order (so the padding should be on the bottom rows)
        # [ Mat ]
        # [0...0]
        # [0...0]
        # Kxi
        Kxi = MathUtils.assembleStiffnessMatrix(xGPN, wGPN, self.context["nDof"])
        self.context["Kxi"]   = MathUtils.matrixPadAndFlatten_ColMajor(Kxi,padSize)
        self.context["Kxi_T"] = MathUtils.matrixPadAndFlatten_RowMajor(Kxi,padSize) #transpose

        # iK1_T
        iK1 = MathUtils.matrixInverse(MathUtils.assembleK1(Kxi, xGPN, self.context["nDof"]))
        self.context["iK1_T"] = MathUtils.matrixPadAndFlatten_RowMajor(iK1,padSize) #transpose

        # dudx
        MM   = MathUtils.assembleMassMatrix(xGPN, wGPN, self.context["nDof"])
        dudx = MathUtils.assembleDiscreteDerivativeOperator(MM,Kxi)
        self.context["dudx"]   = MathUtils.matrixPadAndFlatten_ColMajor(dudx,padSize)
        self.context["dudx_T"] = MathUtils.matrixPadAndFlatten_RowMajor(dudx,padSize) #transpose

        #fineGridProjector1d
        fineGridProjector1d_0 = MathUtils.assembleFineGridProjector1d(xGPN, 0, self.context["nDof"])
        fineGridProjector1d_1 = MathUtils.assembleFineGridProjector1d(xGPN, 1, self.context["nDof"])
        fineGridProjector1d_2 = MathUtils.assembleFineGridProjector1d(xGPN, 2, self.context["nDof"])
        self.context["fineGridProjector1d_0"]   = MathUtils.matrixPadAndFlatten_ColMajor(fineGridProjector1d_0,padSize)
        self.context["fineGridProjector1d_1"]   = MathUtils.matrixPadAndFlatten_ColMajor(fineGridProjector1d_1,padSize)
        self.context["fineGridProjector1d_2"]   = MathUtils.matrixPadAndFlatten_ColMajor(fineGridProjector1d_2,padSize)

        #fineGridProjector1d_T_weighted
        for i in range(self.context["nDof"]):
            for j in range(self.context["nDof"]):
                fineGridProjector1d_0[i][j] *= wGPN[j]/wGPN[i]/3.0
                fineGridProjector1d_1[i][j] *= wGPN[j]/wGPN[i]/3.0
                fineGridProjector1d_2[i][j] *= wGPN[j]/wGPN[i]/3.0
        self.context["fineGridProjector1d_T_weighted_0"] = MathUtils.matrixPadAndFlatten_RowMajor(fineGridProjector1d_0,padSize)
        self.context["fineGridProjector1d_T_weighted_1"] = MathUtils.matrixPadAndFlatten_RowMajor(fineGridProjector1d_1,padSize)
        self.context["fineGridProjector1d_T_weighted_2"] = MathUtils.matrixPadAndFlatten_RowMajor(fineGridProjector1d_2,padSize)

        self.render("DGMatrices.template.h",       "DGMatrices.h")
        self.render("DGMatrices.template.cpp",     "DGMatrices.cpp")
