#!/usr/bin/env python3
from exahype2.solvers.aderdg.ADERDG import ADERDG, Polynomials

aderdg = ADERDG(
  name                 = "MyADERDG",
  order                = 4,
  unknowns             = 5,
  auxiliary_variables  = 2,
  polynomials          = Polynomials.Gauss_Legendre,
  min_h                = 1e-2,
  max_h                = 1e-1,
  plot_grid_properties = None)

print(aderdg)
