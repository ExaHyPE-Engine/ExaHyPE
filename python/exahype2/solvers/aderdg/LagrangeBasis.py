import mpmath as mp
import numpy as np
from sympy.integrals.quadrature import gauss_legendre, gauss_lobatto # nodes by default in [-1,1]

PREC_DIGITS = 300

mp.dps=PREC_DIGITS

class LagrangeBasis():
    def __init__(self,num_points):
        self._num_points = num_points
        self._max_poly_order = num_points - 1
        # nodes, weights
        self._nodes, self._weights = self._compute_nodes_and_weights(num_points)
        # operators
        self._mass_matrix                = self._compute_mass_matrix()
        self._stiffness_matrix           = self._compute_stiffness_matrix()
        self._K1                         = self._compute_K1()
        self._F0                         = self._compute_time_flux_matrix_F0()
        self._dudx                       = self._compute_discrete_derivative_operator()
        self._equidistant_grid_projector = self._compute_equidistant_grid_projector()
        self._fine_grid_projector = [None]*3
        for j in range(0,3):
            self._fine_grid_projector[j] = self._compute_fine_grid_projector(j)
    def __str__(self):
        return "{}: {}".format(type(self).__name__,self.__dict__)

    def _compute_nodes_and_weights(self,num_points):
        """
        To be implemented by subclass.
        """
        assert False, "To be implemented by subclass."
    def _evaluate(self,xi):
        """
        Evaluates a lagrange polynomial and its first derivative at a given point.
        
        Args:
           xi:
              The reference element point the basis functions are evaluated at.
              Here, xi refers to the greek letter that is often used as a reference element coordinate.
        Returns:
           phi:
              Basis function values.
           phi_xi:
              First derivatives of the basis functions.
        """
        phi    = [mp.mpf(1.)]*(self._num_points) 
        phi_xi = [mp.mpf(0.)]*(self._num_points)
        xin = self._nodes
        for m in range(0,self._num_points):
            for j in range(0,self._num_points):
                if j == m:
                    continue 
                phi[m] = phi[m]*(xi-xin[j])/(xin[m]-xin[j])
            for i in range(0,self._num_points):
                if i == m:
                    continue
                tmp = 1.;
                for j in range(0,self._num_points):
                    if j == i:
                        continue
                    if j == m:
                        continue
                    tmp = tmp*(xi-xin[j])/(xin[m]-xin[j])
                phi_xi[m] += tmp/(xin[m]-xin[i])
        return phi, phi_xi    
    def _compute_mass_matrix(self):
        """
        Computes the (reference) element mass matrix for an approximation of
        order self._max_poly_order.
    
        Returns:
           M_xi:
              The (reference) element mass matrix.
        """
        # init matrix with zeros
        MM = [[mp.mpf(0) for _ in range(self._num_points)] for _ in range(self._num_points)]
        
        for i in range(0,self._num_points):
            phi, _ = self._evaluate(self._nodes[i])
            for k in range(0,self._num_points):
                for l in range(0,self._num_points):
                    MM[k][l] += self._weights[i]*phi[k]*phi[l]
        return MM
    def _compute_stiffness_matrix(self):
        """
        Computes the (reference) element stiffness matrix for an approximation of order self._max_poly_order.
        Returns:
           K_xi:
              The (reference) element stiffness matrix.
        """
        # init matrix with zero
        Kxi = [[mp.mpf(0) for _ in range(self._num_points)] for _ in range(self._num_points)]
             
        for i in range(0,self._num_points):
            phi, phi_xi = self._evaluate(self._nodes[i])
            for k in range(0,self._num_points):
                Kxi[k][i] = self._weights[i]*phi_xi[k]*phi[i] 
        return Kxi
    def _compute_K1(self):
        """
        Computes the difference between the reference element mass operator 
        evaluated at point xi=1.0 and the element stiffness matrix.
        
        Returns:
           K1:
              delta between the reference element mass operator and the element stiffness matrix
        """
        phi1, _ = self._evaluate(1.0)
        FRm = [[mp.mpf(0) for _ in range(self._num_points)] for _ in range(self._num_points)]
        
        for k in range(0, self._num_points):
            for l in range(0, self._num_points):
                FRm[k][l] = phi1[k]*phi1[l] 
        
        K1 = np.subtract(FRm,self._stiffness_matrix)
        return K1        
            
    def _compute_time_flux_matrix_F0(self):
        """
        Evaluates the reference basis functions at point xi=0.0.
    
        Returns:
           phi:
              The reference basis functions evaluated at point xi=0.0.
        """
        phi, _ = self._evaluate(mp.mpf(0.0))
        return phi         
            
    def _compute_discrete_derivative_operator(self):
        """
        Computes some derivative values for debugging purposes.
    
        Returns:
           dudx:
              Derivative values for debugging purposes.
        """
        dudx = np.dot(mp.inverse(self._mass_matrix).tolist(),np.transpose(self._stiffness_matrix))
        return dudx
    def _compute_fine_grid_projector(self, j):
        """
        Transforms the degrees of freedom located on a coarse grid edge
        nodes to degrees of freedoms located on nodes of a fine grid edge.
        The difference in levels is 1.
        
        Let us denote by P the  fine grid projector (= equidistantGridProjector). The fine grid DoF 
        are computed according to:
        
        u^{fine;j}_i =  sum_{m} P^{j}_im u^{coarse}_m
       
        Args:
          j:
            subinterval index
        
        Returns:
           fineGridProjector:
              Operator to express polynomial function associated with original interval with basis functions associated with subinterval j
        """
        fineGridProjector = [[mp.mpf(0) for _ in range(self._num_points)] for _ in range(self._num_points)] # 4 x 4 for self._max_poly_order=3
        
        for i in range(0, self._num_points): # Eq. basis
            phi_i, _ = self._evaluate((self._nodes[i]+j)/mp.mpf(3.0)) # comma after phi_i is important
            for m in range(0, self._num_points): # DG basis
                fineGridProjector[m][i] = phi_i[m]
        return fineGridProjector
    def _compute_equidistant_grid_projector(self):
        """
        Transforms the degrees of freedom located at non-equidistant Lagrange support points
        nodes to degrees of freedoms located at nodes of an equidistant grid over (0,1).
        
        Let us denote by P the  projection operator (= equidistantGridProjector). The equidistant DoF 
        are computed according to:
        
        u^eq_i =  sum_{m} P_im u^DG_m
        
        Returns:
           equidistantGridProjector:
              The corresponding degrees of freedom located at nodes of an equidistant grid over (0,1).
        """
        equidistantGridProjector = [[mp.mpf(0) for _ in range(self._num_points)] for _ in range(self._num_points)] # 4 x 4 for self._max_poly_order=3
        subxi = mp.linspace(mp.mpf(0.0), mp.mpf(1.0), self._num_points)
        
        for i in range(0, self._num_points): # Eq. basis
            phi_i, _ = self._evaluate(subxi[i]) # comma after phi_i is important
            for m in range(0, self._num_points): # DG basis
                equidistantGridProjector[m][i] = phi_i[m]
        return equidistantGridProjector

