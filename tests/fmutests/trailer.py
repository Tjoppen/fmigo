import sys
import subprocess
from os.path import join as pjoin
import math
import csv
from pylab import *


def run_trailer(filter_length=0):
    nfmus = 1
    total_time = 10
    comm_step = 0.1
    
    cmdline = ['mpiexec', '-np', str(nfmus+1)]
    cmdline += ['fmigo-mpi', '-t', str(total_time), '-d', str(comm_step)]
    cmdline += ['-p', '0,filter_length,%i' % filter_length]
    cmdline += ['-p', '0,integrator,2']
    cmdline += ['-p', '0,triangle_amplitude,2']
    cmdline += ['-p', '0,triangle_wavelength,1']
    cmdline += ['-p', '0,k_d,1000000:0,gamma_d,5000']
    cmdline += ['-p', '0,integrate_dw,true:0,omega_i,3']
    cmdline  += ['-p', '0,octave_output,true' ]
    cmdline  += ['-p', '0,octave_output,true' ]
    cmdline  += ['-p', '0,octave_output_file,trailer.m']


    
    cmdline += ['../../gsl2/trailer/trailer.fmu' ]
    filename = 'out-filter-%i.csv' % filter_length
    print(' '.join(cmdline) + " > " + filename, file=sys.stderr)
    ret = subprocess.call(cmdline, stdout=open(filename, 'w'))
    return  filename, ret

def parse_file(f):
    # Data layout
    # x     v      a      phi  omega  alpha  coupling force coupling torque
    # 0     1     2      3     4       5       6       7       8
    t = []
    x = []
    for row in csv.reader(open(fn)):
        t.append(float(row[0]))
        x.append([
            float(row[c]) for c in range( 1, len(row) )
        ])
    t = array(t)
    x = array(x)
    fc = x[:, [-2,-1]]
    return t, x, fc

fn, ret = run_trailer(filter_length=0)
t0, x0, fc0 = parse_file( fn )
fn, ret = run_trailer(filter_length=2)
t2, x2, fc2 = parse_file( fn )
figure(1)
clf()
plot(t0, x0[:, 4], t2, x2[:, 4])
title('Angular velocities')
legend(['filter = 0', 'filter = 2'])
figure(2)
clf()
plot(t0, fc0[:,0], t2, fc2[:,0])
title('coupling forces')
legend(['filter = 0', 'filter = 2'])
show()
