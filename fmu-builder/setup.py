#!/usr/bin/env python

from distutils.core import setup

setup(
    name='fmu-builder',
    version='0.0.1',
    description='Build FMUs simply via command line.',
    author='Stefan Hedman',
    author_email='schteppe@gmail.com',
    scripts=['bin/fmu-builder','bin/modeldescription2header']
    )
