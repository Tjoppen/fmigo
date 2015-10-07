Strong coupling core                                                 {#mainpage}
====================

Library for solving a system of strong coupled physical subsystems. Aimed to use
in co-simulation, where each slave has mechanical connector points.

The library has support for a number of constraints, see subclasses of
sc::Constraint. These can be used to mechanically constrain the connectors
to each other.

# Links
* [Documentation](http://umitresearchlab.github.io/strong-coupling-core/docs)
* [Source code & downloads at Github](http://github.com/umitresearchlab/strong-coupling-core)

# Usage

The code is typically used like so:

1.  Create slaves (sc::Slave instances). These correspond to subsystems in your
    co-simulation.
2.  Create connectors (sc::Connector instances) and add them to the slaves. The
    connectors are points on which you can connect to other subsystem connectors.
3.  Constrain two connectors by creating instances of sc::Constraint.
4.  Add slaves, connectors, constraints to the solver (sc::Solver).
5.  For each time step in your stepping loop:
    1.  Set position, quaternion, velocity and angular velocity of the connectors.
    2.  Set future velocity and angular velocity of the connectors (you get this
        by stepping your system one time step forward).
    3.  Set the jacobian for each equation in the system (each sc::Constraint
        contains at least one sc::Equation). The Jacobian can be imagined as the
        connector inertia in the constraint directions.
    4.  Solve the system (sc::Solver::solve()).
    5.  Get resulting constraint force and torque from the connectors. Apply
        these forces to your co-simulation slaves and then do a final step.

Sample code can be found in test/rigid.cpp

# Install

## Ubuntu/Linux

To make an out-of-source build in CMake, make a build directory and make it current:

    mkdir build;
    cd build;
    cmake ..;

You need [UMFPACK](http://www.cise.ufl.edu/research/sparse/umfpack/) to continue.
When running cmake, pass the locations of its include and library paths using -D
flags:

    cmake .. -DUMFPACK_INCLUDE_DIR=<path> -DUMFPACK_LIBRARY_DIR=<path>

In Ubuntu, UMFPACK is included in the
[suitesparse](https://launchpad.net/ubuntu/+source/suitesparse) package.

Install suitesparse using the following command.

    sudo apt-get install suitesparse

And then try running cmake like this:

    cmake .. -DUMFPACK_INCLUDE_DIR=/usr/include/suitesparse -DUMFPACK_LIBRARY_DIR=/usr/lib

Finally, run make to compile the code.

    make
    make install    # optional

## OSX
Install SuiteSparse via MacPorts

    port install SuiteSparse;

CMakeLists.txt should add /opt/local/include and /opt/local/lib to the paths for you. Run:

    mkdir build;
    cd build;
    cmake .. && make;

## Windows
TODO

# Documentation

You need [Doxygen](http://www.stack.nl/~dimitri/doxygen/) to generate the
documentation. Build the documentation by doing this:

    doxygen Doxyfile;

The HTML files will end up in docs/.

# Project info
Funded in part by VINNOVA through project Simovate (dnr 2012-01235) and Umeå University, Umeå, Sweden.

# Change log
*0.2*
* Added class sc::HingeMotorConstraint
* Added method sc::Equation::setRelativeVelocity

*0.1*
* Started change log
