#!/bin/bash
set -e




run_chained_sho() {
    N=$1
    dt=$2
    filter_length=$3
    output_filename=$4
    echo $dt

    mass=$(python <<< "print(1.0 / $N)")
    k_i=$(python <<< "print( $8  / $N)")
    damping_i=$(python <<< "print( $9 / $N)")
    # coupling springs
    k_c=$(python <<< "print( $N )")
    # coupling damping
    zeta=$7
    damping_c=$(python <<< "print( $zeta * $N)")

    omega=$(bc -l  <<< " sqrt( $k_c / $mass ) ")
    homega=$(bc -l  <<< " $dt * $omega " )
    echo " h * omega = $homega "
 

    start=$(python <<< "print(int(0.0 / $dt))")
#    length=$(python <<< "print(int($6 / $dt))")
    length=$(python <<< "print(int(20 / $dt))")

    ampN=1
    amp0=-0
    FMUS=": -np 1 fmi-mpi-server ../../impulse/impulse.fmu"
    # Connect impulse generator as force input on last system
    CONNS="-c 0,0,$N,196"
    PARAMS="-p i,0,4,0:i,0,5,${start}:i,0,6,${length}:0,7,$ampN"
    # Disable coupling spring on first system
    PARAMS="${PARAMS} -p 1,1,0:1,2,0"
    # Dump state of first system in sho.m
    PARAMS="${PARAMS} -p b,1,97,true"

    for i in $(seq 1 $N)
    do
        i2=$(bc <<< "$i + 1")
        if [ $i -lt $N ]
        then
            CONNS="${CONNS} -c $i,12,$i2,5:$i2,10,$i,6"
        fi

        if [ $i -ne 1 ]
        then
            # Set coupling springs except for the first system which has none
            PARAMS="${PARAMS} -p $i,1,${k_c}:$i,2,${damping_c}"
        fi

        PARAMS="${PARAMS} -p i,$i,98,${filter_length} -p $i,0,${mass}:$i,4,${damping_i}:$i,95,${k_i}:$i,7,0"
        FMUS="${FMUS} : -np 1 fmi-mpi-server chained_sho.fmu"
    done

    if false
       then 
    N1=$(bc <<< "$N + 1")
    FMUS="${FMUS} : -np 1 fmi-mpi-server ../../impulse/impulse.fmu"
    PARAMS="${PARAMS} -p i,$N1,4,0:i,$N1,5,${start}:i,$N1,6,${length}:$N1,7,$amp0"
#    # Connect impulse generator as force input on first system
    CONNS="${CONNS}  -c $N1,0,1,196"
    fi

    mpiexec -np 1 fmi-mpi-master -m $5 -t $6 -d $dt ${CONNS} ${PARAMS} ${FMUS} > ${output_filename} || true
    # Cut out mpi crap if GSL exploded
    sed -i '/---.*/{N;N;N;N;N;N;N;N;N;N;s/---.*//}' ${output_filename}
}

pushd gsl2/chained_sho

#for stepper in gs jacobi
#do
#    for N in 1 2 4 8
#    do
#	dfile=\'out-$N-$stepper.csv\'
#	
#    done
#done

## omega = 2 * pi * freq = 1
## period = 1 / freq = 2 * pi 
## Step is defined as # of steps per period
##
## so,
## n_steps * h = period = 2*pi
##  h = 2 * pi / n_steps
N=2
n_steps=5
zeta=0.8
tend=40
filter_length=2
k_i=0.0
d_i=0.0
stepper=gs
print=1
tikz=0


## step size for the fundamental
step0=$(bc -l <<< " 2 * 4 * a( 1 ) / $n_steps " )
step=`bc -l <<< "$step0 / $N  "`


dfile=`printf 'chain-masses-positions-N=%d-steps=%1.2g-damping=%1.2g-stepper=%s-filter-length=%d-ki=%1.2g-di=%1.2g.csv' $N $step0 $zeta $stepper $filter_length $k_i $d_i`


