Order and description of tests:

1. Check that the DG solver leaves the input invariant if the RHS is constructed as b = A_DG * u_{random} and u = u_{random} is the initial guess

2. The same as Test 1, but for CG: Check that the CG solver leaves the input invariant if the RHS is constructed as b_c = A_CG * u_{c,random} and u_c = u_{c,random} is the initial guess

3. Check that the CG solver converges to u_{c,random} for the RHS b_c = A_CG * u_{c,random} with some random u_{c,random}.

4. Check that u_c * R * v = v * P * u_c for random u_c, v, where R and P are the restriction and prolongation operators, respectively.

5. Check that R * A_DG * P * u_{c,random} = A_CG * u_{c,random} and R * M_DG * P u_{c,random} = M_CG * u_{c,random} with some random u_{c,random}, where R and P are the restriction and prolongation operators, respectively, A_DG, A_CG are system matrices and M_DG, M_CG are mass matrices for DG and CG, respectively. 

6. The same as Test 3, but for DG: Check that the DG solver converges to u_{random} for the RHS b = A_DG * u_{random} with some random u_{random}.

7. Check that the multigrid two-cycle converges in one iteration if pre-smoothing is switched off and we set b = A_DG * P * u_{c,random}

8. Check that A_DG and A_CG really approximate the continuum operators: A_DG * u = M * b where u, b are the dof-vectors of the functions u(x), b(x) which satisfy the PDE in the continuum -Laplace u(x) = b(x).