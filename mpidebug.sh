#!/bin/bash
x="$@"
emacs  --eval "(gdb \"gdb --annotate=3  -i=mi -ex run --args $x \")"
