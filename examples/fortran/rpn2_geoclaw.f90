! GeoClaw augmented shallow-water Riemann solver, taken from the Clawpack
! "riemann" package (https://github.com/clawpack/riemann).
!
!   subroutine rpn2               <- src/rpn2_geoclaw.f
!       https://github.com/clawpack/riemann/blob/master/src/rpn2_geoclaw.f
!   subroutines riemann_aug_JCP,  <- src/geoclaw_riemann_utils.f
!     riemanntype
!       https://github.com/clawpack/riemann/blob/master/src/geoclaw_riemann_utils.f
!
! Algorithm: D. L. George, "Augmented Riemann solvers for the shallow water
! equations with steady states and inundation", J. Comput. Phys. 227(6),
! 3089-3113 (2008).
!
! Local changes vs. upstream: converted to free-form Fortran, the three routines
! combined into this single file, a C-callable bind(C) entry point
! (rpn2_augmented_wrapper, at the bottom).
subroutine rpn2(ixy, maxm, num_eqn, num_waves, num_aux, num_ghost, &
  ql, qr, auxl, auxr, fwave, s, amdq, apdq)! bind(C, name="rpn2_geoclaw")
use iso_c_binding, only: c_int, c_double
  ! Normal Riemann solver for the 2D Shallow Water equations
  !     with topography:
  !     #        h_t + (hu)_x + (hv)_y = 0                           #
  !     #        (hu)_t + (hu^2 + 0.5gh^2)_x + (huv)_y = -ghb_x      #
  !     #        (hv)_t + (huv)_x + (hv^2 + 0.5gh^2)_y = -ghb_y      #

  ! This solver is based on David George's solver written for GeoClaw.
  ! It has been modified to be compatible with f2py (and thus PyClaw).

  ! waves:     3
  ! equations: 3

  ! Conserved quantities:
  !       1 depth
  !       2 x_momentum
  !       3 y_momentum

  ! Auxiliary fields:
  !       1 bathymetry

  ! The gravitational constant grav should be in the common block cparam.

  ! See http://www.clawpack.org/riemann.html for a detailed explanation
  ! of the Riemann solver API.

  implicit none

  real(kind=c_double) :: g
  real(kind=c_double), parameter :: drytol = 1.e-14
  !      common /cparam/ grav

  integer(kind=c_int), intent(in) :: maxm,num_eqn,num_aux,num_waves,num_ghost,ixy

  real(kind=c_double), intent(inout) ::  ql(num_eqn, 1-num_ghost:maxm+num_ghost)
  real(kind=c_double), intent(inout) ::  qr(num_eqn, 1-num_ghost:maxm+num_ghost)
  real(kind=c_double), intent(in) ::  auxl(num_aux,1-num_ghost:maxm+num_ghost)
  real(kind=c_double), intent(in) ::  auxr(num_aux,1-num_ghost:maxm+num_ghost)

  real(kind=c_double), intent(out) :: fwave(num_eqn, num_waves, 1-num_ghost:maxm+num_ghost)
  real(kind=c_double), intent(out) ::  s(num_waves, 1-num_ghost:maxm+num_ghost)
  real(kind=c_double), intent(out) ::  apdq(num_eqn,1-num_ghost:maxm+num_ghost)
  real(kind=c_double), intent(out) ::  amdq(num_eqn,1-num_ghost:maxm+num_ghost)

  !local only
  integer(kind=c_int) mw,maxiter,mu,nv
  real(kind=8) wall(3)
  real(kind=8) fw(3,3)
  real(kind=8) sw(3)

  real(kind=8) hR,hL,huR,huL,uR,uL,hvR,hvL,vR,vL,phiR,phiL
  real(kind=8) bR,bL,sL,sR,sRoe1,sRoe2,sE1,sE2,uhat,chat
  real(kind=8) s1m,s2m
  real(kind=8) hstar,hstartest
  ! real(kind=8) tw,dxdc

  logical rare1,rare2

  g = 9.81d0

  !loop through Riemann problems at each grid cell
  ! do i=2-num_ghost,num_cells+num_ghost

    !-----------------------Initializing------------------------------
    !Initialize Riemann problem for grid interface
    do mw=1,num_waves
      s(mw,1)=0.d0
      fwave(1,mw,1)=0.d0
      fwave(2,mw,1)=0.d0
      fwave(3,mw,1)=0.d0
    enddo

    !        !set normal direction
    if (ixy.eq.1) then
      mu=2
      nv=3
    else
      mu=3
      nv=2
    endif

    !zero (small) negative values if they exist
    if (qr(1,1).lt.0.d0) then
      qr(1,1)=0.d0
      qr(2,1)=0.d0
      qr(3,1)=0.d0
    endif

    if (ql(1,1).lt.0.d0) then
      ql(1,1)=0.d0
      ql(2,1)=0.d0
      ql(3,1)=0.d0
    endif

    !skip problem if in a completely dry area
    if (qr(1,1) <= drytol .and. ql(1,1) <= drytol) then
      go to 30
    endif

    !Riemann problem variables
    hL = qr(1,1)
    hR = ql(1,1)
    huL = qr(mu,1)
    huR = ql(mu,1)
    bL = auxr(1,1)
    bR = auxl(1,1)

    hvL=qr(nv,1)
    hvR=ql(nv,1)

    !check for wet/dry boundary
    if (hR.gt.drytol) then
      uR=huR/hR
      vR=hvR/hR
      phiR = 0.5d0*g*hR**2 + huR**2/hR
    else
      hR = 0.d0
      huR = 0.d0
      hvR = 0.d0
      uR = 0.d0
      vR = 0.d0
      phiR = 0.d0
    endif

    if (hL.gt.drytol) then
      uL=huL/hL
      vL=hvL/hL
      phiL = 0.5d0*g*hL**2 + huL**2/hL
    else
      hL=0.d0
      huL=0.d0
      hvL=0.d0
      uL=0.d0
      vL=0.d0
      phiL = 0.d0
    endif

    wall(1) = 1.d0
    wall(2) = 1.d0
    wall(3) = 1.d0
    if (hR.le.drytol) then
      call riemanntype(hL,hL,uL,-uL,hstar,s1m,s2m,rare1,rare2,1,drytol,g)
      hstartest=max(hL,hstar)
      if (hstartest+bL.lt.bR) then !right state should become ghost values that mirror left for wall problem
        !                bR=hstartest+bL
        wall(2)=0.d0
        wall(3)=0.d0
        hR=hL
        huR=-huL
        bR=bL
        phiR=phiL
        uR=-uL
        vR=vL
      elseif (hL+bL.lt.bR) then
        bR=hL+bL
      endif
    elseif (hL.le.drytol) then ! right surface is lower than left topo
      call riemanntype(hR,hR,-uR,uR,hstar,s1m,s2m,rare1,rare2,1,drytol,g)
      hstartest=max(hR,hstar)
      if (hstartest+bR.lt.bL) then  !left state should become ghost values that mirror right
        !               bL=hstartest+bR
        wall(1)=0.d0
        wall(2)=0.d0
        hL=hR
        huL=-huR
        bL=bR
        phiL=phiR
        uL=-uR
        vL=vR
      elseif (hR+bR.lt.bL) then
        bL=hR+bR
      endif
    endif

    !determine wave speeds
    sL=uL-sqrt(g*hL) ! 1 wave speed of left state
    sR=uR+sqrt(g*hR) ! 2 wave speed of right state

    uhat=(sqrt(g*hL)*uL + sqrt(g*hR)*uR)/(sqrt(g*hR)+sqrt(g*hL)) ! Roe average
    chat=sqrt(g*0.5d0*(hR+hL)) ! Roe average
    sRoe1=uhat-chat ! Roe wave speed 1 wave
    sRoe2=uhat+chat ! Roe wave speed 2 wave

    sE1 = min(sL,sRoe1) ! Eindfeldt speed 1 wave
    sE2 = max(sR,sRoe2) ! Eindfeldt speed 2 wave

    !--------------------end initializing...finally----------
    !solve Riemann problem.

    maxiter = 1

    call riemann_aug_JCP(maxiter,3,3,hL,hR,huL, &
    huR,bL,bR,uL,uR,vL,vR,phiL,phiR,sE1,sE2, &
                                drytol,g,sw,fw)

    !        !eliminate ghost fluxes for wall
    do mw=1,3
      sw(mw)=sw(mw)*wall(mw)

      fw(1,mw)=fw(1,mw)*wall(mw)
      fw(2,mw)=fw(2,mw)*wall(mw)
      fw(3,mw)=fw(3,mw)*wall(mw)
    enddo

    do mw=1,num_waves
      s(mw,1)=sw(mw)
      fwave(1,mw,1)=fw(1,mw)
      fwave(mu,mw,1)=fw(2,mw)
      fwave(nv,mw,1)=fw(3,mw)
    enddo

    ! 30      continue
  ! enddo



  !============= compute fluctuations=============================================
  30  amdq(1:3,:) = 0.d0
  apdq(1:3,:) = 0.d0
  ! do i=2-num_ghost,num_cells+num_ghost
    do  mw=1,num_waves
      if (s(mw,1) < 0.d0) then
        amdq(1:3,1) = amdq(1:3,1) + fwave(1:3,mw,1)
      else if (s(mw,1) > 0.d0) then
        apdq(1:3,1)  = apdq(1:3,1) + fwave(1:3,mw,1)
      else
        amdq(1:3,1) = amdq(1:3,1) + 0.5d0 * fwave(1:3,mw,1)
        apdq(1:3,1) = apdq(1:3,1) + 0.5d0 * fwave(1:3,mw,1)
      endif
    enddo
  ! enddo

  return
end subroutine


!-----------------------------------------------------------------------
subroutine riemann_aug_JCP(maxiter,num_eqn,num_waves,hL,hR,huL,huR, &
  bL,bR,uL,uR,vL,vR,phiL,phiR,sE1,sE2,drytol,g,sw,fw)

  ! solve shallow water equations given single left and right states
  ! This solver is described in J. Comput. Phys. (6): 3089-3113, March 2008
  ! Augmented Riemann Solvers for the Shallow Equations with Steady States and Inundation

  ! To use the original solver call with maxiter=1.

  ! This solver allows iteration when maxiter > 1. The iteration seems to help with
  ! instabilities that arise (with any solver) as flow becomes transcritical over variable topo
  ! due to loss of hyperbolicity.



  implicit none

  !input
  integer num_eqn,num_waves,maxiter
  real(kind=8) fw(num_eqn,num_waves)
  real(kind=8) sw(num_waves)
  real(kind=8) hL,hR,huL,huR,bL,bR,uL,uR,phiL,phiR,sE1,sE2
  real(kind=8) vL,vR
  real(kind=8) drytol,g

  !local
  integer mw,k,iter
  real(kind=8) A(3,3)
  real(kind=8) r(3,3)
  real(kind=8) lambda(3)
  real(kind=8) del(3)
  real(kind=8) beta(3)

  real(kind=8) delh,delhu,delphi,delb,delnorm
  real(kind=8) rare1st,rare2st,sdelta,raremin,raremax
  real(kind=8) criticaltol,convergencetol
  real(kind=8) s1s2bar,s1s2tilde,hbar,hLstar,hRstar
  real(kind=8) huRstar,huLstar,uRstar,uLstar,hstarHLL
  real(kind=8) deldelh,deldelphi
  real(kind=8) s1m,s2m,hm
  real(kind=8) det1,det2,det3,determinant

  logical rare1,rare2,rarecorrector,rarecorrectortest,sonic

  !determine del vectors
  delh = hR-hL
  delhu = huR-huL
  delphi = phiR-phiL
  delb = bR-bL
  delnorm = delh**2 + delphi**2

  call riemanntype(hL,hR,uL,uR,hm,s1m,s2m,rare1,rare2,1,drytol,g)

  lambda(1)= min(sE1,s2m) !Modified Einfeldt speed
  lambda(3)= max(sE2,s1m) !Modified Eindfeldt speed
  sE1=lambda(1)
  sE2=lambda(3)
  lambda(2) = 0.d0  ! ### Fix to avoid uninitialized value in loop on mw -- Correct?? ###


  hstarHLL = max((huL-huR+sE2*hR-sE1*hL)/(sE2-sE1),0.d0) ! middle state in an HLL solve

  !     !determine the middle entropy corrector wave------------------------
  rarecorrectortest=.false.
  rarecorrector=.false.
  if (rarecorrectortest) then
    sdelta=lambda(3)-lambda(1)
    raremin = 0.5d0
    raremax = 0.9d0
    if (rare1.and.sE1*s1m.lt.0.d0) raremin=0.2d0
    if (rare2.and.sE2*s2m.lt.0.d0) raremin=0.2d0
    if (rare1.or.rare2) then
      !see which rarefaction is larger
      rare1st=3.d0*(sqrt(g*hL)-sqrt(g*hm))
      rare2st=3.d0*(sqrt(g*hR)-sqrt(g*hm))
      if (max(rare1st,rare2st).gt.raremin*sdelta.and. &
        max(rare1st,rare2st).lt.raremax*sdelta) then
            rarecorrector=.true.
        if (rare1st.gt.rare2st) then
            lambda(2)=s1m
        elseif (rare2st.gt.rare1st) then
            lambda(2)=s2m
        else
            lambda(2)=0.5d0*(s1m+s2m)
        endif
      endif
    endif
    if (hstarHLL.lt.min(hL,hR)/5.d0) rarecorrector=.false.
  endif

  !     ## Is this correct 2-wave when rarecorrector == .true. ??
  do mw=1,num_waves
    r(1,mw)=1.d0
    r(2,mw)=lambda(mw)
    r(3,mw)=(lambda(mw))**2
  enddo
  if (.not.rarecorrector) then
    lambda(2) = 0.5d0*(lambda(1)+lambda(3))
    !         lambda(2) = max(min(0.5d0*(s1m+s2m),sE2),sE1)
    r(1,2)=0.d0
    r(2,2)=0.d0
    r(3,2)=1.d0
  endif
  !     !---------------------------------------------------


  !     !determine the steady state wave -------------------
  criticaltol = 1.d-6
  deldelh = -delb
  deldelphi = -g*0.5d0*(hR+hL)*delb

  !     !determine a few quanitites needed for steady state wave if iterated
  hLstar=hL
  hRstar=hR
  uLstar=uL
  uRstar=uR
  huLstar=uLstar*hLstar
  huRstar=uRstar*hRstar

  !iterate to better determine the steady state wave
  convergencetol=1.d-6
  do iter=1,maxiter
    !determine steady state wave (this will be subtracted from the delta vectors)
    if (min(hLstar,hRstar).lt.drytol.and.rarecorrector) then
      rarecorrector=.false.
      hLstar=hL
      hRstar=hR
      uLstar=uL
      uRstar=uR
      huLstar=uLstar*hLstar
      huRstar=uRstar*hRstar
      lambda(2) = 0.5d0*(lambda(1)+lambda(3))
      !           lambda(2) = max(min(0.5d0*(s1m+s2m),sE2),sE1)
      r(1,2)=0.d0
      r(2,2)=0.d0
      r(3,2)=1.d0
    endif

    hbar =  max(0.5d0*(hLstar+hRstar),0.d0)
    s1s2bar = 0.25d0*(uLstar+uRstar)**2 - g*hbar
    s1s2tilde= max(0.d0,uLstar*uRstar) - g*hbar

    !        !find if sonic problem
    sonic=.false.
    if (abs(s1s2bar).le.criticaltol) sonic=.true.
    if (s1s2bar*s1s2tilde.le.criticaltol) sonic=.true.
    if (s1s2bar*sE1*sE2.le.criticaltol) sonic = .true.
    if (min(abs(sE1),abs(sE2)).lt.criticaltol) sonic=.true.
    if (sE1.lt.0.d0.and.s1m.gt.0.d0) sonic = .true.
    if (sE2.gt.0.d0.and.s2m.lt.0.d0) sonic = .true.
    if ((uL+sqrt(g*hL))*(uR+sqrt(g*hR)).lt.0.d0) sonic=.true.
    if ((uL-sqrt(g*hL))*(uR-sqrt(g*hR)).lt.0.d0) sonic=.true.

    !        !find jump in h, deldelh
    if (sonic) then
      deldelh =  -delb
    else
      deldelh = delb*g*hbar/s1s2bar
    endif
    !        !find bounds in case of critical state resonance, or negative states
    if (sE1.lt.-criticaltol.and.sE2.gt.criticaltol) then
      deldelh = min(deldelh,hstarHLL*(sE2-sE1)/sE2)
      deldelh = max(deldelh,hstarHLL*(sE2-sE1)/sE1)
    elseif (sE1.ge.criticaltol) then
      deldelh = min(deldelh,hstarHLL*(sE2-sE1)/sE1)
      deldelh = max(deldelh,-hL)
    elseif (sE2.le.-criticaltol) then
      deldelh = min(deldelh,hR)
      deldelh = max(deldelh,hstarHLL*(sE2-sE1)/sE2)
    endif

    !        !find jump in phi, deldelphi
    if (sonic) then
      deldelphi = -g*hbar*delb
    else
      deldelphi = -delb*g*hbar*s1s2tilde/s1s2bar
    endif
    !        !find bounds in case of critical state resonance, or negative states
    deldelphi=min(deldelphi,g*max(-hLstar*delb,-hRstar*delb))
    deldelphi=max(deldelphi,g*min(-hLstar*delb,-hRstar*delb))

    del(1)=delh-deldelh
    del(2)=delhu
    del(3)=delphi-deldelphi

    !        !Determine determinant of eigenvector matrix========
    det1=r(1,1)*(r(2,2)*r(3,3)-r(2,3)*r(3,2))
    det2=r(1,2)*(r(2,1)*r(3,3)-r(2,3)*r(3,1))
    det3=r(1,3)*(r(2,1)*r(3,2)-r(2,2)*r(3,1))
    determinant=det1-det2+det3

    !        !solve for beta(k) using Cramers Rule=================
    do k=1,3
      do mw=1,3
          A(1,mw)=r(1,mw)
          A(2,mw)=r(2,mw)
          A(3,mw)=r(3,mw)
      enddo
      A(1,k)=del(1)
      A(2,k)=del(2)
      A(3,k)=del(3)
      det1=A(1,1)*(A(2,2)*A(3,3)-A(2,3)*A(3,2))
      det2=A(1,2)*(A(2,1)*A(3,3)-A(2,3)*A(3,1))
      det3=A(1,3)*(A(2,1)*A(3,2)-A(2,2)*A(3,1))
      beta(k)=(det1-det2+det3)/determinant
    enddo

    !exit if things aren't changing
    if (abs(del(1)**2+del(3)**2-delnorm).lt.convergencetol) exit
    delnorm = del(1)**2+del(3)**2
    !find new states qLstar and qRstar on either side of interface
    hLstar=hL
    hRstar=hR
    uLstar=uL
    uRstar=uR
    huLstar=uLstar*hLstar
    huRstar=uRstar*hRstar
    do mw=1,num_waves
      if (lambda(mw).lt.0.d0) then
        hLstar= hLstar + beta(mw)*r(1,mw)
        huLstar= huLstar + beta(mw)*r(2,mw)
      endif
    enddo
    do mw=num_waves,1,-1
      if (lambda(mw).gt.0.d0) then
        hRstar= hRstar - beta(mw)*r(1,mw)
        huRstar= huRstar - beta(mw)*r(2,mw)
      endif
    enddo

    if (hLstar.gt.drytol) then
      uLstar=huLstar/hLstar
    else
      hLstar=max(hLstar,0.d0)
      uLstar=0.d0
    endif
    if (hRstar.gt.drytol) then
      uRstar=huRstar/hRstar
    else
      hRstar=max(hRstar,0.d0)
      uRstar=0.d0
    endif

  enddo ! end iteration on Riemann problem

  do mw=1,num_waves
    sw(mw)=lambda(mw)
    fw(1,mw)=beta(mw)*r(2,mw)
    fw(2,mw)=beta(mw)*r(3,mw)
    fw(3,mw)=beta(mw)*r(2,mw)
  enddo
    !find transverse components (ie huv jumps).
    fw(3,1)=fw(3,1)*vL
    fw(3,3)=fw(3,3)*vR
    fw(3,2)= hR*uR*vR - hL*uL*vL - fw(3,1)- fw(3,3)

  return

end !subroutine riemann_aug_JCP-------------------------------------------------



!=============================================================================
subroutine riemanntype(hL,hR,uL,uR,hm,s1m,s2m,rare1,rare2, &
     maxiter,drytol,g)

!determine the Riemann structure (wave-type in each family)

  implicit none

  !input
  real(kind=8) hL,hR,uL,uR,drytol,g
  integer maxiter

  !output
  real(kind=8) s1m,s2m
  logical rare1,rare2

  !local
  real(kind=8) hm,u1m,u2m,um,delu
  real(kind=8) h_max,h_min,h0,F_max,F_min,dfdh,F0,slope,gL,gR
  integer iter



  !     !Test for Riemann structure

  h_min=min(hR,hL)
  h_max=max(hR,hL)
  delu=uR-uL

  if (h_min.le.drytol) then
    hm=0.d0
    um=0.d0
    s1m=uR+uL-2.d0*sqrt(g*hR)+2.d0*sqrt(g*hL)
    s2m=uR+uL-2.d0*sqrt(g*hR)+2.d0*sqrt(g*hL)
    if (hL.le.0.d0) then
      rare2=.true.
      rare1=.false.
    else
      rare1=.true.
      rare2=.false.
    endif
  else
    F_min= delu+2.d0*(sqrt(g*h_min)-sqrt(g*h_max))
    F_max= delu + (h_max-h_min)*(sqrt(.5d0*g*(h_max+h_min)/(h_max*h_min)))

    if (F_min.gt.0.d0) then !2-rarefactions

      hm=(1.d0/(16.d0*g))* &
            max(0.d0,-delu+2.d0*(sqrt(g*hL)+sqrt(g*hR)))**2
      um=sign(1.d0,hm)*(uL+2.d0*(sqrt(g*hL)-sqrt(g*hm)))

      s1m=uL+2.d0*sqrt(g*hL)-3.d0*sqrt(g*hm)
      s2m=uR-2.d0*sqrt(g*hR)+3.d0*sqrt(g*hm)

      rare1=.true.
      rare2=.true.

    elseif (F_max.le.0.d0) then !2 shocks

      !           !root finding using a Newton iteration on sqrt(h)===
      h0=h_max
      do iter=1,maxiter
        gL=sqrt(.5d0*g*(1/h0 + 1/hL))
        gR=sqrt(.5d0*g*(1/h0 + 1/hR))
        F0=delu+(h0-hL)*gL + (h0-hR)*gR
        dfdh=gL-g*(h0-hL)/(4.d0*(h0**2)*gL)+gR-g*(h0-hR)/(4.d0*(h0**2)*gR)
        slope=2.d0*sqrt(h0)*dfdh
        h0=(sqrt(h0)-F0/slope)**2
      enddo
      hm=h0
      u1m=uL-(hm-hL)*sqrt((.5d0*g)*(1/hm + 1/hL))
      u2m=uR+(hm-hR)*sqrt((.5d0*g)*(1/hm + 1/hR))
      um=.5d0*(u1m+u2m)

      s1m=u1m-sqrt(g*hm)
      s2m=u2m+sqrt(g*hm)
      rare1=.false.
      rare2=.false.

    else !one shock one rarefaction
      h0=h_min

      do iter=1,maxiter
        F0=delu + 2.d0*(sqrt(g*h0)-sqrt(g*h_max)) &
                + (h0-h_min)*sqrt(.5d0*g*(1/h0+1/h_min))
        slope=(F_max-F0)/(h_max-h_min)
        h0=h0-F0/slope
      enddo

      hm=h0
      if (hL.gt.hR) then
        um=uL+2.d0*sqrt(g*hL)-2.d0*sqrt(g*hm)
        s1m=uL+2.d0*sqrt(g*hL)-3.d0*sqrt(g*hm)
        s2m=uL+2.d0*sqrt(g*hL)-sqrt(g*hm)
        rare1=.true.
        rare2=.false.
      else
        s2m=uR-2.d0*sqrt(g*hR)+3.d0*sqrt(g*hm)
        s1m=uR-2.d0*sqrt(g*hR)+sqrt(g*hm)
        um=uR-2.d0*sqrt(g*hR)+2.d0*sqrt(g*hm)
        rare2=.true.
        rare1=.false.
      endif
    endif
  endif

  return

end ! subroutine riemanntype----------------------------------------------------------------







! subroutine rpn2_augmented_wrapper(ql, qr, amdq, apdq, normal) bind(C, name="rpn2_augmented_wrapper")
function rpn2_augmented_wrapper(ql, qr, amdq, apdq, normal) result(s_max) bind(C, name="rpn2_augmented_wrapper")
use iso_c_binding, only: c_int, c_double
! * @param QL the left state vector
! * @param QR the right state vector
! * @param normal dimension currently being solved for
! * @param AMDQ left going update, has a negative contribution on left state
! * @param APDQ right going update, has a positive contribution on right state

  implicit none

  real(kind=c_double), intent(in) ::  ql(4), qr(4)
  real(kind=c_double), intent(out) ::  apdq(3), amdq(3)
  real(kind=c_double) :: s_max
  integer(kind=c_int), intent(in) ::  normal
  ! real(kind=c_double), intent(out) ::  s

  !local only
  real(kind=c_double) ::  auxl(1,1)
  real(kind=c_double) ::  auxr(1,1)

  real(kind=c_double) :: fwave(3, 3, 1)
  real(kind=c_double) :: s(3, 1)

  real(kind=c_double) :: apdq_rpn(3,1)
  real(kind=c_double) :: amdq_rpn(3,1)
  real(kind=c_double) :: ql_rpn(3,1)
  real(kind=c_double) :: qr_rpn(3,1)

  !  integer(kind=c_int) :: maxm,num_eqn,num_aux,num_waves,num_ghost,num_cells,ixy

  !Map the caller's left/right interface states onto the GeoClaw rpn2 edge
  !convention: rpn2 reads its left state hL from the qr argument and its right
  !state hR from the ql argument, so the caller's left state QL must go into
  !qr_rpn (and its bathymetry into auxr) and the caller's right state QR into
  !ql_rpn (bathymetry into auxl).
  qr_rpn(1:3,1) = ql(1:3)
  ql_rpn(1:3,1) = qr(1:3)

  auxr(1,1) = ql(4)
  auxl(1,1) = qr(4)

  call rpn2(normal+1, 1, 3, 3, 1, 0, ql_rpn, qr_rpn, auxl, auxr, fwave, s, amdq_rpn, apdq_rpn)

  amdq(1:3) = amdq_rpn(1:3,1)
  apdq(1:3) = apdq_rpn(1:3,1)

  s_max = maxval(abs(s))

end function