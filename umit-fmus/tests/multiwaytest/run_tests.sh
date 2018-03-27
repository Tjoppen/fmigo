#!/bin/bash
set -e
pushd ../../..
source boilerplate.sh
popd

testit() {
  g=$1

  echo "Testing multiway differential with g=$g"
  mpiexec -np 4 fmigo-mpi -t 10 \
    -C multiway,3,0,1,2,theta,omega,alpha,tau,-1,x1,v1,a1,f1,${g},x1,v1,a1,f1,${g} \
    -c 0,omega,0,omega_l \
    -p 0,kp,10 \
    -p 1,m1,100:1,m2,100 \
    -p 1,k_internal,10000:1,gamma_internal,100 \
    -p 2,m1,100:2,m2,100 \
    -p 2,k_internal,10000:2,gamma_internal,100 \
    ${FMUS_DIR}/kinematictruck/engine/engine.fmu \
    ${FMUS_DIR}/meWrapper/springs2/wrapper_springs2.fmu \
    ${FMUS_DIR}/meWrapper/springs2/wrapper_springs2.fmu > springs2.csv

  #cp springs2.csv springs2-$g.csv
  python ../../../tests/compare_csv.py springs2-$g.csv springs2.csv "," "1e-6"

  # Check that the angle and angular velocity constraints converge to small values
  # Column indices:
  #                  t = 1
  #             angles = 2, 6, 16
  # angular velocities = 3, 7, 17
  # Last line in output should weighted sum to some small value
  t=$( tail -n1 springs2.csv | cut -d, -f1)
  a1=$(tail -n1 springs2.csv | cut -d, -f2)
  a2=$(tail -n1 springs2.csv | cut -d, -f6)
  a3=$(tail -n1 springs2.csv | cut -d, -f16)
  w1=$(tail -n1 springs2.csv | cut -d, -f3)
  w2=$(tail -n1 springs2.csv | cut -d, -f7)
  w3=$(tail -n1 springs2.csv | cut -d, -f17)

  python <<EOF
d = abs(-1 * $a1 + $g * $a2 + $g * $a2)
l = 0.07 * $g
if d > l:
  print('multiway constraint broke: ad=%f > %f, g=%f' % (d, l, $g)), exit(1)
EOF
  python <<EOF
d = abs(-1 * $w1 + $g * $w2 + $g * $w2)
l = 0.35 * $g
if d > l:
  print('multiway constraint broke: wd=%f > %f, g=%f' % (d, l, $g)), exit(1)
EOF

  rm springs2.csv
  echo "OK"
}

# Test the three different ways to get directional derivatives in wrapper.c
testit 0.1
testit 0.5
testit 1
testit 2
testit 10
