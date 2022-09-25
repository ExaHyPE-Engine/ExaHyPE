import numpy as np
import matplotlib.pyplot as plt
from scipy import io
import os
import time
import argparse
import math


pi=np.pi
G=6.67408*1e-11
M=0.15
rho_c_ini=1
rho_c0_value=1.8788*10**(-26)
a_i=0.001

def file_len(fname):
    with open(fname) as f:
        for i, l in enumerate(f):
            pass
    return i + 1

#lis=os.listdir("./")
#for item in lis:
#	if item.endswith(".pvd") or item.endswith(".vtp"): os.remove(os.path.join("./",item))

parser = argparse.ArgumentParser(description='Tracer_conversion')
parser.add_argument("-if",   "--input-file",  dest="file_name", required="True", type=str, help="input csv file name from paraview(and directory)" )
parser.add_argument("-pq",   "--plot-quantity",  dest="pq", default="rho", type=str, choices=["rho","v_x","v_y","v_z","E_t","P"], help="choose the quantity you want to plot" )
parser.add_argument("-rescale",   "--rescale",  dest="IfRescale", action="store_true", default=False, help="if you need the rescaling using turnaround quantites" )
parser.add_argument("-ylog",   "--yscale-log",  dest="Ifylog", action="store_true", default=False, help="if you need the plot with y axis in log scale" )
parser.add_argument("-XUcode",   "--x-code-unit",  dest="IfXunitInCode", action="store_true", default=False, help="enable this option, if you want the plot in code unit for distance (the default one is in physical unit)" )
parser.add_argument("-Xrescale",   "--x-rescale",  dest="IfXRescale", action="store_true", default=False, help="enable this option, if you want the x axis also get rescaled to \lambda in Bertschinger, 1985. conflict with XUcode" )
parser.add_argument("-RawD",   "--raw-data",  dest="IfRawData", action="store_true", default=False, help="enable this option, if you want the plot of the raw data (in code unit)" )
args = parser.parse_args()

IfXunitInCode=args.IfXunitInCode
#file_name=args.file_name
#dt=args.dt;

#########################################################################################################################
#main code	

#workaround=args.file_name[5:7]
#snapshots=[int(workaround)]
snapshots=[10,30,50]
snapshot=0

rho = [[]]; 
m_1 = [[]]; m_2 = [[]]; m_3 = [[]];
E_t = [[]]
coor= [[]]
for snap in snapshots:
	f=open("para_"+str(snap)+".csv")
	print("Read in datafile: "+"para_"+str(snap)+".csv")

	#skip the description line
	dat=f.readlines()[1:]

	count=0
	for line in dat:
		tem=list(map(float,line.split(',')))
		if math.isnan(tem[1]):
			continue
		coor[snapshot].append(tem[7]); rho[snapshot].append(tem[1]);
		m_1[snapshot].append(tem[2]); m_2[snapshot].append(tem[3]); m_3[snapshot].append(tem[4]);
		E_t[snapshot].append(tem[5]);
		
	print("sample points received:", len(rho[snapshot]),"for snapshot code time t=", snap)
	snapshot+=1	
	rho.append([]);coor.append([])
	m_1.append([]);m_2.append([]);m_3.append([])
	E_t.append([])
	#rho[1].append(12)
	#print(rho)
	f.close()

sample_point=len(rho[0])
origin_point=int(sample_point/2)+1

q_id=args.pq

for snapshot in range(len(snapshots)):
#==========================================================
#  Read in data
#==========================================================
	#t_r_id=tstep-t_id-1
	#t_code=data_set[0,t_r_id,0]
	#data_at_t=data_set[:,t_r_id,:]
	#sort_data=data_at_t[np.argsort(data_at_t[:,1])]
	
	coor_data=np.array(coor[snapshot])
	if q_id=="P":
		e_data   = np.array(E_t[snapshot])
		m1_data  = np.array(m_1[snapshot])
		m2_data  = np.array(m_2[snapshot])
		m3_data  = np.array(m_3[snapshot]) 
		rho_data = np.array(rho[snapshot])
	elif q_id=="rho":
		q_data=np.array(rho[snapshot])
	elif q_id=="v_x":
		q_data=np.array(m_1[snapshot])
	elif q_id=="v_y":
		q_data=np.array(m_2[snapshot])
	elif q_id=="v_z":
		q_data=np.array(m_3[snapshot])
	elif q_id=="E_t":
		q_data=np.array(E_t[snapshot])			
	if q_id in ["v_x","v_y","v_z"]:
		rho_data=np.array(rho[snapshot])
	#print(sort_data)

