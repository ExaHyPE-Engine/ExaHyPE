from math import sqrt
from numpy import zeros
def compute_net_updates(h_l, h_r, hu_l, hu_r, b_l, b_r):
    if b_r >= 0 and b_l >= 0:
        return (0, 0, 0, 0, 0)
    if b_r >= 0 and b_l < 0:
        h_r = h_l
        hu_r = -hu_l
        b_r = b_l
    if b_r < 0 and b_l >= 0:
        h_l = h_r
        hu_l = -hu_r
        b_l = b_r

    if h_l == h_r and b_l == b_r and hu_l == hu_r:
        # steady state
        return (0, 0, 0, 0, 0)

    #############################################################################################
    # The following code is translated from:                                                    #
    # https://github.com/BrisaDavis/adjoint/releases/tag/paper1_aspublished                     #
    # file rp1_geoclaw_adjoint.f                                                                #
    # or                                                                                        #
    # https://github.com/BrisaDavis/adjoint/blob/adjoint_amr/src/riemann/rpn2_geoclaw_adjoint.f #
    #############################################################################################

    hLhat = -b_l
    hRhat = -b_r

    cL = sqrt(9.81 * hLhat) # linearized speed left
    cR = sqrt(9.81 * hRhat) # linearized speed right

    # f-wave splitting
    delta = zeros(3)
    delta[0] = -hu_r * cR**2 + hu_l * cL**2
    delta[1] = -(h_r + b_r) + (h_l + b_l)
    delta[2] = 0

    beta = zeros(3)
    beta[0] = (delta[0] + cR * delta[1]) 
    beta[1] = (-delta[0] + cL * delta[1])
    beta[2] = delta[2]
    if (cL + cR) != 0:
        beta[0] = beta[0] / (cL + cR)
        beta[1] = beta[1] / (cL + cR)

    # compute waves
    fw = zeros((2,2)) # f-waves
    sw = zeros(2) # wave speeds

    fw[0][0] = cL * beta[0]
    fw[1][0] = beta[0]
    sw[0] = -cL

    fw[0][1] = -cR * beta[1]
    fw[1][1] = beta[1]
    sw[1] = cR

    amdq = zeros(2) # left going flux
    apdq = zeros(2) # right going flux

    # determine  left and right going fluxes
    for i in [0, 1]:
        if sw[i] < 0:
            amdq = amdq + fw.T[i]
        elif sw[i] > 0:
            apdq = apdq + fw.T[i]
        else:
            amdq = amdq + 0.5 * fw.T[i]
            apdq = apdq + 0.5 * fw.T[i]

    #########################################################################
    
    return (amdq[0], amdq[1], apdq[0], apdq[1], max(abs(sw)))