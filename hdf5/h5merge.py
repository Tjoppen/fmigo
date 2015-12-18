from sys import *
from h5py import *

if len(argv) < 5:
    print('''USAGE: %s output_filename output_group_prefix input_group [list of input filenames]

Example: %s output.h5 FMU fmu input1.h5 input2.h5 input3.h5

This would put the contents of input1.h5['fmu'] under output.h5['FMU0'], input2.h5['fmu'] under output.h5['FMU1'] and so on.
''' % (argv[0],argv[0]))
    exit(1)

output_filename = argv[1]
output_prefix = argv[2]
input_group = argv[3]

output = File(output_filename, 'w')

i = 0
for filename in argv[4:]:
    output_group = '%s%i' % (output_prefix, i)
    print("%s <-- %s" % (output_group, filename))
    input = File(filename, 'r')
    g = input[input_group]
    output.copy(g, output_group)
    i += 1

output.close()

