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
	
fout = open("cstrikedata/" + foutname + "v2.data", "w")

## Format of output: x y z type
## where type = attacker or victim

line = fin.readline()
while line != "":
	s = string.find(line, "attacker_position")
	if s != -1:
		s += 19
		while line[s] != "\"":
			fout.write(line[s])
			s += 1
		fout.write(" attacker\n")
		s = string.find(line, "victim_position")
		if s!= -1:
			s += 17
			while line[s] != "\"":
				fout.write(line[s])
				s += 1
	fout.write(" victim\n")
	
	line = fin.readline()

fout.close()

