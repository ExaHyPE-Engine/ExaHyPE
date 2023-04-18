import numpy as np
import matplotlib.pyplot as plt
from scipy import io
import os
import time
import argparse

pi=np.pi
G=6.67408*1e-11
M=0.15
rho_c_ini=1
rho_c0_value=1.8788*10**(-26)
a_i=1e-3

def file_len(fname):
    with open(fname) as f:
        for i, l in enumerate(f):
            pass
    return i + 1

#lis=os.listdir("./")
#for item in lis:
#	if item.endswith(".pvd") or item.endswith(".vtp"): os.remove(os.path.join("./",item))

parser = argparse.ArgumentParser(description='Tracer_conversion')
parser.add_argument("-if",   "--input-file",  dest="file_name", required="True", type=str, help="input csv file name (and directory)" )
parser.add_argument("-pq",   "--plot-quantity",  dest="pq", default="rho", type=str, choices=["rho","v_x","v_y","v_z","E_t","P"], help="choose the quantity you want to plot" )
parser.add_argument("-rescale",   "--rescale",  dest="IfRescale", action="store_true", default=False, help="if you need the rescaling using turnaround quantites" )
parser.add_argument("-ylog",   "--yscale-log",  dest="Ifylog", action="store_true", default=False, help="if you need the plot with y axis in log scale" )
parser.add_argument("-XUcode",   "--x-code-unit",  dest="IfXunitInCode", action="store_true", default=False, help="enable this option, if you want the plot in code unit for distance (the default one is in physical unit)" )
args = parser.parse_args()

	
file_name=args.file_name
IfXunitInCode=args.IfXunitInCode
#dt=args.dt;

#########################################################################################################################
#main code	
f=open(file_name)
print("Read in datafile: "+file_name)

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

number_of_data=len(tem)-6

print("number of data entries: "+str(number_of_data))
print("total timesteps: "+str(tstep))

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
data_set=np.zeros((N_tracer,tstep,4+number_of_data))

ID1=list(map(float,dat[0].split(', ')))[1]; ID2=list(map(float,dat[0].split(', ')))[2]
t_count=0 #counting the tsteps
N_count=0 #counting the tracer
for line in dat:
	tem=list(map(float,line.split(', ')))
	if (tem[1]==ID1) and (tem[2]==ID2):
		data_set[N_count][t_count][0]=tem[0]; #timestep
		data_set[N_count][t_count][1]=tem[3]; data_set[N_count][t_count][2]=tem[4]; data_set[N_count][t_count][3]=tem[5] #coordinates
		for i in range(number_of_data):
			data_set[N_count][t_count][4+i]=tem[6+i];
		t_count+=1
	else:
		if t_count<tstep:
			for i in range(4+number_of_data): data_set[N_count][t_count][i]=data_set[N_count][t_count-1][i];
		ID1=tem[1]; ID2=tem[2]
		N_count+=1
		t_count=0
		data_set[N_count][t_count][0]=tem[0]; #timestep
		data_set[N_count][t_count][1]=tem[3]; data_set[N_count][t_count][2]=tem[4]; data_set[N_count][t_count][3]=tem[5] #coordinates
		for i in range(number_of_data):
			data_set[N_count][t_count][4+i]=tem[6+i];
		t_count+=1		
if t_count<tstep:
	for i in range(4+number_of_data): data_set[N_count][t_count][i]=data_set[N_count][t_count-1][i];

f.close()



q_dir={"rho":4,"v_x":5,"v_y":6,"v_z":7,"E_t":8}

t_id=100
q_id=args.pq

