def cellIntegral(patchData: "CellData<double, double>&") -> "void":
    N = Integer("patchData.numberOfCells")
    t = DataBlock([[0, N]], "patchData.t", "double")
    dt = DataBlock([[0, N]], "patchData.dt", "double")
    h = DataBlock([[0, N]], "patchData.cellSize", "tarch::la::Vector<2, double>")
    x = DataBlock([[0, N]], "patchData.cellCentre", "tarch::la::Vector<2, double>")

    input = DataBlock([[0, NumberOfUnknowns + NumberOfAuxiliaryVariables], [0, DGOrder + 1], [0, DGOrder + 1], [0, N]], "patchData.QIn")
    output = DataBlock([[0, NumberOfUnknowns], [0, DGOrder + 1], [0, DGOrder + 1], [0, N]], "patchData.QOut")
    
    output = 0.0
    mass_matrix_1d = DiagonalMatrix(DGOrder + 1, "SolverType::MassMatrixDiagonal1d")
    mass_matrix_2d = kroneckerProduct(mass_matrix_1d, mass_matrix_1d)
    identity_matrix = DiagonalMatrix(DGOrder + 1, 1.0)
    quadrature_positions = DataBlock([[0, 1], [0, DGOrder + 1], [0, DGOrder + 1], [0, N]], eval("getQuadraturePoint", index, x, h, DGOrder, "SolverType::QuadraturePoints1d"), "tarch::la::Vector<2, double>")

    if EvaluateSource:
        source = DataBlock([[0, NumberOfUnknowns], [0, DGOrder + 1], [0, DGOrder + 1], [0, N]], eval("sourceTerm", input, quadrature_positions, t, dt))
        output = output + h(0) * h(1) * mass_matrix_2d * source

    if EvaluateFlux:
        flux_x = DataBlock([[0, NumberOfUnknowns], [0, DGOrder + 1], [0, DGOrder + 1], [0, N]], eval("flux", input, quadrature_positions, t, dt, 0))
        flux_y = DataBlock([[0, NumberOfUnknowns], [0, DGOrder + 1], [0, DGOrder + 1], [0, N]], eval("flux", input, quadrature_positions, t, dt, 1))

        mass_matrix_x = kroneckerProduct(mass_matrix_1d, identity_matrix)
        mass_matrix_y = kroneckerProduct(identity_matrix, mass_matrix_1d)

        stiffness_matrix_1d = Matrix(DGOrder + 1, DGOrder + 1, "SolverType::StiffnessOperator1d")
        stiffness_matrix_x = kroneckerProduct(identity_matrix, stiffness_matrix_1d)
        stiffness_matrix_y = kroneckerProduct(stiffness_matrix_1d, identity_matrix)

        matrixVectorProduct(stiffness_matrix_x, flux_x, flux_x_contribution)
        matrixVectorProduct(stiffness_matrix_y, flux_y, flux_y_contribution)

        flux_x_contribution = h(0) * mass_matrix_x * flux_x_contribution
        flux_y_contribution = h(1) * mass_matrix_y * flux_y_contribution

        output[0:NumberOfUnknowns, :, :] = output[0:NumberOfUnknowns, :, :] + flux_x_contribution + flux_y_contribution
    
    if EvaluateNonconservativeProduct:
        derivative_matrix_1d = Matrix(DGOrder + 1, DGOrder + 1, "SolverType::DerivativeOperator1d")
        derivative_matrix_x = kroneckerProduct(identity_matrix, derivative_matrix_1d)
        derivative_matrix_y = kroneckerProduct(derivative_matrix_1d, identity_matrix)

        matrixVectorProduct(derivative_matrix_x, input, gradient_x)
        matrixVectorProduct(derivative_matrix_y, input, gradient_y)
        gradient_x = gradient_x / h(0)
        gradient_y = gradient_y / h(1)

        nonconservative_product_x = DataBlock([[0, NumberOfUnknowns], [0, DGOrder + 1], [0, DGOrder + 1], [0, N]], eval("nonconservativeProduct", input, gradient_x, quadrature_positions, t, dt, 0))
        nonconservative_product_y = DataBlock([[0, NumberOfUnknowns], [0, DGOrder + 1], [0, DGOrder + 1], [0, N]], eval("nonconservativeProduct", input, gradient_y, quadrature_positions, t, dt, 1))

        output[0:NumberOfUnknowns, :, :] = output[0:NumberOfUnknowns, :, :] - h(0) * h(1) * mass_matrix_2d * (nonconservative_product_x + nonconservative_product_y)
