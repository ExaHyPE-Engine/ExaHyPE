# The following parameters shoule be set when calling Parser().parse(...)

# Parameters used to generate non stateless code (set in parser)
"""
flux: "const fluxFunctor&",                   
nonconservativeProduct: "const nonconservativeProductFunctor&", 
sourceTerm: "const sourceFunctor&",
maxEigenvalue: "const maxEigenvalueFunctor&",   
"""

def timeStepWithMUSCLHancock(patchData: "CellData<double, double>&") -> "void":
    N = Integer("patchData.numberOfCells")
    t = DataBlock([[0, N]], "patchData.t", "double")
    dt = DataBlock([[0, N]], "patchData.dt", "double")
    h = DataBlock([[0, N]], "patchData.cellSize", "tarch::la::Vector<2, double>")
    x = DataBlock([[0, N]], "patchData.cellCentre", "tarch::la::Vector<2, double>")
    maxEigenvalues = DataBlock([[0, N]], "patchData.maxEigenvalue", "double")

    input = DataBlock([[0, NumberOfUnknowns + NumberOfAuxiliaryVariables], [-H, NV + H], [-H, NV + H], [-H, NV + H], [0, N]], "patchData.QIn")
    output = DataBlock([[0, NumberOfUnknowns + NumberOfAuxiliaryVariables], [0, NV], [0, NV], [0, NV], [0, N]], "patchData.QOut")
    output = input[:, 0:NV, 0:NV]

    if EvaluateSource:
       source = DataBlock([[0, NumberOfUnknowns + NumberOfAuxiliaryVariables], [0, NV], [0, NV], [0, NV], [0, N]], eval("sourceTerm", input[:, 0:NV, 0:NV, 0:NV], x, h, t, dt))
       output = output + dt * source

    #Compute slope with van Albada limiter
    epsilon = 0.01
    slopeRight = input[0:NumberOfUnknowns, 0:NV+2, 0:NV, 0:NV] - input[0:NumberOfUnknowns, -1:NV+1, 0:NV, 0:NV]
    slopeLeft = input[0:NumberOfUnknowns, -1:NV+1, 0:NV, 0:NV] - input[0:NumberOfUnknowns, -2:NV, 0:NV, 0:NV]
    slopeX = ((slopeRight * slopeRight + epsilon * epsilon)*slopeLeft + (slopeLeft * slopeLeft + epsilon * epsilon)*slopeRight) \
            / (slopeLeft * slopeLeft + slopeRight * slopeRight + 2 * epsilon * epsilon)

    slopeTop = input[0:NumberOfUnknowns, 0:NV, 0:NV+2, 0:NV] - input[0:NumberOfUnknowns, 0:NV, -1:NV+1, 0:NV]
    slopeBottom = input[0:NumberOfUnknowns, 0:NV, -1:NV+1, 0:NV] - input[0:NumberOfUnknowns, 0:NV, -2:NV, 0:NV]
    slopeY = ((slopeTop * slopeTop + epsilon * epsilon)*slopeBottom + (slopeBottom * slopeBottom + epsilon * epsilon)*slopeTop) \
            / (slopeBottom * slopeBottom + slopeTop * slopeTop + 2 * epsilon * epsilon)
    
    slopeFar = input[0:NumberOfUnknowns, 0:NV, 0:NV, 0:NV+2] - input[0:NumberOfUnknowns, 0:NV, 0:NV, -1:NV+1]
    slopeNear = input[0:NumberOfUnknowns, 0:NV, 0:NV, -1:NV+1] - input[0:NumberOfUnknowns, 0:NV, 0:NV, -2:NV]
    slopeZ = ((slopeFar * slopeFar + epsilon * epsilon)*slopeNear + (slopeNear * slopeNear + epsilon * epsilon)*slopeFar) \
            / (slopeNear * slopeNear + slopeFar * slopeFar + 2 * epsilon * epsilon)

    #Compute face values
    leftFace = input[0:NumberOfUnknowns, -1:NV+1, 0:NV, 0:NV] - 0.5 * slopeX
    rightFace = input[0:NumberOfUnknowns, -1:NV+1, 0:NV, 0:NV] + 0.5 * slopeX
    bottomFace = input[0:NumberOfUnknowns, 0:NV, -1:NV+1, 0:NV] - 0.5 * slopeY
    topFace = input[0:NumberOfUnknowns, 0:NV, -1:NV+1, 0:NV] + 0.5 * slopeY
    nearFace = input[0:NumberOfUnknowns, 0:NV, 0:NV, -1:NV+1] - 0.5 * slopeZ
    farFace = input[0:NumberOfUnknowns, 0:NV, 0:NV, -1:NV+1] + 0.5 * slopeZ

    #Evolve face values by half time step
    fluxLeft = DataBlock([[0, NumberOfUnknowns], [-1, NV+1], [0, NV], [0, NV], [0, N]], eval("flux", leftFace, x, h, t, dt, 0))
    fluxRight = DataBlock([[0, NumberOfUnknowns], [-1, NV+1], [0, NV], [0, NV], [0, N]], eval("flux", rightFace, x, h, t, dt, 0))
    leftFace = leftFace - 0.5 * (dt / h(0)) * (fluxRight - fluxLeft)
    rightFace = rightFace - 0.5 * (dt / h(0)) * (fluxRight - fluxLeft)

    fluxBottom = DataBlock([[0, NumberOfUnknowns], [0, NV], [-1, NV+1], [0, NV], [0, N]], eval("flux", bottomFace, x, h, t, dt, 1))
    fluxTop = DataBlock([[0, NumberOfUnknowns], [0, NV], [-1, NV+1], [0, NV], [0, N]], eval("flux", topFace, x, h, t, dt, 1))
    bottomFace = bottomFace - 0.5 * (dt / h(1)) * (fluxTop - fluxBottom)
    topFace = topFace - 0.5 * (dt / h(1)) * (fluxTop - fluxBottom)

    fluxNear = DataBlock([[0, NumberOfUnknowns], [0, NV], [0, NV], [-1, NV+1], [0, N]], eval("flux", nearFace, x, h, t, dt, 2))
    fluxFar = DataBlock([[0, NumberOfUnknowns], [0, NV], [0, NV], [-1, NV+1], [0, N]], eval("flux", farFace, x, h, t, dt, 2))
    nearFace = bottomFace - 0.5 * (dt / h(2)) * (fluxFar - fluxNear)
    farFace = topFace - 0.5 * (dt / h(2)) * (fluxFar - fluxNear)

    #Evolve volumes
    maxEigenvaluesLeft = DataBlock([[0, 1], [-1, NV+1], [0, NV], [0, NV], [0, N]], eval("maxEigenvalue", leftFace, x, h, t, dt, 0))
    maxEigenvaluesRight = DataBlock([[0, 1], [-1, NV+1], [0, NV], [0, NV], [0, N]], eval("maxEigenvalue", rightFace, x, h, t, dt, 0))    
    lambdaX = max(maxEigenvaluesLeft[0:1, 0:NV+1, 0:NV, 0:NV], maxEigenvaluesRight[0:1, -1:NV, 0:NV, 0:NV])

    maxEigenvaluesBottom = DataBlock([[0, 1], [0, NV], [-1, NV+1], [0, NV], [0, N]], eval("maxEigenvalue", bottomFace, x, h, t, dt, 1))
    maxEigenvaluesTop = DataBlock([[0, 1], [0, NV], [-1, NV+1], [0, NV], [0, N]], eval("maxEigenvalue", topFace, x, h, t, dt, 1))    
    lambdaY = max(maxEigenvaluesBottom[0:1, 0:NV, 0:NV+1, 0:NV], maxEigenvaluesTop[0:1, 0:NV, -1:NV, 0:NV])

    maxEigenvaluesNear = DataBlock([[0, 1], [0, NV], [0, NV], [-1, NV+1], [0, N]], eval("maxEigenvalue", nearFace, x, h, t, dt, 2))
    maxEigenvaluesFar = DataBlock([[0, 1], [0, NV], [0, NV], [-1, NV+1], [0, N]], eval("maxEigenvalue", farFace, x, h, t, dt, 2))    
    lambdaY = max(maxEigenvaluesNear[0:1, 0:NV, 0:NV, 0:NV+1], maxEigenvaluesFar[0:1, 0:NV, 0:NV, -1:NV])


    timeAveragedfluxLeft = DataBlock([[0, NumberOfUnknowns], [-1, NV + 1], [0, NV], [0, NV], [0, N]], eval("flux", leftFace, x, h, t, dt, 0))
    timeAveragedfluxRight = DataBlock([[0, NumberOfUnknowns], [-1, NV + 1], [0, NV], [0, NV], [0, N]], eval("flux", rightFace, x, h, t, dt, 0))
    timeAveragedfluxBottom = DataBlock([[0, NumberOfUnknowns], [0, NV], [-1, NV + 1], [0, NV], [0, N]], eval("flux", bottomFace, x, h, t, dt, 1))
    timeAveragedfluxTop = DataBlock([[0, NumberOfUnknowns], [0, NV], [-1, NV + 1], [0, NV], [0, N]], eval("flux", topFace, x, h, t, dt, 1))
    timeAveragedfluxNear = DataBlock([[0, NumberOfUnknowns], [0, NV], [0, NV], [-1, NV + 1], [0, N]], eval("flux", nearFace, x, h, t, dt, 2))
    timeAveragedfluxFar = DataBlock([[0, NumberOfUnknowns], [0, NV], [0, NV], [-1, NV + 1], [0, N]], eval("flux", farFace, x, h, t, dt, 2))

    rusanovFluxLeft = 0.5 * (timeAveragedfluxLeft[:, 0:NV, 0:NV, 0:NV] + timeAveragedfluxRight[:, -1:NV-1, 0:NV, 0:NV]) - 0.5 * lambdaX[0:1, 0:NV, 0:NV, 0:NV] * (leftFace[:, 0:NV, 0:NV, 0:NV] - rightFace[:, -1:NV-1, 0:NV, 0:NV])
    rusanovFluxRight = 0.5 * (timeAveragedfluxLeft[:, 1:NV+1, 0:NV, 0:NV] + timeAveragedfluxRight[:, 0:NV, 0:NV, 0:NV]) - 0.5 * lambdaX[0:1, 1:NV+1, 0:NV, 0:NV] * (leftFace[:, 1:NV+1, 0:NV, 0:NV] - rightFace[:, 0:NV, 0:NV, 0:NV])
    rusanovFluxBottom = 0.5 * (timeAveragedfluxBottom[:, 0:NV, 0:NV, 0:NV] + timeAveragedfluxTop[:, 0:NV, -1:NV-1, 0:NV]) - 0.5 * lambdaY[0:1, 0:NV, 0:NV, 0:NV] * (bottomFace[:, 0:NV, 0:NV, 0:NV] - topFace[:, 0:NV, -1:NV-1, 0:NV])
    rusanovFluxTop = 0.5 * (timeAveragedfluxBottom[:, 0:NV, 1:NV+1, 0:NV] + timeAveragedfluxTop[:, 0:NV, 0:NV, 0:NV]) - 0.5 * lambdaY[0:1, 0:NV, 1:NV+1, 0:NV] * (bottomFace[:, 0:NV, 1:NV+1, 0:NV] - topFace[:, 0:NV, 0:NV, 0:NV])
    rusanovFluxNear = 0.5 * (timeAveragedfluxNear[:, 0:NV, 0:NV, 0:NV] + timeAveragedfluxFar[:, 0:NV, 0:NV, -1:NV-1]) - 0.5 * lambdaY[0:1, 0:NV, 0:NV, 0:NV] * (nearFace[:, 0:NV, 0:NV, 0:NV] - farFace[:, 0:NV, 0:NV, -1:NV-1])
    rusanovFluxFar = 0.5 * (timeAveragedfluxNear[:, 0:NV, 0:NV, 1:NV+1] + timeAveragedfluxFar[:, 0:NV, 0:NV, 0:NV]) - 0.5 * lambdaY[0:1, 0:NV, 0:NV, 1:NV+1] * (nearFace[:, 0:NV, 0:NV, 1:NV+1] - farFace[:, 0:NV, 0:NV, 0:NV])
    
    output[0:NumberOfUnknowns, :, :] = output[0:NumberOfUnknowns, :, :] + (dt / h(0)) * NV * (rusanovFluxLeft - rusanovFluxRight)
    output[0:NumberOfUnknowns, :, :] = output[0:NumberOfUnknowns, :, :] + (dt / h(1)) * NV * (rusanovFluxBottom - rusanovFluxTop)
    output[0:NumberOfUnknowns, :, :] = output[0:NumberOfUnknowns, :, :] + (dt / h(2)) * NV * (rusanovFluxNear - rusanovFluxFar)

    #if EvaluateNonconservativeProduct:
    #   average_x = 0.5 * (input[0:NumberOfUnknowns + NumberOfAuxiliaryVariables, -1:NV, 0:NV] + input[0:NumberOfUnknowns + NumberOfAuxiliaryVariables, 0:NV + 1, 0:NV])
    #   delta_x = input[0:NumberOfUnknowns + NumberOfAuxiliaryVariables, 0:NV + 1, 0:NV] - input[0:NumberOfUnknowns + NumberOfAuxiliaryVariables, -1:NV, 0:NV]
    #   nonconservative_x = DataBlock([[0, NumberOfUnknowns + NumberOfAuxiliaryVariables], [0, NV + 1], [0, NV], [0, N]], eval("nonconservativeProduct", average_x, delta_x, x, h, t, dt, 0))
    #   output = output + 0.5 * NV * dt / h(0) * (-nonconservative_x[0:NumberOfUnknowns + NumberOfAuxiliaryVariables, 0:NV, 0:NV] - nonconservative_x[0:NumberOfUnknowns + NumberOfAuxiliaryVariables, 1:NV+1, 0:NV])
    #   
    #   average_y = 0.5 * (input[0:NumberOfUnknowns + NumberOfAuxiliaryVariables, 0:NV, -1:NV] + input[0:NumberOfUnknowns + NumberOfAuxiliaryVariables, 0:NV, 0:NV + 1])
    #   delta_y = input[0:NumberOfUnknowns + NumberOfAuxiliaryVariables, 0:NV, 0:NV + 1] - input[0:NumberOfUnknowns + NumberOfAuxiliaryVariables, 0:NV, -1:NV]
    #   nonconservative_y = DataBlock([[0, NumberOfUnknowns + NumberOfAuxiliaryVariables], [0, NV], [0, NV + 1], [0, N]], eval("nonconservativeProduct", average_y, delta_y, x, h, t, dt, 1))
    #   output = output + 0.5 * NV * dt / h(1) * (-nonconservative_y[0:NumberOfUnknowns + NumberOfAuxiliaryVariables, 0:NV, 0:NV] - nonconservative_y[0:NumberOfUnknowns + NumberOfAuxiliaryVariables, 0:NV, 1:NV+1])

    if EvaluateMaximumEigenvalueAfterTimeStep:
      maxEigenvaluesX2 = DataBlock([[0, 1], [0, NV], [0, NV], [0, NV], [0, N]], eval("maxEigenvalue", output, x, h, t, dt, 0))
      maxEigenvaluesY2 = DataBlock([[0, 1], [0, NV], [0, NV], [0, NV], [0, N]], eval("maxEigenvalue", output, x, h, t, dt, 1))
      maxEigenvaluesZ2 = DataBlock([[0, 1], [0, NV], [0, NV], [0, NV], [0, N]], eval("maxEigenvalue", output, x, h, t, dt, 2))
      maxEigenvaluesXY = max(maxEigenvaluesX2, maxEigenvaluesY2)
      maxEigenvaluesPerVolume = max(maxEigenvaluesXY, maxEigenvaluesZ2)
      maxEigenvalues = max(maxEigenvaluesPerVolume)