#!/bin/bash

# Print all commands
set -v;

# Run FMU simulation
./build/master 1 fmu/bouncingBall.fmu 2 0 0 0 1 0 2 0 3 2.5 0.01 1 c &&

# Run double FMU simulation with 2 connections
./build/master 2 fmu/bouncingBall.fmu fmu/bouncingBall.fmu 2 0 0 0 1 0 2 1 2 2.5 0.01 1 c && 

# Make PNG plot
octave -q --eval="x = csvread('result0.csv'); t=x(:,1); h=x(:,2); plot(t,h); print('bouncingBall.png');";
