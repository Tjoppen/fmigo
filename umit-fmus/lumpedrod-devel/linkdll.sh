#!/bin/bash

for i in *.cpp  
do
    for j in `cat $i | grep DEFUN_DLD | sed -e's/[^(]\+([ ]*\([^,]\+\),.*/\1/'`
    do
        echo $j
        k=`basename $i .cpp`
        if [ $j != $k ] && [ ! -e $j.oct ]
        then
            echo "creating link $k.oct to $j.oct "
            ln -s $k.oct $j.oct
        fi
    done
done
