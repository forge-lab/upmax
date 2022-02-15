#!/usr/bin/python    

from __future__ import division
import sys
from random import randint
from random import random

if len(sys.argv) < 4:
	print("Usage: python generator.py <#tags> <#persons> <max tags per person>")
	exit()

tags = int(sys.argv[1])
persons = int(sys.argv[2])
max_tags = int(sys.argv[3])


def main():
	for x in range(0,persons):
		line = str(x)
		value = randint(0,max_tags)
		t=0
		for y in range (0,tags):
			if t == max_tags:
				break
			v = random();
			if v <= float(value/tags):
				line = line + "," + str(y)
				t = t + 1
		print(line)

if __name__ == "__main__":
	main()
