import sys
import subprocess
from os.path import join as pjoin
import math
import csv
from pylab import *


def run_clutch(filter_length=0,clutch_model=2):
    nfmus = 1
    total_time = 10
    comm_step = 0.1
    
    cmdline = ['mpiexec', '-np', str(nfmus+1)]
    cmdline += ['fmigo-mpi', '-t', str(total_time), '-d', str(comm_step)]
    cmdline += ['-p', '0,filter_length,%i' % filter_length]
    cmdline += ['-p', '0,k_ec,1000000:0,gamma_ec,5000']
    cmdline += ['-p', '0,integrate_dx_e,false']
    cmdline += ['-p', '0,gamma_e,0:0,gamma_s,0']
    cmdline += ['-p', '0,is_gearbox,true:0,v_in_e,4']
    cmdline += ['-p', '0,clutch_position,1']
    if ( clutch_model == 2 ):
        cmdline += ['-p', '0,integrator,4' ]
    else:
        cmdline += ['-p', '0,integrator,10' ]
    cmdline += ['-p', '0,octave_output,true' ]
    cmdline += ['-p', '0,octave_output_file,clutch%i.m'
                % clutch_model ]


    
    cmdline += ['../../gsl2/clutch%i/clutch%i.fmu'
                % (clutch_model,  clutch_model)]
    filename = 'out-filter-%i.csv' % filter_length
    print(' '.join(cmdline) + " > " + filename, file=sys.stderr)
    ret = subprocess.call(cmdline, stdout=open(filename, 'w'))
    return  filename, ret

def parse_file(f):
    # Data layout
    # xe     ve      ae     fe    xs  vs   fs
    # 0      1       2      3     4   5     6
    t = []
    x = []
    for row in csv.reader(open(fn)):
        t.append(float(row[0]))
        x.append([
            float(row[c]) for c in range( 1, len(row) )
        ])
    t = array(t)
    x = array(x)
    fc = x[:, [3,6]]
    # we don't care about the rest of the data. 
    v = x[:,[1,5]]
    x = x[:,[0,4]]
    return t, x, v, fc

fn, ret = run_clutch(filter_length=0, clutch_model=3)
t, x30, v30, fc30 = parse_file( fn )
fn, ret = run_clutch(filter_length=2, clutch_model=3)
t, x32, v32, fc32 = parse_file( fn )

fn, ret = run_clutch(filter_length=0, clutch_model=2)
t, x20, v20, fc20 = parse_file( fn )
fn, ret = run_clutch(filter_length=2, clutch_model=2)
t, x22, v22, fc22 = parse_file( fn )

figure(1)
clf()
plot( t, v20[:,0],t, v30[:,0], t, v22[:,0],t, v32[:,0] )
title('Velocities')
legend([ 'clutch 2 filter = 0', 'clutch 3 filter = 0',
    'clutch 2 filter = 2', 'clutch 3 filter = 2'] )
figure(2)
clf()
plot( t, fc20[:,0], t,fc30[:,0], t, fc22[:,0], t, fc32[:,0] )
title('coupling forces')
legend([
    'clutch 2 filter = 0', 'clutch 3 filter = 0',
    'clutch 2 filter = 2', 'clutch 3 filter = 2'] )
show()

