import sys
import csv

delim = ','
thresh = 1e-5

if len(sys.argv) >= 4:
    delim = sys.argv[3]

if len(sys.argv) >= 5:
    thresh = float(sys.argv[4])

# Ignore specified columns
# This allows ignoring coupling torques and accelerations in directionaltests
ignorecols = []
if len(sys.argv) >= 6 and len(sys.argv[5]) > 0:
    ignorecols = [int(s) for s in sys.argv[5].split(',')]

# Relative tolerance
relerr = 0
if len(sys.argv) >= 7:
    relerr = float(sys.argv[6])

csv1 = csv.reader(open(sys.argv[1]), delimiter=delim)
csv2 = csv.reader(open(sys.argv[2]), delimiter=delim)

for row1 in csv1:
    row2 = next(csv2)
    if row1 == row2:
        # Nothing to do if they're completely identical
        # This also takes care of headers not being parsable as float
        continue

    if len(row1) != len(row2):
        print("%s %s %s" % (sys.argv[0], sys.argv[1], sys.argv[2]))
        print("Line length mismatch on line %i: %i vs %i" % (csv1.line_num, len(row1), len(row2)))
        print(delim.join(row1))
        print(delim.join(row2))
        exit(1)

    for i in range(len(row1)):
        diff = abs(float(row1[i]) - float(row2[i]))
        mag  = max(abs(float(row1[i])), abs(float(row2[i])))

        if diff > thresh and diff/mag > relerr and not i in ignorecols:
            print("%s %s %s" % (sys.argv[0], sys.argv[1], sys.argv[2]))
            print("Line %i differs too much (abs(%f - %f) = %f @ column %i):" % (csv1.line_num, float(row1[i]), float(row2[i]), diff, i))
            print(delim.join(row1))
            print(delim.join(row2))
            exit(1)
