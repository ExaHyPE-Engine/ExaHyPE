# The following parameters shoule be set when calling Parser().parse(...)

# Parameters used to generate non stateless code (set in parser)
"""
flux: "const Flux&",                   
nonconservativeProduct: "const NonconservativeProduct&", 
sourceTerm: "const Source&",
maxEigenvalue: "const MaxEigenvalue&",   
"""

# Parameter added for loop parallelisation (set in parser)
"""
loopParallelism: "peano4::utils::LoopPlacement"
"""

# we do not scale timestep size as this kernel go with RK schme, so always use 1 instead of dt
# finite differnces for certain variables will be tuned accordingly for accuracy
# KO disspation is added with a coefficient

def timeStepWithFD4(patchData: "CellData<double, double>&") -> "void":
  N = Integer("patchData.numberOfCells")
  t = DataBlock([[0, N]], "patchData.t", "double")
  dt = DataBlock([[0, N]], "patchData.dt", "double")
  h = DataBlock([[0, N]], "patchData.cellSize", "tarch::la::Vector<3, double>")
  x = DataBlock([[0, N]], "patchData.cellCentre", "tarch::la::Vector<3, double>")

  #H: HaloSize, NC:NumberOfCellsPerAxisInPatch
  input = DataBlock([[0, NumberOfUnknowns + NumberOfAuxiliaryVariables], [-H, NC + H], [-H, NC + H], [-H, NC + H], [0, N]], "patchData.QIn")
  output = DataBlock([[0, NumberOfUnknowns], [0, NC], [0, NC], [0, NC], [0, N]], "patchData.QOut")
  #initializing with all 0s
  output = 0*input[0:NumberOfUnknowns, 0:NC, 0:NC, 0:NC] 

  if EvaluateSource:
    source = DataBlock([[0, NumberOfUnknowns], [0, NC], [0, NC], [0, NC], [0, N]], eval("sourceTerm", input[:, 0:NC, 0:NC, 0:NC], x, h, t, dt))
    output = output + 1 * source

  if EvaluateFlux:
    flux_x = DataBlock([[0, NumberOfUnknowns], [-H, NC + H], [-H, NC + H], [-H, NC + H], [0, N]], eval("flux", input[:, 0:NC, 0:NC, 0:NC], x, h, t, dt, 0))
    flux_y = DataBlock([[0, NumberOfUnknowns], [-H, NC + H], [-H, NC + H], [-H, NC + H], [0, N]], eval("flux", input[:, 0:NC, 0:NC, 0:NC], x, h, t, dt, 1))
    flux_z = DataBlock([[0, NumberOfUnknowns], [-H, NC + H], [-H, NC + H], [-H, NC + H], [0, N]], eval("flux", input[:, 0:NC, 0:NC, 0:NC], x, h, t, dt, 2))
    output = output + (1/12.0) * NC * 1 / h(0) * (-1*flux_x[0:NumberOfUnknowns, -2:NC-2, 0:NC, 0:NC] + 1*flux_x[0:NumberOfUnknowns, 2:NC+2, 0:NC, 0:NC] + 8*flux_x[0:NumberOfUnknowns, -1:NC-1, 0:NC, 0:NC] - 8*flux_x[0:NumberOfUnknowns, 1:NC+1, 0:NC, 0:NC])
    output = output + (1/12.0) * NC * 1 / h(1) * (-1*flux_y[0:NumberOfUnknowns, 0:NC, -2:NC-2, 0:NC] + 1*flux_y[0:NumberOfUnknowns, 0:NC, 2:NC+2, 0:NC] + 8*flux_y[0:NumberOfUnknowns, 0:NC, -1:NC-1, 0:NC] - 8*flux_y[0:NumberOfUnknowns, 0:NC, 1:NC+1, 0:NC])
    output = output + (1/12.0) * NC * 1 / h(2) * (-1*flux_z[0:NumberOfUnknowns, 0:NC, 0:NC, -2:NC-2] + 1*flux_z[0:NumberOfUnknowns, 0:NC, 0:NC, 2:NC+2] + 8*flux_z[0:NumberOfUnknowns, 0:NC, 0:NC, -1:NC-1] - 8*flux_z[0:NumberOfUnknowns, 0:NC, 0:NC, 1:NC+1])

  if EvaluateNonconservativeProduct:
    #for direct SO below we need an extra 1/cellsize scaling 
    average_Q = input[:, 0:NC, 0:NC, 0:NC]

    delta_x = 0 * input[:, 0:NC, 0:NC, 0:NC]
    delta_x[:, 0:NC, 0:NC, 0:NC]  = (1/12.0) * (8*input[:, 1:NC+1, 0:NC, 0:NC] - 8*input[:, -1:NC-1, 0:NC, 0:NC] + 1*input[:, -2:NC-2, 0:NC, 0:NC] - 1*input[:, 2:NC+2, 0:NC, 0:NC])
    nonconservative_x = DataBlock([[0, NumberOfUnknowns], [0, NC], [0, NC], [0, NC], [0, N]], eval("nonconservativeProduct", average_Q, delta_x, x, h, t, dt, 0))
    delta_y = 0 * input[:, 0:NC, 0:NC, 0:NC]
    delta_y[:, 0:NC, 0:NC, 0:NC]  = (1/12.0) * (8*input[:, 0:NC, 1:NC+1, 0:NC] - 8*input[:, 0:NC, -1:NC-1, 0:NC] + 1*input[:, 0:NC, -2:NC-2, 0:NC] - 1*input[:, 0:NC, 2:NC+2, 0:NC])
    nonconservative_y = DataBlock([[0, NumberOfUnknowns], [0, NC], [0, NC], [0, NC], [0, N]], eval("nonconservativeProduct", average_Q, delta_y, x, h, t, dt, 1))      
    delta_z = 0 * input[:, 0:NC, 0:NC, 0:NC]
    delta_z[:, 0:NC, 0:NC, 0:NC]  = (1/12.0) * (8*input[:, 0:NC, 0:NC, 1:NC+1] - 8*input[:, 0:NC, 0:NC, -1:NC-1] + 1*input[:, 0:NC, 0:NC, -2:NC-2] - 1*input[:, 0:NC, 0:NC, 2:NC+2])
    nonconservative_z = DataBlock([[0, NumberOfUnknowns], [0, NC], [0, NC], [0, NC], [0, N]], eval("nonconservativeProduct", average_Q, delta_z, x, h, t, dt, 2))

    output = output - NC / h(0) * nonconservative_x - NC / h(1) * nonconservative_y - NC / h(2) * nonconservative_z #remember no need to scale with timestepsize

  #KO disspation
  KO_term_x = (1)  * input[0:NumberOfUnknowns, -3:NC-3, 0:NC, 0:NC] + (1)  * input[0:NumberOfUnknowns, 3:NC+3, 0:NC, 0:NC] + (-6) * input[0:NumberOfUnknowns, -2:NC-2, 0:NC, 0:NC] + (-6) * input[0:NumberOfUnknowns, 2:NC+2, 0:NC, 0:NC] \
            + (15) * input[0:NumberOfUnknowns, -1:NC-1, 0:NC, 0:NC] + (15) * input[0:NumberOfUnknowns, 1:NC+1, 0:NC, 0:NC] +(-20) * input[0:NumberOfUnknowns, 0:NC, 0:NC, 0:NC]
  KO_term_y = (1)  * input[0:NumberOfUnknowns, 0:NC, -3:NC-3, 0:NC] + (1)  * input[0:NumberOfUnknowns, 0:NC, 3:NC+3, 0:NC] + (-6) * input[0:NumberOfUnknowns, 0:NC, -2:NC-2, 0:NC] + (-6) * input[0:NumberOfUnknowns, 0:NC, 2:NC+2, 0:NC] \
            + (15) * input[0:NumberOfUnknowns, 0:NC, -1:NC-1, 0:NC] + (15) * input[0:NumberOfUnknowns, 0:NC, 1:NC+1, 0:NC] +(-20) * input[0:NumberOfUnknowns, 0:NC, 0:NC, 0:NC]
  KO_term_z = (1)  * input[0:NumberOfUnknowns, 0:NC, 0:NC, -3:NC-3] + (1)  * input[0:NumberOfUnknowns, 0:NC, 0:NC, 3:NC+3] + (-6) * input[0:NumberOfUnknowns, 0:NC, 0:NC, -2:NC-2] + (-6) * input[0:NumberOfUnknowns, 0:NC, 0:NC, 2:NC+2] \
            + (15) * input[0:NumberOfUnknowns, 0:NC, 0:NC, -1:NC-1] + (15) * input[0:NumberOfUnknowns, 0:NC, 0:NC, 1:NC+1] +(-20) * input[0:NumberOfUnknowns, 0:NC, 0:NC, 0:NC]
  output = output  + (KOsigma/64.0) * NC / h(0) * KO_term_x + (KOsigma/64.0) * NC / h(1) * KO_term_y + (KOsigma/64.0) * NC / h(2) * KO_term_z