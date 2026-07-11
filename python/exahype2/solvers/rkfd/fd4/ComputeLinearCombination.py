# Template variables: 
# RKOrder: Order of the Runge-Kutta method
# RKStep: Which step of the Runge-Kutta method we are in
# NC: Number of cells per axis in the patch

def computeLinearCombination(estimatesArray: "double*", outputArray: "double*", timeStepSize: "double") -> "void":
    dt = Float("timeStepSize")
    estimates = DataBlock([[0, NumberOfUnknowns], [0, NC], [0, NC], [0, NC], [0, RKOrder]], "estimatesArray")
    output = DataBlock([[0, NumberOfUnknowns + NumberOfAuxiliaryVariables], [-H, NC + H], [-H, NC + H], [-H, NC + H], [0, 1]], "outputArray")
    weights_matrix = DiagonalMatrix(RKOrder, "SolverType::RKWeightMatrix[RKStep]")
    dot(weights_matrix, estimates[0:NumberOfUnknowns, :, :, :, :], estimates_contribution)
    output[0:NumberOfUnknowns, 0:NC, 0:NC, 0:NC, 0] = output[0:NumberOfUnknowns, 0:NC, 0:NC, 0:NC, 0] + dt * estimates_contribution

# Template variables: 
# RKOrder: Order of the Runge-Kutta method
# NC: Number of cells per axis in the patch

def computeFinalLinearCombination(estimatesArray: "double*", outputArray: "double*", timeStepSize: "double") -> "void":
    dt = Float("timeStepSize")
    estimates = DataBlock([[0, NumberOfUnknowns], [0, NC], [0, NC], [0, NC], [0, RKOrder]], "estimatesArray")
    output = DataBlock([[0, NumberOfUnknowns + NumberOfAuxiliaryVariables], [0, NC], [0, NC], [0, NC], [0, 1]], "outputArray")
    weights_matrix = DiagonalMatrix(RKOrder, "SolverType::RKWeights")
    dot(weights_matrix, estimates[0:NumberOfUnknowns, :, :, :, :], estimates_contribution)
    #output[0:NumberOfUnknowns, :, :, :, 0] = output[0:NumberOfUnknowns, :, :, :, 0] + estimates[0:NumberOfUnknowns, :, :, :, 0]
    output[0:NumberOfUnknowns, :, :, :, 0] = output[0:NumberOfUnknowns, :, :, :, 0] + dt * estimates_contribution
