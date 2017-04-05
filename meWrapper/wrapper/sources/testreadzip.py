#!/usr/bin/python
from __future__ import print_function
import sys
import uuid
import xml.etree.ElementTree as e
import os
from os import listdir
from os.path import isfile, join
import zipfile

sources= 'sources/'
cwd = os.getcwd() + '/' +  sources
print('cwd %s'%cwd)
onlyfiles = [sources + f for f in listdir(cwd) if isfile(join(cwd, f))]

for files in onlyfiles:
    if '.fmu' in files:
        fmufile = files
        print(fmufile)

#zipfile.namelist(fmuFile)
import zipfile

zip=zipfile.ZipFile(fmufile)
f=zip.open('modelDescription.xml')
contents=f.read()
f.close()
print (contents)
