import numpy as np
import matplotlib.pyplot as plt
from scipy import io
import os
import time
import glob

pi=np.pi

def arctan2_2pi(yy,xx):
	tem=np.arctan2(yy,xx)
	for i in range(len(tem)):
		if tem[i]<0: tem[i]+=2*np.pi
	return tem

def sph_design(func, t=5, author='Hardin', **kwargs):
    if author == 'WomersleySym':
        if not glob.glob('PointDistFiles/sphdesigns/WomersleySym/ss%03d.*'%t):
            return (float('nan'),float('nan'))
        coord = np.loadtxt(glob.glob('PointDistFiles/sphdesigns/WomersleySym/ss%03d.*'%t)[0])
    elif author == 'WomersleyNonSym':
        if not glob.glob('PointDistFiles/sphdesigns/WomersleyNonSym/sf%03d.*'%t):
            return (float('nan'),float('nan'))
        coord = np.loadtxt(glob.glob('PointDistFiles/sphdesigns/WomersleyNonSym/sf%03d.*'%t)[0])
    else:
        if not glob.glob('PointDistFiles/sphdesigns/HardinSloane/hs%03.*.txt'%t):
            return (float('nan'),float('nan'))
        coord = np.loadtxt(sorted(glob.glob('PointDistFiles/sphdesigns/HardinSloane/hs%03d.txt'%t))[0])
    xx = coord[:,0]; yy = coord[:,1]; zz = coord[:,2]
    theta = np.arccos(zz)
    phi = arctan2_2pi(yy,xx)
    N = len(theta)
    I = 0
    for (i,x) in enumerate(phi):
        I += func(theta[i],phi[i],**kwargs)
    I = 4*np.pi*I/N # Normalisation
    return (I,N)
## Gauss-Legendre 1D integration
def gauss_legendre(func, N=40, x_low=0, x_up=pi, **kwargs):
    (x_sample, w) = np.polynomial.legendre.leggauss(N)
    # Transform to [x_low,x_up]
    x_sample = (x_up-x_low)*x_sample/2 + (x_up + x_low)/2
    I = 0
    for (i,x) in enumerate(x_sample):
        I += w[i]*func(x, **kwargs)
    I = (x_up-x_low)/2 * I
    return I

## Gauss 1D integration
def trapezoidal(func, N=40, x_low=0, x_up=pi, **kwargs):
    x_sample = np.linspace(x_low,x_up,N)
    f = np.zeros(N)
    for (i,x) in enumerate(x_sample):
        f[i] = func(x, **kwargs)
    I = np.trapz(f,x_sample)
    return I

## Gaussian product quadrature using Trapezoidal for azimuthal direction and Gauss-Legendre for polar angle
def prod_quad(func, N=20, M=40, **kwargs):
    ## Create function to integrate over sphere in [0,pi]x[0,2pi]
    def func_sphere(phi,theta,**kwargs): return func(phi,theta,**kwargs)*np.sin(phi)
    ## Outer integration over theta using trapezoidal
    I = trapezoidal(lambda x: gauss_legendre(func_sphere, N=N,x_low=0,x_up=pi,theta=x,**kwargs),
            N=M,x_low=0,x_up=2*pi)
    ## Return normalised integral
    return I/(4*pi)

def file_len(fname):
    with open(fname) as f:
        for i, l in enumerate(f):
            pass
    return i + 1

#########################################################################################################################
#main code
#scheme="Gauss_Legendre"
scheme="t-design"


if scheme=="Gauss_Legendre":
	file_name="zz.csv"
elif scheme=="t-design":
	file_name="t-de.csv"
	
f=open(file_name)
print("Scheme: "+scheme+" datafile: "+file_name)

#skip the description line
dat=f.readlines()[1:]

#first find out the number of timesteps
tstep=0
told=1e6
for line in dat:
	tem=list(map(float,line.split(', ')))
	tnew=tem[0]
	if (told-tnew)<0:
		break
	else:
		tstep+=1
		told=tnew
		
print("timesteps: "+str(tstep))

#second, find out how many tracer recorded here.
N_tracer=0
ID1=-100; ID2=-100
coors=[]
for line in dat:
	tem=list(map(float,line.split(', ')))
	if (not tem[1]==ID1) or (not tem[2]==ID2) and (not tem[6]==0):
		ID1=tem[1]; ID2=tem[2]
		N_tracer+=1
		coor=[tem[3],tem[4],tem[5]]
		coors.append(coor)
		
coors=np.array(coors)
print("valid tracers: "+str(len(coors))) 

#create the real data set
data_set=np.zeros((N_tracer,tstep,7))

ID1=0; ID2=1
t_count=0 #counting the tsteps
N_count=0 #counting the tracer
for line in dat:
	tem=list(map(float,line.split(', ')))
	if (tem[1]==ID1) and (tem[2]==ID2):
		data_set[N_count][t_count][0]=tem[0]; #timestep
		data_set[N_count][t_count][1]=tem[3]; data_set[N_count][t_count][2]=tem[4]; data_set[N_count][t_count][3]=tem[5] #coordinates
		data_set[N_count][t_count][4]=tem[6]; data_set[N_count][t_count][5]=tem[7] #psi4 Re and Im
		t_count+=1
	else:
		ID1=tem[1]; ID2=tem[2]
		N_count+=1
		t_count=0
		data_set[N_count][t_count][0]=tem[0]; #timestep
		data_set[N_count][t_count][1]=tem[3]; data_set[N_count][t_count][2]=tem[4]; data_set[N_count][t_count][3]=tem[5] #coordinates
		data_set[N_count][t_count][4]=tem[6]; data_set[N_count][t_count][5]=tem[7] #psi4 Re and Im
		t_count+=1

