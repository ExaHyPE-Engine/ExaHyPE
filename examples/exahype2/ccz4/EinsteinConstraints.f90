SUBROUTINE EnforceCCZ4Constraints(luh)
    USE MainVariables, ONLY : nVar  
    IMPLICIT NONE
    ! Argument list
    REAL, INTENT(INOUT)  :: luh(nVar)              ! numerical solution  
    ! Local variables
    INTEGER :: i,j,k,l,iVar,iDim, iter
    REAL    :: Q(nVar) 
    REAL    :: g_cov(3,3), det, g_contr(3,3), Aex(3,3), traceA, phi  
    REAL    :: DD(3,3,3), traceDk 
#if defined(CCZ4EINSTEIN) || defined(CCZ4GRMHD) || defined(CCZ4GRHD) || defined(CCZ4GRGPR) 
    !
            Q = luh(:) 
            ! 
            g_cov(1,1) = Q(1)
            g_cov(1,2) = Q(2)
            g_cov(1,3) = Q(3)
            g_cov(2,1) = Q(2)
            g_cov(2,2) = Q(4)
            g_cov(2,3) = Q(5)
            g_cov(3,1) = Q(3)
            g_cov(3,2) = Q(5)
            g_cov(3,3) = Q(6)
            ! This determinant should be close to unity, since we use the conformal decomposition 
            det = (Q(1)*Q(4)*Q(6)-Q(1)*Q(5)**2-Q(2)**2*Q(6)+2*Q(2)*Q(3)*Q(5)-Q(3)**2*Q(4)) 
            g_contr(1,1) = (Q(4)*Q(6)-Q(5)**2)   / det 
            g_contr(1,2) = -(Q(2)*Q(6)-Q(3)*Q(5))/ det 
            g_contr(1,3) = (Q(2)*Q(5)-Q(3)*Q(4)) / det 
            g_contr(2,1) = -(Q(2)*Q(6)-Q(3)*Q(5))/ det 
            g_contr(2,2) = (Q(1)*Q(6)-Q(3)**2)   / det 
            g_contr(2,3) = -(Q(1)*Q(5)-Q(2)*Q(3))/ det 
            g_contr(3,1) = (Q(2)*Q(5)-Q(3)*Q(4)) / det 
            g_contr(3,2) = -(Q(1)*Q(5)-Q(2)*Q(3))/ det 
            g_contr(3,3) = (Q(1)*Q(4)-Q(2)**2)   / det 
            !
            phi = det**(-1./6.) 
            g_cov = phi**2*g_cov
            det = (g_cov(1,1)*g_cov(2,2)*g_cov(3,3)-g_cov(1,1)*g_cov(2,3)*g_cov(3,2)-g_cov(2,1)*g_cov(1,2)*g_cov(3,3)+g_cov(2,1)*g_cov(1,3)*g_cov(3,2)+g_cov(3,1)*g_cov(1,2)*g_cov(2,3)-g_cov(3,1)*g_cov(1,3)*g_cov(2,2)) 
            g_contr(1,1) =  (g_cov(2,2)*g_cov(3,3)-g_cov(2,3)*g_cov(3,2)) / det 
            g_contr(1,2) = -(g_cov(1,2)*g_cov(3,3)-g_cov(1,3)*g_cov(3,2)) / det
            g_contr(1,3) = -(-g_cov(1,2)*g_cov(2,3)+g_cov(1,3)*g_cov(2,2))/ det 
            g_contr(2,1) = -(g_cov(2,1)*g_cov(3,3)-g_cov(2,3)*g_cov(3,1)) / det 
            g_contr(2,2) = (g_cov(1,1)*g_cov(3,3)-g_cov(1,3)*g_cov(3,1))  / det 
            g_contr(2,3) = -(g_cov(1,1)*g_cov(2,3)-g_cov(1,3)*g_cov(2,1)) / det 
            g_contr(3,1) = -(-g_cov(2,1)*g_cov(3,2)+g_cov(2,2)*g_cov(3,1))/ det 
            g_contr(3,2) = -(g_cov(1,1)*g_cov(3,2)-g_cov(1,2)*g_cov(3,1)) / det 
            g_contr(3,3) = (g_cov(1,1)*g_cov(2,2)-g_cov(1,2)*g_cov(2,1))  / det 
            !
            Aex(1,1) = Q(7) 
            Aex(1,2) = Q(8) 
            Aex(1,3) = Q(9) 
            Aex(2,1) = Q(8) 
            Aex(2,2) = Q(10) 
            Aex(2,3) = Q(11) 
            Aex(3,1) = Q(9) 
            Aex(3,2) = Q(11) 
            Aex(3,3) = Q(12)             
            !
            traceA = SUM(g_contr*Aex) 
            !
            Aex = Aex - 1./3.*g_cov*traceA 
            !
            luh( 1) = g_cov(1,1) 
            luh( 2) = g_cov(1,2) 
            luh( 3) = g_cov(1,3) 
            luh( 4) = g_cov(2,2) 
            luh( 5) = g_cov(2,3) 
            luh( 6) = g_cov(3,3) 
            !
            luh( 7) = Aex(1,1) 
            luh( 8) = Aex(1,2) 
            luh( 9) = Aex(1,3) 
            luh(10) = Aex(2,2) 
            luh(11) = Aex(2,3) 
            luh(12) = Aex(3,3)             
            !
            ! As suggested by our PRD referee, we also enforce the algebraic constraint that results from the first spatial derivative of the constraint
            ! det \tilde{\gamma}_ij = 0, which leads to
            !
            ! \tilde{\gamma}^{ij} D_kij = 0
            !
            ! and is thus a condition of trace-freeness on all submatrices D_kij for k=1,2,3.
            !
            DD(1,1,1)=Q(36)
            DD(1,1,2)=Q(37)
            DD(1,1,3)=Q(38)
            DD(1,2,1)=Q(37)
            DD(1,2,2)=Q(39)
            DD(1,2,3)=Q(40)
            DD(1,3,1)=Q(38)
            DD(1,3,2)=Q(40)
            DD(1,3,3)=Q(41)
            !
            DD(2,1,1)=Q(42)
            DD(2,1,2)=Q(43)
            DD(2,1,3)=Q(44)
            DD(2,2,1)=Q(43)
            DD(2,2,2)=Q(45)
            DD(2,2,3)=Q(46)
            DD(2,3,1)=Q(44)
            DD(2,3,2)=Q(46)
            DD(2,3,3)=Q(47)
            !
            DD(3,1,1)=Q(48)
            DD(3,1,2)=Q(49)
            DD(3,1,3)=Q(50)
            DD(3,2,1)=Q(49)
            DD(3,2,2)=Q(51)
            DD(3,2,3)=Q(52)
            DD(3,3,1)=Q(50)
            DD(3,3,2)=Q(52)
            DD(3,3,3)=Q(53)
            !!
            DO l = 1, 3
                traceDk = SUM(g_contr*DD(l,:,:))
                DD(l,:,:) = DD(l,:,:) - 1./3.*g_cov*traceDk
            ENDDO
            !
            luh(36) = DD(1,1,1)
            luh(37) = DD(1,1,2)
            luh(38) = DD(1,1,3)
            luh(39) = DD(1,2,2)
            luh(40) = DD(1,2,3)
            luh(41) = DD(1,3,3)
            !
            luh(42) = DD(2,1,1)
            luh(43) = DD(2,1,2)
            luh(44) = DD(2,1,3)
            luh(45) = DD(2,2,2)
            luh(46) = DD(2,2,3)
            luh(47) = DD(2,3,3)
            !
            luh(48) = DD(3,1,1)
            luh(49) = DD(3,1,2)
            luh(50) = DD(3,1,3)
            luh(51) = DD(3,2,2)
            luh(52) = DD(3,2,3)
            luh(53) = DD(3,3,3)            
            !
#ifdef CCZ4GRHD 
            !IF( Q(60) < 1e-6) THEN
            !    luh(61:63,i,j,k) = 0.0                  ! in the atmosphere, there is no velocity 
            !ENDIF            
#endif 
            !

#endif 
END SUBROUTINE EnforceCCZ4Constraints 
                
