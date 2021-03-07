#!/bin/bash
set -e

FMU="${FMUS_DIR}/tests/stringtest/stringtest.fmu"

# Strip first column from CSV output so we don't depend on how time becomes formatted
${MPIEXEC} -np 2 fmigo-mpi -t 0.0 $FMU | sed -e 's/[^,]*,//' > temp.out
diff --ignore-all-space - temp.out <<EOF
"",""
EOF

${MPIEXEC} -np 2 fmigo-mpi -t 0.0 -p s,0,5,hello $FMU | sed -e 's/[^,]*,//' > temp.out
diff --ignore-all-space - temp.out <<EOF
"hello",""
EOF

# CSV escapes double quotes with double-double quotes (" -> "")
# Need to use a here-document because MS-MPI messes up quotes in program parameters (\"hello\" becomes hello, not "hello")
${MPIEXEC} -np 2 fmigo-mpi -t 0.0 -a - $FMU <<EOF | sed -e 's/[^,]*,//' > temp.out
-p s,0,5,"hello"
EOF
diff --ignore-all-space - temp.out <<EOF
"""hello""",""
EOF

# Test with string in second position. We had a bug where main.cpp would not pop_front() m_getStringValues
${MPIEXEC} -np 2 fmigo-mpi -t 0.0 -p s,0,6,wello $FMU | sed -e 's/[^,]*,//' > temp.out
diff --ignore-all-space - temp.out <<EOF
"","wello"
EOF

# Here-document again
${MPIEXEC} -np 2 fmigo-mpi -t 0.0 -a - $FMU <<EOF | sed -e 's/[^,]*,//' > temp.out
-p s,0,6,"wello"
EOF
diff --ignore-all-space - temp.out <<EOF
"","""wello"""
EOF

# Do some tests with setting string parameters by name both via command line and via here-document
${MPIEXEC} -np 2 fmigo-mpi -t 0.0 -p 0,s02,wello $FMU | sed -e 's/[^,]*,//' > temp.out
diff --ignore-all-space - temp.out <<EOF
"","wello"
EOF

${MPIEXEC} -np 2 fmigo-mpi -t 0.0 -a - $FMU <<EOF | sed -e 's/[^,]*,//' > temp.out
-p 0,s02,"wello"
EOF
diff --ignore-all-space - temp.out <<EOF
"","""wello"""
EOF

rm temp.out
echo String tests OK
