import sys
import subprocess
from os.path import join as pjoin
import math
import csv
from pylab import *


def run_trailer():
    nfmus = 1
    total_time = 10
    comm_step = 0.1
    
    cmdline = ['mpiexec', '-np', str(nfmus+1)]
    cmdline += ['fmigo-mpi', '-t', str(total_time), '-d', str(comm_step)]
    cmdline += ['-p', '0,integrator,10']
    cmdline += ['-p', '0,triangle_amplitude,0']
    cmdline += ['-p', '0,k_d,1000000:0,gamma_d,5000']
    cmdline += ['-p', '0,integrate_dw,true:0,omega_i,3']
    cmdline += ['../../gsl2/trailer/trailer.fmu' ]
    cmdline  += ['-p', '0,octave_output,true' ]
    cmdline  += ['-p', '0,octave_output,true' ]
    cmdline  += ['-p', '0,octave_output_file,trailer.m']
    filename = 'out.csv' 
    print(' '.join(cmdline) + " > " + filename, file=sys.stderr)
    ret = subprocess.call(cmdline, stdout=open(filename, 'w'))
    return  filename, ret



fn, ret = run_trailer()

t = []
x = []
fc = []
# Data layout
# x     v      a      phi  omega  alpha  coupling force coupling torque
# 0     1     2      3     4       5       6       7       8
for row in csv.reader(open(fn)):
  t.append(float(row[0]))
  x.append([
    float(row[c]) for c in range( 1, len(row) )
  ])

t = array(t)
x = array(x)
fc = x[:, [-2,-1]]

figure(1)
clf()
plot(t, x[:, 4])
show()