if true
then
run_chained_sho $N  $step $filter_length $dfile  $stepper $tend $zeta $k_i $d_i
fi 


dfile=\'$dfile\'
n_steps=`printf '%1.2g' $n_steps`

newline_tikz='\\'
newline_pdf="\\n"

titx=`printf 'Positions, $\zeta$=%1.2g $H$=%1.2g  $k_i$=%1.2g $d_i$=%1.2g %s N=%d Stepper=%s Filter length=%d'  $zeta $n_steps  $k_i $d_i $newline_tikz $N $stepper $filter_length`
titx_tikz=\'$titx\'

titxp=`printf 'Positions, damping=%1.2g steps=%1.2g  ki=%1.2g di=%1.2g %s N=%d Stepper=%s Filter length=%d'  $zeta $n_steps  $k_i $d_i $newline_pdf $N $stepper $filter_length`
titx_pdf=\"$titxp\"

titv=`printf 'Velocities, $\zeta$=%1.2g $H$=%1.2g  $k_i$=%1.2g $d_i$=%1.2g %s N=%d Stepper=%s Filter length=%d'  $zeta $n_steps  $k_i $d_i $newline_tikz $N $stepper $filter_length`
titv_tikz=\'$titv\'
titvp=`printf 'Velocities, damping=%1.2g steps=%1.2g  ki=%1.2g di=%1.2g %s N=%d Stepper=%s Filter length=%d'  $zeta $n_steps  $k_i $d_i $newline_pdf $N $stepper $filter_length`
titv_pdf=\"$titvp\"


printx=`printf 'chain-masses-positions-N=%d-steps=%1.2g-damping=%1.2g-stepper=%s-filter-length=%d-ki=%1.2g-di=%1.2g' $N $n_steps $zeta $stepper $filter_length $k_i $d_i`
printx_tikz=\'${printx}.tikz\'
printx_pdf=\'${printx}.pdf\'

printv=`printf 'chain-masses-velocities-N=%d-steps=%1.2g-damping=%1.2g-stepper=%s-filter-length=%d-ki=%1.2g-di=%1.2g' $N $step0 $zeta $stepper $filter_length $k_i $d_i`
printv_tikz=\'${printv}.tikz\'
printv_pdf=\'${printv}.pdf\'

END=`bc -l <<< "$N * 4 + 5 "`

octave -q --no-gui --persist  --eval "

         d=csvread($dfile);
         npoints = length(d(:,1));
         maxpoints = 500;
         stride = ceil( npoints / maxpoints );
         rg = 1:stride:npoints;
         % This is the force impulse

         figure( 1 );
         hold on;
         plot(d(rg,1),d(rg,2),'k-;pulse N;')
         %pulse0 = 5 + 4 * $N 
         %plot(d(rg,1),d(rg,pulse0),'k-;pulse 0;')
         H=plot(d(rg,1),d(rg,6:4:$END),'.-');
         set(findall(H, '-property', 'linewidth'), 'linewidth', 2);
         hold off;
         %axis( [0 max( d(rg, 1) ) -0.5 2.0 ] );
         ylabel('position')
         xlabel('time')
         if ( $tikz )
         title($titx_tikz)
         else
         title($titx_pdf)
         endif
         legend('location', 'northwest')

         if ( $print ) 
          if ( $tikz )
           print($printx_tikz, '-dtikz', '-F:10', '-r100');
          else
           print($printx_pdf);
          endif
         endif
         figure( 2 );
         H=plot(d(rg,1),d(rg,7:4:$END),'.-');
         set(findall(H, '-property', 'linewidth'), 'linewidth', 2);
         if ( $tikz )
         title($titv_tikz)
         else
         title($titv_pdf)
         endif
         ylabel('velocity')
         xlabel('time')
         if ( $print ) 
          if ( $tikz )
           print($printv_tikz, '-dtikz', '-F:10', '-r100');
          else
           print($printv_pdf);
          endif
         endif

"

popd

