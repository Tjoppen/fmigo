Stiff co-simulation in FMI 2.0 RC1
==================================

This piece of code demonstrates how to do stiff co-simulation of rigid body
systems using the stiff co-simulation technique developed by Claude Lacoursière
and Stefan Hedman using FMI 2.0.

Note that this code does not use a real FMU. Instead, it implements all relevant
FMI functions and just runs them without having to dynamically link the FMU,
parse XML, etc, etc..

# Change log

## v0.1

