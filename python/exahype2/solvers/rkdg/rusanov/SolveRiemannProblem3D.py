def solveRiemannProblemX(faceCentre: "const tarch::la::Vector<Dimensions,double>&", cellSize: "const tarch::la::Vector<Dimensions,double>&", t: "double", dt: "double", faceData : "const double* __restrict__", solution: "double* __restrict__") -> "void":
    input = FaceDataBlock([[0, NumberOfUnknowns + NumberOfAuxiliaryVariables], [0, 2], [0, DGOrder + 1], [0, DGOrder + 1], [0, 1]], "faceData", "const double")
    output = FaceDataBlock([[0, NumberOfUnknowns], [0, 2], [0, DGOrder + 1], [0, DGOrder + 1], [0, 1]], "solution")
    output = 0.0

    dsl_t = Float("t")
    dsl_dt = Float("dt")
    dsl_faceCentre = Vector("faceCentre")
    dsl_cellSize = Vector("cellSize")
    quadrature_positions = FaceDataBlock([[0, 1], [0, 1], [0, DGOrder + 1], [0, DGOrder + 1], [0, 1]], eval("getQuadraturePointInFace", index, dsl_faceCentre, dsl_cellSize, DGOrder, 0, "SolverType::QuadraturePoints1d"), "tarch::la::Vector<3, double>")

    deltaQ = input[:, 1, :, :] - input[:, 0, :, :]
    averageQ = 0.5 * (input[:, 1, :, :] + input[:, 0, :, :])

    leftEigenvalues = FaceDataBlock([[0, 1], [0, 1], [0, DGOrder + 1], [0, DGOrder + 1], [0, 1]], eval("maxEigenvalue", input[:, 0, :, :], quadrature_positions, dsl_t, dsl_dt, 0))
    rightEigenvalues = FaceDataBlock([[0, 1], [0, 1], [0, DGOrder + 1], [0, DGOrder + 1], [0, 1]], eval("maxEigenvalue", input[:, 1, :, :], quadrature_positions, dsl_t, dsl_dt, 0))
    maxEigenvalues = max(leftEigenvalues, rightEigenvalues)

    output[:, 0, :, :] = output[:, 0, :, :] - 0.5 * maxEigenvalues * deltaQ
    output[:, 1, :, :] = output[:, 1, :, :] - 0.5 * maxEigenvalues * deltaQ

    if EvaluateFlux:
        fluxLeft = FaceDataBlock([[0, NumberOfUnknowns], [0, 1], [0, DGOrder + 1], [0, DGOrder + 1], [0, 1]], eval("flux", input[:, 0, :, :], quadrature_positions, dsl_t, dsl_dt, 0))
        fluxRight = FaceDataBlock([[0, NumberOfUnknowns], [0, 1], [0, DGOrder + 1], [0, DGOrder + 1], [0, 1]], eval("flux", input[:, 1, :, :], quadrature_positions, dsl_t, dsl_dt, 0))
        output[:, 0, :, :] = output[:, 0, :, :] + 0.5 * (fluxLeft + fluxRight)
        output[:, 1, :, :] = output[:, 1, :, :] + 0.5 * (fluxLeft + fluxRight)

    if EvaluateNonconservativeProduct:
        ncp = FaceDataBlock([[0, NumberOfUnknowns], [0, 1], [0, DGOrder + 1], [0, DGOrder + 1], [0, 1]], eval("nonconservativeProduct", averageQ, deltaQ, quadrature_positions, dsl_t, dsl_dt, 0))
        output[:, 0, :, :] = output[:, 0, :, :] + 0.5 * ncp
        output[:, 1, :, :] = output[:, 1, :, :] - 0.5 * ncp