def _transform_and_sort_nodes_and_weights(nodes,weights):
    """
    Transform quadrature nodes and weights for interval [-1,1]
    to nodes and weights for interval [0,1].
    """
    transformed_weights = []
    transformed_nodes   = []
    # weight
    for i,w in enumerate(weights):
        x = nodes[i]
        wT = mp.mpf(w)/mp.mpf(2.0)
        xT = (mp.mpf(x)+mp.mpf(1.0))/mp.mpf(2.0)
        transformed_weights.append(wT)
        transformed_nodes.append(xT)
    # sort
    sorted_transformed_nodes = sorted(transformed_nodes)
    sorted_transformed_weights = []
    for i,xs in enumerate(sorted_transformed_nodes):
        n  = transformed_nodes.index(xs) 
        ws = transformed_weights[n]
        sorted_transformed_weights.append(ws)
    return sorted_transformed_nodes, sorted_transformed_weights

class GaussLegendreBasis(LagrangeBasis):
    def _compute_nodes_and_weights(self,num_points):
        x, w = gauss_legendre(num_points,PREC_DIGITS)
        return _transform_and_sort_nodes_and_weights(x,w)    

class GaussLobattoBasis(LagrangeBasis):
    def _compute_nodes_and_weights(self,num_points):
        x, w = gauss_lobatto(num_points,PREC_DIGITS)
        return _transform_and_sort_nodes_and_weights(x,w)    
