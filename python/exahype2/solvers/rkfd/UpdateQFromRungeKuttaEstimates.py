# ButcherTableauWeights is a constant matrix passed as a double* template parameter
# RKStep is a 0 indexed integer passed as a template indicating which sub step we are on
def updateQFromRungeKuttaEstimates(oldQ: "double*", estimates: "double*", timestep: "double") -> "void":
    oldQ = oldQ + timestep * dot(ButcherTableauWeights[RKStep], estimates[RKStep])