###################################################
#find weight

if scheme=="Gauss_Legendre":		
	(thetas,ws)= np.polynomial.legendre.leggauss(20)		
	zs=np.cos(np.pi*thetas/2 + np.pi/2)*0.4		
	zip_iterator=zip(zs,ws)
	w_dict=dict(zip_iterator)

	#print(w_dict)

	for n in range(N_tracer):
		for z in zs:
			if abs(data_set[n][0][3]-z)<1e-4:
				data_set[n,:,6]=w_dict[z] #weight
	
	#for t in range(tstep) : print(any(data_set[:,t,6]==0))		
	#print(data_set[80])	
elif scheme=="t-design":
	for n in range(N_tracer):
		data_set[n,:,6]=1
	#for t in range(tstep) : print(any(data_set[:,t,6]==0))
	
	
##############################################################################################			
#ready for mode decomposition
#remember to add sin(theta) for GL scheme
l_mode=2; m_mode=2;
for t in range(tstep):
	for n in range(N_tracer):
		x=data_set[n][t][1]; y=data_set[n][t][2]; z=data_set[n][t][3];
		p4re=data_set[n][t][4]; p4im=data_set[n][t][5]
		#we calculate the triangle function directly to reduce numerical error
		sintheta=(x**2+y**2)**0.5/(x**2+y**2+z**2)**0.5; costheta=z/(x**2+y**2+z**2)**0.5;
		if x**2+y**2==0: x+=1e-6;
		sinphi=y/(x**2+y**2)**0.5; cosphi=x/(x**2+y**2)**0.5
		cos2phi=2*cosphi**2-1; sin2phi=2*sinphi*cosphi
		#mode 22
		if l_mode==2 and m_mode==2:
			data_set[n][t][4]=(p4re*cos2phi-p4im*sin2phi)*(1+costheta)**2*(5/64/np.pi)**0.5
			data_set[n][t][5]=(p4re*sin2phi-p4im*cos2phi)*(1+costheta)**2*(5/64/np.pi)**0.5
			#data_set[n][t][4]=p4re*4*sintheta
		if scheme=="Gauss_Legendre":
			data_set[n][t][4]*=sintheta;
			data_set[n][t][5]*=sintheta;

##############################################################################################
#start real surface integral here
ModeRe=np.zeros(tstep)
ModeIm=np.zeros(tstep)
if scheme=="Gauss_Legendre":
	for t in range(tstep):
		for n in range(N_tracer):
			w=(1.0/40)*(2*np.pi)*data_set[n][t][6]*(np.pi/2.0)
			x=data_set[n][t][1]; y=data_set[n][t][2]; z=data_set[n][t][3];
			ModeRe[t]+=w*data_set[n][t][4]#notice we do not need to do this when we add things above
			#if t==0 :
				#print(data_set[n][t][4])
			ModeIm[t]+=w*data_set[n][t][5]			
elif scheme=="t-design":
	for t in range(tstep):
		for n in range(N_tracer):
			w=(1.0/N_tracer)*(4*np.pi)
			ModeRe[t]+=w*data_set[n][t][4] #notice we do not need to do this when we add things above
			#if t==0 :
				#print(data_set[n][t][4])
			ModeIm[t]+=w*data_set[n][t][5]

#plt.plot(data_set[0,:,0],ModeRe[:]/4/np.pi)
#plt.show()
print(ModeRe/4/np.pi)
print(ModeIm/4/np.pi)






'''
z=coors[:,2]
zz=list(set(z))
zzz=[]
print("z coors number: "+str(len(zz))+" "+str(zz))
for z3 in zz:
	if len(z[z==z3])<40:
		print(z3,len(z[z==z3]))
		zzz.append(z3)


plt.figure(figsize=(10,10),dpi=80)
for p in coors:
	if p[2] in zzz:
		plt.plot(p[0],p[1],'o',color='blue')
plt.show()
'''
'''
I_exa=np.pi**2
#print(I_exa)
tnum=51
t_list=np.array(range(tnum))
I_num=np.zeros(tnum)
N_num=np.zeros(tnum)

def f(theta,phi):
	return np.sin(theta)
	#return 1

#(I,N)=sph_design(f,21,author='WomersleySym')

#print(I-I_exa,N)

for (i,t) in enumerate(t_list):
	(I_num[i],N_num[i])=sph_design(f,t,author='WomersleySym')

I_num=I_num[~np.isnan(N_num)]
N_num=N_num[~np.isnan(N_num)]
#print(arctan2_2pi([1,1,-1,-1],[1,-1,-1,1])/np.pi)

#print(I_num-I_exa)
#print(N_num)
#plt.plot(N_num,abs(I_num-I_exa)/I_exa)
#plt.yscale("log")
#plt.show()

print(prod_quad(f,10,20)*4*pi-I_exa)
'''





















