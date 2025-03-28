import sympy
from sympy import Matrix, symbols
from sympy.tensor.array import Array, tensorcontraction, tensorproduct
import numpy as np

import peano4
import exahype2


def prepare_hydrodynamic_kernel(): 
	'''
	function to created the relativistic hydrodynamic kernel in the conserved formulation
	We treat the primitive variables and spacetime variables as auxiliary variables
	'''
	hydro_pde= exahype2.symhype.FirstOrderConservativePDEFormulation(
		unknowns=5, auxiliary_variables=5+51+1, dimensions=3)

	#evolving quantities
	tildeD  = hydro_pde.name_Q_entry(0,"tildeD")
	tildeS1	= hydro_pde.name_Q_entry(1,"tildeS1")
	tildeS2	= hydro_pde.name_Q_entry(2,"tildeS2")
	tildeS3	= hydro_pde.name_Q_entry(3,"tildeS3")
	tildetau= hydro_pde.name_Q_entry(4,"tildetau")

	#primitive quantities, those should be calculated before the kernel
	rho0    = hydro_pde.name_auxiliary_variable(0,"rho0")
	v1      = hydro_pde.name_auxiliary_variable(1,"v1")
	v2      = hydro_pde.name_auxiliary_variable(2,"v2")
	v3      = hydro_pde.name_auxiliary_variable(3,"v3")
	epsilon = hydro_pde.name_auxiliary_variable(4,"epsilon")
	W 		= hydro_pde.name_auxiliary_variable(56,"W") #lorentz factor, can be recalculated with the primitives, but receiving it saves computations

	#spacetime quantities
	tg11	= hydro_pde.name_auxiliary_variable(5,"tg11");	tg12	= hydro_pde.name_auxiliary_variable(6,"tg12")
	tg13	= hydro_pde.name_auxiliary_variable(7,"tg13");	tg22	= hydro_pde.name_auxiliary_variable(8,"tg22")
	tg23	= hydro_pde.name_auxiliary_variable(9,"tg23");	tg33	= hydro_pde.name_auxiliary_variable(10,"tg33")

	tA11	= hydro_pde.name_auxiliary_variable(11,"tA11");	tA12	= hydro_pde.name_auxiliary_variable(12,"tA12")
	tA13	= hydro_pde.name_auxiliary_variable(13,"tA13");	tA22	= hydro_pde.name_auxiliary_variable(14,"tA22")
	tA23	= hydro_pde.name_auxiliary_variable(15,"tA23");	tA33	= hydro_pde.name_auxiliary_variable(16,"tA33")
	#notice we do not need \Theta and \hat{\Gamma}^i
	alpha	= hydro_pde.name_auxiliary_variable(17,"alpha")
	beta1	= hydro_pde.name_auxiliary_variable(18,"beta1")
	beta2	= hydro_pde.name_auxiliary_variable(19,"beta2")
	beta3	= hydro_pde.name_auxiliary_variable(20,"beta3")

	A1		= hydro_pde.name_auxiliary_variable(21,"A1")
	A2		= hydro_pde.name_auxiliary_variable(22,"A2")
	A3		= hydro_pde.name_auxiliary_variable(23,"A3")

	B11		= hydro_pde.name_auxiliary_variable(24,"B11")
	B12		= hydro_pde.name_auxiliary_variable(25,"B12")
	B13		= hydro_pde.name_auxiliary_variable(26,"B13")
	B21		= hydro_pde.name_auxiliary_variable(27,"B21")
	B22		= hydro_pde.name_auxiliary_variable(28,"B22")
	B23		= hydro_pde.name_auxiliary_variable(29,"B23")
	B31		= hydro_pde.name_auxiliary_variable(30,"B31")
	B32		= hydro_pde.name_auxiliary_variable(31,"B32")
	B33		= hydro_pde.name_auxiliary_variable(32,"B33")

	D111 = hydro_pde.name_auxiliary_variable(33,"D111"); D112 = hydro_pde.name_auxiliary_variable(34,"D112"); D113 = hydro_pde.name_auxiliary_variable(35,"D113");
	D122 = hydro_pde.name_auxiliary_variable(36,"D122"); D123 = hydro_pde.name_auxiliary_variable(37,"D123"); D133 = hydro_pde.name_auxiliary_variable(38,"D133");
	D211 = hydro_pde.name_auxiliary_variable(39,"D211"); D212 = hydro_pde.name_auxiliary_variable(40,"D212"); D213 = hydro_pde.name_auxiliary_variable(41,"D213");
	D222 = hydro_pde.name_auxiliary_variable(42,"D222"); D223 = hydro_pde.name_auxiliary_variable(43,"D223"); D233 = hydro_pde.name_auxiliary_variable(44,"D233");
	D311 = hydro_pde.name_auxiliary_variable(45,"D311"); D312 = hydro_pde.name_auxiliary_variable(46,"D312"); D313 = hydro_pde.name_auxiliary_variable(47,"D313");
	D322 = hydro_pde.name_auxiliary_variable(48,"D322"); D323 = hydro_pde.name_auxiliary_variable(49,"D323"); D333 = hydro_pde.name_auxiliary_variable(50,"D333");

	K		= hydro_pde.name_auxiliary_variable(51,"K")
	phi		= hydro_pde.name_auxiliary_variable(52,"phi")
	P1		= hydro_pde.name_auxiliary_variable(53,"P1")
	P2		= hydro_pde.name_auxiliary_variable(54,"P2")
	P3		= hydro_pde.name_auxiliary_variable(55,"P3")

	#construct tensors/vectors
	tildeS	= Array([tildeS1, tildeS2, tildeS3])
	v		= Array([v1, v2, v3])
	tg		= Array([[tg11, tg12, tg13],
					[tg12, tg22, tg23],
					[tg13, tg23, tg33]])
	tA		= Array([[tA11, tA12, tA13],
					[tA12, tA22, tA23],
					[tA13, tA23, tA33]])
	beta 	= Array([beta1, beta2, beta3])
	A 		= Array([A1, A2, A3])
	B 		= Array([[B11, B12, B13],
					[B21, B22, B23],
					[B31, B32, B33]])
	Dijk 		= Array([[[D111, D112, D113], [D112, D122, D123], [D113, D123, D133]],
						[[D211, D212, D213], [D212, D222, D223], [D213, D223, D233]],
						[[D311, D312, D313], [D312, D322, D323], [D313, D323, D333]]])
	P 		= Array([P1, P2, P3])

	#main calculation
	Gamma_index = 4/3

	g = tg / (phi*phi) #\gamma_ij
	detg = Matrix(g).det()
	g_up = Array(Matrix(g).inv().tolist()) #\gamma^ij
	tg_up = Array(Matrix(tg).inv().tolist()) #\tilde{\gamma}^ij

	p = (Gamma_index-1)*rho0*epsilon #assuming \Gamma-1 law, the value of \Gamma depends on physics scenarios
	h = 1+epsilon+p/rho0 #specific enthalpy
	V = alpha*tensorcontraction(tensorproduct(g_up, v), (1,2)) - beta #transport velocity, V^i=\alpha \gamma^ij v_j - \beta^i
	v_up = tensorcontraction(tensorproduct(g_up, v), (1,2)) #v^i
	D = tildeD/detg #unscaled density
	tau = tildetau/detg #unscaled energy variables

	#source components in Tij
	E = rho0*h*W**2 - p
	S = rho0*h*W**2*v 	#S_j
	Sij = tensorproduct(S, S)/(rho0*h*W**2) + p*g #S_ij

	Kij = tA/(phi*phi) + (1/3)*K*g #K_ij
	Kij_up = tensorcontraction(tensorproduct(g_up, g_up, Kij), (1,4), (3,5)) #K^ij
	dg_up = 2*phi*tensorproduct(P, tg_up)-2*phi**2*tensorcontraction(tensorproduct(tg_up, tg_up, Dijk), (1, 5), (3, 6)) #\partial_i \gamma^jk

	# -EA_i + S_k B_i^k -0.5 \aplha S_jk \partial_i \gamma^jk 
	S_source = -E*A + tensorcontraction(tensorproduct(S, B), (0,2)) - 0.5*alpha*tensorcontraction(tensorproduct(Sij, dg_up), (0, 3), (1, 4))

	# \alpha S_ij K^ij - \gamma^ij S_j A_i
	tau_source = alpha*tensorcontraction(tensorproduct(Sij, Kij_up), (0, 2), (1, 3)) - tensorcontraction(tensorproduct(g_up, S, A), (0, 3), (1, 2))

	# Flux [unknowns, dimensions]
	for i in range(3):
		hydro_pde.F[0,i] = detg*D*V[i]
		hydro_pde.F[4,i] = tau*V[i] + alpha*p*v_up[i]

	hydro_pde.F[1,0] = detg *(S[0]*V[0]+alpha*p)
	hydro_pde.F[1,1] = detg * S[0]*V[1]
	hydro_pde.F[1,2] = detg * S[0]*V[2]
	hydro_pde.F[2,0] = detg * S[1]*V[0]
	hydro_pde.F[2,1] = detg *(S[1]*V[1]+alpha*p)
	hydro_pde.F[2,2] = detg * S[1]*V[2]
	hydro_pde.F[3,0] = detg * S[2]*V[0]
	hydro_pde.F[3,1] = detg * S[2]*V[1]
	hydro_pde.F[3,2] = detg *(S[2]*V[2]+alpha*p)

	# Source [unknowns]
	hydro_pde.sources[0] = 0
	hydro_pde.sources[1] = S_source[0]
	hydro_pde.sources[2] = S_source[1]
	hydro_pde.sources[3] = S_source[2]
	hydro_pde.sources[4] = tau_source

	v_mag_2 = 1 - 1/W**2 # =\gamma_ij v^i v^j
	c_s_2 = Gamma_index*(Gamma_index-1)*epsilon/(1+Gamma_index*epsilon)  #assuming \Gamma-1 law, (c_s)^2, square of sound speed
	# Eigenvalues [unknowns, dimensions] 
	# only max value matters, so the assignment is not strictly aligned
	for i in range(3):
		# \lambda_+
		hydro_pde.eigenvalues[0, i]= -beta[i]+ alpha/(1-c_s_2*v_mag_2) * ( v[i]*(1-c_s_2) + (c_s_2*(1-v_mag_2)*(g[i][i]*(1-c_s_2*v_mag_2)-v_up[i]*v_up[i]*(1-c_s_2)))**0.5 )
		# \lambda_-
		hydro_pde.eigenvalues[4, i]= -beta[i]+ alpha/(1-c_s_2*v_mag_2) * ( v[i]*(1-c_s_2) - (c_s_2*(1-v_mag_2)*(g[i][i]*(1-c_s_2*v_mag_2)-v_up[i]*v_up[i]*(1-c_s_2)))**0.5 )

		hydro_pde.eigenvalues[1, i]= V[i]
		hydro_pde.eigenvalues[2, i]= V[i]
		hydro_pde.eigenvalues[3, i]= V[i]


	# no initial condition and boundary condition yet

	print(hydro_pde.implementation_of_eigenvalues())

	return hydro_pde