def solveRiemannProblemY(faceCentre: "const tarch::la::Vector<Dimensions,double>&", cellSize: "const tarch::la::Vector<Dimensions,double>&", t: "double", dt: "double", faceData : "const double* __restrict__", solution: "double* __restrict__") -> "void":
    input = FaceDataBlock([[0, NumberOfUnknowns + NumberOfAuxiliaryVariables], [0, DGOrder + 1], [0, 2], [0, DGOrder + 1], [0, 1]], "faceData", "const double")
    output = FaceDataBlock([[0, NumberOfUnknowns], [0, DGOrder + 1], [0, 2], [0, DGOrder + 1], [0, 1]], "solution")
    output = 0.0

    dsl_t = Float("t")
    dsl_dt = Float("dt")
    dsl_faceCentre = Vector("faceCentre")
    dsl_cellSize = Vector("cellSize")
    quadrature_positions = FaceDataBlock([[0, 1], [0, DGOrder + 1], [0, 1], [0, DGOrder + 1], [0, 1]], eval("getQuadraturePointInFace", index, dsl_faceCentre, dsl_cellSize, DGOrder, 1, "SolverType::QuadraturePoints1d"), "tarch::la::Vector<3, double>")

    deltaQ = input[:, :, 1, :] - input[:, :, 0, :]
    averageQ = 0.5 * (input[:, :, 1, :] + input[:, :, 0, :])

    bottomEigenvalues = FaceDataBlock([[0, 1], [0, DGOrder + 1], [0, 1], [0, DGOrder + 1], [0, 1]], eval("maxEigenvalue", input[:, :, 0, :], quadrature_positions, dsl_t, dsl_dt, 1))
    topEigenvalues = FaceDataBlock([[0, 1], [0, DGOrder + 1], [0, 1], [0, DGOrder + 1], [0, 1]], eval("maxEigenvalue", input[:, :, 1, :], quadrature_positions, dsl_t, dsl_dt, 1))
    maxEigenvalues = max(bottomEigenvalues, topEigenvalues)

    output[:, :, 0, :] = output[:, :, 0, :] - 0.5 * maxEigenvalues * deltaQ
    output[:, :, 1, :] = output[:, :, 1, :] - 0.5 * maxEigenvalues * deltaQ

    if EvaluateFlux:
        fluxBottom = FaceDataBlock([[0, NumberOfUnknowns], [0, DGOrder + 1], [0, 1], [0, DGOrder + 1], [0, 1]], eval("flux", input[:, :, 0, :], quadrature_positions, dsl_t, dsl_dt, 1))
        fluxTop = FaceDataBlock([[0, NumberOfUnknowns], [0, DGOrder + 1], [0, 1], [0, DGOrder + 1], [0, 1]], eval("flux", input[:, :, 1, :], quadrature_positions, dsl_t, dsl_dt, 1))
        output[:, :, 0, :] = output[:, :, 0, :] + 0.5 * (fluxBottom + fluxTop)
        output[:, :, 1, :] = output[:, :, 1, :] + 0.5 * (fluxBottom + fluxTop)

    if EvaluateNonconservativeProduct:
        ncp = FaceDataBlock([[0, NumberOfUnknowns], [0, DGOrder + 1], [0, 1], [0, DGOrder + 1], [0, 1]], eval("nonconservativeProduct", averageQ, deltaQ, quadrature_positions, dsl_t, dsl_dt, 1))
        output[:, :, 0, :] = output[:, :, 0, :] + 0.5 * ncp
        output[:, :, 1, :] = output[:, :, 1, :] - 0.5 * ncp


def solveRiemannProblemZ(faceCentre: "const tarch::la::Vector<Dimensions,double>&", cellSize: "const tarch::la::Vector<Dimensions,double>&", t: "double", dt: "double", faceData : "const double* __restrict__", solution: "double* __restrict__") -> "void":
    input = FaceDataBlock([[0, NumberOfUnknowns + NumberOfAuxiliaryVariables], [0, DGOrder + 1], [0, DGOrder + 1], [0, 2], [0, 1]], "faceData", "const double")
    output = FaceDataBlock([[0, NumberOfUnknowns], [0, DGOrder + 1], [0, DGOrder + 1], [0, 2], [0, 1]], "solution")
    output = 0.0

    dsl_t = Float("t")
    dsl_dt = Float("dt")
    dsl_faceCentre = Vector("faceCentre")
    dsl_cellSize = Vector("cellSize")
    quadrature_positions = FaceDataBlock([[0, 1], [0, DGOrder + 1], [0, DGOrder + 1], [0, 1], [0, 1]], eval("getQuadraturePointInFace", index, dsl_faceCentre, dsl_cellSize, DGOrder, 1, "SolverType::QuadraturePoints1d"), "tarch::la::Vector<3, double>")

    deltaQ = input[:, :, :, 1] - input[:, :, :, 0]
    averageQ = 0.5 * (input[:, :, :, 1] + input[:, :, :, 0])

    nearEigenvalues = FaceDataBlock([[0, 1], [0, DGOrder + 1], [0, DGOrder + 1], [0, 1], [0, 1]], eval("maxEigenvalue", input[:, :, :, 0], quadrature_positions, dsl_t, dsl_dt, 2))
    farEigenvalues = FaceDataBlock([[0, 1], [0, DGOrder + 1], [0, DGOrder + 1], [0, 1], [0, 1]], eval("maxEigenvalue", input[:, :, :, 1], quadrature_positions, dsl_t, dsl_dt, 2))
    maxEigenvalues = max(nearEigenvalues, farEigenvalues)

    output[:, :, :, 0] = output[:, :, :, 0] - 0.5 * maxEigenvalues * deltaQ
    output[:, :, :, 1] = output[:, :, :, 1] - 0.5 * maxEigenvalues * deltaQ

    if EvaluateFlux:
        fluxNear = FaceDataBlock([[0, NumberOfUnknowns], [0, DGOrder + 1], [0, DGOrder + 1], [0, 1], [0, 1]], eval("flux", input[:, :, :, 0], quadrature_positions, dsl_t, dsl_dt, 2))
        fluxFar = FaceDataBlock([[0, NumberOfUnknowns], [0, DGOrder + 1], [0, DGOrder + 1], [0, 1], [0, 1]], eval("flux", input[:, :, :, 1], quadrature_positions, dsl_t, dsl_dt, 2))
        output[:, :, :, 0] = output[:, :, :, 0] + 0.5 * (fluxNear + fluxFar)
        output[:, :, :, 1] = output[:, :, :, 1] + 0.5 * (fluxNear + fluxFar)

    if EvaluateNonconservativeProduct:
        ncp = FaceDataBlock([[0, NumberOfUnknowns], [0, DGOrder + 1], [0, DGOrder + 1], [0, 1], [0, 1]], eval("nonconservativeProduct", averageQ, deltaQ, quadrature_positions, dsl_t, dsl_dt, 2))
        output[:, :, :, 0] = output[:, :, :, 0] + 0.5 * ncp
        output[:, :, :, 1] = output[:, :, :, 1] - 0.5 * ncp