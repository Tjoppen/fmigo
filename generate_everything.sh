#!/bin/bash
set -e
SSD2SSP=$(pwd)/ssp/ssd2ssp.py

for d in ssp/ssp-test ssp/ssp-work-reports ssp/ssp-kinematic-example
do
    pushd $d
    python $SSD2SSP SystemStructure.ssd
    popd
done

pushd umit-fmus
./generate_everything.sh
popd

