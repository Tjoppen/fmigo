#!/usr/bin/python3
from __future__ import print_function
import sys
from lxml import etree
import zipfile
import argparse
import os

def warning(*objs):
  print("WARNING: ", *objs, file=sys.stderr)

def error(*objs):
  print("ERROR: ", *objs, file=sys.stderr)
  exit(1)

def xml2wrappedxml(args_xml, args_fmu, args_identifier, file=sys.stdout):
  # Preserve comments
  # Need to remove whitespace for pretty_print to work
  parser = etree.XMLParser(remove_blank_text=True, remove_comments=False)

  if len(args_xml):
    tree = etree.parse(args_xml, parser=parser)
    if len(args_fmu):
      # Both XML and FMU specified
      fmu = args_fmu
    else:
      # No filename for the FMU - guess that it's modelName + '.fmu'
      fmu = tree.getroot().attrib['modelName'] + '.fmu'
  elif len(args_fmu):
    zip=zipfile.ZipFile(args_fmu, 'r')
    f=zip.open('modelDescription.xml')
    tree = etree.parse(f, parser=parser)
    f.close()
    fmu = args_fmu
  else:
    warning(sys.args)
    error('%s expect either: -f myfmu.fmu or -x myxml.xml' %(sys.args[0]))

  root = tree.getroot()

  me = root.find('ModelExchange')

  if me == None:
    error('Not a ModelExchange FMU')

  guid = root.attrib['guid']

  # Make predictable derived guid by flipping the lowest bit
  # Deal with both {guid} and bare guid
  pos = -2 if guid[-1] == '}' else -1
  guid2 = list(guid)
  guid2[pos] = '%1x' % (int(guid[pos], 16) ^ 1)
  guid2 = "".join(guid2)

  # Transform from ME to CS
  del root.attrib['numberOfEventIndicators']

  root.attrib['modelName']      = args_identifier
  root.attrib['guid']           = guid2
  root.attrib['generationTool'] = 'fmigo ME FMU wrapper (original generationTool: %s)' % (
    root.attrib['generationTool'] if 'generationTool' in root.attrib else 'unknown'
  )

  me.tag = 'CoSimulation'
  me.attrib['modelIdentifier']                        = args_identifier
  me.attrib['canHandleVariableCommunicationStepSize'] = 'true'
  me.attrib['canGetAndSetFMUstate']                   = 'true'
  me.attrib['canSerializeFMUstate']                   = 'false' #unfortunately
  me.attrib['providesDirectionalDerivative']          = 'true'

  # Find some free VRs for "integrator" and such
  # First grab all VRs
  mvs = root.find('ModelVariables')
  vrs = [int(sv.attrib['valueReference']) for sv in mvs.findall('ScalarVariable')]

  extravars = [
    ('integrator',    'Integer',  '4',   'cgsl integrator'),
    ('octave_output', 'String',   '',    'dump output to octave compatible file with given filename'),
  ]

  vr = max(vrs)+1
  for extra in extravars:
    if vr >= 2**32:
      error("VR %i overflow @ extravar %s" % (vr, extra[0]))
    el = etree.SubElement(mvs, 'ScalarVariable')
    el.attrib['variability']    = 'fixed'
    el.attrib['causality']      = 'parameter'
    el.attrib['valueReference'] = str(vr)
    el.attrib['name']           = extra[0]
    el.attrib['description']    = extra[3]

    el2 = etree.SubElement(el, extra[1])
    el2.attrib['start']         = extra[2]

    # Mark variable as being for wrapper by adding 'fmigo' Annotation
    tool = etree.SubElement(etree.SubElement(el, 'Annotations'), 'Tool')
    tool.attrib['name'] = 'fmigo'

    # Add size to string. Guess 1 KiB is enough
    if extra[1] == 'String':
      etree.SubElement(tool, 'size').text = '1024'

    vr = vr + 1

  # Add the filename of the FMU as a VendorAnnotation
  vas = etree.Element('VendorAnnotations')
  # VendorAnnotation go before ModelVariables
  mvs.addprevious(vas)
  tool = etree.SubElement(vas, 'Tool')
  tool.attrib['name'] = 'fmigo'
  etree.SubElement(tool, 'fmu').text = os.path.basename(fmu)

  print(etree.tostring(root, pretty_print=True, encoding='unicode'), file=file)

if __name__ == '__main__':
  # Parse command line arguments
  parser = argparse.ArgumentParser(description='Transform modelDescription.xml for an ME FMU to CS, for the FMU wrapper')

  parser.add_argument('-f','--fmu',
                      type=str,
                      help='Path to FMU. If -x if set, override internal modelDescription.xml',
                      default='')
  parser.add_argument('-x','--xml',
                      type=str,
                      help='Path to xml. If -f is set, use that as the FMU but use this XML file for basing modelDescription.xml off of',
                      default='')
  parser.add_argument('-i','--identifier', required=True, help='modelIdentifier')
  args = parser.parse_args()

  xml2wrappedxml(args.xml, args.fmu, args.identifier)

