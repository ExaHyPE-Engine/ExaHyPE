# The following parameters shoule be set when calling Parser().parse(...)

# Parameter added for loop parallelisation (set in parser)
"""
loopParallelism: "peano4::utils::LoopPlacement"
"""

# this solver is specialized for ExaGRyPE (numerical relativity code on ExaHyPE), so basically can not be treated as an
# general finite difference solver implementation.
# we do not scale timestep size as this kernel go with RK schme, so always use 1 instead of dt
# finite differnces for certain variables will be tuned accordingly for accuracy
# KO disspation is added with a coefficient

def DerivativesCalInExaGRyPE(patchData: "CellData<double, double>&") -> "void":
    N = Integer("patchData.numberOfCells")
    t = DataBlock([[0, N]], "patchData.t", "double")
    dt = DataBlock([[0, N]], "patchData.dt", "double")
    h = DataBlock([[0, N]], "patchData.cellSize", "tarch::la::Vector<3, double>")
    x = DataBlock([[0, N]], "patchData.cellCentre", "tarch::la::Vector<3, double>")

    #H: HaloSize, NC:NumberOfCellsPerAxisInPatch
    input = DataBlock([[0, NumberOfUnknowns + NumberOfAuxiliaryVariables], [-H, NC + H], [-H, NC + H], [-H, NC + H], [0, N]], "patchData.QIn")
    output = DataBlock([[0, NumberOfUnknowns], [0, NC], [0, NC], [0, NC], [0, N]], "patchData.QOut")
    
    if SecondOrderFormulation:
      #A[normal] = \partial_{normal} \alpha
      #alpha is Q[16]
      input[23, 0:NC, 0:NC, 0:NC] = (1/12.0) * NC / h(0) * (8*input[16, 1:NC+1, 0:NC, 0:NC] - 8*input[16, -1:NC-1, 0:NC, 0:NC] + 1*input[16, -2:NC-2, 0:NC, 0:NC] - 1*input[16, 2:NC+2, 0:NC, 0:NC])
      input[24, 0:NC, 0:NC, 0:NC] = (1/12.0) * NC / h(0) * (8*input[16, 0:NC, 1:NC+1, 0:NC] - 8*input[16, 0:NC, -1:NC-1, 0:NC] + 1*input[16, 0:NC, -2:NC-2, 0:NC] - 1*input[16, 0:NC, 2:NC+2, 0:NC])
      input[25, 0:NC, 0:NC, 0:NC] = (1/12.0) * NC / h(0) * (8*input[16, 0:NC, 0:NC, 1:NC+1] - 8*input[16, 0:NC, 0:NC, -1:NC-1] + 1*input[16, 0:NC, 0:NC, -2:NC-2] - 1*input[16, 0:NC, 0:NC, 2:NC+2])

      #B[normal][i] = \partial_{normal}\beta^i
      #beta^i is Q[17] - Q[19]
      input[26:29, 0:NC, 0:NC, 0:NC] = (1/12.0) * NC / h(0) * (8*input[17:20, 1:NC+1, 0:NC, 0:NC] - 8*input[17:20, -1:NC-1, 0:NC, 0:NC] + 1*input[17:20, -2:NC-2, 0:NC, 0:NC] - 1*input[17:20, 2:NC+2, 0:NC, 0:NC])
      input[29:32, 0:NC, 0:NC, 0:NC] = (1/12.0) * NC / h(0) * (8*input[17:20, 0:NC, 1:NC+1, 0:NC] - 8*input[17:20, 0:NC, -1:NC-1, 0:NC] + 1*input[17:20, 0:NC, -2:NC-2, 0:NC] - 1*input[17:20, 0:NC, 2:NC+2, 0:NC])
      input[32:35, 0:NC, 0:NC, 0:NC] = (1/12.0) * NC / h(0) * (8*input[17:20, 0:NC, 0:NC, 1:NC+1] - 8*input[17:20, 0:NC, 0:NC, -1:NC-1] + 1*input[17:20, 0:NC, 0:NC, -2:NC-2] - 1*input[17:20, 0:NC, 0:NC, 2:NC+2])

      #D[normal][i][j] = 0.5*\partial_{normal}\tilde{\gamma}_{ij} -- note the 1/2 factor
      #\tilde{\gamma}_{ij} is Q[0] - Q[5]
      input[35:41, 0:NC, 0:NC, 0:NC] = (1/24.0) * NC / h(0) * (8*input[0:6, 1:NC+1, 0:NC, 0:NC] - 8*input[0:6, -1:NC-1, 0:NC, 0:NC] + 1*input[0:6, -2:NC-2, 0:NC, 0:NC] - 1*input[0:6, 2:NC+2, 0:NC, 0:NC])
      input[41:47, 0:NC, 0:NC, 0:NC] = (1/24.0) * NC / h(0) * (8*input[0:6, 0:NC, 1:NC+1, 0:NC] - 8*input[0:6, 0:NC, -1:NC-1, 0:NC] + 1*input[0:6, 0:NC, -2:NC-2, 0:NC] - 1*input[0:6, 0:NC, 2:NC+2, 0:NC])
      input[47:53, 0:NC, 0:NC, 0:NC] = (1/24.0) * NC / h(0) * (8*input[0:6, 0:NC, 0:NC, 1:NC+1] - 8*input[0:6, 0:NC, 0:NC, -1:NC-1] + 1*input[0:6, 0:NC, 0:NC, -2:NC-2] - 1*input[0:6, 0:NC, 0:NC, 2:NC+2])

      #P[normal] = \partial_{normal}\phi
      #phi is Q[54]
      input[55, 0:NC, 0:NC, 0:NC] = (1/12.0) * NC / h(0) * (8*input[54, 1:NC+1, 0:NC, 0:NC] - 8*input[54, -1:NC-1, 0:NC, 0:NC] + 1*input[54, -2:NC-2, 0:NC, 0:NC] - 1*input[54, 2:NC+2, 0:NC, 0:NC])
      input[56, 0:NC, 0:NC, 0:NC] = (1/12.0) * NC / h(0) * (8*input[54, 0:NC, 1:NC+1, 0:NC] - 8*input[54, 0:NC, -1:NC-1, 0:NC] + 1*input[54, 0:NC, -2:NC-2, 0:NC] - 1*input[54, 0:NC, 2:NC+2, 0:NC])
      input[57, 0:NC, 0:NC, 0:NC] = (1/12.0) * NC / h(0) * (8*input[54, 0:NC, 0:NC, 1:NC+1] - 8*input[54, 0:NC, 0:NC, -1:NC-1] + 1*input[54, 0:NC, 0:NC, -2:NC-2] - 1*input[54, 0:NC, 0:NC, 2:NC+2])

    output = input[0:NumberOfUnknowns, 0:NC, 0:NC, 0:NC]


    