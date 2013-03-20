#!/usr/bin/python

import csv, sys
import matplotlib.pyplot as plt
import numpy as np
import argparse

# Parse arguments
parser = argparse.ArgumentParser(description='Plot CSV data.')
parser.add_argument('files',
                    metavar='FILE',
                    type=str,
                    nargs='+',
                    help='Files to read data from')
parser.add_argument('--columns',
                    type=str,
                    help='Columns to plot, separated by commas.')
parser.add_argument('--xcolumn',
                    type=str,
                    default="time",
                    help='Columns to plot, separated by commas.')

args = parser.parse_args()

# Needed?
if not len(args.files):
    print "Expecting at least one file!";
    exit()

# Data loaded from files
names = []       # index => name
series_dict = {} # name  => data

# Process each input file
for fName in args.files:

    # Open the file
    f = open(fName, 'r')

    # Check to see if the file starts with headers or data:
    dialect = csv.Sniffer().has_header(f.read(10*1024))
    f.seek(0)
    reader = csv.reader(f)

    first = True
    for row in reader:
        i = 0
        for column in row:
            if first:
                names.append(column)
                series_dict[column] = []
            else:
                if series_dict.has_key(names[i]):
                    try:
                        series_dict[names[i]].append(float(column))
                    except ValueError:
                        pass
            i += 1
        first = False

# Check if xcolumn is found
if not series_dict.has_key(args.xcolumn):
    print "There is no data column \"%s\" in the files." % args.xcolumn
    exit()


if args.columns:
    # Check if the --columns are found
    for c in args.columns.split(","):
        if not series_dict.has_key(c):
            print "There is no data column \"%s\" in the files. Available columns are: %s" % (c,", ".join(series_dict.keys()))
            exit()

    # Remove everything but the --columns and the --xcolumn
    for c in series_dict.keys():
        if not c in args.columns.split(",") and c != args.xcolumn:
            del series_dict[c]

# Plot each data series
num_cols = len(names)
i = 0
for name in series_dict:
    i += 1
    if name != args.xcolumn:
        plt.plot(series_dict[args.xcolumn], series_dict[name], ("o" if i>=6 else "-"), label=name)

# Get axis labels
xaxis_label = args.xcolumn
yaxis_label = ""

# Show the plot
plt.ylabel(yaxis_label)
plt.xlabel(xaxis_label)
plt.legend()
plt.show()

# Stop
f.close()
