#!/bin/bash
set -e

# Strip first column from CSV output so we don't depend on how time becomes formatted
mpiexec -np 2 fmigo-mpi -t 0.0 stringtest.fmu | sed -e 's/[^,]*,//' > temp.out
diff --ignore-all-space - temp.out <<EOF
"",""
EOF

mpiexec -np 2 fmigo-mpi -t 0.0 -p s,0,5,hello stringtest.fmu | sed -e 's/[^,]*,//' > temp.out
diff --ignore-all-space - temp.out <<EOF
"hello",""
EOF

# CSV escapes double quotes with double-double quotes (" -> "")
# Need to use a here-document because MS-MPI messes up quotes in program parameters (\"hello\" becomes hello, not "hello")
mpiexec -np 2 fmigo-mpi -t 0.0 -a - stringtest.fmu <<EOF | sed -e 's/[^,]*,//' > temp.out
-p s,0,5,"hello"
EOF
diff --ignore-all-space - temp.out <<EOF
"""hello""",""
EOF

# Test with string in second position. We had a bug where main.cpp would not pop_front() m_getStringValues
mpiexec -np 2 fmigo-mpi -t 0.0 -p s,0,6,wello stringtest.fmu | sed -e 's/[^,]*,//' > temp.out
diff --ignore-all-space - temp.out <<EOF
"","wello"
EOF

# Here-document again
mpiexec -np 2 fmigo-mpi -t 0.0 -a - stringtest.fmu <<EOF | sed -e 's/[^,]*,//' > temp.out
-p s,0,6,"wello"
EOF
diff --ignore-all-space - temp.out <<EOF
"","""wello"""
EOF

# Do some tests with setting string parameters by name both via command line and via here-document
mpiexec -np 2 fmigo-mpi -t 0.0 -p 0,s02,wello stringtest.fmu | sed -e 's/[^,]*,//' > temp.out
diff --ignore-all-space - temp.out <<EOF
"","wello"
EOF

mpiexec -np 2 fmigo-mpi -t 0.0 -a - stringtest.fmu <<EOF | sed -e 's/[^,]*,//' > temp.out
-p 0,s02,"wello"
EOF
diff --ignore-all-space - temp.out <<EOF
"","""wello"""
EOF

rm temp.out
echo String tests OK
