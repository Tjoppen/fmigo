#!/bin/bash
set -e

mpiexec -np 2 fmigo-mpi -t 0.0 stringtest.fmu > temp.out
diff temp.out - <<EOF
0.000000,"",""
EOF

mpiexec -np 2 fmigo-mpi -t 0.0 -p s,0,5,hello stringtest.fmu > temp.out
diff temp.out - <<EOF
0.000000,"hello",""
EOF

# CSV escapes double quotes with double-double quotes (" -> "")
mpiexec -np 2 fmigo-mpi -t 0.0 -p s,0,5,\"hello\" stringtest.fmu > temp.out
diff temp.out - <<EOF
0.000000,"""hello""",""
EOF

# Test with string in second position. We had a bug where main.cpp would not pop_front() m_getStringValues
mpiexec -np 2 fmigo-mpi -t 0.0 -p s,0,6,wello stringtest.fmu > temp.out
diff temp.out - <<EOF
0.000000,"","wello"
EOF

mpiexec -np 2 fmigo-mpi -t 0.0 -p s,0,6,\"wello\" stringtest.fmu > temp.out
diff temp.out - <<EOF
0.000000,"","""wello"""
EOF

rm temp.out
echo String tests OK