for t_id in [200,400,590]:
#==========================================================
#  Read in data
#==========================================================

	t_r_id=tstep-t_id-1
	t_code=data_set[0,t_r_id,0]
	data_at_t=data_set[:,t_r_id,:]
	sort_data=data_at_t[np.argsort(data_at_t[:,1])]
	
	coor_data=sort_data[:,1]
	if q_id=="P":
		e_data   = np.array(sort_data[:,q_dir["E_t"]])
		m1_data  = np.array(sort_data[:,q_dir["v_x"]])
		m2_data  = np.array(sort_data[:,q_dir["v_y"]])
		m3_data  = np.array(sort_data[:,q_dir["v_z"]]) 
		rho_data = np.array(sort_data[:,q_dir["rho"]])
	else:
		q_data=sort_data[:,q_dir[q_id]]
	if q_id in ["v_x","v_y","v_z"]:
		rho_data=sort_data[:,q_dir["rho"]]
	#print(sort_data)

#==========================================================
#  start to calculate the real physical quantites
#==========================================================

	t_real=(0.1694*a_i**(-0.5)-t_code/11.8)**(-3)*10**15 #*h^(-1)  
	a_scale=0.0287*(0.1694*a_i**(-0.5)-t_code/11.8)**(-2) # when code time ~ 2*(a_i^(-0.5)-1), a~1
	print("cosmological time:", t_real)
	print("a_scale:", a_scale)
	
	coor_data_physical=100*a_scale*coor_data
	
	if q_id in ["v_x","v_y","v_z"]:
		q_id_show="$"+q_id+"/(km/s)$"
		q_data=np.array(q_data)/np.array(rho_data) #remove density from momentum
		q_data=q_data*10**4/a_scale  #H_0*L= 100h*100h^(-1) km/s/Mpc * Mpc = 1e4
		v_hubble=100*(2.057*10**17/t_real)*coor_data_physical #hubble velocity here
		q_data=q_data+v_hubble
	elif q_id=="rho":
		q_id_show=r"$\rho/\rho_c $"
		q_data=q_data/(a_scale**3)
	elif q_id=="P":
	  q_id_show="P/(Pa)"
	  q_data=(5/3-1)*(e_data-0.5*(m1_data**2+m2_data**2+m3_data**2)/rho_data)
	  q_data=q_data*10**(8+6)*rho_c0_value/(a_scale**5)
	else:	
		q_id_show="$"+q_id+"/(Code Unit)"
		
#==============================================================		
#  now do the rescaling
#==============================================================

	if args.IfRescale:
		if q_id=="rho":
			q_data=q_data*rho_c0_value # *h^2 kg/m^3
			rho_scaling=1/(6*pi*G*t_real**2) #h^2 kg/m^3
			q_data=q_data/rho_scaling #actually it is equal to code density
			q_id_show=r"$\rho/\rho_{ta}(t) $"
		if q_id in ["v_x","v_y","v_z"]:
			r_ta=a_scale*100*3.086*1e19*((3*M)/(4*pi*rho_c_ini))**(1/3)*(4/3/pi)**(8/9)*(t_real/6.505/1e12)**(8/9) # km. eq(31), RAMSES
			v_scaling=r_ta/t_real
			q_data=q_data/v_scaling
			q_id_show=r"$u/u_{ta}(t) $"
			print("scaling velocity (km/s):", v_scaling)
		if q_id=="P":
			rho_scaling=1/(6*pi*G*t_real**2)
			r_ta=a_scale*100*3.086*1e19*((3*M)/(4*pi*rho_c_ini))**(1/3)*(4/3/pi)**(8/9)*(t_real/6.505/1e12)**(8/9) # eq(31), RAMSES
			P_scaling=rho_scaling*r_ta**2*1e6/(t_real**2)
			q_data=q_data/P_scaling
			q_id_show=r"$P/P_{ta}(t) $"
			
	if IfXunitInCode:
		plt.plot(coor_data,q_data,label="t= "+str(t_code)+" a= "+str(a_scale))
	else:
		plt.plot(coor_data_physical,q_data,label="t= "+str(t_code)+" a= "+str(a_scale))

#plt.title("code time="+str(t_code))
plt.legend()
if IfXunitInCode:
	plt.xlabel("$x/(Code Unit)$")
else:
	plt.xlabel("$x/(h^{-1}Mpc)$")
plt.ylabel(q_id_show)
#plt.ylim([1.0475,1.0525])
if args.Ifylog:
	plt.yscale("log")

plt.show()









