#!/usr/bin/env python

import sys
import math
import string

fin = open(sys.argv[1], "r")
line = fin.readline()
error = string.find(line, "\"")
foutname = ""
if error != -1:
	s = error
	s += 1
	while line[s] != "\"":
		foutname += line[s]
		s += 1
	
fout = open("cstrikedata/" + foutname + ".data", "w")

## Format of output: x y z x y z
## attacker followed by victim

line = fin.readline()
while line != "":
	s = string.find(line, "attacker_position")
	if s != -1:
		s += 19
		while line[s] != "\"":
			fout.write(line[s])
			s += 1
		fout.write(" ")
		s = string.find(line, "victim_position")
		if s!= -1:
			s += 17
			while line[s] != "\"":
				fout.write(line[s])
				s += 1
	fout.write("\n")
	
	line = fin.readline()

fout.close()

