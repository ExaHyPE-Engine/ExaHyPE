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

    input = DataBlock([[0, NumberOfUnknowns + NumberOfAuxiliaryVariables], [-H, NV + H], [-H, NV + H], [0, N]], "patchData.QIn")
    output = DataBlock([[0, NumberOfUnknowns + NumberOfAuxiliaryVariables], [0, NV], [0, NV], [0, N]], "patchData.QOut")
    output = input[:, 0:NV, 0:NV]

    if EvaluateSource:
       source = DataBlock([[0, NumberOfUnknowns], [0, NV], [0, NV], [0, N]], eval("sourceTerm", input[:, 0:NV, 0:NV], x, h, t, dt))
       output[0:NumberOfUnknowns, 0:NV, 0:NV] = output[0:NumberOfUnknowns, 0:NV, 0:NV] + dt * source
    
    #Compute slope with van Albada limiter
    epsilon = 0.01
    slopeRight = input[0:NumberOfUnknowns + NumberOfAuxiliaryVariables, 0:NV+2, 0:NV] - input[0:NumberOfUnknowns + NumberOfAuxiliaryVariables, -1:NV+1, 0:NV]
    slopeLeft = input[0:NumberOfUnknowns + NumberOfAuxiliaryVariables, -1:NV+1, 0:NV] - input[0:NumberOfUnknowns + NumberOfAuxiliaryVariables, -2:NV, 0:NV]
    slopeX = ((slopeRight * slopeRight + epsilon * epsilon)*slopeLeft + (slopeLeft * slopeLeft + epsilon * epsilon)*slopeRight) \
            / (slopeLeft * slopeLeft + slopeRight * slopeRight + 2 * epsilon * epsilon)

    slopeTop = input[0:NumberOfUnknowns + NumberOfAuxiliaryVariables, 0:NV, 0:NV+2] - input[0:NumberOfUnknowns + NumberOfAuxiliaryVariables, 0:NV, -1:NV+1]
    slopeBottom = input[0:NumberOfUnknowns + NumberOfAuxiliaryVariables, 0:NV, -1:NV+1] - input[0:NumberOfUnknowns + NumberOfAuxiliaryVariables, 0:NV, -2:NV]
    slopeY = ((slopeTop * slopeTop + epsilon * epsilon)*slopeBottom + (slopeBottom * slopeBottom + epsilon * epsilon)*slopeTop) \
            / (slopeBottom * slopeBottom + slopeTop * slopeTop + 2 * epsilon * epsilon)

    #Compute face values
    leftFace = input[0:NumberOfUnknowns + NumberOfAuxiliaryVariables, -1:NV+1, 0:NV] - 0.5 * slopeX
    rightFace = input[0:NumberOfUnknowns + NumberOfAuxiliaryVariables, -1:NV+1, 0:NV] + 0.5 * slopeX
    bottomFace = input[0:NumberOfUnknowns + NumberOfAuxiliaryVariables, 0:NV, -1:NV+1] - 0.5 * slopeY
    topFace = input[0:NumberOfUnknowns + NumberOfAuxiliaryVariables, 0:NV, -1:NV+1] + 0.5 * slopeY

    #Evolve face values by half time step
    if EvaluateFlux:
        fluxLeft = DataBlock([[0, NumberOfUnknowns], [-1, NV+1], [0, NV], [0, N]], eval("flux", leftFace, x, h, t, dt, 0))
        fluxRight = DataBlock([[0, NumberOfUnknowns], [-1, NV+1], [0, NV], [0, N]], eval("flux", rightFace, x, h, t, dt, 0))
        fluxBottom = DataBlock([[0, NumberOfUnknowns], [0, NV], [-1, NV+1], [0, N]], eval("flux", bottomFace, x, h, t, dt, 1))
        fluxTop = DataBlock([[0, NumberOfUnknowns], [0, NV], [-1, NV+1], [0, N]], eval("flux", topFace, x, h, t, dt, 1))
        
        leftFace[0:NumberOfUnknowns, -1:NV+1, 0:NV] = leftFace[0:NumberOfUnknowns, -1:NV+1, 0:NV] \
                        - 0.5 * (dt / h(0)) * (fluxRight - fluxLeft) 
        rightFace[0:NumberOfUnknowns, -1:NV+1, 0:NV] = rightFace[0:NumberOfUnknowns, -1:NV+1, 0:NV] \
                        - 0.5 * (dt / h(0)) * (fluxRight - fluxLeft) 
        bottomFace[0:NumberOfUnknowns, 0:NV, -1:NV+1] = bottomFace[0:NumberOfUnknowns, 0:NV, -1:NV+1] \
                        - 0.5 * (dt / h(1)) * (fluxTop - fluxBottom) 
        topFace[0:NumberOfUnknowns, 0:NV, -1:NV+1] = topFace[0:NumberOfUnknowns, 0:NV, -1:NV+1] \
                        - 0.5 * (dt / h(1)) * (fluxTop - fluxBottom) 
        
    if EvaluateSource:
       sourceLeftFace = DataBlock([[0, NumberOfUnknowns], [-1, NV+1], [0, NV], [0, N]], eval("sourceTerm", leftFace, x, h, t, dt))
       sourceRightFace = DataBlock([[0, NumberOfUnknowns], [-1, NV+1], [0, NV], [0, N]], eval("sourceTerm", rightFace, x, h, t, dt))
       sourceBottomFace = DataBlock([[0, NumberOfUnknowns], [0, NV], [-1, NV+1], [0, N]], eval("sourceTerm", bottomFace, x, h, t, dt))
       sourceTopFace = DataBlock([[0, NumberOfUnknowns], [0, NV], [-1, NV+1], [0, N]], eval("sourceTerm", topFace, x, h, t, dt))
       
       leftFace[0:NumberOfUnknowns, 0:NV, 0:NV] = leftFace[0:NumberOfUnknowns, 0:NV, 0:NV] + 0.5 * dt * sourceLeftFace
       rightFace[0:NumberOfUnknowns, 0:NV, 0:NV] = rightFace[0:NumberOfUnknowns, 0:NV, 0:NV] + 0.5 * dt * sourceRightFace
       bottomFace[0:NumberOfUnknowns, 0:NV, 0:NV] = bottomFace[0:NumberOfUnknowns, 0:NV, 0:NV] + 0.5 * dt * sourceBottomFace
       topFace[0:NumberOfUnknowns, 0:NV, 0:NV] = topFace[0:NumberOfUnknowns, 0:NV, 0:NV] + 0.5 * dt * sourceTopFace

    #Evolve volumes
    maxEigenvaluesLeft = DataBlock([[0, 1], [-1, NV+1], [0, NV], [0, N]], eval("maxEigenvalue", leftFace, x, h, t, dt, 0))
    maxEigenvaluesRight = DataBlock([[0, 1], [-1, NV+1], [0, NV], [0, N]], eval("maxEigenvalue", rightFace, x, h, t, dt, 0))    
    lambdaX = max(maxEigenvaluesLeft[0:1, 0:NV+1, 0:NV], maxEigenvaluesRight[0:1, -1:NV, 0:NV])

    maxEigenvaluesBottom = DataBlock([[0, 1], [0, NV], [-1, NV+1], [0, N]], eval("maxEigenvalue", bottomFace, x, h, t, dt, 1))
    maxEigenvaluesTop = DataBlock([[0, 1], [0, NV], [-1, NV+1], [0, N]], eval("maxEigenvalue", topFace, x, h, t, dt, 1))    
    lambdaY = max(maxEigenvaluesBottom[0:1, 0:NV, 0:NV+1], maxEigenvaluesTop[0:1, 0:NV, -1:NV])

    rusanovFluxLeft = - 0.5 * lambdaX[0:1, 0:NV, 0:NV] * (leftFace[0:NumberOfUnknowns, 0:NV, 0:NV] - rightFace[0:NumberOfUnknowns, -1:NV-1, 0:NV])
    rusanovFluxRight = - 0.5 * lambdaX[0:1, 1:NV+1, 0:NV] * (leftFace[0:NumberOfUnknowns, 1:NV+1, 0:NV] - rightFace[0:NumberOfUnknowns, 0:NV, 0:NV])
    rusanovFluxBottom = - 0.5 * lambdaY[0:1, 0:NV, 0:NV] * (bottomFace[0:NumberOfUnknowns, 0:NV, 0:NV] - topFace[0:NumberOfUnknowns, 0:NV, -1:NV-1])
    rusanovFluxTop = - 0.5 * lambdaY[0:1, 0:NV, 1:NV+1] * (bottomFace[0:NumberOfUnknowns, 0:NV, 1:NV+1] - topFace[0:NumberOfUnknowns, 0:NV, 0:NV])

    if EvaluateFlux:
        timeAveragedfluxLeft = DataBlock([[0, NumberOfUnknowns], [-1, NV + 1], [0, NV], [0, N]], eval("flux", leftFace, x, h, t, dt, 0))
        timeAveragedfluxRight = DataBlock([[0, NumberOfUnknowns], [-1, NV + 1], [0, NV], [0, N]], eval("flux", rightFace, x, h, t, dt, 0))
        timeAveragedfluxBottom = DataBlock([[0, NumberOfUnknowns], [0, NV], [-1, NV + 1], [0, N]], eval("flux", bottomFace, x, h, t, dt, 1))
        timeAveragedfluxTop = DataBlock([[0, NumberOfUnknowns], [0, NV], [-1, NV + 1], [0, N]], eval("flux", topFace, x, h, t, dt, 1))

        rusanovFluxLeft = rusanovFluxLeft + 0.5 * (timeAveragedfluxLeft[:, 0:NV, 0:NV] + timeAveragedfluxRight[:, -1:NV-1, 0:NV])
        rusanovFluxRight = rusanovFluxRight + 0.5 * (timeAveragedfluxLeft[:, 1:NV+1, 0:NV] + timeAveragedfluxRight[:, 0:NV, 0:NV])
        rusanovFluxBottom = rusanovFluxBottom + 0.5 * (timeAveragedfluxBottom[:, 0:NV, 0:NV] + timeAveragedfluxTop[:, 0:NV, -1:NV-1])
        rusanovFluxTop = rusanovFluxTop + 0.5 * (timeAveragedfluxBottom[:, 0:NV, 1:NV+1] + timeAveragedfluxTop[:, 0:NV, 0:NV])
    
    output[0:NumberOfUnknowns, :, :] = output[0:NumberOfUnknowns, :, :] + (dt / h(0)) * NV * (rusanovFluxLeft - rusanovFluxRight)
    output[0:NumberOfUnknowns, :, :] = output[0:NumberOfUnknowns, :, :] + (dt / h(1)) * NV * (rusanovFluxBottom - rusanovFluxTop)

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
      maxEigenvaluesX2 = DataBlock([[0, 1], [0, NV], [0, NV], [0, N]], eval("maxEigenvalue", output, x, h, t, dt, 0))
      maxEigenvaluesY2 = DataBlock([[0, 1], [0, NV], [0, NV], [0, N]], eval("maxEigenvalue", output, x, h, t, dt, 1))
      maxEigenvaluesPerVolume = max(maxEigenvaluesX2, maxEigenvaluesY2)
      maxEigenvalues = max(maxEigenvaluesPerVolume)