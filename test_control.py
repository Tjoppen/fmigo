#!/usr/bin/env python3
import zmq
import os
import subprocess
import time
import sys
sys.path.append('fmigo/src/master')
import control_pb2

FMUS_DIR = 'build/umit-fmus' if not 'FMUS_DIR' in os.environ else os.environ['FMUS_DIR']
fmu = os.path.join(FMUS_DIR, 'kinematictruck/body/body.fmu')

print('ZMQ control + pause test')

# Have timestep close to duration, to test that we only delay *between* steps
t_end = 0.7
dt = 0.1

control_port = 5555

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

context = zmq.Context()
socket = context.socket(zmq.REQ) # control is REQ-REP
socket.connect('tcp://localhost:%i' % control_port)

def fail():
  print('ZMQ control + pause test failed')
  exit(1)


def send_cmd_expect_state(cmd, expected_state):
  ctrl = control_pb2.control_message()
  ctrl.version = 1
  ctrl.command = cmd
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
# Finally it takes about 10 ms to shut everything down
expected = t_end - 2*dt - tmid + 0.01

elapsed = t4 - t3
print('Last half: %f vs %f' % (elapsed, expected))

# 10% tolerance on time elapsed
if ret != 0 or abs(elapsed - expected) > expected / 10:
  fail()
else:
  print('ZMQ control + pause test passed')
