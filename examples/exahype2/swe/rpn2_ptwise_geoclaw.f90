subroutine rpn2(ixy,meqn,maux,mwaves,ql,qr,auxl,auxr,fwave,s,amdq,apdq)

    use geoclaw_module, only: g => grav, drytol => dry_tolerance, rho
    use geoclaw_module, only: earth_radius, deg2rad
    use amr_module, only: mcapa

    implicit none

    ! Input
    integer :: meqn, maux, mwaves, ixy
    real(kind=8) :: ql(meqn), qr(meqn)
    real(kind=8) :: auxl(maux), auxr(maux)

    ! Output
    real(kind=8) :: apdq(meqn), amdq(meqn)
    real(kind=8) :: fwave(meqn, mwaves), s(mwaves)

    !local only
    integer :: m,i,mw,maxiter,mu,nv
    real(kind=8) :: wall(3)
    real(kind=8) :: fw(3,3)
    real(kind=8) :: sw(3)

    real(kind=8) :: hR,hL,huR,huL,uR,uL,hvR,hvL,vR,vL,phiR,phiL,pL,pR
    real(kind=8) :: bR,bL,sL,sR,sRoe1,sRoe2,sE1,sE2,uhat,chat
    real(kind=8) :: s1m,s2m
    real(kind=8) :: hstar,hstartest,hstarHLL,sLtest,sRtest
    real(kind=8) :: tw,dxdc

    logical :: rare1,rare2
    
!    logical :: pressure_forcing = .false.
!    integer :: pressure_index = 4

    ! In case there is no pressure forcing
    pL = 0.d0
    pR = 0.d0

    if (qr(1) < 0.d0 .or. ql(1) < 0.d0) then
        print *, "Negative input: hl, hr = ", qr(1), ql(1)
    end if

    amdq = 0.d0
    apdq = 0.d0
    s = 0.d0
    fwave = 0d0

    if (ixy == 1) then
        mu = 2
        nv = 3
    else
        mu = 3
        nv = 2
    end if

    if (qr(1) < 0.d0) then
        qr(:) = 0.d0
    end if
    if (ql(1) < 0.d0) then
        ql(:) = 0.d0
    end if

    if (qr(1) <= drytol .and. ql(1) <= drytol) then
        return
    end if

    ! Extract Riemann problem variables
    hL = qr(1) 
    hR = ql(1) 
    huL = qr(mu) 
    huR = ql(mu) 
    bL = auxr(1)
    bR = auxl(1)
!    if (pressure_forcing) then
!        pL = auxr(pressure_index)
!        pR = auxl(pressure_index)
!    end if
    hvL = qr(nv)
    hvR = ql(nv)


    if (hR < drytol) then
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

    if (hL > drytol) then
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

    wall(:) = 1.d0

    if (hR <= drytol) then
        call riemanntype(hL,hL,uL,-uL,hstar,s1m,s2m,rare1,rare2,1,drytol,g)
        hstartest=max(hL,hstar)
        if (hstartest+bL.lt.bR) then 
            wall(2) = 0.d0
            wall(3) = 0.d0
            hR=hL
            huR=-huL
            bR=bL
            phiR=phiL
            uR=-uL
            vR=vL
        else if (hL+bL < bR) then
            bR=hL+bL
        endif
    elseif (hL.le.drytol) then
        call riemanntype(hR,hR,-uR,uR,hstar,s1m,s2m,rare1,rare2,1,drytol,g)
        hstartest=max(hR,hstar)
        if (hstartest+bR.lt.bL) then
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

    maxiter = 1

    call riemann_aug_JCP(maxiter,3,3,hL,hR,huL,huR,hvL,hvR,bL,bR,uL,uR,vL,vR,  &
                         phiL,phiR,pL,pR,sE1,sE2,drytol,g,rho,sw,fw)


    do mw=1,3
        sw(mw)=sw(mw)*wall(mw)

        fw(1,mw)=fw(1,mw)*wall(mw) 
        fw(2,mw)=fw(2,mw)*wall(mw)
        fw(3,mw)=fw(3,mw)*wall(mw)
    enddo
    do mw=1,mwaves
        s(mw)=sw(mw)
        fwave(1,mw)=fw(1,mw)
        fwave(mu,mw)=fw(2,mw)
        fwave(nv,mw)=fw(3,mw)
    enddo

    if (mcapa > 0) then
        if (ixy == 1) then
            dxdc = (earth_radius * deg2rad)
        else
            dxdc = earth_radius * cos(auxl(3)) * deg2rad
        end if

        do mw=1, mwaves
            s(mw)=dxdc*s(mw)
            fwave(1,mw)=dxdc*fwave(1,mw)
            fwave(2,mw)=dxdc*fwave(2,mw)
            fwave(3,mw)=dxdc*fwave(3,mw)
        end do
    end if

    do mw=1, mwaves
        if (s(mw) < 0.d0) then
            amdq(1:3) = amdq(1:3) + fwave(1:3, mw)
        else if (s(mw) > 0.d0) then
            apdq(1:3) = apdq(1:3) + fwave(1:3, mw)
        else
            amdq(1:3) = amdq(1:3) + 0.5d0 * fwave(1:3, mw)
            apdq(1:3) = apdq(1:3) + 0.5d0 * fwave(1:3, mw)
        end if
    end do


end subroutine rpn2
