import sympy
import numpy as np

import peano4
import exahype2

hydro_pde= exahype2.symhype.FirstOrderConservativePDEFormulation(
	unknowns=5, auxiliary_variables=5+51, dimensions=3)

#evolving quantities
tildeD  = hydro_pde.name_Q_entry(0,"tildeD")
tildeS1	= hydro_pde.name_Q_entry(1,"tildeS1")
tildeS2	= hydro_pde.name_Q_entry(2,"tildeS2")
tildeS3	= hydro_pde.name_Q_entry(3,"tildeS3")
tildetau= hydro_pde.name_Q_entry(4,"tildetau")

#primitive quantities
rho0    = hydro_pde.name_auxiliary_variable(0,"rho0")
v1      = hydro_pde.name_auxiliary_variable(1,"v1")
v2      = hydro_pde.name_auxiliary_variable(2,"v2")
v3      = hydro_pde.name_auxiliary_variable(3,"v3")
epsilon = hydro_pde.name_auxiliary_variable(4,"epsilon")

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


tildeS	= Array([tildeS1,tildeS2,tildeS3])
v		= Array([v1,v2,v3])
tg		= Array([[tg11,tg12,tg13],
		  		 [tg12,tg22,tg23],
		  		 [tg13,tg23,tg33]])
tA		= Array([[tA11,tA12,tA13],
		  		 [tA12,tA22,tA23],
		  		 [tA13,tA23,tA33]])

