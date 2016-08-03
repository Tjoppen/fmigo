#!/bin/bash

##
## Connections:
## Engine:
###     path to Scania module ? 
##      input:  velocity from load   (
##      output: torque to clutch    
##  Simple engine: ~/vtb/umit/umit-fmus/kinematictruck/engine/engine.fmu
## 
##      input: velocity from load to port omega_l, vr 6
##
##      output: torque on tau vr 3    omega vr 1
## Clutch: umit-fmus/gsl2/clutch_ef/clutch_ef.fmu
##       input: force_in1 vr 9 : torque from engine
##              force_in2 vr 10 : counter torque for attached load 
## 
##       output: v2   vr 12  velocity of the out plate 
##   
##  Mass (acts like shaft):  
##  
## 
## Mass: umit-fmus/gsl2/mass_force_fe/mass_force_fe.fmu
##      input: vin on vr 0    counter torque force_c on vr 1
##      output:  counter torque on clutch  force_out1 at vr 8
##       and  force_out2  torque on load
##
## AgX truck (or mass effort flow ):  
##        input from force_out2  ???
##        output to omega input of engine. 
