from .block_matrix import BlockMatrix
from .nodal_basis import NodalBasisGaussLegendre
from .finiteelement import DGScalarElement, DGFacetElement
from .quadrature import GaussianQuadrature, Quadrature2d
import numpy as np


class DGPoisson2dBlockMatrix(BlockMatrix):
    """Classical DG discretisation of 2d classical Poisson formulation with spurious facet function spaces

    Order of function spaces (used in the output dictionaries):
    0 |  0   1   2
    p | p^+ p^- p^f
    """

    def __init__(self, r_press, r_vel, basis=NodalBasisGaussLegendre(2)):
        """Initialise instance
        :arg r_press: polynomial degree of pressure space
        :arg r_vel: polynomial degree of velocity
        :arg basis: nodal basis
        """
        dim = 2
        super().__init__(dim, basis)
        self.basis = basis
        self._L_C = [r_press]
        self._L_F = 3 * [r_press]
        self._fs_labels_C = ["p"]
        self._fs_labels_F = ["p+", "p-", "pf"]

        # All pairs of function spaces that couple
        self._coupling_CC = [(0, 0)] # _p_p
        
        self._coupling_CF = []
        
        self._coupling_FC = []
        
        self._coupling_FF = []
        
        # Lists of entity types
        self._ent_types_CF_FC = ["left", "right", "bottom", "top"]
        #self._ent_types_FF = ["ih", "iv", "bl", "br", "bb", "bt"] # "interior horizontal/vertical" and "boundary left/right/bottom/top"

        # Cell elements
        self._fe_C = [None] * len(self._L_C)
        for i in range(len(self._L_C)):
            self._fe_C[i] = DGScalarElement(dim, self._L_C[i])

        # Facet elements
        self._fe_F = [None] * len(self._L_F)
        for i in range(len(self._L_F)):
            self._fe_F[i] = DGFacetElement(dim, self._L_F[i])


        self._A_CC = {fs_pair: self._assemble_A_CC(fs_pair) for fs_pair in self._coupling_CC}

        # self._A_CF = {fs_pair: {ent_type: self._assemble_A_CF(fs_pair, ent_type) for ent_type in self._ent_types_CF_FC} 
        #                   for fs_pair in self._coupling_CF
        #                  }

        # self._A_FC = {fs_pair: {ent_type: self._assemble_A_FC(fs_pair, ent_type) for ent_type in self._ent_types_CF_FC} 
        #                   for fs_pair in self._coupling_FC
        #                  }

        # self._A_FF = {fs_pair: {ent_type: self._assemble_A_FF(fs_pair, ent_type) for ent_type in self._ent_types_FF} 
        #                   for fs_pair in self._coupling_FF
        #                  }

    ###################################
    ### Assemble weak form for A_CC ###
    ###################################
    def _assemble_A_CC(self, fs_pair):
        """Assemble weak form for A_{CC}^{(k,ell)}
        :arg fs_pair: pair of cell function space indices that couple
        k = fs_pair[0]: first function space (to-space): a cell function space
        ell = fs_pair[1]: second function space (from-space): a cell function space
        """
        #assert fs_pair in self._coupling_CC

        k = fs_pair[0]
        ell = fs_pair[1]
        ndof_k = self._fe_C[k].ndof
        ndof_ell = self._fe_C[ell].ndof
        A_k_ell = np.empty((ndof_k, ndof_ell))
        gamma = (0, 0) # no sacling, as the contribution h^2 from dxdy is compensated by 1/h^2 from derivatives \nambla u \cdot \nabla v

        # Quadrature rule: based on the max degree of basis in the function spaces pair
        max_degree = max(self._fe_C[k].degree, self._fe_C[ell].degree)
        quad2d = Quadrature2d(GaussianQuadrature(max_degree + 1))

        if fs_pair == (0, 0):
            #func_k = self._fe_C[k].evaluate
            #func_ell = self._fe_C[ell].evaluate
            factor = 1.0
            
            # Assemble local matrix A^(k,ell)
            for i in range(ndof_k):
                for j in range(ndof_ell):
                    A_k_ell[i, j] = 0.0
                    for q, w in zip(quad2d.points, quad2d.weights):
                        A_k_ell[i, j] += factor * w * (  self._fe_C[k].evaluate_derivative(i, 0, q) * self._fe_C[ell].evaluate_derivative(j, 0, q)
                                                         +
                                                         self._fe_C[k].evaluate_derivative(i, 1, q) * self._fe_C[ell].evaluate_derivative(j, 1, q)
                                                      )
        else:
            A_k_ell = np.zeros((k, ell))

        return {gamma: A_k_ell}

    # ###################################
    # ### Assemble weak form for A_CF ###
    # ###################################   
    # def _assemble_A_CF(self, fs_pair, relative_position):
    #     """Assemble weak form for A_{CF}^{(k,ell)}
    #     :arg fs_pair: pair of function space indices that couple
    #     k = fs_pair[0]: first function space (to-space): a cell function space
    #     ell = fs_pair[1]: second function space (from-space): a facet function space
    #     :arg relative_position: relative position of the facet within a cell given by
    #     the "type of entity"
    #     """
    #     k = fs_pair[0]
    #     ell = fs_pair[1]
    #     ndof_k = self._fe_C[k].ndof
    #     ndof_ell = self._fe_F[ell].ndof
    #     A_k_ell = np.empty((ndof_k, ndof_ell))
    #     gamma = (0, 0)

    #     # Quadrature rule: based on the max degree of basis in the function spaces pair
    #     max_degree = max(self._fe_C[k].degree, self._fe_F[ell].degree)
    #     quad = GaussianQuadrature(max_degree + 1)

    #     # Matrix restr applied to [1, q] gives [-1, q], [1, q], [q, -1] or [q, 1] -- 
    #     # 2D coordinates on a facet depending on its relative location
    #     # Is's used to later restrict a cell-function to a facet for integrating over facet
    #     if relative_position == "left":
    #         restr = np.array([[-1.0, 0.0],
    #                           [ 0.0, 1.0]])
    #         nx, ny = -1.0, 0.0 # outward unit normal components
    #     elif relative_position == "right":
    #         restr = np.array([[1.0, 0.0],
    #                           [0.0, 1.0]])
    #         nx, ny = +1.0, 0.0
    #     elif relative_position == "bottom":
    #         restr = np.array([[ 0.0, 1.0],
    #                           [-1.0, 0.0]])
    #         nx, ny = 0.0, -1.0
    #     elif relative_position == "top":
    #         restr = np.array([[0.0, 1.0],
    #                           [1.0, 0.0]])
    #         nx, ny = 0.0, +1.0
    #     else:
    #         raise ValueError("Invalid value of relative_position")        
        
    #     # The following sets factors and integrands for all the pairs (k,ell);
    #     # Normal conponents nx, ny appear in the matrices A_Qx_pf and A_Qy_pf
    #     if k == 0:
    #         if ell == 5: # A_p_qf
    #             factor = 0.5                
    #         else:
    #             return {gamma, np.zeros((ndof_k, ndof_ell))}
            
    #     elif k == 1:
    #         if ell == 4: # A_Qx_pf
    #             factor = 0.5 * nx
    #         else:
    #             return {gamma, np.zeros((ndof_k, ndof_ell))}
            
    #     elif k == 2:
    #         if ell == 4: # A_Qy_pf
    #             factor = 0.5 * ny
    #         else:
    #             return {gamma, np.zeros((ndof_k, ndof_ell))}
            
    #     else:
    #         return {gamma, np.zeros((ndof_k, ndof_ell))}
        
    #     # Scaling factors
    #     if   relative_position in ["bottom", "top"]:
    #         gamma = (1, 0)
    #     elif relative_position in ["left", "right"]:
    #         gamma = (0, 1)
    #     else:
    #         raise ValueError("Invalid value of relative_position") 
        
    #     # Define the integrand function for different pairs (k,ell)
    #     # Here we have \int_F v^k * v^ell dF for all cases, no derivatives
    #     def func_k(i, q):
    #         facet_coord = np.dot(restr, np.array([1.0, q])) # gives [-1, q], [1, q], [q, -1] or [q, 1]
    #         return self._fe_C[k].evaluate(i, facet_coord) # restriction of the cell-function to a facet
    #     func_ell = self._fe_F[ell].evaluate
        
    #     # Assemble local matrix A^(k,ell)
    #     for i in range(ndof_k):
    #         for j in range(ndof_ell):
    #             A_k_ell[i, j] = 0.0
    #             for q, w in zip(quad.points, quad.weights):
    #                 A_k_ell[i, j] += factor * w * func_k(i, q) * func_ell(j, q)

    #     #print(k, ell, relative_position, "\n", A_k_ell, "\n")
    #     return {gamma: A_k_ell}

    # ###################################
    # ### Assemble weak form for A_FC ###
    # ###################################
    # def _assemble_A_FC(self, fs_pair, relative_position):
    #     """Assemble weak form for A_{CF}^{(k,ell)}
    #     :arg fs_pair: pair of function space indices that couple
    #     k = fs_pair[0]: first function space (to-space): a facet function space
    #     ell = fs_pair[1]: second function space (from-space): a cell function space
    #     :arg relative_position: relative position of the facet within a cell
    #     """
    #     k = fs_pair[0]
    #     ell = fs_pair[1]
    #     ndof_k = self._fe_F[k].ndof
    #     ndof_ell = self._fe_C[ell].ndof
    #     A_k_ell = np.empty((ndof_k, ndof_ell))
    #     gamma = (0, 0)

    #     # Quadrature rule: based on the max degree of basis in the function spaces pair
    #     max_degree = max(self._fe_F[k].degree, self._fe_C[ell].degree)
    #     quad = GaussianQuadrature(max_degree + 1)

    #     # Matrix restr applied to [1, q] gives [-1, q], [1, q], [q, -1] or [q, 1] -- 
    #     # 2D coordinates on a facet depending on its relative location
    #     # Is's used to later restrict a cell-function to a facet for integrating over facet
    #     if relative_position == "left":
    #         restr = np.array([[-1.0, 0.0],
    #                           [ 0.0, 1.0]])
    #         nx, ny = -1.0, 0.0 # outward unit normal components
    #     elif relative_position == "right":
    #         restr = np.array([[1.0, 0.0],
    #                           [0.0, 1.0]])
    #         nx, ny = +1.0, 0.0
    #     elif relative_position == "bottom":
    #         restr = np.array([[ 0.0, 1.0],
    #                           [-1.0, 0.0]])
    #         nx, ny = 0.0, -1.0
    #     elif relative_position == "top":
    #         restr = np.array([[0.0, 1.0],
    #                           [1.0, 0.0]])
    #         nx, ny = 0.0, +1.0
    #     else:
    #         raise ValueError("Invalid value of relative_position")
        
    #     # The following sets factors for all the pairs (k,ell)
    #     if k == 0:
    #         if ell == 0: # A_pp_p
    #             if relative_position in ["left", "bottom"]:
    #                 factor = 0.5
    #             else:
    #                 # "+"-projections are only defined for the left and bottom facets,
    #                 # so A_pp_p = 0 for the right and top
    #                 # ! This is because the cell is "+" from the perspective of the left and bottom facet !
    #                 return {gamma: np.zeros((ndof_k, ndof_ell))}
    #         else:
    #             return {gamma: np.zeros((ndof_k, ndof_ell))}
            
    #     elif k == 1:
    #         if ell == 0: # A_pm_p
    #             if relative_position in ["right", "top"]:
    #                 factor = -0.5 # minus comes from the def. of L2-projection: <p^\pm, v^{p^\pm}> - <p|_F, v^{p^\pm}> = 0
    #             else:
    #                 # "-"-projections are only defined for the right and top facets,
    #                 # so A_pm_p = 0 for the left and bottom
    #                 # ! This is because the cell is "+" from the perspective of the left and bottom facet !
    #                 return {gamma: np.zeros((ndof_k, ndof_ell))}
    #         else:
    #             return {gamma: np.zeros((ndof_k, ndof_ell))}
            
    #     elif k == 2:
    #         if ell == 1: # A_qp_Qx
    #             if relative_position == "left":
    #                 factor = -0.5 * nx # minus comes from the def. of L2-projection in the form <...> - <...> = 0
    #             else:
    #                 # A_qp_Qx is non-zero only on the left facet because of 1) definition of plus-pjections, 2) nx = 0 on the top and bottom
    #                 return {gamma: np.zeros((ndof_k, ndof_ell))}
                
    #         elif ell == 2: # A_qp_Qy
    #             if relative_position == "bottom":
    #                 factor = -0.5 * ny
    #             else:
    #                 # A_qp_Qy is non-zero only on the bottom facet
    #                 return {gamma: np.zeros((ndof_k, ndof_ell))}
            
    #     elif k == 3:
    #         if ell == 1: # A_qm_Qx
    #             if relative_position == "right":
    #                 factor = -0.5 * nx
    #             else:
    #                 return {gamma: np.zeros((ndof_k, ndof_ell))}
                
    #         elif ell == 2: # A_qm_Qy
    #             if relative_position == "top":
    #                 factor = -0.5 * ny
    #             else:
    #                 return {gamma: np.zeros((ndof_k, ndof_ell))}
                
    #         else:
    #             return {gamma: np.zeros((ndof_k, ndof_ell))}
            
    #     else:
    #         return {gamma: np.zeros((ndof_k, ndof_ell))}
        
    #     # Scaling factors
    #     if   relative_position in ["bottom", "top"]:
    #         gamma = (1, 0)
    #     elif relative_position in ["left", "right"]:
    #         gamma = (0, 1)
    #     else:
    #         raise ValueError("Invalid value of relative_position")
        
    #     # Define the integrand function
    #     # Here we have \int_F v^k * v^ell dF for all (k,ell) pairs, no derivatives
    #     func_k = self._fe_F[k].evaluate
    #     def func_ell(i, q):
    #         facet_coord = np.dot(restr, np.array([1.0, q])) # gives [-1, q], [1, q], [q, -1] or [q, 1]
    #         return self._fe_C[ell].evaluate(i, facet_coord) # restriction of the cell-function to a facet
        
    #     # Assemble local matrix A^(k,ell)
    #     for i in range(ndof_k):
    #         for j in range(ndof_ell):
    #             A_k_ell[i, j] = 0.0
    #             for q, w in zip(quad.points, quad.weights):
    #                 A_k_ell[i, j] += factor * w * func_k(i, q) * func_ell(j, q)

    #     #print(k, ell, relative_position, "\n", A_k_ell, "\n")
    #     return {gamma: A_k_ell}
    
    # ###################################
    # ### Assemble weak form for A_FF ###
    # ###################################
    # def _assemble_A_FF(self, fs_pair, relative_position):
    #     """Assemble weak form for A_{FF}^{(k,ell)}
    #                      :arg fs_pair: pair of function space indices that couple
    #     k = fs_pair[0]: first function space (to-space): a facet function space
    #     ell = fs_pair[1]: second function space (from-space): a facet function space
    #     :arg relative_position: relative position of the facet within a cell
    #     """
    #     k = fs_pair[0]
    #     ell = fs_pair[1]
    #     ndof_k = self._fe_F[k].ndof
    #     ndof_ell = self._fe_F[ell].ndof
    #     A_k_ell = np.empty((ndof_k, ndof_ell))
    #     gamma = (0, 0)

    #     # Quadrature rule: based on the max degree of basis in the function spaces pair
    #     max_degree = max(self._fe_F[k].degree, self._fe_F[ell].degree)
    #     quad = GaussianQuadrature(max_degree + 1)
        
    #     if fs_pair in [(0,0), (1,1), (2,2), (3,3)]: # Facet space mass matrices A_pp_pp, _pm_pm, _qp_qp, _qm_qm
    #         func_k = self._fe_F[k].evaluate
    #         func_ell = self._fe_F[ell].evaluate
    #         factor = 0.5
    #         # Assemble local matrix A^(k,ell)
    #         for i in range(ndof_k):
    #             for j in range(ndof_ell):
    #                 A_k_ell[i, j] = 0.0
    #                 for q, w in zip(quad.points, quad.weights):
    #                     A_k_ell[i, j] += factor * w * func_k(i, q) * func_ell(j, q)

    #         if relative_position in ["iv", "bl", "br"]:
    #             gamma = (0, 1) # vertical facets, scale by hy
    #         elif relative_position in ["ih", "bb", "bt"]:
    #             gamma = (1, 0) # horizontal facets, scale by hy
    #         else:
    #             raise ValueError("Invalid value of relative_position")
            
    #     elif fs_pair == (4, 4): # A_pf_pf
    #         if relative_position in ["ih", "iv"]:
    #             # This if for defining the flux: p^f = 0.5( p^+ + p^- ). (0, 0) means it doesn't need to be scaled
    #             gamma = (0, 0)
    #             A_k_ell = np.eye(ndof_k)
    #         elif relative_position in ["bl", "br", "bb", "bt"]:
    #             # Return mas matrix A_pf_pf for Dirichelt boundary conditions
    #             func_k = self._fe_F[k].evaluate
    #             func_ell = self._fe_F[ell].evaluate
    #             factor = 0.5
    #             # Assemble local matrix A^(k,ell)
    #             for i in range(ndof_k):
    #                 for j in range(ndof_ell):
    #                     A_k_ell[i, j] = 0.0
    #                     for q, w in zip(quad.points, quad.weights):
    #                         A_k_ell[i, j] += factor * w * func_k(i, q) * func_ell(j, q)

    #             if relative_position in ["bl", "br"]:
    #                 gamma = (0, 1)
    #             else:
    #                 gamma = (1, 0)
    #         else:
    #             raise ValueError("Invalid value of relative_position")
            
    #     elif fs_pair == (5, 5): # A_qf_qf
    #         # Defines flux variable for the interior facet and decouples for the boundaries
    #         gamma = (0, 0)
    #         A_k_ell = np.eye(ndof_k)
        
    #     elif fs_pair in [(4,0), (4,1), (5,2), (5,3)]:
    #         # Coupling projection variables p^\pm, q^pm with fluxes p^f, q^f
    #         # Here we actually assume that projections and fluxes are from the same space and linked linearly
    #         if relative_position in ["ih", "iv"]:
    #             gamma = (0, 0)
    #             A_k_ell = -0.5*np.eye(ndof_k)
    #         else:
    #             gamma = (0, 0)
    #             A_k_ell = np.zeros((ndof_k, ndof_ell))
            
    #     else:
    #         return {(0, 0): np.zeros((ndof_k, ndof_ell))}
        
    #     #print(k, ell, relative_position, "\n", A_k_ell, "\n")
    #     return {gamma: A_k_ell}

# r_press = 1
# r_vel = 1
# BM = DGPoisson2dBlockMatrix(r_press, r_vel)
# pass