#==========================================================
#  start to calculate the real physical quantites
#==========================================================
	if not args.IfRawData:
		t_code=snapshots[snapshot]
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
		r_ta=a_i*100*3.086*1e19*((3*M)/(4*pi*rho_c_ini))**(1/3)*(4/3/pi)**(8/9)*(t_real/6.505/1e12)**(8/9) # km. eq(31), RAMSES
		if args.IfRescale:
			if q_id=="rho":
				q_data=q_data*rho_c0_value # *h^2 kg/m^3
				rho_scaling=1/(6*pi*G*t_real**2) #h^2 kg/m^3
				q_data=q_data/rho_scaling #actually it is equal to code density
				q_id_show=r"$\rho/\rho_{ta}(t) $"
			if q_id in ["v_x","v_y","v_z"]:
				#r_ta=a_scale*100*3.086*1e19*((3*M)/(4*pi*rho_c_ini))**(1/3)*(4/3/pi)**(8/9)*(t_real/6.505/1e12)**(8/9) # km. eq(31), RAMSES
				v_scaling=r_ta/t_real
				q_data=q_data/v_scaling
				q_id_show=r"$u/u_{ta}(t) $"
				print("scaling velocity (km/s):", v_scaling)
			if q_id=="P":
				rho_scaling=1/(6*pi*G*t_real**2)
				P_scaling=rho_scaling*r_ta**2*1e6/(t_real**2)
				q_data=q_data/P_scaling
				q_id_show=r"$P/P_{ta}(t) $"
		
		if IfXunitInCode:
			plt.plot(coor_data[origin_point:],q_data[origin_point:],label="t= "+str(t_code)+" a= "+str(a_scale))
		elif args.IfXRescale:
			plt.plot(np.log10(coor_data_physical[origin_point:]*3.086*1e19/r_ta),q_data[origin_point:],label="t= "+str(t_code)+" a= "+str(a_scale))
		else:
			plt.plot(coor_data_physical[origin_point:],q_data[origin_point:],label="t= "+str(t_code)+" a= "+str(a_scale))

#=================================================================
# or we just plot original raw data
#=================================================================			
	else:
		t_code=snapshots[snapshot]
		a_scale=0.0287*(0.1694*a_i**(-0.5)-t_code/11.8)**(-2) # when code time ~ 2*(a_i^(-0.5)-1), a~1
		if q_id in ["v_x","v_y","v_z"]:
			q_data=np.array(q_data)/np.array(rho_data) #remove density from momentum
		elif q_id=="P":
			q_data=(5/3-1)*(e_data-0.5*(m1_data**2+m2_data**2+m3_data**2)/rho_data)
		
		q_id_show="$"+q_id+"/(Code Unit)$"
		plt.plot(coor_data[origin_point:],q_data[origin_point:],label="t= "+str(t_code)+" a= "+str(a_scale))
			
plt.title(q_id)
plt.legend()
if IfXunitInCode:
	plt.xlabel("$x/(Code Unit)$")
elif args.IfXRescale:
	plt.xlabel("$Log_{10}\lambda$")
else:
	plt.xlabel("$x/(h^{-1}Mpc)$")
plt.ylabel(q_id_show)
#plt.ylim([1.0475,1.0525])
if args.Ifylog:
	plt.yscale("log")

#if IfXunitInCode:
	#plt.xscale("log")
	#plt.xlim([0.02,0.52])

plt.show()









