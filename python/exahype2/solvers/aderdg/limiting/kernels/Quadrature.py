        if(self.context["kernelType"]=="limiter"):
            l_padSize = self.context["nDofPad"] - self.context["nDof"]
            uh2lob = MathUtils.assembleQuadratureConversion(QuadratureNodes, OtherQuadratureNodes, self.context["nDof"]) #TODO JMG adapt when allowing Lobatto as node
            self.context["uh2lob"] = MathUtils.matrixPadAndFlatten_ColMajor(uh2lob,l_padSize)
            self.context["uh2lobSize"] = len(self.context["uh2lob"])
            self.context["uh2lob_seq"] = range(self.context["uh2lobSize"])

            l_padSize = self.context["nDofPad"] - self.context["nDof"]
            dg2fv = MathUtils.assembleDGToFV(QuadratureNodes, QuadratureWeights, self.context["nDof"], self.context["nDofLim"])
            self.context["dg2fv"] = MathUtils.matrixPadAndFlatten_ColMajor(dg2fv,l_padSize)
            self.context["dg2fvSize"] = len(self.context["dg2fv"])
            self.context["dg2fv_seq"] = range(self.context["dg2fvSize"])

            l_padSize = self.context["nDofLimPad"] - self.context["nDofLim"]
            fv2dg = MathUtils.assembleFVToDG(dg2fv, QuadratureWeights, self.context["nDof"], self.context["nDofLim"])
            self.context["fv2dg"] = MathUtils.matrixPadAndFlatten_ColMajor(fv2dg,l_padSize)
            self.context["fv2dgSize"] = len(self.context["fv2dg"])
            self.context["fv2dg_seq"] = range(self.context["fv2dgSize"])