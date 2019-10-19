#!/usr/bin/env python3
import zmq
import os
import subprocess
import time
import io
from numpy import * # genfromtxt, array
import sys
sys.path.append('../src/master')
import control_pb2

FMUS_DIR = '../build/umit-fmus' if not 'FMUS_DIR' in os.environ else os.environ['FMUS_DIR']
fmu = os.path.join(FMUS_DIR, 'tests/typeconvtest/typeconvtest.fmu')

print('ZMQ control + pause test')

# Have timestep close to duration, to test that we only delay *between* steps
t_end = 3.0
dt = 0.1

control_port = 5555
results_port = 5556

# Realtime, ZMQ control, start paused, no output
proc = subprocess.Popen([
  'mpiexec', '-np', '2', 'fmigo-mpi',
    '-t', str(t_end),
    '-d', str(dt),
    '-r',
    '-z', str(control_port),
    '-Z',
    '-f', 'none',
    fmu
])

context = zmq.Context(2)
socket = context.socket(zmq.REQ) # control is REQ-REP
socket.connect('tcp://localhost:%i' % control_port)

def fail():
  print('ZMQ control + pause test failed')
  exit(1)


def send_cmd_expect_state(cmd, expected_state, variables=None):
  ctrl = control_pb2.control_message()
  ctrl.version = 1
  ctrl.command = cmd
  if not variables is None:
    ctrl.variables.extend([variables])

  socket.send(ctrl.SerializeToString())
  state = control_pb2.state_message()
  state.ParseFromString(socket.recv())
  #print(state)

  if state.state != expected_state:
    print('state.state != %s' % str(expected_state))
    fail()

  return state

# Poll state first, make sure we're paused
send_cmd_expect_state(
  control_pb2.control_message.command_state,
  control_pb2.state_message.state_paused
)

# Sleep for a little bit to exercize how realtime mode interacts with pause
time.sleep(t_end/4)

# Now unpause. Measure how much time it takes to complete - should be same as t_end
t1 = time.time()
send_cmd_expect_state(
  control_pb2.control_message.command_unpause,
  control_pb2.state_message.state_running
)

# Do a pause in the middle
# Send the pause in the middle of the step, dt/2
time.sleep(t_end/3)
tmid = send_cmd_expect_state(
  control_pb2.control_message.command_pause,
  control_pb2.state_message.state_paused
).t
t2 = time.time()

# Expect simulation to have progressed roughly t2-t1
print('Midpoint: %f vs %f' % (t2 - t1, tmid))
if abs(t2 - t1 - tmid) > tmid / 10:
  failt()

# Unpause, run to completion
time.sleep(t_end/3)
t3 = time.time()
send_cmd_expect_state(
  control_pb2.control_message.command_unpause,
  control_pb2.state_message.state_running
)

# Wait for process to complete
ret = proc.wait()
t4 = time.time()

# Expected elapsed time for the end part of the simulation
# The -2*dt accounts for unpause happening immediately
# -tmid for the time already simulated
expected = t_end - 2*dt - tmid

elapsed = t4 - t3
print('Last half: %f vs %f' % (elapsed, expected))

# 10% tolerance on time elapsed, plus another 50 ms for shutdown
if ret != 0 or abs(elapsed - expected) > expected / 10 + 0.05:
  fail()
else:
  print('ZMQ control + pause test passed')



#################################################################
# Check that setting parameters via the control channel works   #
#################################################################

print('ZMQ parameter control + results test..')

# *Not* realtime, ZMQ control, start paused, CSV output
proc = subprocess.Popen([
  'mpiexec', '-np', '2', 'fmigo-mpi',
    '-t', '1',
    '-d', '1',
    '-z', '%i:%i' % (control_port, results_port),
    '-Z',
    fmu
])

results_socket = context.socket(zmq.PULL)
results_socket.connect('tcp://localhost:%i' % results_port)

# Set some known values, unpause
params = control_pb2.fmu_results()
params.fmu_id = 0
params.reals.vrs.append(4)
params.reals.values.append(4.0)
params.ints.vrs.append(5)
params.ints.values.append(5)
params.bools.vrs.append(6)
params.bools.values.append(True)

send_cmd_expect_state(
  control_pb2.control_message.command_unpause,
  control_pb2.state_message.state_running,
  params
)

# Grab output, compare to reference
def fail2():
  print('ZMQ parameter control + results test failed')
  exit(1)

def vr2val(vrval, vr):
  # figure out index
  for index in range(len(vrval.vrs)):
    if vrval.vrs[index] == vr:
      return vrval.values[index]
  print("Couldn't look up VR=%i" % vr)
  fail2()

results = control_pb2.results_message()

# Testing float equality may look suspicious, but 0.0 and 4.0 should carry over properly if we have some level of sanity
results.ParseFromString(results_socket.recv())
if not (vr2val(results.results[0].reals, 4), vr2val(results.results[0].ints, 5), vr2val(results.results[0].bools, 6)) == (0.0, 0, False):
  print(results)
  fail2()

results.ParseFromString(results_socket.recv())
if not (vr2val(results.results[0].reals, 4), vr2val(results.results[0].ints, 5), vr2val(results.results[0].bools, 6)) == (4.0, 5, True):
  print(results)
  fail2()

print('ZMQ parameter control + results test passed')
