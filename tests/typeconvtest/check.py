import sys
import csv

#a = sys.stdin.readline()
#b = sys.stdin.readline()
#print(a)
#print(b)

if len(sys.argv) == 3:
    k = float(sys.argv[1])
    m = float(sys.argv[2])
else:
    k = 1.0
    m = 0.0

rows = [row for row in csv.reader(sys.stdin)]
#print(rows)

first  = [float(rows[0][1]), int(rows[0][2]), int(rows[0][3])]
second = [float(rows[1][1]), int(rows[1][2]), int(rows[1][3])]
#print(first)
#print(second)

# There should be exactly one element in first set, and the other two should be the type converted versions of it
if first[0] != 0:
    ref = [first[0], int(first[0]*k+m), 0 if int(first[0]*k+m) == 0 else 1]
elif first[1] != 0:
    ref = [float(first[1]*k+m), first[1], 0 if int(first[1]*k+m) == 0 else 1]
elif first[2] != 0:
    ref = [float(first[2]*k+m), int(first[2]*k+m), first[2]]
else:
    # If it's all zeroes then the second row should also be all zeroes
    ref = first

#print('ref = ' + str(ref))
if second != ref:
    print('Type conversion didn\'t behave as expected: ' + str(second) + ' != ' + str(ref))
    exit(1)
