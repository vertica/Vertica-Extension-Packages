#!/usr/bin/env python

import sys
import math
import random

# Seed value should be given to create the file
s = int(sys.argv[1])

# Followed by the number of data you want
n = int(sys.argv[2])

# Followed by the range of the data minX maxX minY maxY
mx = float(sys.argv[3])
xx = float(sys.argv[4])
rx = xx-mx

my = float(sys.argv[5])
xy = float(sys.argv[6])
ry = xy-my


random.seed(s)

i = 0
while i < n:
	print repr(random.random()*rx+mx)+","+repr(random.random()*ry+my)
	i += 1
