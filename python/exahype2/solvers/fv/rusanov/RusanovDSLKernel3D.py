# The following parameters shoule be set when calling Parser().parse(...)

# Parameters used to generate non stateless code (set in parser)
"""
flux: "const fluxFunctor&",                   
nonconservativeProduct: "const nonconservativeProductFunctor&", 
sourceTerm: "const sourceFunctor&",
maxEigenvalue: "const maxEigenvalueFunctor&",   
"""

# Parameter added for loop parallelisation (set in parser)
"""
loopParallelism: "peano4::utils::LoopPlacement"
"""

def timeStepWithRusanov(patchData: "CellData<double, double>&") -> "void":
    N = Integer("patchData.numberOfCells")
    t = DataBlock([[0, N]], "patchData.t", "double")
    dt = DataBlock([[0, N]], "patchData.dt", "double")
    h = DataBlock([[0, N]], "patchData.cellSize", "tarch::la::Vector<3, double>")
    x = DataBlock([[0, N]], "patchData.cellCentre", "tarch::la::Vector<3, double>")
    maxEigenvalues = DataBlock([[0, N]], "patchData.maxEigenvalue", "double")

    input = DataBlock([[0, NumberOfUnknowns + NumberOfAuxiliaryVariables], [-HaloSize, NumberOfVolumesPerAxisInPatch + HaloSize], [-HaloSize, NumberOfVolumesPerAxisInPatch + HaloSize], [-HaloSize, NumberOfVolumesPerAxisInPatch + HaloSize], [0, N]], "patchData.QIn")
    output = DataBlock([[0, NumberOfUnknowns + NumberOfAuxiliaryVariables], [0, NumberOfVolumesPerAxisInPatch], [0, NumberOfVolumesPerAxisInPatch], [0, NumberOfVolumesPerAxisInPatch], [0, N]], "patchData.QOut")
    output = input[:, 0:NumberOfVolumesPerAxisInPatch, 0:NumberOfVolumesPerAxisInPatch, 0:NumberOfVolumesPerAxisInPatch]

    if EvaluateSource:
       source = DataBlock([[0, NumberOfUnknowns + NumberOfAuxiliaryVariables], [0, NumberOfVolumesPerAxisInPatch], [0, NumberOfVolumesPerAxisInPatch], [0, NumberOfVolumesPerAxisInPatch], [0, N]], eval("sourceTerm", input[:, 0:NumberOfVolumesPerAxisInPatch, 0:NumberOfVolumesPerAxisInPatch, 0:NumberOfVolumesPerAxisInPatch], x, h, t, dt))
       output = output + dt * source

    #logDataBlock(input[:, :, :, :, 0], "Input.txt")
    maxEigenvaluesX = DataBlock([[0, 1], [-HaloSize, NumberOfVolumesPerAxisInPatch + HaloSize], [-HaloSize, NumberOfVolumesPerAxisInPatch + HaloSize], [-HaloSize, NumberOfVolumesPerAxisInPatch + HaloSize], [0, N]], eval("maxEigenvalue", input, x, h, t, dt, 0))
    maxEigenvaluesY = DataBlock([[0, 1], [-HaloSize, NumberOfVolumesPerAxisInPatch + HaloSize], [-HaloSize, NumberOfVolumesPerAxisInPatch + HaloSize], [-HaloSize, NumberOfVolumesPerAxisInPatch + HaloSize], [0, N]], eval("maxEigenvalue", input, x, h, t, dt, 1))
    maxEigenvaluesZ = DataBlock([[0, 1], [-HaloSize, NumberOfVolumesPerAxisInPatch + HaloSize], [-HaloSize, NumberOfVolumesPerAxisInPatch + HaloSize], [-HaloSize, NumberOfVolumesPerAxisInPatch + HaloSize], [0, N]], eval("maxEigenvalue", input, x, h, t, dt, 2))
    #logDataBlock(maxEigenvaluesX[:, :, :, :, 0], "EigenX.txt")
    
    lambdaLeft = max(maxEigenvaluesX[0:1, -1:NumberOfVolumesPerAxisInPatch-1, 0:NumberOfVolumesPerAxisInPatch, 0:NumberOfVolumesPerAxisInPatch], maxEigenvaluesX[0:1, 0:NumberOfVolumesPerAxisInPatch, 0:NumberOfVolumesPerAxisInPatch, 0:NumberOfVolumesPerAxisInPatch])
    lambdaRight = max(maxEigenvaluesX[0:1, 0:NumberOfVolumesPerAxisInPatch, 0:NumberOfVolumesPerAxisInPatch, 0:NumberOfVolumesPerAxisInPatch], maxEigenvaluesX[0:1, 1:NumberOfVolumesPerAxisInPatch + 1, 0:NumberOfVolumesPerAxisInPatch, 0:NumberOfVolumesPerAxisInPatch])
    lambdaBottom = max(maxEigenvaluesY[0:1, 0:NumberOfVolumesPerAxisInPatch, -1:NumberOfVolumesPerAxisInPatch-1, 0:NumberOfVolumesPerAxisInPatch], maxEigenvaluesY[0:1, 0:NumberOfVolumesPerAxisInPatch, 0:NumberOfVolumesPerAxisInPatch, 0:NumberOfVolumesPerAxisInPatch])
    lambdaTop = max(maxEigenvaluesY[0:1, 0:NumberOfVolumesPerAxisInPatch, 0:NumberOfVolumesPerAxisInPatch, 0:NumberOfVolumesPerAxisInPatch], maxEigenvaluesY[0:1, 0:NumberOfVolumesPerAxisInPatch, 1:NumberOfVolumesPerAxisInPatch + 1, 0:NumberOfVolumesPerAxisInPatch])
    lambdaNear = max(maxEigenvaluesZ[0:1, 0:NumberOfVolumesPerAxisInPatch, 0:NumberOfVolumesPerAxisInPatch, -1:NumberOfVolumesPerAxisInPatch-1], maxEigenvaluesZ[0:1, 0:NumberOfVolumesPerAxisInPatch, 0:NumberOfVolumesPerAxisInPatch, 0:NumberOfVolumesPerAxisInPatch])
    lambdaFar = max(maxEigenvaluesZ[0:1, 0:NumberOfVolumesPerAxisInPatch, 0:NumberOfVolumesPerAxisInPatch, 0:NumberOfVolumesPerAxisInPatch], maxEigenvaluesZ[0:1, 0:NumberOfVolumesPerAxisInPatch, 0:NumberOfVolumesPerAxisInPatch, 1:NumberOfVolumesPerAxisInPatch + 1])
    #logDataBlock(lambdaLeft[:, :, :, :, 0], "LambdaLeft.txt")
    #logDataBlock(lambdaRight[:, :, :, :, 0], "LambdaRight.txt")
    fluxLeft = -1.0 * lambdaLeft * (input[0:NumberOfUnknowns + NumberOfAuxiliaryVariables, 0:NumberOfVolumesPerAxisInPatch, 0:NumberOfVolumesPerAxisInPatch, 0:NumberOfVolumesPerAxisInPatch] - input[0:NumberOfVolumesPerAxisInPatch, -1:NumberOfVolumesPerAxisInPatch-1, 0:NumberOfVolumesPerAxisInPatch, 0:NumberOfVolumesPerAxisInPatch])
    fluxRight = -1.0 * lambdaRight * (input[0:NumberOfUnknowns + NumberOfAuxiliaryVariables, 1:NumberOfVolumesPerAxisInPatch+1, 0:NumberOfVolumesPerAxisInPatch, 0:NumberOfVolumesPerAxisInPatch] - input[0:NumberOfVolumesPerAxisInPatch, 0:NumberOfVolumesPerAxisInPatch, 0:NumberOfVolumesPerAxisInPatch, 0:NumberOfVolumesPerAxisInPatch])
    output = output + dt / h(0) * (fluxLeft - fluxRight) * NumberOfVolumesPerAxisInPatch * 0.5
    #logDataBlock(fluxLeft[:, :, :, :, 0], "FluxLeft.txt")
    #logDataBlock(fluxLeft[:, :, :, :, 0], "FluxRight.txt")
    #logDataBlock(output[:, :, :, :, 0], "Output.txt")
    fluxBottom = -1.0 * lambdaBottom * (input[0:NumberOfUnknowns + NumberOfAuxiliaryVariables, 0:NumberOfVolumesPerAxisInPatch, 0:NumberOfVolumesPerAxisInPatch, 0:NumberOfVolumesPerAxisInPatch] - input[0:NumberOfVolumesPerAxisInPatch, 0:NumberOfVolumesPerAxisInPatch, -1:NumberOfVolumesPerAxisInPatch-1, 0:NumberOfVolumesPerAxisInPatch])
    fluxTop = -1.0 * lambdaTop * (input[0:NumberOfUnknowns + NumberOfAuxiliaryVariables, 0:NumberOfVolumesPerAxisInPatch, 1:NumberOfVolumesPerAxisInPatch+1, 0:NumberOfVolumesPerAxisInPatch] - input[0:NumberOfVolumesPerAxisInPatch, 0:NumberOfVolumesPerAxisInPatch, 0:NumberOfVolumesPerAxisInPatch, 0:NumberOfVolumesPerAxisInPatch])
    output = output + dt / h(1) * (fluxBottom - fluxTop) * NumberOfVolumesPerAxisInPatch * 0.5

    fluxNear = -1.0 * lambdaNear * (input[0:NumberOfUnknowns + NumberOfAuxiliaryVariables, 0:NumberOfVolumesPerAxisInPatch, 0:NumberOfVolumesPerAxisInPatch, 0:NumberOfVolumesPerAxisInPatch] - input[0:NumberOfVolumesPerAxisInPatch, 0:NumberOfVolumesPerAxisInPatch, 0:NumberOfVolumesPerAxisInPatch, -1:NumberOfVolumesPerAxisInPatch-1])
    fluxFar = -1.0 * lambdaFar * (input[0:NumberOfUnknowns + NumberOfAuxiliaryVariables, 0:NumberOfVolumesPerAxisInPatch, 0:NumberOfVolumesPerAxisInPatch, 1:NumberOfVolumesPerAxisInPatch+1] - input[0:NumberOfVolumesPerAxisInPatch, 0:NumberOfVolumesPerAxisInPatch, 0:NumberOfVolumesPerAxisInPatch, 0:NumberOfVolumesPerAxisInPatch])
    output = output + dt / h(2) * (fluxNear - fluxFar) * NumberOfVolumesPerAxisInPatch * 0.5

    if EvaluateFlux:
      flux_x = DataBlock([[0, NumberOfUnknowns + NumberOfAuxiliaryVariables], [-HaloSize, NumberOfVolumesPerAxisInPatch + HaloSize], [0, NumberOfVolumesPerAxisInPatch], [0, NumberOfVolumesPerAxisInPatch], [0, N]], eval("flux", input[:, -1:NumberOfVolumesPerAxisInPatch+1, 0:NumberOfVolumesPerAxisInPatch, 0:NumberOfVolumesPerAxisInPatch], x, h, t, dt, 0))
      flux_y = DataBlock([[0, NumberOfUnknowns + NumberOfAuxiliaryVariables], [0, NumberOfVolumesPerAxisInPatch], [-HaloSize, NumberOfVolumesPerAxisInPatch + HaloSize], [0, NumberOfVolumesPerAxisInPatch], [0, N]], eval("flux", input[:, 0:NumberOfVolumesPerAxisInPatch, -1:NumberOfVolumesPerAxisInPatch+1, 0:NumberOfVolumesPerAxisInPatch], x, h, t, dt, 1))
      flux_z = DataBlock([[0, NumberOfUnknowns + NumberOfAuxiliaryVariables], [0, NumberOfVolumesPerAxisInPatch], [0, NumberOfVolumesPerAxisInPatch], [-HaloSize, NumberOfVolumesPerAxisInPatch + HaloSize], [0, N]], eval("flux", input[:, 0:NumberOfVolumesPerAxisInPatch, 0:NumberOfVolumesPerAxisInPatch, -1:NumberOfVolumesPerAxisInPatch+1], x, h, t, dt, 2))
      output = output + 0.5 * NumberOfVolumesPerAxisInPatch * dt / h(0) *  (flux_x[0:NumberOfUnknowns + NumberOfAuxiliaryVariables, -1:NumberOfVolumesPerAxisInPatch-1, 0:NumberOfVolumesPerAxisInPatch, 0:NumberOfVolumesPerAxisInPatch] - flux_x[0:NumberOfUnknowns + NumberOfAuxiliaryVariables, 1:NumberOfVolumesPerAxisInPatch+1, 0:NumberOfVolumesPerAxisInPatch, 0:NumberOfVolumesPerAxisInPatch])
      output = output + 0.5 * NumberOfVolumesPerAxisInPatch * dt / h(1) * (flux_y[0:NumberOfUnknowns + NumberOfAuxiliaryVariables, 0:NumberOfVolumesPerAxisInPatch, -1:NumberOfVolumesPerAxisInPatch-1, 0:NumberOfVolumesPerAxisInPatch] - flux_y[0:NumberOfUnknowns + NumberOfAuxiliaryVariables, 0:NumberOfVolumesPerAxisInPatch, 1:NumberOfVolumesPerAxisInPatch+1, 0:NumberOfVolumesPerAxisInPatch])
      output = output + 0.5 * NumberOfVolumesPerAxisInPatch * dt / h(2) * (flux_z[0:NumberOfUnknowns + NumberOfAuxiliaryVariables, 0:NumberOfVolumesPerAxisInPatch, 0:NumberOfVolumesPerAxisInPatch, -1:NumberOfVolumesPerAxisInPatch-1] - flux_z[0:NumberOfUnknowns + NumberOfAuxiliaryVariables, 0:NumberOfVolumesPerAxisInPatch, 0:NumberOfVolumesPerAxisInPatch, 1:NumberOfVolumesPerAxisInPatch+1])

    if EvaluateNonconservativeProduct:
       average_x = 0.5 * (input[0:NumberOfUnknowns + NumberOfAuxiliaryVariables, -1:NumberOfVolumesPerAxisInPatch, 0:NumberOfVolumesPerAxisInPatch, 0:NumberOfVolumesPerAxisInPatch] + input[0:NumberOfUnknowns + NumberOfAuxiliaryVariables, 0:NumberOfVolumesPerAxisInPatch + 1, 0:NumberOfVolumesPerAxisInPatch, 0:NumberOfVolumesPerAxisInPatch])
       delta_x = input[0:NumberOfUnknowns + NumberOfAuxiliaryVariables, 0:NumberOfVolumesPerAxisInPatch + HaloSize, 0:NumberOfVolumesPerAxisInPatch, 0:NumberOfVolumesPerAxisInPatch] - input[0:NumberOfUnknowns + NumberOfAuxiliaryVariables, -HaloSize:NumberOfVolumesPerAxisInPatch, 0:NumberOfVolumesPerAxisInPatch, 0:NumberOfVolumesPerAxisInPatch]
       nonconservative_x = DataBlock([[0, NumberOfUnknowns + NumberOfAuxiliaryVariables], [0, NumberOfVolumesPerAxisInPatch + HaloSize], [0, NumberOfVolumesPerAxisInPatch], [0, NumberOfVolumesPerAxisInPatch], [0, N]], eval("nonconservativeProduct", average_x, delta_x, x, h, t, dt, 0))
       output = output + 0.5 * NumberOfVolumesPerAxisInPatch * dt / h(0) * (-nonconservative_x[0:NumberOfUnknowns + NumberOfAuxiliaryVariables, 0:NumberOfVolumesPerAxisInPatch, 0:NumberOfVolumesPerAxisInPatch, 0:NumberOfVolumesPerAxisInPatch] - nonconservative_x[0:NumberOfUnknowns + NumberOfAuxiliaryVariables, 1:NumberOfVolumesPerAxisInPatch+1, 0:NumberOfVolumesPerAxisInPatch, 0:NumberOfVolumesPerAxisInPatch])
       
       average_y = 0.5 * (input[0:NumberOfUnknowns + NumberOfAuxiliaryVariables, 0:NumberOfVolumesPerAxisInPatch, -1:NumberOfVolumesPerAxisInPatch, 0:NumberOfVolumesPerAxisInPatch] + input[0:NumberOfUnknowns + NumberOfAuxiliaryVariables, 0:NumberOfVolumesPerAxisInPatch, 0:NumberOfVolumesPerAxisInPatch + 1, 0:NumberOfVolumesPerAxisInPatch])
       delta_y = input[0:NumberOfUnknowns + NumberOfAuxiliaryVariables, 0:NumberOfVolumesPerAxisInPatch, 0:NumberOfVolumesPerAxisInPatch + HaloSize, 0:NumberOfVolumesPerAxisInPatch] - input[0:NumberOfUnknowns + NumberOfAuxiliaryVariables, 0:NumberOfVolumesPerAxisInPatch, -HaloSize:NumberOfVolumesPerAxisInPatch, 0:NumberOfVolumesPerAxisInPatch]
       nonconservative_y = DataBlock([[0, NumberOfUnknowns + NumberOfAuxiliaryVariables], [0, NumberOfVolumesPerAxisInPatch], [0, NumberOfVolumesPerAxisInPatch + HaloSize], [0, NumberOfVolumesPerAxisInPatch], [0, N]], eval("nonconservativeProduct", average_y, delta_y, x, h, t, dt, 1))
       output = output + 0.5 * NumberOfVolumesPerAxisInPatch * dt / h(1) * (-nonconservative_y[0:NumberOfUnknowns + NumberOfAuxiliaryVariables, 0:NumberOfVolumesPerAxisInPatch, 0:NumberOfVolumesPerAxisInPatch, 0:NumberOfVolumesPerAxisInPatch] - nonconservative_y[0:NumberOfUnknowns + NumberOfAuxiliaryVariables, 0:NumberOfVolumesPerAxisInPatch, 1:NumberOfVolumesPerAxisInPatch+1, 0:NumberOfVolumesPerAxisInPatch])

       average_z = 0.5 * (input[0:NumberOfUnknowns + NumberOfAuxiliaryVariables, 0:NumberOfVolumesPerAxisInPatch, 0:NumberOfVolumesPerAxisInPatch, -1:NumberOfVolumesPerAxisInPatch] + input[0:NumberOfUnknowns + NumberOfAuxiliaryVariables, 0:NumberOfVolumesPerAxisInPatch, 0:NumberOfVolumesPerAxisInPatch, 0:NumberOfVolumesPerAxisInPatch + 1])
       delta_z = input[0:NumberOfUnknowns + NumberOfAuxiliaryVariables, 0:NumberOfVolumesPerAxisInPatch, 0:NumberOfVolumesPerAxisInPatch, 0:NumberOfVolumesPerAxisInPatch + HaloSize] - input[0:NumberOfUnknowns + NumberOfAuxiliaryVariables, 0:NumberOfVolumesPerAxisInPatch, 0:NumberOfVolumesPerAxisInPatch, -HaloSize:NumberOfVolumesPerAxisInPatch]
       nonconservative_z = DataBlock([[0, NumberOfUnknowns + NumberOfAuxiliaryVariables], [0, NumberOfVolumesPerAxisInPatch], [0, NumberOfVolumesPerAxisInPatch], [0, NumberOfVolumesPerAxisInPatch + HaloSize], [0, N]], eval("nonconservativeProduct", average_z, delta_z, x, h, t, dt, 2))
       output = output + 0.5 * NumberOfVolumesPerAxisInPatch * dt / h(2) * (-nonconservative_z[0:NumberOfUnknowns + NumberOfAuxiliaryVariables, 0:NumberOfVolumesPerAxisInPatch, 0:NumberOfVolumesPerAxisInPatch, 0:NumberOfVolumesPerAxisInPatch] - nonconservative_z[0:NumberOfUnknowns + NumberOfAuxiliaryVariables, 0:NumberOfVolumesPerAxisInPatch, 0:NumberOfVolumesPerAxisInPatch, 1:NumberOfVolumesPerAxisInPatch+1])

    if EvaluateMaximumEigenvalueAfterTimeStep:
      maxEigenvaluesX2 = DataBlock([[0, 1], [0, NumberOfVolumesPerAxisInPatch], [0, NumberOfVolumesPerAxisInPatch], [0, NumberOfVolumesPerAxisInPatch], [0, N]], eval("maxEigenvalue", output, x, h, t, dt, 0))
      maxEigenvaluesY2 = DataBlock([[0, 1], [0, NumberOfVolumesPerAxisInPatch], [0, NumberOfVolumesPerAxisInPatch], [0, NumberOfVolumesPerAxisInPatch], [0, N]], eval("maxEigenvalue", output, x, h, t, dt, 1))
      maxEigenvaluesZ2 = DataBlock([[0, 1], [0, NumberOfVolumesPerAxisInPatch], [0, NumberOfVolumesPerAxisInPatch], [0, NumberOfVolumesPerAxisInPatch], [0, N]], eval("maxEigenvalue", output, x, h, t, dt, 2))
      maxEigenvaluesXY = max(maxEigenvaluesX2, maxEigenvaluesY2)
      maxEigenvaluesPerVolume = max(maxEigenvaluesXY, maxEigenvaluesZ2)
      maxEigenvalues = max(maxEigenvaluesPerVolume)
    